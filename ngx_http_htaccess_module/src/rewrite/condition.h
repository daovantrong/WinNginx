#ifndef _NGX_HTACCESS_REWRITE_CONDITION_H_INCLUDED_
#define _NGX_HTACCESS_REWRITE_CONDITION_H_INCLUDED_

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include "rewrite.h"

/* Evaluate an array of conditions for a rule */
ngx_int_t ngx_htaccess_evaluate_conditions(ngx_http_request_t *r, ngx_array_t *conditions, ngx_str_t *document_root, int *cond_captures);

#endif /* _NGX_HTACCESS_REWRITE_CONDITION_H_INCLUDED_ */
