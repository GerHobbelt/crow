#define CROW_MAIN
#define CROW_ENABLE_COMPRESSION
#include "crow.h"
#include "crow/compression.h"

#if defined(BUILD_MONOLITHIC)
#define main()	crow_example_compression_main()
#endif

int main()
{
    crow::SimpleApp app;
    //crow::App<crow::CompressionGzip> app;

    CROW_ROUTE(app, "/hello")
    ([&](const crow::request&, crow::response& res){
        res.compressed = false;

        res.body = "Hello World! This is uncompressed!";
        res.end();
    });

    CROW_ROUTE(app, "/hello_compressed")
    ([](){
        return "Hello World! This is compressed by default!";
    });


    app.port(18080)
        .use_compression(crow::compression::algorithm::DEFLATE)
      //.use_compression(crow::compression::algorithm::GZIP)
        .loglevel(crow::LogLevel::Debug)
        .multithreaded()
        .run();

	// when we get here, we may assume failure as the server code above should run indefinitely.
	return EXIT_FAILURE;
}
