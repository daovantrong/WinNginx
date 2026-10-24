#ifndef _NGX_SERVICE_H_INCLUDED_
#define _NGX_SERVICE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


int ngx_cdecl ngx_main_run(int argc, char *const *argv);

ngx_int_t ngx_service_dispatcher(int argc, char **argv);

int ngx_service_install(int argc, char **argv);
int ngx_service_uninstall(int argc, char **argv);
int ngx_service_start(int argc, char **argv);
int ngx_service_stop(int argc, char **argv);

extern ngx_uint_t ngx_is_service;

void ngx_service_report_running(void);
void ngx_service_report_stopping(DWORD wait_hint);
void ngx_service_report_stopped(DWORD error);


#endif /* _NGX_SERVICE_H_INCLUDED_ */
