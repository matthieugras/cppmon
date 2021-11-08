#ifndef CPPMON_CPPMON_EVENT_TYPES_EXPORT_H
#define CPPMON_CPPMON_EVENT_TYPES_EXPORT_H

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

#endif// CPPMON_CPPMON_EVENT_TYPES_EXPORT_H
