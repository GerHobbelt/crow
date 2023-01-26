// Testing whether crow routes can be defined in an external function.
#include "crow.h"
#include "crow/monolithic_examples.h"


void define_endpoints(crow::SimpleApp& app)
{
    CROW_ROUTE(app, "/")
    ([]() {
        return "Hello, world!";
    });
    CROW_WEBSOCKET_ROUTE(app, "/ws")
      .onaccept([&](const crow::request&, void**) {
          return true;
      })
      .onopen([](crow::websocket::connection&) {})
      .onclose([](crow::websocket::connection&, const std::string&) {});
}


#if defined(BUILD_MONOLITHIC)
#define main	crow_test_external_definition_main
#endif

int main(void)
{
    crow::SimpleApp app;

    define_endpoints(app);

    app.port(18080).run();
	return 0;
}
