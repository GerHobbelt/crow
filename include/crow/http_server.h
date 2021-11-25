#pragma once

#include "crow/settings.h"

#include <chrono>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/asio.hpp>
#ifdef CROW_ENABLE_SSL
#include <boost/asio/ssl.hpp>
#endif
#include <cstdint>
#include <atomic>
#include <future>
#include <vector>

#include <memory>

#include "crow/version.h"
#include "crow/http_connection.h"
#include "crow/logging.h"
#include "crow/task_timer.h"

namespace crow
{
    using namespace boost;
    using tcp = asio::ip::tcp;

    template <typename Handler, typename Adaptor = SocketAdaptor, typename ... Middlewares>
    class Server
    {
    public:
    Server(Handler* handler, std::string bindaddr, uint16_t port, std::string server_name = std::string("Crow/") + VERSION, std::tuple<Middlewares...>* middlewares = nullptr, uint16_t concurrency = 1, uint8_t timeout = 5, typename Adaptor::context* adaptor_ctx = nullptr)
            : acceptor_(io_service_, tcp::endpoint(boost::asio::ip::address::from_string(bindaddr), port)),
            signals_(io_service_, SIGINT, SIGTERM),
            tick_timer_(io_service_),
            handler_(handler),
            concurrency_(concurrency == 0 ? 1 : concurrency),
            timeout_(timeout),
            server_name_(server_name),
            port_(port),
            bindaddr_(bindaddr),
            middlewares_(middlewares),
            adaptor_ctx_(adaptor_ctx)
        {
        }

        void set_tick_function(std::chrono::milliseconds d, std::function<void()> f)
        {
            tick_interval_ = d;
            tick_function_ = f;
        }

        void on_tick()
        {
            tick_function_();
            tick_timer_.expires_from_now(boost::posix_time::milliseconds(tick_interval_.count()));
            tick_timer_.async_wait([this](const boost::system::error_code& ec)
                    {
                        if (ec)
                            return;
                        on_tick();
                    });
        }

        void run()
        {
            for(int i = 0; i < concurrency_;  i++)
                io_service_pool_.emplace_back(new boost::asio::io_service());
            get_cached_date_str_pool_.resize(concurrency_);
            task_timer_pool_.resize(concurrency_);

            std::vector<std::future<void>> v;
            std::atomic<int> init_count(0);
            for(uint16_t i = 0; i < concurrency_; i ++)
                v.push_back(
                        std::async(std::launch::async, [this, i, &init_count]{

                            // thread local date string get function
                            auto last = std::chrono::steady_clock::now();

                            std::string date_str;
                            auto update_date_str = [&]
                            {
                                auto last_time_t = time(0);
                                tm my_tm;

#if defined(_MSC_VER) || defined(__MINGW32__)
                                gmtime_s(&my_tm, &last_time_t);
#else
                                gmtime_r(&last_time_t, &my_tm);
#endif
                                date_str.resize(100);
                                size_t date_str_sz = strftime(&date_str[0], 99, "%a, %d %b %Y %H:%M:%S GMT", &my_tm);
                                date_str.resize(date_str_sz);
                            };
                            update_date_str();
                            get_cached_date_str_pool_[i] = [&]()->std::string
                            {
                                if (std::chrono::steady_clock::now() - last >= std::chrono::seconds(1))
                                {
                                    last = std::chrono::steady_clock::now();
                                    update_date_str();
                                }
                                return date_str;
                            };

                            // initializing task timers
                            detail::task_timer task_timer(*io_service_pool_[i]);
                            task_timer.set_default_timeout(timeout_);
                            task_timer_pool_[i] = &task_timer;

                            init_count ++;
                            while(1)
                            {
                                try
                                {
                                    if (io_service_pool_[i]->run() == 0)
                                    {
                                        // when io_service.run returns 0, there are no more works to do.
                                        break;
                                    }
                                } catch(std::exception& e)
                                {
                                    CROW_LOG_ERROR << "Worker Crash: An uncaught exception occurred: " << e.what();
                                }
                            }
                        }));

            if (tick_function_ && tick_interval_.count() > 0)
            {
                tick_timer_.expires_from_now(boost::posix_time::milliseconds(tick_interval_.count()));
                tick_timer_.async_wait([this](const boost::system::error_code& ec)
                        {
                            if (ec)
                                return;
                            on_tick();
                        });
            }

            port_ = acceptor_.local_endpoint().port();
            handler_->port(port_);

            CROW_LOG_INFO << server_name_ << " server is running at " << bindaddr_ <<":" << acceptor_.local_endpoint().port()
                          << " using " << concurrency_ << " threads";
            CROW_LOG_INFO << "Call `app.loglevel(crow::LogLevel::Warning)` to hide Info level logs.";

            signals_.async_wait(
                [&](const boost::system::error_code& /*error*/, int /*signal_number*/){
                    stop();
                });

            while(concurrency_ != init_count)
                std::this_thread::yield();

            do_accept();

            std::thread([this]{
                io_service_.run();
                CROW_LOG_INFO << "Exiting.";
            }).join();
        }

        void stop()
        {
            should_close_ = false; //Prevent the acceptor from taking new connections
            while (handler_->websocket_count.load(std::memory_order_release) != 0) //Wait for the websockets to close properly
            {
            }
            for(auto& io_service:io_service_pool_)
            {
                if (io_service != nullptr)
                {
                CROW_LOG_INFO << "Closing IO service " << &io_service;
                io_service->stop(); //Close all io_services (and HTTP connections)
                }
            }

            CROW_LOG_INFO << "Closing main IO service (" << &io_service_ << ')';
            io_service_.stop(); //Close main io_service
        }

        void signal_clear()
        {
            signals_.clear();
        }

        void signal_add(int signal_number)
        {
            signals_.add(signal_number);
        }

    private:
        asio::io_service& pick_io_service()
        {
            // TODO load balancing
            roundrobin_index_++;
            if (roundrobin_index_ >= io_service_pool_.size())
                roundrobin_index_ = 0;
            return *io_service_pool_[roundrobin_index_];
        }

        void do_accept()
        {
            asio::io_service& is = pick_io_service();
            auto p = new Connection<Adaptor, Handler, Middlewares...>(
                is, handler_, server_name_, middlewares_,
                get_cached_date_str_pool_[roundrobin_index_], *task_timer_pool_[roundrobin_index_], adaptor_ctx_);
            if (!should_close_)
            {
                acceptor_.async_accept(p->socket(),
                    [this, p, &is](boost::system::error_code ec)
                    {
                        if (!ec)
                        {
                            is.post([p]
                            {
                                p->start();
                            });
                        }
                        else
                        {
                            delete p;
                        }
                        do_accept();
                    });
            }
        }

    private:
        asio::io_service io_service_;
        std::vector<std::unique_ptr<asio::io_service>> io_service_pool_;
        std::vector<detail::task_timer*> task_timer_pool_;
        std::vector<std::function<std::string()>> get_cached_date_str_pool_;
        tcp::acceptor acceptor_;
        bool should_close_ = false;
        boost::asio::signal_set signals_;
        boost::asio::deadline_timer tick_timer_;

        Handler* handler_;
        uint16_t concurrency_{1};
        std::uint8_t timeout_;
        std::string server_name_;
        uint16_t port_;
        std::string bindaddr_;
        unsigned int roundrobin_index_{};

        std::chrono::milliseconds tick_interval_;
        std::function<void()> tick_function_;

        std::tuple<Middlewares...>* middlewares_;

#ifdef CROW_ENABLE_SSL
        bool use_ssl_{false};
        boost::asio::ssl::context ssl_context_{boost::asio::ssl::context::sslv23};
#endif
        typename Adaptor::context* adaptor_ctx_;
    };
}
