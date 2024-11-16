#include "crow.h"
#include "crow/monolithic_examples.h"


#if defined(BUILD_MONOLITHIC)
#define main	crow_example_catch_all_main
#endif

int main(void)
{
    crow::SimpleApp app;

    CROW_ROUTE(app, "/")
    ([]() {
        return "Hello";
    });

    //Setting a custom route for any URL that isn't defined, instead of a simple 404.
    CROW_CATCHALL_ROUTE(app)
    ([](crow::response& res) {
        if (res.code == 404)
        {
            res.body = "The URL does not seem to be correct.";
        }
        else if (res.code == 405)
        {
            res.body = "The HTTP method does not seem to be correct.";
        }
        res.end();
    });

    app.port(18080).run();

	// when we get here, we may assume failure as the server code above should run indefinitely.
	return EXIT_FAILURE;
}
