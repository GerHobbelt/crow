// test for #define CROW_ENABLE_SSL, which should've been set up in crow/settings.h
#define CROW_MAIN
#include "crow.h"

#if defined(BUILD_MONOLITHIC)
#define main()	crow_example_ssl_main()
#endif

int main()
{
#ifdef CROW_ENABLE_SSL

	crow::SimpleApp app;

    CROW_ROUTE(app, "/")
    ([]() {
        return "Hello world!";
    });

    app.port(18080).ssl_file("test.crt", "test.key").run();

    // Use .pem file
    //app.port(18080).ssl_file("test.pem").run();
    
    // Use custom context; see boost::asio::ssl::context
    /*
     * crow::ssl_context_t ctx;
     * ctx.set_verify_mode(...)
     *
     *   ... configuring ctx
     *
     *   app.port(18080).ssl(ctx).run();
     */

#else

	std::cerr << "\nERROR: SSL (OpenSSL) is not enabled in this build.\n\n";

#endif

	// when we get here, we may assume failure as the server code above should run indefinitely.
	return EXIT_FAILURE;
}
