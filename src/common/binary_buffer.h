#ifndef CPPMON_BINARY_BUFFER_H
#define CPPMON_BINARY_BUFFER_H

#include <boost/container/devector.hpp>
#include <vector>
#include <iterator>

namespace common {

template<typename T>
class binary_buffer {
public:
  binary_buffer() : is_l(false){};

  template<typename F>
  std::vector<T> update_and_reduce(std::vector<T> &new_l, std::vector<T> &new_r, F &f) {
    std::vector<T> res;
    auto it1 = new_l.begin(), eit1 = new_l.end(), it2 = new_r.begin(),
         eit2 = new_r.end();
    if (!is_l) {
      std::swap(it1, it2);
      std::swap(eit1, eit2);
    }
    for (; !buf.empty() && it2 != eit2; buf.pop_front(), ++it2) {
      if (is_l)
        res.push_back(f(buf.front(), *it2));
      else
        res.push_back(f(*it2, buf.front()));
    }
    for (; it1 != eit1 && it2 != eit2; it1++, it2++) {
      if (is_l)
        res.push_back(f(*it1, *it2));
      else
        res.push_back(f(*it2, *it1));
    }
    if (it1 == eit1) {
      is_l = !is_l;
      std::swap(it1, it2);
      std::swap(eit1, eit2);
    }
    buf.insert(buf.end(), std::make_move_iterator(it1),
               std::make_move_iterator(eit1));
    return res;
  }

private:
  boost::container::devector<T> buf;
  bool is_l;
};

}// namespace common


#endif// CPPMON_BINARY_BUFFER_H
