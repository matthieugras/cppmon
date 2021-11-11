#ifndef CPPMON_SERIALIZATION_TYPES_H
#define CPPMON_SERIALIZATION_TYPES_H
namespace ipc::serialization {
enum control_bits
{
  CTRL_NEW_EVENT,
  CTRL_NEW_DATABASE,
  CTRL_END_DATABASE,
  CTRL_TERMINATED
};
}// namespace ipc::serialization

#endif// CPPMON_SERIALIZATION_TYPES_H
