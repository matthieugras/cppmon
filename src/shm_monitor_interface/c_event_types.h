#ifndef CPPMON_C_EVENT_TYPES_H
#define CPPMON_C_EVENT_TYPES_H

extern "C" {
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
}

#endif// CPPMON_C_EVENT_TYPES_H
