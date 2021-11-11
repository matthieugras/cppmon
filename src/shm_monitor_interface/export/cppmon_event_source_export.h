#ifndef CPPMON_CPPMON_EVENT_SOURCE_EXPORT_H
#define CPPMON_CPPMON_EVENT_SOURCE_EXPORT_H

#include <stddef.h>

typedef struct ev_src_ctxt ev_src_ctxt;

typedef enum
{
  TY_INT,
  TY_FLOAT,
  TY_STRING
} c_ev_ty;

typedef union {
  int i;
  double d;
  char *s;
} c_ev_data;

typedef struct {
  int log_to_file : 1;
  int log_to_stdout : 1;
  int unbounded_buf : 1;
} ev_src_init_flags;

typedef struct {
  ev_src_init_flags flags;
  char *log_path;
  char *uds_sock_path;
} ev_src_init_opts;

ev_src_ctxt *ev_src_init(const ev_src_init_opts *options);
void ev_src_free(ev_src_ctxt *ctx);
int ev_src_teardown(ev_src_ctxt *ctx);
int ev_src_add_db(ev_src_ctxt *ctx, size_t timestamp);
int ev_src_add_ev(ev_src_ctxt *ctx, char *ev_name, const c_ev_ty *ev_types,
                  const c_ev_data *data, size_t arity);
const char *ev_src_last_err(ev_src_ctxt *ctx);

#endif// CPPMON_CPPMON_EVENT_SOURCE_EXPORT_H
