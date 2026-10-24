#ifndef _NGX_HTACCESS_PARSER_PARSER_H_INCLUDED_
#define _NGX_HTACCESS_PARSER_PARSER_H_INCLUDED_

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include "lexer.h"
#include "../rewrite/rewrite.h"

/*
 * Compile Lexer tokens into AST representation
 */
ngx_int_t ngx_htaccess_parse_ast(ngx_pool_t *pool, ngx_htaccess_lexer_result_t *lexer_result, ngx_htaccess_ast_t *ast);

#endif /* _NGX_HTACCESS_PARSER_PARSER_H_INCLUDED_ */
