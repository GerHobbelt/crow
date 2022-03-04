#include "crow.h"
#include "crow/monolithic_examples.h"


struct RequestLogger
{
    struct context
    {};

    void before_handle(crow::request& req, crow::response& /*res*/, context& /*ctx*/)
    {
        CROW_LOG_INFO << "Request to:" + req.url;
    }

    void after_handle(crow::request& /*req*/, crow::response& /*res*/, context& /*ctx*/)
    {}
};

// Per handler middleware has to extend ILocalMiddleware
// It is called only if enabled
struct SecretContentGuard : crow::ILocalMiddleware
{
    struct context
    {};

    void before_handle(crow::request& /*req*/, crow::response& res, context& /*ctx*/)
    {
        res.write("SECRET!");
        res.code = 403;
        res.end();
    }

    void after_handle(crow::request& /*req*/, crow::response& /*res*/, context& /*ctx*/)
    {}
};



#if defined(BUILD_MONOLITHIC)
#define main	crow_example_middleware_main
#endif

int main(void)
{
    // ALL middleware (including per handler) is listed
    crow::App<RequestLogger, SecretContentGuard> app;

    CROW_ROUTE(app, "/")
    ([]() {
        return "Hello, world!";
    });

    CROW_ROUTE(app, "/secret")
      // Enable SecretContentGuard for this handler
      .CROW_MIDDLEWARES(app, SecretContentGuard)([]() {
          return "";
      });

    app.port(18080).run();

    return 0;
}
