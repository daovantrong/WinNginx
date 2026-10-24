#include "condition.h"
#include "variables.h"

ngx_int_t
ngx_htaccess_evaluate_conditions(ngx_http_request_t *r, ngx_array_t *conditions, ngx_str_t *document_root, int *cond_captures)
{
    ngx_uint_t i;
    ngx_htaccess_cond_t *conds = conditions->elts;
    ngx_str_t val;
    ngx_int_t rc, match;
    ngx_str_t filepath;
    ngx_file_info_t fi;

    /* Pre-fill cond_captures with -1 for safety */
    for (i = 0; i < 30; i++) {
        cond_captures[i] = -1;
    }

    if (conditions->nelts == 0) {
        return NGX_OK;
    }

    for (i = 0; i < conditions->nelts; i++) {
        /* Evaluate test_string */
        if (conds[i].test_string.len > 0 && conds[i].test_string.data[0] == '%') {
            if (ngx_htaccess_evaluate_variable(r, &conds[i].test_string, &val) != NGX_OK) {
                val.len = 0;
                val.data = (u_char *)"";
            }
        } else {
            val = conds[i].test_string;
        }

        match = 0;

        /* Filesystem checks */
        if (conds[i].cond_pattern.len >= 2 && conds[i].cond_pattern.data[0] == '-') {
            u_char type = conds[i].cond_pattern.data[1];
            
            filepath.len = document_root->len + val.len + 1;
            filepath.data = ngx_pnalloc(r->pool, filepath.len);
            if (filepath.data) {
                u_char *p = ngx_copy(filepath.data, document_root->data, document_root->len);
                if (document_root->data[document_root->len - 1] != '/' && 
                    document_root->data[document_root->len - 1] != '\\' &&
                    val.len > 0 && val.data[0] != '/' && val.data[0] != '\\') {
                    *p++ = '/';
                }
                p = ngx_copy(p, val.data, val.len);
                *p = '\0';
                filepath.len = p - filepath.data;

                if (ngx_file_info(filepath.data, &fi) == NGX_FILE_ERROR) {
                    match = 0; /* File/dir not found */
                } else {
                    if (type == 'f' && ngx_is_file(&fi)) {
                        match = 1;
                    } else if (type == 'd' && ngx_is_dir(&fi)) {
                        match = 1;
                    } else if (type == 's' && ngx_is_file(&fi) && ngx_file_size(&fi) > 0) {
                        match = 1;
                    }
                }
            }
        } 
        else if (conds[i].cond_pattern.len >= 3 && conds[i].cond_pattern.data[0] == '!' && conds[i].cond_pattern.data[1] == '-') {
            u_char type = conds[i].cond_pattern.data[2];
            match = 1; /* Negation implies true until proven false */
            
            filepath.len = document_root->len + val.len + 1;
            filepath.data = ngx_pnalloc(r->pool, filepath.len);
            if (filepath.data) {
                u_char *p = ngx_copy(filepath.data, document_root->data, document_root->len);
                if (document_root->data[document_root->len - 1] != '/' && 
                    document_root->data[document_root->len - 1] != '\\' &&
                    val.len > 0 && val.data[0] != '/' && val.data[0] != '\\') {
                    *p++ = '/';
                }
                p = ngx_copy(p, val.data, val.len);
                *p = '\0';
                filepath.len = p - filepath.data;

                if (ngx_file_info(filepath.data, &fi) != NGX_FILE_ERROR) {
                    if (type == 'f' && ngx_is_file(&fi)) {
                        match = 0;
                    } else if (type == 'd' && ngx_is_dir(&fi)) {
                        match = 0;
                    } else if (type == 's' && ngx_is_file(&fi) && ngx_file_size(&fi) > 0) {
                        match = 0;
                    }
                }
            }
        }
        else if (conds[i].rc != NULL) {
            /* PCRE match */
            ngx_uint_t j;
            for (j = 0; j < 30; j++) cond_captures[j] = -1;

            rc = ngx_regex_exec(conds[i].rc->regex, &val, cond_captures, 30);
            if (rc >= 0) {
                match = 1;
            } else if (rc == NGX_REGEX_NO_MATCHED) {
                match = 0;
            } else {
                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "htaccess: regex exec failed");
                return NGX_ERROR;
            }

            /* Negation flag handling (if ! was in pattern) */
            if (conds[i].cond_pattern.data[0] == '!') {
                match = !match;
            }
        }

        if (!match) {
            ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0, "htaccess: condition failed: test='%V' (evaluated to '%V') pattern='%V'", &conds[i].test_string, &val, &conds[i].cond_pattern);
            return NGX_DECLINED; /* Condition failed */
        } else {
            ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0, "htaccess: condition passed: test='%V' (evaluated to '%V') pattern='%V'", &conds[i].test_string, &val, &conds[i].cond_pattern);
        }
    }

    return NGX_OK;
}
