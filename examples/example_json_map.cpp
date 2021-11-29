#define CROW_JSON_USE_MAP
#include "crow.h"
#include "crow/monolithic_examples.h"



#if defined(BUILD_MONOLITHIC)
#define main	crow_example_json_map_main
#endif

int main(void)
{
    crow::SimpleApp app;

    // simple json response using a map
    // To see it in action enter {ip}:18080/json
    // it should show `amessage` before `zmessage` despite adding `zmessage` first.
    CROW_ROUTE(app, "/json")
    ([] {
        crow::json::wvalue x({{"zmessage", "Hello, World!"},
                              {"amessage", "Hello, World2!"}});
        return x;
    });

    app.port(18080)
      .multithreaded()
      .run();

// when we get here, we may assume failure as the server code above should run indefinitely.
return EXIT_FAILURE;
}
