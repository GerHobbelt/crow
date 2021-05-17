#define CROW_MAIN
#define CROW_JSON_USE_MAP
#include "crow.h"



#if defined(BUILD_MONOLITHIC)
#define main()	crow_example_json_map_main()
#endif

int main()
{
    crow::SimpleApp app;

// simple json response using a map
// To see it in action enter {ip}:18080/json
// it shoud show amessage before zmessage despite adding zmessage first.
CROW_ROUTE(app, "/json")
([]{
    crow::json::wvalue x;
    x["zmessage"] = "Hello, World!";
    x["amessage"] = "Hello, World2!";
    return x;
});

// enables all log
app.loglevel(crow::LogLevel::Debug);

app.port(18080)
    .multithreaded()
    .run();

// when we get here, we may assume failure as the server code above should run indefinitely.
return EXIT_FAILURE;
}
