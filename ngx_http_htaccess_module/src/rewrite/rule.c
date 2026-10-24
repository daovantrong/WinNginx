#include "rule.h"
#include "condition.h"

static ngx_int_t
ngx_htaccess_interpolate(ngx_http_request_t *r, ngx_str_t *subst, ngx_str_t *uri_str, int *rule_captures, int *cond_captures, ngx_str_t *out)
{
    u_char *p, *dest;
    size_t len = subst->len;
    ngx_uint_t i;
    int n;

    /* First pass: calculate length */
    for (i = 0; i < subst->len; i++) {
        if (subst->data[i] == '$' && i + 1 < subst->len && subst->data[i+1] >= '0' && subst->data[i+1] <= '9') {
            n = subst->data[i+1] - '0';
            if (rule_captures[n * 2] != -1) {
                len += rule_captures[n * 2 + 1] - rule_captures[n * 2];
            }
            len -= 2; /* Remove $N */
            i++;
        }
        else if (subst->data[i] == '%' && i + 1 < subst->len && subst->data[i+1] >= '0' && subst->data[i+1] <= '9') {
            n = subst->data[i+1] - '0';
            if (cond_captures[n * 2] != -1) {
                /* We don't have the original string of condition here for length easily without passing it...
                 * Actually, cond_captures is meaningless without the matched string.
                 * To simplify for M4, we will ignore %N size calculation and substitution unless we track cond match string. 
                 * For now, we will just allocate a large buffer if there are references to avoid complex passing. */
            }
        }
    }

    /* Allocate max possible for now (M4 simplicity) */
    out->data = ngx_pnalloc(r->pool, len + 1024);
    if (out->data == NULL) {
        return NGX_ERROR;
    }

    dest = out->data;
    for (i = 0; i < subst->len; i++) {
        if (subst->data[i] == '$' && i + 1 < subst->len && subst->data[i+1] >= '0' && subst->data[i+1] <= '9') {
            n = subst->data[i+1] - '0';
            if (rule_captures[n * 2] != -1) {
                dest = ngx_copy(dest, uri_str->data + rule_captures[n * 2], rule_captures[n * 2 + 1] - rule_captures[n * 2]);
            }
            i++;
        }
        else if (subst->data[i] == '%' && i + 1 < subst->len && subst->data[i+1] >= '0' && subst->data[i+1] <= '9') {
            /* %N not fully supported without original cond test_string, just skip for now to prevent crash */
            i++;
        }
        else {
            *dest++ = subst->data[i];
        }
    }
    *dest = '\0';
    out->len = dest - out->data;

    return NGX_OK;
}

ngx_int_t
ngx_htaccess_execute_rules(ngx_http_request_t *r, ngx_array_t *rules, ngx_str_t *document_root, ngx_str_t *base_prefix)
{
    ngx_uint_t i;
    ngx_htaccess_rule_t *rs = rules->elts;
    int rule_captures[30];
    int cond_captures[30];
    ngx_str_t uri_to_match;
    ngx_int_t match;
    ngx_str_t new_uri;

    /* Strip base_prefix from r->uri to emulate per-dir context */
    uri_to_match = r->uri;
    if (uri_to_match.len >= base_prefix->len) {
        if (ngx_strncmp(uri_to_match.data, base_prefix->data, base_prefix->len) == 0) {
            uri_to_match.data += base_prefix->len;
            uri_to_match.len -= base_prefix->len;
            if (uri_to_match.len > 0 && uri_to_match.data[0] == '/') {
                uri_to_match.data++;
                uri_to_match.len--;
            }
        }
    }

    for (i = 0; i < rules->nelts; i++) {
        ngx_htaccess_rule_t *rule = &rs[i];

        if (rule->rc == NULL) {
            continue;
        }

        ngx_uint_t j;
        for (j = 0; j < 30; j++) rule_captures[j] = -1;

        match = ngx_regex_exec(rule->rc->regex, &uri_to_match, rule_captures, 30);
        if (match >= 0) {
            /* Rule matched. Evaluate conditions */
            if (ngx_htaccess_evaluate_conditions(r, rule->conditions, document_root, cond_captures) == NGX_OK) {
                
                /* Interpolate substitution */
                if (ngx_htaccess_interpolate(r, &rule->substitution, &uri_to_match, rule_captures, cond_captures, &new_uri) != NGX_OK) {
                    return NGX_HTTP_INTERNAL_SERVER_ERROR;
                }

                ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0, "htaccess: Rule matched! Rewriting %V -> %V", &uri_to_match, &new_uri);

                /* Handle Redirect [R] */
                if (rule->flags & HTACCESS_FLAG_R) {
                    ngx_table_elt_t *location = ngx_list_push(&r->headers_out.headers);
                    if (location == NULL) return NGX_HTTP_INTERNAL_SERVER_ERROR;
                    
                    location->hash = 1;
                    ngx_str_set(&location->key, "Location");
                    
                    /* If relative URI, we should prefix it with scheme://host, but Nginx handles relative redirects well.
                       Let's prepend '/' if it doesn't start with http or / */
                    location->value = new_uri;
                    
                    if (rule->redirect_code == 0) rule->redirect_code = 302;
                    
                    r->headers_out.location = location;
                    
                    /* Setting status will trigger redirect */
                    return rule->redirect_code;
                }

                /* Handle QSA */
                ngx_str_t final_args = r->args;
                if (rule->flags & HTACCESS_FLAG_QSA) {
                    /* Basic implementation: we'll just let Nginx handle args if we parsed '?' out of new_uri. */
                    u_char *qm = ngx_strlchr(new_uri.data, new_uri.data + new_uri.len, '?');
                    if (qm) {
                        /* new_uri has args */
                        ngx_str_t new_args;
                        new_args.data = qm + 1;
                        new_args.len = new_uri.data + new_uri.len - qm - 1;
                        new_uri.len = qm - new_uri.data;
                        
                        if (r->args.len > 0) {
                            /* Merge */
                            final_args.len = new_args.len + 1 + r->args.len;
                            final_args.data = ngx_pnalloc(r->pool, final_args.len);
                            if (final_args.data) {
                                u_char *p = ngx_copy(final_args.data, new_args.data, new_args.len);
                                *p++ = '&';
                                ngx_copy(p, r->args.data, r->args.len);
                            }
                        } else {
                            final_args = new_args;
                        }
                    }
                } else {
                    /* Discard original query string if substitution contains '?' */
                    u_char *qm = ngx_strlchr(new_uri.data, new_uri.data + new_uri.len, '?');
                    if (qm) {
                        final_args.data = qm + 1;
                        final_args.len = new_uri.data + new_uri.len - qm - 1;
                        new_uri.len = qm - new_uri.data;
                    }
                }

                /* Internal Redirect */
                /* Prepend base_prefix unless absolute path */
                ngx_str_t full_uri;
                if (new_uri.len > 0 && new_uri.data[0] == '/') {
                    full_uri = new_uri;
                } else {
                    full_uri.len = base_prefix->len + new_uri.len;
                    full_uri.data = ngx_pnalloc(r->pool, full_uri.len);
                    if (full_uri.data == NULL) return NGX_HTTP_INTERNAL_SERVER_ERROR;
                    u_char *p = ngx_copy(full_uri.data, base_prefix->data, base_prefix->len);
                    ngx_copy(p, new_uri.data, new_uri.len);
                }

                ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0, "htaccess: Internal redirect to %V (args: %V)", &full_uri, &final_args);
                
                ngx_http_internal_redirect(r, &full_uri, &final_args);
                return NGX_DONE;
            }
        }
    }

    return NGX_DECLINED;
}

ngx_int_t
ngx_htaccess_execute_redirects(ngx_http_request_t *r, ngx_array_t *redirects, ngx_str_t *base_prefix)
{
    ngx_uint_t i;
    ngx_htaccess_redirect_t *rs = redirects->elts;
    int captures[30];
    ngx_str_t uri_to_match;
    ngx_int_t match;
    ngx_str_t new_uri;

    /* For Redirect, we usually match against the original URI without stripping base_prefix, 
       but mod_alias matches against the URL-path (full absolute path).
       Let's use r->uri directly. */
    uri_to_match = r->uri;

    for (i = 0; i < redirects->nelts; i++) {
        ngx_htaccess_redirect_t *redir = &rs[i];

        if (redir->rc != NULL) {
            /* RedirectMatch */
            ngx_uint_t j;
            for (j = 0; j < 30; j++) captures[j] = -1;
            match = ngx_regex_exec(redir->rc->regex, &uri_to_match, captures, 30);
            if (match >= 0) {
                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "htaccess: RedirectMatch matched! captures: 0=%i, 1=%i, 2=%i, 3=%i", captures[0], captures[1], captures[2], captures[3]);
                
                int dummy_cond_captures[30];
                ngx_uint_t j;
                for (j = 0; j < 30; j++) dummy_cond_captures[j] = -1;
                
                if (ngx_htaccess_interpolate(r, &redir->url, &uri_to_match, captures, dummy_cond_captures, &new_uri) != NGX_OK) {
                    return NGX_HTTP_INTERNAL_SERVER_ERROR;
                }
                
                ngx_table_elt_t *location = ngx_list_push(&r->headers_out.headers);
                if (location == NULL) return NGX_HTTP_INTERNAL_SERVER_ERROR;
                location->hash = 1;
                ngx_str_set(&location->key, "Location");
                location->value = new_uri;
                r->headers_out.location = location;
                return redir->status;
            }
        } else {
            /* Redirect */
            if (uri_to_match.len >= redir->path.len && 
                ngx_strncmp(uri_to_match.data, redir->path.data, redir->path.len) == 0) {
                
                /* Simple prefix match */
                ngx_str_t suffix;
                suffix.data = uri_to_match.data + redir->path.len;
                suffix.len = uri_to_match.len - redir->path.len;
                
                new_uri.len = redir->url.len + suffix.len;
                new_uri.data = ngx_pnalloc(r->pool, new_uri.len);
                if (new_uri.data == NULL) return NGX_HTTP_INTERNAL_SERVER_ERROR;
                
                u_char *p = ngx_copy(new_uri.data, redir->url.data, redir->url.len);
                ngx_copy(p, suffix.data, suffix.len);
                
                ngx_table_elt_t *location = ngx_list_push(&r->headers_out.headers);
                if (location == NULL) return NGX_HTTP_INTERNAL_SERVER_ERROR;
                location->hash = 1;
                ngx_str_set(&location->key, "Location");
                location->value = new_uri;
                r->headers_out.location = location;
                return redir->status;
            }
        }
    }

    return NGX_DECLINED;
}
