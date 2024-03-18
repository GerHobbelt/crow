
#pragma once

#if defined(BUILD_MONOLITHIC)

#ifdef __cplusplus
extern "C" {
#endif

extern int crow_example_basic_main(void);
extern int crow_example_catch_all_main(void);
extern int crow_example_chat_main(void);
extern int crow_example_compression_main(void);
extern int crow_example_json_map_main(void);
extern int crow_example_static_file_main(void);
extern int crow_example_vs_main(void);
extern int crow_example_with_all_main(void);
extern int crow_hello_world_main(void);
extern int crow_example_ssl_main(void);
extern int crow_example_ws_main(void);
extern int crow_mustache_main(void);
extern int crow_example_blueprint_main(void);
extern int crow_example_middleware_main(void);
extern int crow_test_multi_file_main(void);
extern int crow_test_external_definition_main(void);
extern int crow_example_session_main(void);
extern int crow_example_file_upload_main(void);

#ifdef __cplusplus
}
#endif

#endif
