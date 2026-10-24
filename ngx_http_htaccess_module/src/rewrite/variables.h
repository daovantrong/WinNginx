#ifndef _NGX_HTACCESS_REWRITE_VARIABLES_H_INCLUDED_
#define _NGX_HTACCESS_REWRITE_VARIABLES_H_INCLUDED_

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

/* Evaluate an Apache variable %{VAR} and return its ngx_str_t value */
ngx_int_t ngx_htaccess_evaluate_variable(ngx_http_request_t *r, ngx_str_t *apache_var, ngx_str_t *val);

#endif /* _NGX_HTACCESS_REWRITE_VARIABLES_H_INCLUDED_ */
