#include "ngx_http_htaccess_module.h"
#include "filesystem/file.h"
#include "parser/lexer.h"
#include "cache/cache.h"
#include "rewrite/rule.h"
#include "auth/auth_basic.h"

static ngx_htaccess_cache_t htaccess_cache;

static void *ngx_http_htaccess_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_htaccess_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);
static ngx_int_t ngx_http_htaccess_init(ngx_conf_t *cf);

static ngx_http_output_header_filter_pt  ngx_http_next_header_filter;

/* External Autoindex Module Dependencies */
extern ngx_module_t ngx_http_autoindex_module;

typedef struct {
    ngx_flag_t     enable;
    ngx_uint_t     format;
    ngx_flag_t     localtime;
    ngx_flag_t     exact_size;
} ngx_http_autoindex_loc_conf_t;



static u_short *
ngx_htaccess_utf8_to_utf16(ngx_pool_t *pool, u_char *utf8)
{
    int len = MultiByteToWideChar(CP_UTF8, 0, (LPCCH)utf8, -1, NULL, 0);
    if (len == 0) return NULL;
    u_short *utf16 = ngx_pcalloc(pool, len * sizeof(u_short));
    if (utf16 == NULL) return NULL;
    MultiByteToWideChar(CP_UTF8, 0, (LPCCH)utf8, -1, (LPWSTR)utf16, len);
    return utf16;
}

static ngx_command_t  ngx_http_htaccess_commands[] = {

    { ngx_string("htaccess"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_htaccess_loc_conf_t, enable),
      NULL },

    { ngx_string("htaccess_filename"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_htaccess_loc_conf_t, filename),
      NULL },

      ngx_null_command
};


static ngx_http_module_t  ngx_http_htaccess_module_ctx = {
    NULL,                                  /* preconfiguration */
    ngx_http_htaccess_init,                /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    ngx_http_htaccess_create_loc_conf,     /* create location configuration */
    ngx_http_htaccess_merge_loc_conf       /* merge location configuration */
};


ngx_module_t  ngx_http_htaccess_module = {
    NGX_MODULE_V1,
    &ngx_http_htaccess_module_ctx,         /* module context */
    ngx_http_htaccess_commands,            /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};

static ngx_int_t
ngx_http_htaccess_handler(ngx_http_request_t *r)
{
    ngx_http_htaccess_loc_conf_t  *hlcf;
    size_t                         root_len;
    ngx_str_t                      path, normalized_path;
    ngx_htaccess_file_info_t       fi;
    ngx_int_t                      rc;
    ngx_file_t                     file;
    ssize_t                        n;
    u_char                        *buf;

    hlcf = ngx_http_get_module_loc_conf(r, ngx_http_htaccess_module);
    
    if (!hlcf->enable) {
        return NGX_DECLINED;
    }

    if (hlcf->filename.len == 0) {
        return NGX_DECLINED;
    }

    /* Map URI to physical path to find document root */
    if (ngx_http_map_uri_to_path(r, &path, &root_len, 0) == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    /* For Milestone 1 & 2, check if .htaccess exists at the document root */
    path.len = root_len;

    /* Construct the full path to .htaccess */
    path.data[path.len] = '\0';
    
    size_t need_slash = 0;
    if (path.data[path.len - 1] != '/' && path.data[path.len - 1] != '\\') {
        need_slash = 1;
    }

    /* Using Nginx memory pool to build the path safely */
    u_char *htpath = ngx_pcalloc(r->pool, path.len + need_slash + hlcf->filename.len + 1);
    if (htpath == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    
    ngx_memcpy(htpath, path.data, path.len);
    if (need_slash) {
        htpath[path.len] = '\\';
    }
    ngx_memcpy(htpath + path.len + need_slash, hlcf->filename.data, hlcf->filename.len);
    htpath[path.len + need_slash + hlcf->filename.len] = '\0';
    
    ngx_str_t search_path = { path.len + need_slash + hlcf->filename.len, htpath };

    if (ngx_htaccess_path_normalize(r->pool, &search_path, &normalized_path) != NGX_OK) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    rc = ngx_htaccess_file_stat(&normalized_path, &fi);
    if (rc == NGX_DECLINED) {
        /* No .htaccess file found */
        return NGX_DECLINED;
    }

    if (rc == NGX_ERROR) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "htaccess: stat failed on %V", &normalized_path);
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    if (!fi.is_file) {
        return NGX_DECLINED;
    }

    /* Bypass cache entirely to avoid multithreading crashes on Windows */
    ngx_htaccess_ast_t *parsed_ast = ngx_pcalloc(r->pool, sizeof(ngx_htaccess_ast_t));
    if (parsed_ast == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    ngx_memzero(&file, sizeof(ngx_file_t));
    file.name = normalized_path;
    file.log = r->connection->log;
    
    file.fd = ngx_open_file(normalized_path.data, NGX_FILE_RDONLY, NGX_FILE_OPEN, 0);

    if (file.fd == NGX_INVALID_FILE) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, ngx_errno, "htaccess: open() failed on %V", &normalized_path);
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    buf = ngx_pcalloc(r->pool, fi.size + 1);
    if (buf == NULL) {
        ngx_close_file(file.fd);
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    n = ngx_read_file(&file, buf, fi.size, 0);
    ngx_close_file(file.fd);

    if (n == NGX_ERROR) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, ngx_errno, "htaccess: read() failed on %V", &normalized_path);
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    buf[n] = '\0';

    ngx_htaccess_lexer_result_t lex_res;
    if (ngx_htaccess_lex(r->pool, buf, n, &lex_res) != NGX_OK) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "htaccess: lexer error in %V", &normalized_path);
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    if (ngx_htaccess_parse_ast(r->pool, &lex_res, parsed_ast) != NGX_OK) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "htaccess: parser error in %V", &normalized_path);
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    /* 
     * Block Context Execution (<FilesMatch>)
     */
    ngx_htaccess_ast_t *active_asts[16];
    ngx_uint_t num_asts = 0;
    
    active_asts[num_asts++] = parsed_ast;

    if (parsed_ast->blocks && parsed_ast->blocks->nelts > 0) {
        ngx_htaccess_block_t *blocks = parsed_ast->blocks->elts;
        ngx_uint_t b;
        for (b = 0; b < parsed_ast->blocks->nelts && num_asts < 16; b++) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "htaccess: Eval block %ui type %V", b, &blocks[b].type);
            if (blocks[b].type.len == 10 && ngx_strncasecmp(blocks[b].type.data, (u_char *)"FilesMatch", 10) == 0 && blocks[b].rc != NULL) {
                if (blocks[b].rc->regex == NULL) {
                    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "htaccess: block regex is NULL");
                    continue;
                }
                if (ngx_regex_exec(blocks[b].rc->regex, &r->uri, NULL, 0) == NGX_REGEX_NO_MATCHED) {
                    continue;
                }
                active_asts[num_asts++] = blocks[b].ast;
            }
        }
    }

    ngx_http_htaccess_ctx_t *ctx = ngx_http_get_module_ctx(r, ngx_http_htaccess_module);
    if (ctx == NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "htaccess: allocating ctx with %ui asts", num_asts);
        ctx = ngx_palloc(r->pool, sizeof(ngx_http_htaccess_ctx_t));
        if (ctx) {
            ctx->asts = ngx_palloc(r->pool, sizeof(ngx_htaccess_ast_t *) * num_asts);
            if (ctx->asts) {
                ngx_memcpy(ctx->asts, active_asts, sizeof(ngx_htaccess_ast_t *) * num_asts);
                ctx->num_asts = num_asts;
                ngx_http_set_ctx(r, ctx, ngx_http_htaccess_module);
            }
        }
    } else {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "htaccess: ctx already exists");
    }

    /* 
     * Authz & Basic Auth Evaluation (Merged from all active ASTs)
     */
    ngx_uint_t ast_idx;
    for (ast_idx = 0; ast_idx < num_asts; ast_idx++) {
        ngx_htaccess_ast_t *curr_ast = active_asts[ast_idx];

        if (curr_ast->authz.rules && curr_ast->authz.rules->nelts > 0) {
            ngx_htaccess_authz_rule_t *rules = curr_ast->authz.rules->elts;
            ngx_uint_t matched = 0;
            ngx_uint_t action = HTACCESS_AUTHZ_NOT_SET;
            ngx_uint_t i;
            
            for (i = 0; i < curr_ast->authz.rules->nelts; i++) {
                ngx_array_t temp_cidrs;
                temp_cidrs.elts = &rules[i].cidr;
                temp_cidrs.nelts = 1;
                temp_cidrs.size = sizeof(ngx_cidr_t);
                temp_cidrs.nalloc = 1;
                temp_cidrs.pool = r->pool;

                if (ngx_cidr_match(r->connection->sockaddr, &temp_cidrs) == NGX_OK) {
                    matched = 1;
                    action = rules[i].action;
                    break;
                }
            }

            if (matched) {
                if (action == HTACCESS_AUTHZ_DENIED) {
                    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "htaccess: access denied by IP rule in %V", &normalized_path);
                    return NGX_HTTP_FORBIDDEN;
                } else if (action == HTACCESS_AUTHZ_GRANTED) {
                    goto check_basic_auth;
                }
            }
        }

        if (curr_ast->authz.default_state == HTACCESS_AUTHZ_DENIED) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "htaccess: access denied by %V", &normalized_path);
            return NGX_HTTP_FORBIDDEN;
        }

check_basic_auth:
        if (curr_ast->auth.auth_type.len > 0 && ngx_strncasecmp(curr_ast->auth.auth_type.data, (u_char *)"Basic", 5) == 0) {
            ngx_int_t auth_rc = ngx_htaccess_execute_auth_basic(r, curr_ast);
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "htaccess: [first] ngx_htaccess_execute_auth_basic returned %i", auth_rc);
            if (auth_rc != NGX_OK && auth_rc != NGX_DECLINED) {
                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "htaccess: [first] returning %i from handler", auth_rc);
                return auth_rc; /* Could be 401, 500, etc. */
            }
        }
    }

    /*
     * DirectoryIndex Execution
     */
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "htaccess: Starting DirectoryIndex eval");
    if (r->uri.len > 0 && r->uri.data[r->uri.len - 1] == '/') {
        for (ast_idx = 0; ast_idx < num_asts; ast_idx++) {
            ngx_htaccess_ast_t *curr_ast = active_asts[ast_idx];
            if (curr_ast->directory_index && curr_ast->directory_index->nelts > 0) {
                ngx_str_t *idx_names = curr_ast->directory_index->elts;
                ngx_uint_t k;
                for (k = 0; k < curr_ast->directory_index->nelts; k++) {
                    size_t strip_len = hlcf->filename.len + 1; /* Strip \ and filename */
                    size_t dir_len = 0;
                    if (normalized_path.len > strip_len) {
                        dir_len = normalized_path.len - strip_len;
                    }
                    u_char *test_path = ngx_pcalloc(r->pool, dir_len + idx_names[k].len + 2);
                    if (test_path) {
                        u_char *p = ngx_cpymem(test_path, normalized_path.data, dir_len);
                        if (dir_len > 0 && normalized_path.data[dir_len - 1] != '\\' && normalized_path.data[dir_len - 1] != '/') {
                            *p++ = '\\';
                        }
                        ngx_memcpy(p, idx_names[k].data, idx_names[k].len);
                        p[idx_names[k].len] = '\0';
                        
                        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "htaccess: Check index %s", test_path);
                        
                        u_short *utf16_path = ngx_htaccess_utf8_to_utf16(r->pool, test_path);
                        if (utf16_path) {
                            WIN32_FILE_ATTRIBUTE_DATA fad;
                            if (GetFileAttributesExW(utf16_path, GetFileExInfoStandard, &fad)) {
                                if (!(fad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                                    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "htaccess: Index found!");
                                    ngx_str_t new_uri;
                                    new_uri.len = r->uri.len + idx_names[k].len;
                                    new_uri.data = ngx_pcalloc(r->pool, new_uri.len);
                                    if (new_uri.data) {
                                        u_char *p = ngx_cpymem(new_uri.data, r->uri.data, r->uri.len);
                                        ngx_memcpy(p, idx_names[k].data, idx_names[k].len);
                                        return ngx_http_internal_redirect(r, &new_uri, &r->args);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
execute_rules:
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "htaccess: entered execute_rules with num_asts=%ui", num_asts);

    /* 
     * Redirect Execution (mod_alias)
     */
    for (ast_idx = 0; ast_idx < num_asts; ast_idx++) {
        ngx_htaccess_ast_t *curr_ast = active_asts[ast_idx];
        if (curr_ast->redirects && curr_ast->redirects->nelts > 0) {
            ngx_int_t redir_rc = ngx_htaccess_execute_redirects(r, curr_ast->redirects, NULL);
            if (redir_rc != NGX_DECLINED && redir_rc != NGX_OK) {
                return redir_rc; /* 301, 302, etc. */
            }
        }
    }

    /* 
     * Rule Execution (Milestone 4)
     * If rewrite engine is On, execute the rules from all active ASTs.
     */
    for (ast_idx = 0; ast_idx < num_asts; ast_idx++) {
        ngx_htaccess_ast_t *curr_ast = active_asts[ast_idx];
        if (curr_ast->rewrite_engine) {
            ngx_http_htaccess_ctx_t *ctx = ngx_http_get_module_ctx(r, ngx_http_htaccess_module);
            if (ctx == NULL) {
                ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_htaccess_ctx_t));
                if (ctx == NULL) return NGX_HTTP_INTERNAL_SERVER_ERROR;
                ctx->redirects = 0;
                ngx_http_set_ctx(r, ctx, ngx_http_htaccess_module);
            }

            if (ctx->redirects >= 10) {
                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "htaccess: Internal redirect cycle detected (>10)");
                return NGX_HTTP_INTERNAL_SERVER_ERROR;
            }
            ctx->redirects++;

            /* Base Prefix for the rules. */
            ngx_str_t root_str = { root_len, path.data };
            ngx_str_t base_prefix;
            size_t strip_len = hlcf->filename.len + 1;
            
            if (normalized_path.len > root_len + strip_len) {
                /* Path relative to root */
                base_prefix.len = normalized_path.len - root_len - strip_len;
                base_prefix.data = ngx_pcalloc(r->pool, base_prefix.len);
                if (base_prefix.data == NULL) return NGX_HTTP_INTERNAL_SERVER_ERROR;
                u_char *p = ngx_cpymem(base_prefix.data, normalized_path.data + root_len, base_prefix.len);
                
                /* Convert backslashes to forward slashes */
                ngx_uint_t idx;
                for (idx = 0; idx < base_prefix.len; idx++) {
                    if (base_prefix.data[idx] == '\\') {
                        base_prefix.data[idx] = '/';
                    }
                }
                
                /* Enforce trailing slash for prefix matching */
                if (base_prefix.len > 0 && base_prefix.data[base_prefix.len - 1] != '/') {
                    u_char *np = ngx_pcalloc(r->pool, base_prefix.len + 1);
                    if (np) {
                        ngx_memcpy(np, base_prefix.data, base_prefix.len);
                        np[base_prefix.len] = '/';
                        base_prefix.data = np;
                        base_prefix.len++;
                    }
                }
            } else {
                ngx_str_set(&base_prefix, "/");
            }

            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "htaccess: executing rules for ast %ui", ast_idx);
            ngx_int_t rule_rc = ngx_htaccess_execute_rules(r, curr_ast->rules, &root_str, &base_prefix);
            if (rule_rc == NGX_DONE) {
                return NGX_DONE; /* Internal redirect occurred */
            }
            if (rule_rc != NGX_DECLINED && rule_rc != NGX_OK) {
                return rule_rc; /* External redirect code or error */
            }
        }
            
        if ((curr_ast->options.indexes != -1) || 
            (curr_ast->error_documents && curr_ast->error_documents->nelts > 0)) {
            
            /* Apply ErrorDocument natively by injecting into clcf->error_pages */
            if (curr_ast->error_documents && curr_ast->error_documents->nelts > 0) {
                ngx_http_core_loc_conf_t *clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);
                
                /* Create a shallow copy of clcf for this request */
                ngx_http_core_loc_conf_t *new_clcf = ngx_palloc(r->pool, sizeof(ngx_http_core_loc_conf_t));
                if (new_clcf != NULL) {
                    *new_clcf = *clcf;
                    
                    /* Create a new error_pages array */
                    ngx_uint_t old_nelts = clcf->error_pages ? clcf->error_pages->nelts : 0;
                    ngx_array_t *new_error_pages = ngx_array_create(r->pool, 
                        old_nelts + curr_ast->error_documents->nelts, 
                        sizeof(ngx_http_err_page_t));
                        
                    if (new_error_pages != NULL) {
                        ngx_htaccess_err_doc_t *docs;
                        ngx_uint_t i;
                        void **new_loc_conf;
                        
                        if (clcf->error_pages) {
                            ngx_http_err_page_t *dst = ngx_array_push_n(new_error_pages, old_nelts);
                            if (dst != NULL) {
                                ngx_memcpy(dst, clcf->error_pages->elts, old_nelts * sizeof(ngx_http_err_page_t));
                            }
                        }
                        
                        docs = curr_ast->error_documents->elts;
                        for (i = 0; i < curr_ast->error_documents->nelts; i++) {
                            ngx_http_err_page_t *dst = ngx_array_push(new_error_pages);
                            if (dst != NULL) {
                                dst->status = docs[i].status;
                                dst->overwrite = -1;
                                dst->args.len = 0;
                                dst->args.data = NULL;
                                ngx_memzero(&dst->value, sizeof(ngx_http_complex_value_t));
                                dst->value.value.len = docs[i].value.len;
                                dst->value.value.data = docs[i].value.data;
                                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "htaccess: injected ErrorDocument %i -> %V", dst->status, &dst->value.value);
                            }
                        }
                        
                        new_clcf->error_pages = new_error_pages;
                        
                        /* Inject into the current request */
                        new_loc_conf = ngx_palloc(r->pool, sizeof(void *) * ngx_http_max_module);
                        if (new_loc_conf != NULL) {
                            ngx_memcpy(new_loc_conf, r->loc_conf, sizeof(void *) * ngx_http_max_module);
                            new_loc_conf[ngx_http_core_module.ctx_index] = new_clcf;
                            r->loc_conf = new_loc_conf;
                            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "htaccess: successfully replaced r->loc_conf for core module");
                        }
                    }

                }
            }
            
            /* Apply Options Indexes by injecting into autoindex loc conf */
            if (curr_ast->options.indexes != -1) {
                ngx_http_autoindex_loc_conf_t *alcf = ngx_http_get_module_loc_conf(r, ngx_http_autoindex_module);
                ngx_http_autoindex_loc_conf_t *new_alcf = ngx_palloc(r->pool, sizeof(ngx_http_autoindex_loc_conf_t));
                if (new_alcf != NULL) {
                    *new_alcf = *alcf;
                    new_alcf->enable = curr_ast->options.indexes;
                    
                    void **new_loc_conf = ngx_palloc(r->pool, sizeof(void *) * ngx_http_max_module);
                    if (new_loc_conf != NULL) {
                        ngx_memcpy(new_loc_conf, r->loc_conf, sizeof(void *) * ngx_http_max_module);
                        new_loc_conf[ngx_http_autoindex_module.ctx_index] = new_alcf;
                        r->loc_conf = new_loc_conf;
                        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "htaccess: injected autoindex.enable = %d", new_alcf->enable);
                    }
                }
            }
        }
    }

    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "htaccess: module handler completed");
    return NGX_DECLINED;
}


static ngx_int_t
ngx_http_htaccess_header_filter(ngx_http_request_t *r)
{
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "htaccess: header_filter start");
    ngx_http_htaccess_ctx_t *ctx = ngx_http_get_module_ctx(r, ngx_http_htaccess_module);
    if (ctx == NULL || ctx->num_asts == 0) {
        return ngx_http_next_header_filter(r);
    }

    ngx_uint_t i, j;
    for (i = 0; i < ctx->num_asts; i++) {
        ngx_htaccess_ast_t *ast = ctx->asts[i];
        if (ast == NULL) continue;
        
        /* Apply Headers */
        if (ast->headers && ast->headers->nelts > 0) {
            ngx_htaccess_header_t *hdrs = ast->headers->elts;
            for (j = 0; j < ast->headers->nelts; j++) {
                ngx_htaccess_header_t *h = &hdrs[j];
                
                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "htaccess: setting header %V: %V", &h->header, &h->value);
                if (h->action == HTACCESS_HEADER_SET || h->action == HTACCESS_HEADER_ADD || h->action == HTACCESS_HEADER_APPEND) {
                    /* Very simplified header application */
                    ngx_table_elt_t *out_h = ngx_list_push(&r->headers_out.headers);
                    if (out_h) {
                        out_h->hash = 1;
                        out_h->key = h->header;
                        out_h->value = h->value;
                    }
                }
                /* TODO: implement UNSET and properly handle SET vs ADD (this just adds for now) */
            }
        }
        
        /* Apply Expires */
        if (ast->expires.active == 1) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "htaccess: setting expires active");
            /* If active, set Expires and Cache-Control */
            ngx_table_elt_t *cc = r->headers_out.cache_control;
            if (cc == NULL) {
                cc = ngx_list_push(&r->headers_out.headers);
                if (cc) {
                    cc->hash = 1;
                    ngx_str_set(&cc->key, "Cache-Control");
                    r->headers_out.cache_control = cc;
                }
            }
            if (cc) {
                ngx_str_set(&cc->value, "max-age=2592000"); /* hardcoded 1 month for simplicity */
            }
        }
    }

    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "htaccess: header_filter end");
    return ngx_http_next_header_filter(r);
}

static void *
ngx_http_htaccess_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_htaccess_loc_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_htaccess_loc_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    conf->enable = NGX_CONF_UNSET;
    
    return conf;
}


static char *
ngx_http_htaccess_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_htaccess_loc_conf_t *prev = parent;
    ngx_http_htaccess_loc_conf_t *conf = child;

    ngx_conf_merge_value(conf->enable, prev->enable, 0);
    ngx_conf_merge_str_value(conf->filename, prev->filename, ".htaccess");

    return NGX_CONF_OK;
}


static ngx_int_t
ngx_http_htaccess_init(ngx_conf_t *cf)
{
    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "htaccess: initializing module and header filter");
    ngx_http_handler_pt        *h;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_REWRITE_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_htaccess_handler;

    ngx_http_next_header_filter = ngx_http_top_header_filter;
    ngx_http_top_header_filter = ngx_http_htaccess_header_filter;

    return NGX_OK;
}
