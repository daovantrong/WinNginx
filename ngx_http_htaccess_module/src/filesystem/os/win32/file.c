#include <ngx_config.h>
#include <ngx_core.h>
#include "../../file.h"

#if (NGX_WIN32)

ngx_int_t
ngx_htaccess_file_stat(ngx_str_t *path, ngx_htaccess_file_info_t *fi)
{
    WIN32_FILE_ATTRIBUTE_DATA  fa;
    WCHAR                      utf16[1024];
    size_t                     len;
    int                        wlen;

    wlen = MultiByteToWideChar(CP_UTF8, 0, (const char *)path->data, path->len, NULL, 0);
    if (wlen == 0 || wlen >= 1024) {
        return NGX_ERROR;
    }

    MultiByteToWideChar(CP_UTF8, 0, (const char *)path->data, path->len, utf16, wlen);
    utf16[wlen] = 0; /* Null terminate */

    if (GetFileAttributesExW(utf16, GetFileExInfoStandard, &fa) == 0) {
        return NGX_DECLINED;
    }

    fi->size = (size_t) fa.nFileSizeLow; /* ignore high for htaccess */
    fi->mtime = (time_t) ((((uint64_t) fa.ftLastWriteTime.dwHighDateTime << 32) | 
                           fa.ftLastWriteTime.dwLowDateTime) / 10000000ULL - 11644473600ULL);
                           
    fi->is_dir = (fa.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? 1 : 0;
    fi->is_file = !fi->is_dir;

    return NGX_OK;
}

ngx_int_t
ngx_htaccess_path_normalize(ngx_pool_t *pool, ngx_str_t *path, ngx_str_t *normalized)
{
    /* Basic normalization: convert all forward slashes to backslashes for Windows */
    size_t i;
    u_char *p = ngx_palloc(pool, path->len + 1);
    
    if (p == NULL) {
        return NGX_ERROR;
    }

    for (i = 0; i < path->len; i++) {
        if (path->data[i] == '/') {
            p[i] = '\\';
        } else {
            p[i] = path->data[i];
        }
    }
    p[path->len] = '\0';
    
    normalized->data = p;
    normalized->len = path->len;
    
    return NGX_OK;
}

#endif
