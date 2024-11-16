// Test of a project containing more than 1 source file which includes Crow headers.
// The test succeeds if the project is linked successfully.
#include "crow.h"
#include "crow/monolithic_examples.h"


#if defined(BUILD_MONOLITHIC)
#define main	crow_test_multi_file_main
#endif

int main(void)
{
    crow::SimpleApp app;

    CROW_ROUTE(app, "/")
    ([]() {
        return "Hello, world!";
    });

    app.port(18080).run();
	return EXIT_SUCCESS;
}
