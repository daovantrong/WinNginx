#include "cache.h"

ngx_int_t 
ngx_htaccess_cache_init(ngx_htaccess_cache_t *cache)
{
    ngx_rbtree_init(&cache->rbtree, &cache->sentinel, ngx_str_rbtree_insert_value);
    return NGX_OK;
}

ngx_htaccess_cache_node_t *
ngx_htaccess_cache_lookup(ngx_htaccess_cache_t *cache, ngx_str_t *path, time_t mtime, size_t size)
{
    uint32_t hash = ngx_crc32_short(path->data, path->len);
    ngx_rbtree_node_t *node = ngx_str_rbtree_lookup(&cache->rbtree, path, hash);

    if (node) {
        ngx_htaccess_cache_node_t *cnode = (ngx_htaccess_cache_node_t *) node;
        /* Check invalidation */
        if (cnode->mtime == mtime && cnode->size == size) {
            return cnode; /* Cache Hit */
        }
        
        /* Cache Miss due to modification. Remove old node */
        ngx_rbtree_delete(&cache->rbtree, node);
        if (cnode->pool) {
            ngx_destroy_pool(cnode->pool);
        }
    }

    return NULL;
}

ngx_htaccess_cache_node_t *
ngx_htaccess_cache_insert(ngx_htaccess_cache_t *cache, ngx_str_t *path, time_t mtime, size_t size)
{
    ngx_pool_t *pool;
    ngx_htaccess_cache_node_t *cnode;

    /* Create a dedicated memory pool for this .htaccess file (AST and content) */
    pool = ngx_create_pool(4096, ngx_cycle->log);
    if (pool == NULL) {
        return NULL;
    }

    cnode = ngx_pcalloc(pool, sizeof(ngx_htaccess_cache_node_t));
    if (cnode == NULL) {
        ngx_destroy_pool(pool);
        return NULL;
    }

    cnode->pool = pool;
    cnode->mtime = mtime;
    cnode->size = size;
    
    cnode->sn.str.len = path->len;
    cnode->sn.str.data = ngx_pnalloc(pool, path->len);
    if (cnode->sn.str.data == NULL) {
        ngx_destroy_pool(pool);
        return NULL;
    }
    ngx_memcpy(cnode->sn.str.data, path->data, path->len);

    cnode->sn.node.key = ngx_crc32_short(cnode->sn.str.data, cnode->sn.str.len);

    ngx_rbtree_insert(&cache->rbtree, &cnode->sn.node);

    return cnode;
}
