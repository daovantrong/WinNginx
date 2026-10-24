#include "parser.h"

static ngx_int_t
ngx_htaccess_parse_flags(ngx_str_t *flags_str, ngx_htaccess_rule_t *rule)
{
    u_char *p, *start, *end;
    ngx_str_t flag;

    /* flags_str is something like "[L,NC,R=301]" */
    if (flags_str->len < 2 || flags_str->data[0] != '[' || flags_str->data[flags_str->len - 1] != ']') {
        return NGX_OK; /* Not a flags block, maybe a normal arg but should be flags */
    }

    p = flags_str->data + 1;
    end = flags_str->data + flags_str->len - 1;

    while (p < end) {
        start = p;
        while (p < end && *p != ',') {
            p++;
        }

        flag.data = start;
        flag.len = p - start;

        if (flag.len == 1 && flag.data[0] == 'L') {
            rule->flags |= HTACCESS_FLAG_L;
        } else if (flag.len == 2 && ngx_strncasecmp(flag.data, (u_char *)"NC", 2) == 0) {
            rule->flags |= HTACCESS_FLAG_NC;
        } else if (flag.len == 3 && ngx_strncasecmp(flag.data, (u_char *)"QSA", 3) == 0) {
            rule->flags |= HTACCESS_FLAG_QSA;
        } else if (flag.len == 1 && flag.data[0] == 'R') {
            rule->flags |= HTACCESS_FLAG_R;
            rule->redirect_code = 302;
        } else if (flag.len > 2 && flag.data[0] == 'R' && flag.data[1] == '=') {
            rule->flags |= HTACCESS_FLAG_R;
            rule->redirect_code = ngx_atoi(flag.data + 2, flag.len - 2);
            if (rule->redirect_code == (ngx_uint_t) NGX_ERROR) {
                rule->redirect_code = 302;
            }
        }

        if (p < end && *p == ',') {
            p++;
        }
    }

    return NGX_OK;
}

static ngx_int_t
ngx_htaccess_parse_ast_recursive(ngx_pool_t *pool, ngx_htaccess_directive_t *dirs, ngx_uint_t *idx, ngx_uint_t nelts, ngx_htaccess_ast_t *ast, ngx_uint_t skip_mode)
{
    ngx_array_t *pending_conds;
    u_char errstr[NGX_MAX_CONF_ERRSTR];

    pending_conds = ngx_array_create(pool, 4, sizeof(ngx_htaccess_cond_t));
    if (pending_conds == NULL) return NGX_ERROR;

    for (; *idx < nelts; (*idx)++) {
        ngx_htaccess_directive_t *dir = &dirs[*idx];
        ngx_htaccess_token_t *args = dir->args->elts;
        
        /* If in skip_mode, we just want to find the matching close tag */
        if (dir->name.len > 1 && dir->name.data[0] == '<' && dir->name.data[1] == '/') {
            return NGX_OK;
        }

        if (dir->name.len == 9 && ngx_strncasecmp(dir->name.data, (u_char *)"<IfModule", 9) == 0) {
            ngx_uint_t condition_met = 1;
            if (dir->args->nelts > 0) {
                ngx_str_t mod = args[0].value;
                if (mod.len > 0 && mod.data[mod.len - 1] == '>') mod.len--;
                if (mod.len > 0 && mod.data[0] == '!') condition_met = 0; /* assume all supported unless ! */
            }
            
            (*idx)++;
            if (ngx_htaccess_parse_ast_recursive(pool, dirs, idx, nelts, ast, skip_mode || !condition_met) != NGX_OK) {
                return NGX_ERROR;
            }
            continue;
        }

        if (dir->name.len == 11 && ngx_strncasecmp(dir->name.data, (u_char *)"<FilesMatch", 11) == 0) {
            (*idx)++;
            if (skip_mode) {
                ngx_htaccess_parse_ast_recursive(pool, dirs, idx, nelts, ast, 1);
                continue;
            }

            ngx_htaccess_block_t *block = ngx_array_push(ast->blocks);
            if (block == NULL) return NGX_ERROR;
            
            ngx_str_set(&block->type, "FilesMatch");
            if (dir->args->nelts > 0) {
                block->arg = args[0].value;
                if (block->arg.len > 0 && block->arg.data[block->arg.len - 1] == '>') block->arg.len--;
            } else {
                ngx_str_null(&block->arg);
            }
            
            block->ast = ngx_pcalloc(pool, sizeof(ngx_htaccess_ast_t));
            if (block->ast == NULL) return NGX_ERROR;
            
            /* Initialize nested AST */
            block->ast->rewrite_engine = ast->rewrite_engine;
            block->ast->authz.default_state = HTACCESS_AUTHZ_NOT_SET;
            block->ast->options.indexes = -1;
            block->ast->authz.rules = ngx_array_create(pool, 4, sizeof(ngx_htaccess_authz_rule_t));
            block->ast->rules = ngx_array_create(pool, 4, sizeof(ngx_htaccess_rule_t));
            block->ast->redirects = ngx_array_create(pool, 2, sizeof(ngx_htaccess_redirect_t));
            block->ast->blocks = ngx_array_create(pool, 4, sizeof(ngx_htaccess_block_t));
            block->ast->directory_index = ngx_array_create(pool, 2, sizeof(ngx_str_t));
            ngx_str_null(&block->ast->auth.auth_type);
            ngx_str_null(&block->ast->auth.auth_name);
            ngx_str_null(&block->ast->auth.auth_user_file);
            block->ast->auth.require_valid_user = 0;

            /* Compile Block Regex */
            block->rc = ngx_pcalloc(pool, sizeof(ngx_regex_compile_t));
            if (block->rc) {
                block->rc->pattern = block->arg;
                block->rc->pool = pool;
                block->rc->err.len = NGX_MAX_CONF_ERRSTR;
                block->rc->err.data = errstr;
                block->rc->options = NGX_REGEX_CASELESS;
                ngx_regex_compile(block->rc);
            }
            
            if (ngx_htaccess_parse_ast_recursive(pool, dirs, idx, nelts, block->ast, 0) != NGX_OK) {
                return NGX_ERROR;
            }
            continue;
        }

        if (skip_mode) continue;

        if (dir->name.len == 13 && ngx_strncasecmp(dir->name.data, (u_char *)"RewriteEngine", 13) == 0) {
            if (dir->args->nelts > 0) {
                if (args[0].value.len == 2 && ngx_strncasecmp(args[0].value.data, (u_char *)"On", 2) == 0) {
                    ast->rewrite_engine = 1;
                } else {
                    ast->rewrite_engine = 0;
                }
            }
        }
        else if (dir->name.len == 7 && ngx_strncasecmp(dir->name.data, (u_char *)"Require", 7) == 0) {
            if (dir->args->nelts >= 1) {
                if (dir->args->nelts >= 2 && ngx_strncasecmp(args[0].value.data, (u_char *)"all", 3) == 0) {
                    if (ngx_strncasecmp(args[1].value.data, (u_char *)"granted", 7) == 0) {
                        ast->authz.default_state = HTACCESS_AUTHZ_GRANTED;
                    } else if (ngx_strncasecmp(args[1].value.data, (u_char *)"denied", 6) == 0) {
                        ast->authz.default_state = HTACCESS_AUTHZ_DENIED;
                    }
                } else if (dir->args->nelts >= 2 && ngx_strncasecmp(args[0].value.data, (u_char *)"ip", 2) == 0) {
                    ngx_uint_t k;
                    for (k = 1; k < dir->args->nelts; k++) {
                        ngx_htaccess_authz_rule_t *rule = ngx_array_push(ast->authz.rules);
                        if (rule) {
                            if (ngx_ptocidr(&args[k].value, &rule->cidr) == NGX_OK) {
                                rule->action = HTACCESS_AUTHZ_GRANTED;
                            } else {
                                ast->authz.rules->nelts--; /* invalid IP, rollback */
                            }
                        }
                    }
                } else if (ngx_strncasecmp(args[0].value.data, (u_char *)"valid-user", 10) == 0) {
                    ast->auth.require_valid_user = 1;
                }
            }
        }
        else if (dir->name.len == 5 && ngx_strncasecmp(dir->name.data, (u_char *)"Allow", 5) == 0) {
            if (dir->args->nelts >= 2 && ngx_strncasecmp(args[0].value.data, (u_char *)"from", 4) == 0) {
                if (ngx_strncasecmp(args[1].value.data, (u_char *)"all", 3) == 0) {
                    ast->authz.default_state = HTACCESS_AUTHZ_GRANTED;
                } else {
                    ngx_uint_t k;
                    for (k = 1; k < dir->args->nelts; k++) {
                        ngx_htaccess_authz_rule_t *rule = ngx_array_push(ast->authz.rules);
                        if (rule) {
                            if (ngx_ptocidr(&args[k].value, &rule->cidr) == NGX_OK) {
                                rule->action = HTACCESS_AUTHZ_GRANTED;
                            } else {
                                ast->authz.rules->nelts--; /* invalid IP, rollback */
                            }
                        }
                    }
                }
            }
        }
        else if (dir->name.len == 4 && ngx_strncasecmp(dir->name.data, (u_char *)"Deny", 4) == 0) {
            if (dir->args->nelts >= 2 && ngx_strncasecmp(args[0].value.data, (u_char *)"from", 4) == 0) {
                if (ngx_strncasecmp(args[1].value.data, (u_char *)"all", 3) == 0) {
                    ast->authz.default_state = HTACCESS_AUTHZ_DENIED;
                } else {
                    ngx_uint_t k;
                    for (k = 1; k < dir->args->nelts; k++) {
                        ngx_htaccess_authz_rule_t *rule = ngx_array_push(ast->authz.rules);
                        if (rule) {
                            if (ngx_ptocidr(&args[k].value, &rule->cidr) == NGX_OK) {
                                rule->action = HTACCESS_AUTHZ_DENIED;
                            } else {
                                ast->authz.rules->nelts--; /* invalid IP, rollback */
                            }
                        }
                    }
                }
            }
        }
        else if (dir->name.len == 6 && ngx_strncasecmp(dir->name.data, (u_char *)"Header", 6) == 0) {
            if (dir->args->nelts >= 2) {
                ngx_htaccess_header_t *hdr = ngx_array_push(ast->headers);
                if (hdr) {
                    if (ngx_strncasecmp(args[0].value.data, (u_char *)"set", 3) == 0) hdr->action = HTACCESS_HEADER_SET;
                    else if (ngx_strncasecmp(args[0].value.data, (u_char *)"append", 6) == 0) hdr->action = HTACCESS_HEADER_APPEND;
                    else if (ngx_strncasecmp(args[0].value.data, (u_char *)"add", 3) == 0) hdr->action = HTACCESS_HEADER_ADD;
                    else if (ngx_strncasecmp(args[0].value.data, (u_char *)"unset", 5) == 0) hdr->action = HTACCESS_HEADER_UNSET;
                    else if (ngx_strncasecmp(args[0].value.data, (u_char *)"echo", 4) == 0) hdr->action = HTACCESS_HEADER_ECHO;
                    else { ast->headers->nelts--; continue; } /* Invalid action */
                    
                    hdr->header = args[1].value;
                    if (dir->args->nelts >= 3 && hdr->action != HTACCESS_HEADER_UNSET) {
                        hdr->value = args[2].value;
                    } else {
                        ngx_str_null(&hdr->value);
                    }
                }
            }
        }
        else if (dir->name.len == 13 && ngx_strncasecmp(dir->name.data, (u_char *)"ExpiresActive", 13) == 0) {
            if (dir->args->nelts >= 1) {
                if (ngx_strncasecmp(args[0].value.data, (u_char *)"On", 2) == 0) ast->expires.active = 1;
                else ast->expires.active = 0;
            }
        }
        else if (dir->name.len == 14 && ngx_strncasecmp(dir->name.data, (u_char *)"ExpiresDefault", 14) == 0) {
            if (dir->args->nelts >= 1) {
                ast->expires.default_expires = args[0].value;
            }
        }
        else if (dir->name.len == 13 && ngx_strncasecmp(dir->name.data, (u_char *)"ExpiresByType", 13) == 0) {
            if (dir->args->nelts >= 2) {
                ngx_htaccess_expires_type_t *et = ngx_array_push(ast->expires.by_type);
                if (et) {
                    et->type = args[0].value;
                    et->value = args[1].value;
                }
            }
        }
        else if (dir->name.len == 11 && ngx_strncasecmp(dir->name.data, (u_char *)"RewriteCond", 11) == 0) {
            if (dir->args->nelts >= 2) {
                ngx_htaccess_cond_t *cond = ngx_array_push(pending_conds);
                if (cond == NULL) return NGX_ERROR;

                cond->test_string = args[0].value;
                cond->cond_pattern = args[1].value;
                cond->flags = 0;
                cond->rc = NULL;

                /* Handle flags if present */
                if (dir->args->nelts >= 3) {
                    /* Not fully parsing cond flags yet, but prepare structure */
                    if (ngx_strcasestrn(args[2].value.data, "NC", 2 - 1)) {
                        cond->flags |= HTACCESS_FLAG_NC;
                    }
                }

                /* Compile Regex if not a special filesystem check */
                if (cond->cond_pattern.len > 0 && cond->cond_pattern.data[0] != '-') {
                    cond->rc = ngx_pcalloc(pool, sizeof(ngx_regex_compile_t));
                    if (cond->rc == NULL) return NGX_ERROR;

                    /* Apache Cond pattern might start with '!' for NOT, skip it for regex */
                    ngx_str_t pat = cond->cond_pattern;
                    if (pat.data[0] == '!') {
                        pat.data++;
                        pat.len--;
                    }

                    cond->rc->pattern = pat;
                    cond->rc->pool = pool;
                    cond->rc->err.len = NGX_MAX_CONF_ERRSTR;
                    cond->rc->err.data = errstr;
                    cond->rc->options = (cond->flags & HTACCESS_FLAG_NC) ? NGX_REGEX_CASELESS : 0;

                    if (ngx_regex_compile(cond->rc) != NGX_OK) {
                        /* In strict mode, compilation failure is an error */
                        return NGX_ERROR;
                    }
                }
            }
        }
        else if (dir->name.len == 11 && ngx_strncasecmp(dir->name.data, (u_char *)"RewriteRule", 11) == 0) {
            if (dir->args->nelts >= 2) {
                ngx_htaccess_rule_t *rule = ngx_array_push(ast->rules);
                if (rule == NULL) return NGX_ERROR;

                rule->pattern = args[0].value;
                rule->substitution = args[1].value;
                rule->flags = 0;
                rule->redirect_code = 0;
                
                /* Assign pending conditions */
                rule->conditions = pending_conds;
                
                /* Create a new pending array for next rule */
                pending_conds = ngx_array_create(pool, 4, sizeof(ngx_htaccess_cond_t));
                if (pending_conds == NULL) return NGX_ERROR;

                /* Parse Flags */
                if (dir->args->nelts >= 3) {
                    ngx_htaccess_parse_flags(&args[2].value, rule);
                }

                /* Compile Regex */
                rule->rc = ngx_pcalloc(pool, sizeof(ngx_regex_compile_t));
                if (rule->rc == NULL) return NGX_ERROR;

                rule->rc->pattern = rule->pattern;
                rule->rc->pool = pool;
                rule->rc->err.len = NGX_MAX_CONF_ERRSTR;
                rule->rc->err.data = errstr;
                rule->rc->options = (rule->flags & HTACCESS_FLAG_NC) ? NGX_REGEX_CASELESS : 0;

                if (ngx_regex_compile(rule->rc) != NGX_OK) {
                    return NGX_ERROR;
                }
            }
        }
        else if (dir->name.len == 8 && ngx_strncasecmp(dir->name.data, (u_char *)"AuthType", 8) == 0) {
            if (dir->args->nelts >= 1) {
                ast->auth.auth_type = args[0].value;
            }
        }
        else if (dir->name.len == 8 && ngx_strncasecmp(dir->name.data, (u_char *)"AuthName", 8) == 0) {
            if (dir->args->nelts >= 1) {
                ast->auth.auth_name = args[0].value;
            }
        }
        else if (dir->name.len == 12 && ngx_strncasecmp(dir->name.data, (u_char *)"AuthUserFile", 12) == 0) {
            if (dir->args->nelts >= 1) {
                ast->auth.auth_user_file = args[0].value;
            }
        }
        else if (dir->name.len == 14 && ngx_strncasecmp(dir->name.data, (u_char *)"DirectoryIndex", 14) == 0) {
            ngx_uint_t k;
            for (k = 0; k < dir->args->nelts; k++) {
                ngx_str_t *idx_name = ngx_array_push(ast->directory_index);
                if (idx_name) *idx_name = args[k].value;
            }
        }
        else if (dir->name.len == 7 && ngx_strncasecmp(dir->name.data, (u_char *)"Options", 7) == 0) {
            ngx_uint_t k;
            for (k = 0; k < dir->args->nelts; k++) {
                if (ngx_strncasecmp(args[k].value.data, (u_char *)"+Indexes", 8) == 0) {
                    ast->options.indexes = 1;
                } else if (ngx_strncasecmp(args[k].value.data, (u_char *)"-Indexes", 8) == 0) {
                    ast->options.indexes = 0;
                } else if (ngx_strncasecmp(args[k].value.data, (u_char *)"Indexes", 7) == 0) {
                    ast->options.indexes = 1;
                }
            }
        }
        else if (dir->name.len == 13 && ngx_strncasecmp(dir->name.data, (u_char *)"ErrorDocument", 13) == 0) {
            if (dir->args->nelts >= 2) {
                ngx_htaccess_err_doc_t *err_doc = ngx_array_push(ast->error_documents);
                if (err_doc) {
                    err_doc->status = ngx_atoi(args[0].value.data, args[0].value.len);
                    err_doc->value = args[1].value;
                }
            }
        }
        else if (dir->name.len == 8 && ngx_strncasecmp(dir->name.data, (u_char *)"Redirect", 8) == 0) {
            if (dir->args->nelts >= 2) {
                ngx_htaccess_redirect_t *redir = ngx_array_push(ast->redirects);
                if (redir) {
                    ngx_uint_t arg_idx = 0;
                    redir->status = 302; /* Default */
                    redir->rc = NULL;
                    
                    /* Optional status code like 301, temp, permanent */
                    if (dir->args->nelts >= 3) {
                        if (args[0].value.len == 3 && args[0].value.data[0] >= '3' && args[0].value.data[0] <= '5') {
                            redir->status = ngx_atoi(args[0].value.data, args[0].value.len);
                        } else if (ngx_strncasecmp(args[0].value.data, (u_char *)"permanent", 9) == 0) {
                            redir->status = 301;
                        } else if (ngx_strncasecmp(args[0].value.data, (u_char *)"temp", 4) == 0) {
                            redir->status = 302;
                        }
                        arg_idx++;
                    }
                    redir->path = args[arg_idx].value;
                    redir->url = args[arg_idx + 1].value;
                }
            }
        }
        else if (dir->name.len == 13 && ngx_strncasecmp(dir->name.data, (u_char *)"RedirectMatch", 13) == 0) {
            if (dir->args->nelts >= 2) {
                ngx_htaccess_redirect_t *redir = ngx_array_push(ast->redirects);
                if (redir) {
                    ngx_uint_t arg_idx = 0;
                    redir->status = 302; /* Default */
                    
                    if (dir->args->nelts >= 3) {
                        if (args[0].value.len == 3 && args[0].value.data[0] >= '3' && args[0].value.data[0] <= '5') {
                            redir->status = ngx_atoi(args[0].value.data, args[0].value.len);
                        } else if (ngx_strncasecmp(args[0].value.data, (u_char *)"permanent", 9) == 0) {
                            redir->status = 301;
                        } else if (ngx_strncasecmp(args[0].value.data, (u_char *)"temp", 4) == 0) {
                            redir->status = 302;
                        }
                        arg_idx++;
                    }
                    redir->path = args[arg_idx].value;
                    redir->url = args[arg_idx + 1].value;

                    /* Compile Regex */
                    redir->rc = ngx_pcalloc(pool, sizeof(ngx_regex_compile_t));
                    if (redir->rc) {
                        redir->rc->pattern = redir->path;
                        redir->rc->pool = pool;
                        redir->rc->err.len = NGX_MAX_CONF_ERRSTR;
                        redir->rc->err.data = errstr;
                        redir->rc->options = 0;
                        if (ngx_regex_compile(redir->rc) != NGX_OK) {
                            redir->rc = NULL; /* Ignore if invalid */
                        }
                    }
                }
            }
        }
    }

    return NGX_OK;
}

ngx_int_t
ngx_htaccess_parse_ast(ngx_pool_t *pool, ngx_htaccess_lexer_result_t *lexer_result, ngx_htaccess_ast_t *ast)
{
    ngx_uint_t idx = 0;

    ast->rewrite_engine = 0; /* Default Off */
    ast->rules = ngx_array_create(pool, 4, sizeof(ngx_htaccess_rule_t));
    ast->redirects = ngx_array_create(pool, 4, sizeof(ngx_htaccess_redirect_t));
    ast->authz.default_state = HTACCESS_AUTHZ_NOT_SET;
    ast->authz.rules = ngx_array_create(pool, 2, sizeof(ngx_htaccess_authz_rule_t));
    ast->directory_index = ngx_array_create(pool, 2, sizeof(ngx_str_t));
    ast->blocks = ngx_array_create(pool, 2, sizeof(ngx_htaccess_block_t));
    ast->options.indexes = -1; /* -1 for UNSET */
    ast->error_documents = ngx_array_create(pool, 2, sizeof(ngx_htaccess_err_doc_t));
    ast->headers = ngx_array_create(pool, 4, sizeof(ngx_htaccess_header_t));
    ast->expires.by_type = ngx_array_create(pool, 4, sizeof(ngx_htaccess_expires_type_t));
    ast->expires.active = 2; /* Unset */
    
    ngx_str_null(&ast->auth.auth_type);
    ngx_str_null(&ast->auth.auth_name);
    ngx_str_null(&ast->auth.auth_user_file);
    ast->auth.require_valid_user = 0;
    
    if (ast->rules == NULL || ast->authz.rules == NULL) {
        return NGX_ERROR;
    }

    return ngx_htaccess_parse_ast_recursive(pool, lexer_result->directives->elts, &idx, lexer_result->directives->nelts, ast, 0);
}
