#ifndef _NGX_HTACCESS_REWRITE_RULE_H_INCLUDED_
#define _NGX_HTACCESS_REWRITE_RULE_H_INCLUDED_

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include "rewrite.h"

ngx_int_t ngx_htaccess_evaluate_conditions(ngx_http_request_t *r, ngx_array_t *conditions, ngx_str_t *document_root, int *cond_captures);
ngx_int_t ngx_htaccess_execute_rules(ngx_http_request_t *r, ngx_array_t *rules, ngx_str_t *document_root, ngx_str_t *base_prefix);
ngx_int_t ngx_htaccess_execute_redirects(ngx_http_request_t *r, ngx_array_t *redirects, ngx_str_t *base_prefix);

#endif /* _NGX_HTACCESS_REWRITE_RULE_H_INCLUDED_ */
