#pragma once
#include <chrono>
#include <string>
#include <functional>
#include <memory>
#include <future>
#include <cstdint>
#include <type_traits>
#include <thread>
#include <condition_variable>

#include "cc/settings.h"
#include "cc/logging.h"
#include "cc/utility.h"
#include "cc/routing.h"
#include "cc/middleware_context.h"
#include "cc/http_request.h"
#include "cc/http_server.h"
#include "cc/detail.h"
#include "cc/file.h"
#include "cc/mustache.h"
#ifdef ENABLE_COMPRESSION
#include "cc/compression.h"
#endif

#ifdef _WIN32
#include <locale.h>
#define ROUTE(app, url) app.route(url)
#else
#define ROUTE(app, url) app.route_url<cc::spell::get_parameter_tag(url)>(url)
#endif
#define CATCHALL_ROUTE(app) app.default_route()

namespace cc {
  static std::string RES_home = HOME_PAGE;
  int detail::dumb_timer_queue::tick = 4;//Prevent being stuck by long connection
#ifdef ENABLE_SSL
  using ssl_context_t = boost::asio::ssl::context;
#endif
  ///The main server application
  /// Use `SimpleApp` or `App<Middleware1, Middleware2, etc...>`
  template <typename ... Middlewares>
  class Crow {
  public:
	using self_t = Crow;
	using server_t = Server<Crow, SocketAdaptor, Middlewares...>;
#ifdef ENABLE_SSL
	///An HTTP server that runs on SSL with an SSLAdaptor
	using ssl_server_t = Server<Crow, SSLAdaptor, Middlewares...>;
#endif
	Crow() {
#ifdef _WIN32
	  ::system("chcp 65001 >nul"); setlocale(LC_ALL, ".UTF8");
#endif
	  std::cout << "C++ web[服务] run on http://localhost";
	}
	///Process an Upgrade Req
	///Currently used to upgrrade an HTTP connection to a WebSocket connection
	template <typename Adaptor>
	void handle_upgrade(const Req& req, Res& res, Adaptor&& adaptor) { router_.handle_upgrade(req, res, adaptor); }
	///Process the Req and generate a Res for it
	void handle(const Req& req, Res& res) { router_.handle(req, res); }
	///Create a dynamic route using a rule (**Use ROUTE instead**)
	DynamicRule& route(std::string&& rule) { return router_.new_rule_dynamic(std::move(rule)); }
	///Create a route using a rule (**Use ROUTE instead**)
	template <uint64_t Tag>
	auto route_url(std::string&& rule)
	  -> typename std::result_of<decltype(&Router::new_rule_tagged<Tag>)(Router, std::string&&)>::type {
	  return router_.new_rule_tagged<Tag>(std::move(rule));
	}
	///Create a route for any requests without a proper route (**Use CATCHALL_ROUTE instead**)
	CatchallRule& default_route() { return router_.catchall_rule(); }
	self_t& signal_clear() { signals_.clear(); return *this; }
	self_t& signal_add(int signal_number) { signals_.push_back(signal_number); return *this; }
	///Set the port that Crow will handle requests on
	self_t& port(std::uint16_t port) { port_ = port; std::cout << ":" << port_ << std::endl; return *this; }
	///Set the maximum number of seconds (latency) per request (default is 4)
	self_t& timeout(std::uint8_t timeout) {
	  if (timeout > 10)timeout = 10;
	  if (timeout < 1)timeout = 1; detail::dumb_timer_queue::tick = timeout; return *this;
	}
	///The IP address that Crow will handle requests on (default is 0.0.0.0)
	self_t& bindaddr(std::string bindaddr) { bindaddr_ = bindaddr; return *this; }
	//Set static directory
	self_t& directory(std::string path) {
	  if (path.back() != '\\' && path.back() != '/') path += '/'; detail::directory_ = path; return *this;
	}
	self_t& home(std::string path) { RES_home = path; return *this; }
	//Set content types 
	self_t& file_type(const std::vector<std::string>& line) {
	  for (auto iter = line.cbegin(); iter != line.cend(); ++iter) {
		std::string types = ""; types = content_any_types[*iter];
		if (types != "") content_types.emplace(*iter, types);
	  } is_not_set_types = false; return *this;
	}
	///Run the server on multiple threads using all available threads
	self_t& multithreaded() { return concurrency(std::thread::hardware_concurrency()); }
	///Run the server on multiple threads using a specific number
	self_t& concurrency(std::uint16_t concurrency) {
	  if (concurrency < 1) concurrency = 1; concurrency_ = 1 + concurrency; return *this;
	}
	///Set the server's log level
	/// cc::LogLevel::Debug       (0)<br>
	/// cc::LogLevel::Info        (1)<br>
	/// cc::LogLevel::Warning     (2)<br>
	/// cc::LogLevel::Error       (3)<br>
	/// cc::LogLevel::Critical    (4)<br>
	self_t& loglevel(cc::LogLevel level) { cc::logger::setLogLevel(level); return *this; }

#ifdef ENABLE_COMPRESSION
	self_t& use_compression(compression::algorithm algorithm) {
	  comp_algorithm_ = algorithm;
	  return *this;
	}
	compression::algorithm compression_algorithm() {
	  return comp_algorithm_;
	}
#endif
	///A wrapper for `validate()` in the router
	///Go through the rules, upgrade them if possible, and add them to the list of rules
	void validate() { router_.validate(); }
	///Notify anything using `wait_for_server_start()` to proceed
	void notify_server_start() {
	  std::unique_lock<std::mutex> lock(start_mutex_);
	  server_started_ = true;
	  cv_started_.notify_all();
	}
	///Run the server
	void run() {
	  if (is_not_set_types) {
		this->file_type({ "html","ico","css","js","json","svg","png","jpg","gif","txt" });//default types
		is_not_set_types = false;
	  }
#ifndef DISABLE_HOME
	  route_url<cc::spell::get_parameter_tag("/")>("/")([] {
		return (std::string)mustache::load(RES_home);
		});
#endif
	  validate();

#ifdef ENABLE_SSL
	  if (use_ssl_) {
		ssl_server_ = std::move(std::unique_ptr<ssl_server_t>(new ssl_server_t(this, bindaddr_, port_, &middlewares_, concurrency_, &ssl_context_)));
		notify_server_start();
		ssl_server_->run();
	  } else
#endif
	  {
		server_ = std::move(std::unique_ptr<server_t>(new server_t(this, bindaddr_, port_, &middlewares_, concurrency_, nullptr)));
		server_->signal_clear();
		for (auto snum : signals_) {
		  server_->signal_add(snum);
		}
		notify_server_start();
		server_->run();
	  }
	}
	///Stop the server
	void stop() {
#ifdef ENABLE_SSL
	  if (use_ssl_) {
		if (ssl_server_) {
		  ssl_server_->stop();
		}
	  } else
#endif
	  {
		if (server_) {
		  server_->stop();
		}
	  }
	}
	void debug_print() {
	  LOG_DEBUG << "Routing:";
	  router_.debug_print();
	}

#ifdef ENABLE_SSL
	///use certificate and key files for SSL
	self_t& ssl_file(const std::string& crt_filename, const std::string& key_filename) {
	  use_ssl_ = true;
	  ssl_context_.set_verify_mode(boost::asio::ssl::verify_peer);
	  ssl_context_.set_verify_mode(boost::asio::ssl::verify_client_once);
	  ssl_context_.use_certificate_file(crt_filename, ssl_context_t::pem);
	  ssl_context_.use_private_key_file(key_filename, ssl_context_t::pem);
	  ssl_context_.set_options(
		boost::asio::ssl::context::default_workarounds
		| boost::asio::ssl::context::no_sslv2
		| boost::asio::ssl::context::no_sslv3
	  );
	  return *this;
	}
	///use .pem file for SSL
	self_t& ssl_file(const std::string& pem_filename) {
	  use_ssl_ = true;
	  ssl_context_.set_verify_mode(boost::asio::ssl::verify_peer);
	  ssl_context_.set_verify_mode(boost::asio::ssl::verify_client_once);
	  ssl_context_.load_verify_file(pem_filename);
	  ssl_context_.set_options(
		boost::asio::ssl::context::default_workarounds
		| boost::asio::ssl::context::no_sslv2
		| boost::asio::ssl::context::no_sslv3
	  );
	  return *this;
	}
	self_t& ssl(boost::asio::ssl::context&& ctx) {
	  use_ssl_ = true;
	  ssl_context_ = std::move(ctx);
	  return *this;
	}
	bool use_ssl_{ false };
	ssl_context_t ssl_context_{ boost::asio::ssl::context::sslv23 };
#else
	template <typename T, typename ... Remain>
	self_t& ssl_file(T&&, Remain&&...) {
	  // We can't call .ssl() member function unless ENABLE_SSL is defined.
	  static_assert(
		// make static_assert dependent to T; always false
		std::is_base_of<T, void>::value,
		"Define ENABLE_SSL to enable ssl support.");
	  return *this;
	}
	template <typename T>
	self_t& ssl(T&&) {
	  // We can't call .ssl() member function unless ENABLE_SSL is defined.
	  static_assert(
		// make static_assert dependent to T; always false
		std::is_base_of<T, void>::value,
		"Define ENABLE_SSL to enable ssl support.");
	  return *this;
	}
#endif
	// middleware
	using context_t = detail::Ctx<Middlewares...>;
	template <typename T>
	typename T::Ctx& get_context(const Req& req) {
	  static_assert(spell::contains<T, Middlewares...>::value, "App doesn't have the specified middleware type.");
	  auto& ctx = *reinterpret_cast<context_t*>(req.middleware_context);
	  return ctx.template get<T>();
	}
	template <typename T>
	T& get_middleware() {
	  return utility::get_element_by_type<T, Middlewares...>(middlewares_);
	}
	///Wait until the server has properly started
	void wait_for_server_start() {
	  std::unique_lock<std::mutex> lock(start_mutex_);
	  if (server_started_)
		return;
	  cv_started_.wait(lock);
	}

  private:
	uint16_t port_ = DEFAULT_PORT;
	uint16_t concurrency_ = 1;
	std::string bindaddr_ = "0.0.0.0";
	Router router_;
	bool is_not_set_types = true;
#ifdef ENABLE_COMPRESSION
	compression::algorithm comp_algorithm_;
#endif
	std::tuple<Middlewares...> middlewares_;

#ifdef ENABLE_SSL
	std::unique_ptr<ssl_server_t> ssl_server_;
#endif
	std::unique_ptr<server_t> server_;

	std::vector<int> signals_{ SIGINT, SIGTERM };

	bool server_started_{ false };
	std::condition_variable cv_started_;
	std::mutex start_mutex_;
  };
  template <typename ... Middlewares>
  using App = Crow<Middlewares...>;
}