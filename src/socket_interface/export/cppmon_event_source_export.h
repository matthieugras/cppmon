#ifndef CPPMON_CPPMON_EVENT_SOURCE_EXPORT_H
#define CPPMON_CPPMON_EVENT_SOURCE_EXPORT_H

#include <stddef.h>
#include <stdint.h>


/*!
 * @file cppmon_event_source_export.h
 */

/*!
 * Context that contains the state of the socket interface
 */
typedef struct ev_src_ctxt ev_src_ctxt;

/*! Possible types of event arguments */
typedef enum
{
  TY_INT,
  TY_FLOAT,
  TY_STRING
} c_ev_ty;

/*! A single event argument
 * @warning The type with the name Float is actually a double
 */
typedef union {
  int64_t i;
  double d;
  char *s;
} c_ev_data;

/*!
 * @struct flags
 *
 * @var log_to_file
 * Write trace to file
 *
 * @var log_to_stdout
 * Print trace to stdout
 *
 * @var unbounded_buf
 * Use an unbounded buffer to avoid blocking. This is slower than the blocking
 * version because of synchronization overhead.
 */
typedef struct {
  int log_to_file : 1;
  int log_to_stdout : 1;
  int unbounded_buf : 1;
} ev_src_init_flags;

/*!
 * @struct ev_src_init_opts
 * Intialization options for the socket interface
 *
 * @var flags
 * Flags
 *
 * @var wbuf_size
 * Number of bytes that are buffered before writing to the UDS. If
 * unbounded_buf is set to false, exactly wbuf_size bytes are buffered.
 * Otherwise at least wbuf_size are buffered.
 *
 * @var log_path
 * Path for the output trace. Can be set to NULL if file logging is not used.
 *
 * @var uds_sock_path
 * Path of the UDS. Uses a default value if set to NULL
 */
typedef struct {
  ev_src_init_flags flags;
  size_t wbuf_size;
  char *log_path;
  char *uds_sock_path;
} ev_src_init_opts;

/*!
 * Creates a context object of unspecified type that can be used to interface
 * with cppmon. If the initialization fails, an error description is printed to
 * stderr and the calling process is aborted.
 * @param options
 * @return A valid api context
 */
ev_src_ctxt *ev_src_init(const ev_src_init_opts *options);

/*!
 * Releases all resources associated with the context. Must be called after
 * ev_src_teardown()
 * @param ctx
 */
void ev_src_free(ev_src_ctxt *ctx);

/*!
 * Flushes all buffers and does EOF-handling
 * @param ctx Valid api context
 * @return -1 failure (sets last_err), 0 if successful
 */
int ev_src_teardown(ev_src_ctxt *ctx);

/*!
 * Starts a new database with the specified timestamp
 * @param ctx Valid api context
 * @param timestamp
 * @return -1 on failure (sets last_err), 0 if successful
 */
int ev_src_add_db(ev_src_ctxt *ctx, size_t timestamp);

/*!
 * Adds an event to the current database
 * @param ctx Valid api context
 * @param ev_name Name of the event
 * @param ev_types Event argument types
 * @param data Event arguments
 * @param arity Number of event arguments
 * @return -1 failure (sets last_err), 0 if successful
 */
int ev_src_add_ev(ev_src_ctxt *ctx, char *ev_name, const c_ev_ty *ev_types,
                  const c_ev_data *data, size_t arity);

/*!
 * Adds a database that contains a single event
 * @param ctx Valid api context
 * @param timestamp
 * @param ev_name Name of the event
 * @param ev_types Event argument types
 * @param data Event arguments
 * @param arity Number of event arguments
 * @return -1 failure (sets last_err), 0 if successful
 */
int ev_src_add_singleton_db(ev_src_ctxt *ctx, size_t timestamp, char *ev_name,
                            const c_ev_ty *ev_types, const c_ev_data *data,
                            size_t arity);

/*!
 * Get the last error
 * @param ctx Valid api context
 * @return A description of the last error, NULL if no such error occured
 */
const char *ev_src_last_err(ev_src_ctxt *ctx);

#endif// CPPMON_CPPMON_EVENT_SOURCE_EXPORT_H
