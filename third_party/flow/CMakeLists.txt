add_executable(flow flow.c flow_parser.c flow_renderer.c flow_url.c flow_formatter.c flow_utils.c flow_emoji.c flow_syntax_highlighter.c flow_html.c flow_content.c)
target_link_libraries(flow common)

# 检查是否有 libcurl
find_library(CURL_LIBRARY curl)
if(CURL_LIBRARY)
    target_link_libraries(flow ${CURL_LIBRARY})
    target_compile_definitions(flow PRIVATE HAVE_LIBCURL)
    message(STATUS "Found libcurl: ${CURL_LIBRARY}")
else()
    message(WARNING "libcurl not found, URL support will be disabled")
endif()

# 安装目标（与 dsa 保持一致）
install(TARGETS flow DESTINATION bin)

