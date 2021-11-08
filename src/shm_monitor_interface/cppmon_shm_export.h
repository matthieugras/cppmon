#ifndef CPPMON_CPPMON_SHM_EXPORT_H
#define CPPMON_CPPMON_SHM_EXPORT_H

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

#define LINK_API __attribute__((visibility("default")))

#endif// CPPMON_CPPMON_SHM_EXPORT_H
