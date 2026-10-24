#ifndef _NGX_HTACCESS_FILESYSTEM_FILE_H_INCLUDED_
#define _NGX_HTACCESS_FILESYSTEM_FILE_H_INCLUDED_

#include <ngx_config.h>
#include <ngx_core.h>

typedef struct {
    size_t     size;
    time_t     mtime;
    ngx_uint_t is_dir;
    ngx_uint_t is_file;
} ngx_htaccess_file_info_t;

/* Returns NGX_OK if success, NGX_DECLINED if not found, NGX_ERROR on error */
ngx_int_t ngx_htaccess_file_stat(ngx_str_t *path, ngx_htaccess_file_info_t *fi);

/* Normalizes the path securely to prevent traversal attacks */
ngx_int_t ngx_htaccess_path_normalize(ngx_pool_t *pool, ngx_str_t *path, ngx_str_t *normalized);

#endif /* _NGX_HTACCESS_FILESYSTEM_FILE_H_INCLUDED_ */
