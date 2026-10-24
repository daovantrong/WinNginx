#ifndef _NGX_HTTP_HTACCESS_MODULE_H_INCLUDED_
#define _NGX_HTTP_HTACCESS_MODULE_H_INCLUDED_

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

typedef struct {
    ngx_flag_t   enable;
    ngx_str_t    filename;
    ngx_uint_t   max_depth;
    size_t       max_file_size;
} ngx_http_htaccess_loc_conf_t;

typedef struct {
    ngx_uint_t            redirects;
    struct ngx_htaccess_ast_s  **asts;
    ngx_uint_t            num_asts;
} ngx_http_htaccess_ctx_t;

extern ngx_module_t ngx_http_htaccess_module;

#endif /* _NGX_HTTP_HTACCESS_MODULE_H_INCLUDED_ */
