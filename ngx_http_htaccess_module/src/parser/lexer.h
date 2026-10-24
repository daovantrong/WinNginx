#ifndef _NGX_HTACCESS_PARSER_LEXER_H_INCLUDED_
#define _NGX_HTACCESS_PARSER_LEXER_H_INCLUDED_

#include <ngx_config.h>
#include <ngx_core.h>

/* A parsed token (argument) */
typedef struct {
    ngx_str_t value;
} ngx_htaccess_token_t;

/* A parsed directive line */
typedef struct {
    ngx_str_t     name;
    ngx_array_t  *args; /* Array of ngx_htaccess_token_t */
} ngx_htaccess_directive_t;

/* Lexer result containing array of directives */
typedef struct {
    ngx_array_t *directives; /* Array of ngx_htaccess_directive_t */
} ngx_htaccess_lexer_result_t;

/* Tokenize htaccess content 
 * Allocated from the given pool.
 * returns NGX_OK on success, NGX_ERROR on failure.
 */
ngx_int_t ngx_htaccess_lex(ngx_pool_t *pool, u_char *content, size_t len, ngx_htaccess_lexer_result_t *out);

#endif /* _NGX_HTACCESS_PARSER_LEXER_H_INCLUDED_ */
