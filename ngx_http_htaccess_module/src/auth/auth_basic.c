#include "auth_basic.h"
#include <ngx_crypt.h>

static ngx_int_t ngx_htaccess_auth_set_realm(ngx_http_request_t *r, ngx_str_t *realm) {
    size_t len;
    u_char *basic, *p;
    ngx_table_elt_t *h;

    ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0, "htaccess: entering ngx_htaccess_auth_set_realm");

    h = ngx_list_push(&r->headers_out.headers);
    if (h == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    len = sizeof("Basic realm=\"\"") - 1 + realm->len;
    basic = ngx_pnalloc(r->pool, len);
    if (basic == NULL) {
        h->hash = 0;
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    p = ngx_cpymem(basic, "Basic realm=\"", sizeof("Basic realm=\"") - 1);
    p = ngx_cpymem(p, realm->data, realm->len);
    *p = '"';

    h->hash = 1;
    h->next = NULL;
    ngx_str_set(&h->key, "WWW-Authenticate");
    h->value.data = basic;
    h->value.len = len;
    
    r->headers_out.www_authenticate = h;

    ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0, "htaccess: returning NGX_HTTP_UNAUTHORIZED from ngx_htaccess_auth_set_realm");

    return NGX_HTTP_UNAUTHORIZED;
}

ngx_int_t ngx_htaccess_execute_auth_basic(ngx_http_request_t *r, ngx_htaccess_ast_t *ast) {
    ngx_int_t rc;
    ngx_fd_t fd;
    ngx_file_info_t fi;
    ssize_t n;
    u_char *buf, *p, *last, *hash_start, *hash_end, *encrypted;
    ngx_str_t *realm;
    
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "htaccess: entering ngx_htaccess_execute_auth_basic");
    
    if (!ast->auth.require_valid_user) {
        return NGX_DECLINED;
    }

    realm = &ast->auth.auth_name;
    if (realm->len == 0) {
        ngx_http_core_loc_conf_t *clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);
        realm = &clcf->name;
    }

    rc = ngx_http_auth_basic_user(r);
    if (rc == NGX_DECLINED) {
        ngx_log_error(NGX_LOG_INFO, r->connection->log, 0, "htaccess: no user/password was provided for basic authentication");
        return ngx_htaccess_auth_set_realm(r, realm);
    }
    if (rc == NGX_ERROR) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    if (ast->auth.auth_user_file.len == 0) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "htaccess: AuthUserFile is not set");
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    fd = ngx_open_file(ast->auth.auth_user_file.data, NGX_FILE_RDONLY, NGX_FILE_OPEN, 0);
    if (fd == NGX_INVALID_FILE) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, ngx_errno, "htaccess: ngx_open_file(\"%V\") failed", &ast->auth.auth_user_file);
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "htaccess: auth_basic opened file %V", &ast->auth.auth_user_file);

    if (ngx_fd_info(fd, &fi) == NGX_FILE_ERROR) {
        ngx_close_file(fd);
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    buf = ngx_pnalloc(r->pool, ngx_file_size(&fi) + 1);
    if (buf == NULL) {
        ngx_close_file(fd);
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    n = ngx_read_fd(fd, buf, ngx_file_size(&fi));
    ngx_close_file(fd);

    if (n == -1) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    buf[n] = '\0';

    /* parse .htpasswd */
    p = buf;
    last = buf + n;

    while (p < last) {
        if (p + r->headers_in.user.len + 1 <= last
            && ngx_strncmp(p, r->headers_in.user.data, r->headers_in.user.len) == 0
            && p[r->headers_in.user.len] == ':') {
            
            hash_start = p + r->headers_in.user.len + 1;
            hash_end = hash_start;
            while (hash_end < last && *hash_end != '\r' && *hash_end != '\n') {
                hash_end++;
            }

            /* Null-terminate the hash for ngx_crypt */
            *hash_end = '\0';
            
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "htaccess: calling ngx_crypt for user %V", &r->headers_in.user);
            rc = ngx_crypt(r->pool, r->headers_in.passwd.data, hash_start, &encrypted);
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "htaccess: ngx_crypt returned %i", rc);
            if (rc == NGX_OK) {
                if (ngx_strcmp(encrypted, hash_start) == 0) {
                    return NGX_OK;
                }
            }
            
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "htaccess: user \"%V\": password mismatch", &r->headers_in.user);
            return ngx_htaccess_auth_set_realm(r, realm);
        }
        
        while (p < last && *p != '\n') p++;
        p++;
    }

    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "htaccess: user \"%V\" was not found in \"%V\"", &r->headers_in.user, &ast->auth.auth_user_file);
    return ngx_htaccess_auth_set_realm(r, realm);
}
