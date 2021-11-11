/*
 * compile with: gcc -O3 -g -Wall -Wextra -I install/include install/lib/libcppmon_event_source.so main.c -o main
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <cppmon_event_source_export.h>

c_ev_ty cool_event_types[] = {TY_STRING, TY_STRING, TY_STRING};
size_t cool_event_arity = 3;

c_ev_ty hello_event_types[] = {TY_INT, TY_FLOAT, TY_STRING};
size_t hello_event_arity = 3;

c_ev_ty random_event_types[] = {TY_INT, TY_INT};
size_t random_event_arity = 2;

char* possible_strings[6] = {"string1", "string2", "hello", "world", "bla", "lol"};

int rand_ex(int u) {
  return rand() % u;
}

double rand_dbl() {
  return rand() / (RAND_MAX + 1.);
}

int add_random_event(ev_src_ctxt *ctx) {
  int which_event = rand_ex(3);
  if (which_event == 0) {
    char* s1 = possible_strings[rand_ex(6)];
    char* s2 = possible_strings[rand_ex(6)];
    char* s3 = possible_strings[rand_ex(6)];
    c_ev_data ev_data[] = {{.s = s1}, {.s = s2}, {.s = s3}};
    return ev_src_add_ev(ctx, "cool_event", cool_event_types, ev_data, cool_event_arity);
  } else if (which_event == 1) {
    int i1 = rand_ex(10000);
    double f1 = rand_dbl();
    char *s3 = possible_strings[rand_ex(6)];
    c_ev_data ev_data[] = {{.i = i1}, {.d = f1}, {.s = s3}};
    return ev_src_add_ev(ctx, "hello_event", hello_event_types, ev_data, hello_event_arity);    
  } else {
    int i1 = rand_ex(10000);
    int i2 = rand_ex(10000);
    c_ev_data ev_data[] = {{.i = i1}, {.i = i2}};
    return ev_src_add_ev(ctx, "random_event", random_event_types, ev_data, random_event_arity);
  }
}

int main() {
  int ret;
  ev_src_init_flags flags = {
    .log_to_file = 0,
    .log_to_stdout = 0,
    .unbounded_buf = 0
  };
  ev_src_init_opts opts = {
    flags,
    .wbuf_size = 6000,
    .log_path = "input_log",
    .uds_sock_path = NULL
  };
  ev_src_ctxt *ctx = ev_src_init(&opts);

  for (size_t i = 0; i < 1000000; ++i) {
    if ((ret = ev_src_add_db(ctx, i)) < 0)
      goto failure_string;
    size_t n_ev_for_db = rand_ex(10);
    for (size_t j = 0; j < n_ev_for_db; ++j) {
      //usleep(10);
      if ((ret = add_random_event(ctx)) < 0)
        goto failure_string;
    }
  }
  if ((ret = ev_src_teardown(ctx)) < 0)
    goto failure_string;
  ev_src_free(ctx);
  return 0;
  failure_string:
    fprintf(stderr, "cerr %s\n", ev_src_last_err(ctx));
    ev_src_free(ctx);
    return 1;
}
