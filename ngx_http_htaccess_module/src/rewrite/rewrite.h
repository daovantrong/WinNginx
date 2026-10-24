#ifndef _NGX_HTACCESS_REWRITE_H_INCLUDED_
#define _NGX_HTACCESS_REWRITE_H_INCLUDED_

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

/* Flags mask for RewriteRule */
#define HTACCESS_FLAG_L   0x0001
#define HTACCESS_FLAG_NC  0x0002
#define HTACCESS_FLAG_QSA 0x0004
#define HTACCESS_FLAG_R   0x0008

/* 
 * RewriteCond structure
 * Represents a single RewriteCond line
 */
typedef struct {
    ngx_str_t                 test_string;   /* e.g., "%{HTTP_HOST}" */
    ngx_str_t                 cond_pattern;  /* raw pattern */
    ngx_regex_compile_t      *rc;            /* Compiled PCRE regex for condition, if applicable */
    ngx_uint_t                flags;         /* e.g., NC, OR */
} ngx_htaccess_cond_t;

/* 
 * RewriteRule structure 
 * Represents a single RewriteRule line with its preceding conditions
 */
typedef struct {
    ngx_array_t              *conditions;    /* Array of ngx_htaccess_cond_t */
    ngx_str_t                 pattern;       /* raw pattern */
    ngx_regex_compile_t      *rc;            /* Compiled PCRE regex for the rule */
    ngx_str_t                 substitution;  /* e.g., "index.php" */
    ngx_uint_t                flags;         /* bitmask of HTACCESS_FLAG_* */
    ngx_uint_t                redirect_code; /* HTTP code for [R=301], default 302 if [R] */
} ngx_htaccess_rule_t;

/* Authz Configuration */
#define HTACCESS_AUTHZ_NOT_SET 0
#define HTACCESS_AUTHZ_GRANTED 1
#define HTACCESS_AUTHZ_DENIED  2

typedef struct {
    ngx_cidr_t cidr;
    ngx_uint_t action; /* HTACCESS_AUTHZ_GRANTED or HTACCESS_AUTHZ_DENIED */
} ngx_htaccess_authz_rule_t;

typedef struct {
    ngx_uint_t default_state; /* HTACCESS_AUTHZ_GRANTED or HTACCESS_AUTHZ_DENIED */
    ngx_array_t *rules;       /* Array of ngx_htaccess_authz_rule_t */
} ngx_htaccess_authz_t;

typedef struct {
    ngx_str_t auth_type;
    ngx_str_t auth_name;
    ngx_str_t auth_user_file;
    ngx_uint_t require_valid_user; /* 1 if Require valid-user */
} ngx_htaccess_auth_t;

/* 
 * Redirect structure (mod_alias)
 */
typedef struct {
    ngx_uint_t                status;        /* e.g., 301, 302 */
    ngx_str_t                 path;          /* prefix (Redirect) or pattern (RedirectMatch) */
    ngx_regex_compile_t      *rc;            /* compiled regex if RedirectMatch */
    ngx_str_t                 url;           /* destination URL */
} ngx_htaccess_redirect_t;

typedef struct ngx_htaccess_ast_s ngx_htaccess_ast_t;

/*
 * Block structure for <FilesMatch>
 */
typedef struct {
    ngx_str_t                 type; /* e.g., "FilesMatch" */
    ngx_str_t                 arg;  /* e.g., "\.ini$" */
    ngx_regex_compile_t      *rc;   /* Compiled regex */
    ngx_htaccess_ast_t       *ast;  /* Nested AST */
} ngx_htaccess_block_t;

/* 
 * Options Configuration
 */
typedef struct {
    ngx_int_t indexes; /* 1 for +Indexes, 0 for -Indexes, -1 for unset */
} ngx_htaccess_options_t;

/* 
 * ErrorDocument Configuration
 */
typedef struct {
    ngx_uint_t status;
    ngx_str_t value;
} ngx_htaccess_err_doc_t;

/* mod_headers */
typedef enum {
    HTACCESS_HEADER_SET,
    HTACCESS_HEADER_APPEND,
    HTACCESS_HEADER_ADD,
    HTACCESS_HEADER_UNSET,
    HTACCESS_HEADER_ECHO
} ngx_htaccess_header_action_t;

typedef struct {
    ngx_htaccess_header_action_t action;
    ngx_str_t header;
    ngx_str_t value; /* empty for UNSET */
} ngx_htaccess_header_t;

/* mod_expires */
typedef struct {
    ngx_str_t type; /* e.g., "image/gif" */
    ngx_str_t value; /* e.g., "access plus 1 month" */
} ngx_htaccess_expires_type_t;

typedef struct {
    ngx_uint_t active; /* 1 = On, 0 = Off, 2 = Unset */
    ngx_str_t default_expires; /* e.g., "access plus 1 month" */
    ngx_array_t *by_type; /* Array of ngx_htaccess_expires_type_t */
} ngx_htaccess_expires_t;

/* 
 * Abstract Syntax Tree (AST) for .htaccess
 */
struct ngx_htaccess_ast_s {
    ngx_uint_t                rewrite_engine; /* 1 = On, 0 = Off */
    ngx_array_t              *rules;          /* Array of ngx_htaccess_rule_t */
    ngx_array_t              *redirects;      /* Array of ngx_htaccess_redirect_t */
    ngx_htaccess_authz_t      authz;          /* Access control */
    ngx_htaccess_auth_t       auth;           /* Basic Authentication */
    
    ngx_array_t              *directory_index;/* Array of ngx_str_t */
    ngx_array_t              *blocks;         /* Array of ngx_htaccess_block_t */
    
    ngx_htaccess_options_t    options;
    ngx_array_t              *error_documents;/* Array of ngx_htaccess_err_doc_t */

    ngx_array_t              *headers;        /* Array of ngx_htaccess_header_t */
    ngx_htaccess_expires_t    expires;
};

#endif /* _NGX_HTACCESS_REWRITE_H_INCLUDED_ */
