#ifndef _NGX_HTACCESS_CACHE_H_INCLUDED_
#define _NGX_HTACCESS_CACHE_H_INCLUDED_

#include <ngx_config.h>
#include <ngx_core.h>
#include "../parser/parser.h"

typedef struct {
    ngx_str_node_t               sn;
    time_t                       mtime;
    size_t                       size;
    ngx_pool_t                  *pool;
    ngx_htaccess_ast_t           ast;
} ngx_htaccess_cache_node_t;

typedef struct {
    ngx_rbtree_t       rbtree;
    ngx_rbtree_node_t  sentinel;
} ngx_htaccess_cache_t;

/* Initialize the cache */
ngx_int_t ngx_htaccess_cache_init(ngx_htaccess_cache_t *cache);

/* Lookup cache. Returns node if matched, NULL if miss or invalid */
ngx_htaccess_cache_node_t *ngx_htaccess_cache_lookup(ngx_htaccess_cache_t *cache, ngx_str_t *path, time_t mtime, size_t size);

/* Insert into cache. Returns the new node. */
ngx_htaccess_cache_node_t *ngx_htaccess_cache_insert(ngx_htaccess_cache_t *cache, ngx_str_t *path, time_t mtime, size_t size);

#endif /* _NGX_HTACCESS_CACHE_H_INCLUDED_ */
