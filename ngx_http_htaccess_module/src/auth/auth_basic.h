#ifndef _NGX_HTACCESS_AUTH_BASIC_H_INCLUDED_
#define _NGX_HTACCESS_AUTH_BASIC_H_INCLUDED_

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include "../rewrite/rewrite.h"

ngx_int_t ngx_htaccess_execute_auth_basic(ngx_http_request_t *r, ngx_htaccess_ast_t *ast);

#endif /* _NGX_HTACCESS_AUTH_BASIC_H_INCLUDED_ */
