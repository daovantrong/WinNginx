#include "variables.h"

static struct {
    ngx_str_t apache_name;
    ngx_str_t nginx_name;
} ngx_htaccess_var_map[] = {
    { ngx_string("HTTP_HOST"),         ngx_string("http_host") },
    { ngx_string("HTTP_USER_AGENT"),   ngx_string("http_user_agent") },
    { ngx_string("HTTP_REFERER"),      ngx_string("http_referer") },
    { ngx_string("REQUEST_URI"),       ngx_string("request_uri") },
    { ngx_string("REQUEST_METHOD"),    ngx_string("request_method") },
    { ngx_string("REQUEST_FILENAME"),  ngx_string("request_filename") },
    { ngx_string("QUERY_STRING"),      ngx_string("query_string") },
    { ngx_string("REMOTE_ADDR"),       ngx_string("remote_addr") },
    { ngx_string("REMOTE_PORT"),       ngx_string("remote_port") },
    { ngx_string("SERVER_ADDR"),       ngx_string("server_addr") },
    { ngx_string("SERVER_PORT"),       ngx_string("server_port") },
    { ngx_string("SERVER_NAME"),       ngx_string("server_name") },
    { ngx_string("SERVER_PROTOCOL"),   ngx_string("server_protocol") },
    { ngx_string("HTTPS"),             ngx_string("https") },
    { ngx_null_string,                 ngx_null_string }
};

ngx_int_t
ngx_htaccess_evaluate_variable(ngx_http_request_t *r, ngx_str_t *apache_var, ngx_str_t *val)
{
    ngx_uint_t i;
    ngx_str_t nginx_var = ngx_null_string;
    ngx_http_variable_value_t *v;
    ngx_uint_t hash;
    
    /* Ensure variable syntax is %{...} */
    if (apache_var->len < 3 || apache_var->data[0] != '%' || apache_var->data[1] != '{' || apache_var->data[apache_var->len - 1] != '}') {
        return NGX_ERROR;
    }

    ngx_str_t name = { apache_var->len - 3, apache_var->data + 2 };

    /* Lookup Nginx mapped name */
    for (i = 0; ngx_htaccess_var_map[i].apache_name.len > 0; i++) {
        if (name.len == ngx_htaccess_var_map[i].apache_name.len &&
            ngx_strncasecmp(name.data, ngx_htaccess_var_map[i].apache_name.data, name.len) == 0) 
        {
            nginx_var = ngx_htaccess_var_map[i].nginx_name;
            break;
        }
    }

    if (nginx_var.len == 0) {
        val->len = 0;
        val->data = (u_char *)"";
        return NGX_OK;
    }

    hash = ngx_hash_key(nginx_var.data, nginx_var.len);
    v = ngx_http_get_variable(r, &nginx_var, hash);

    if (v == NULL || v->not_found) {
        val->len = 0;
        val->data = (u_char *)"";
        return NGX_OK;
    }

    val->len = v->len;
    val->data = v->data;

    return NGX_OK;
}
