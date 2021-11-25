#include "crow.h"
#include "crow/monolithic_examples.h"


#if defined(BUILD_MONOLITHIC)
#define main	crow_hello_world_main
#endif

int main(void)
{
    crow::SimpleApp app;

    CROW_ROUTE(app, "/")
    ([]() {
        return "Hello, world!";
    });

    app.port(18080).run();

	// when we get here, we may assume failure as the server code above should run indefinitely.
	return EXIT_FAILURE;
}
