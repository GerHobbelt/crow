#define CROW_MAIN
#include "crow.h"


#if defined(BUILD_MONOLITHIC)
#define main()	crow_example_catch_all_main()
#endif

int main()
{
    crow::SimpleApp app;

    CROW_ROUTE(app, "/")([](){return "Hello";});

    //Setting a custom route for any URL that isn't defined, instead of a simple 404.
    CROW_CATCHALL_ROUTE(app)
            ([](crow::response& res) {
                res.body = "The URL does not seem to be correct.";
                res.end();
            });

    app.port(18080).run();

	// when we get here, we may assume failure as the server code above should run indefinitely.
	return EXIT_FAILURE;
}
