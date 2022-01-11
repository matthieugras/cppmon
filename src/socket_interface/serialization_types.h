#ifndef CPPMON_SERIALIZATION_TYPES_H
#define CPPMON_SERIALIZATION_TYPES_H
namespace ipc::serialization {
enum control_bits
{
  CTRL_NEW_EVENT = 0x1,
  CTRL_NEW_DATABASE = 0x2,
  CTRL_END_DATABASE = 0x3,
  CTRL_LATENCY_MARKER = 0x4,
  CTRL_EOF = 0x5
};
}// namespace ipc::serialization

#endif// CPPMON_SERIALIZATION_TYPES_H
