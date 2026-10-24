#include "lexer.h"

ngx_int_t
ngx_htaccess_lex(ngx_pool_t *pool, u_char *content, size_t len, ngx_htaccess_lexer_result_t *out)
{
    u_char *p, *end;
    ngx_htaccess_directive_t *current_dir = NULL;
    u_char *token_start = NULL;
    ngx_uint_t in_quotes = 0;
    ngx_uint_t escaped = 0;
    
    out->directives = ngx_array_create(pool, 16, sizeof(ngx_htaccess_directive_t));
    if (out->directives == NULL) {
        return NGX_ERROR;
    }

    p = content;
    end = content + len;

    while (p < end) {
        /* Skip leading whitespace */
        while (p < end && (*p == ' ' || *p == '\t' || *p == '\r')) {
            p++;
        }

        if (p >= end) {
            break;
        }

        /* Handle comments */
        if (*p == '#') {
            while (p < end && *p != '\n') {
                p++;
            }
            if (p < end) {
                p++; /* skip \n */
            }
            continue;
        }

        /* Handle end of line */
        if (*p == '\n') {
            current_dir = NULL;
            p++;
            continue;
        }

        /* We found a token */
        if (current_dir == NULL) {
            current_dir = ngx_array_push(out->directives);
            if (current_dir == NULL) {
                return NGX_ERROR;
            }
            current_dir->name.data = NULL;
            current_dir->name.len = 0;
            current_dir->args = ngx_array_create(pool, 4, sizeof(ngx_htaccess_token_t));
            if (current_dir->args == NULL) {
                return NGX_ERROR;
            }
        }

        in_quotes = 0;
        escaped = 0;
        
        if (*p == '"' || *p == '\'') {
            in_quotes = *p;
            p++;
        }
        
        token_start = p;
        
        /* Fast forward to end of token */
        while (p < end) {
            if (escaped) {
                escaped = 0;
                p++;
                continue;
            }
            if (*p == '\\') {
                escaped = 1;
                p++;
                continue;
            }
            if (in_quotes) {
                if (*p == in_quotes) {
                    break;
                }
            } else {
                if (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') {
                    break;
                }
            }
            p++;
        }

        /* Save token */
        if (current_dir->name.data == NULL) {
            current_dir->name.data = ngx_pnalloc(pool, p - token_start + 1);
            if (current_dir->name.data == NULL) return NGX_ERROR;
            ngx_memcpy(current_dir->name.data, token_start, p - token_start);
            current_dir->name.data[p - token_start] = '\0';
            current_dir->name.len = p - token_start;
        } else {
            ngx_htaccess_token_t *t = ngx_array_push(current_dir->args);
            if (t == NULL) return NGX_ERROR;
            t->value.data = ngx_pnalloc(pool, p - token_start + 1);
            if (t->value.data == NULL) return NGX_ERROR;
            ngx_memcpy(t->value.data, token_start, p - token_start);
            t->value.data[p - token_start] = '\0';
            t->value.len = p - token_start;
        }

        if (in_quotes && p < end && *p == in_quotes) {
            p++; /* Skip closing quote */
        }
        
        /* If not at EOL, loop will continue and skip whitespace. If EOL, it will process \n next iteration */
    }

    return NGX_OK;
}
