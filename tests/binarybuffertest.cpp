#include <binary_buffer.h>
#include <gtest/gtest.h>
#include <utility>
#include <vector>

using test_buf = common::binary_buffer<int>;
using int_vec = std::vector<int>;
using res_vec = std::vector<std::pair<int, int>>;
static constexpr auto lambda = [](auto l, auto r) { return std::pair(l, r); };

TEST(BinaryBuffer, SequentialInsertLeft) {
  test_buf buf;
  {
    int_vec l{}, r{};
    auto res = buf.update_and_reduce(l, r, lambda);
    EXPECT_EQ(res, res_vec{});
  }
  {
    int_vec l({1, 2, 3, 4, 5}), r{};
    auto res = buf.update_and_reduce(l, r, lambda);
    EXPECT_EQ(res, res_vec{});
  }
  {
    int_vec l({6, 7, 8, 9}), r{};
    auto res = buf.update_and_reduce(l, r, lambda);
    EXPECT_EQ(res, res_vec{});
  }
  {
    int_vec l({}), r{1, 2, 3};
    auto res = buf.update_and_reduce(l, r, lambda);
    EXPECT_EQ(res, res_vec({{1, 1}, {2, 2}, {3, 3}}));
  }
  {
    int_vec l({}), r{4, 5, 6, 7, 8, 9};
    auto res = buf.update_and_reduce(l, r, lambda);
    EXPECT_EQ(res, res_vec({{4, 4}, {5, 5}, {6, 6}, {7, 7}, {8, 8}, {9, 9}}));
  }
}

TEST(BinaryBuffer, SequentialInsertRight) {
  test_buf buf;
  {
    int_vec l{}, r{};
    auto res = buf.update_and_reduce(l, r, lambda);
    EXPECT_EQ(res, res_vec{});
  }
  {
    int_vec r({1, 2, 3, 4, 5}), l{};
    auto res = buf.update_and_reduce(l, r, lambda);
    EXPECT_EQ(res, res_vec{});
  }
  {
    int_vec r({6, 7, 8, 9}), l{};
    auto res = buf.update_and_reduce(l, r, lambda);
    EXPECT_EQ(res, res_vec{});
  }
  {
    int_vec r({}), l{1, 2, 3};
    auto res = buf.update_and_reduce(l, r, lambda);
    EXPECT_EQ(res, res_vec({{1, 1}, {2, 2}, {3, 3}}));
  }
  {
    int_vec r({}), l{4, 5, 6, 7, 8, 9};
    auto res = buf.update_and_reduce(l, r, lambda);
    EXPECT_EQ(res, res_vec({{4, 4}, {5, 5}, {6, 6}, {7, 7}, {8, 8}, {9, 9}}));
  }
}

TEST(BinaryBuffer, PingPongInsert) {
  test_buf buf;
  {
    int_vec l({1, 2, 3}), r({});
    auto res = buf.update_and_reduce(l, r, lambda);
    EXPECT_EQ(res, res_vec({}));
  }
  {
    int_vec l({}), r({1, 2, 3, 4, 5, 6});
    auto res = buf.update_and_reduce(l, r, lambda);
    EXPECT_EQ(res, res_vec({{1, 1}, {2, 2}, {3, 3}}));
  }
  {
    int_vec l({4, 5, 6, 7, 8, 9}), r({});
    auto res = buf.update_and_reduce(l, r, lambda);
    EXPECT_EQ(res, res_vec({{4, 4}, {5, 5}, {6, 6}}));
  }
  {
    int_vec l({}), r({7, 8, 9});
    auto res = buf.update_and_reduce(l, r, lambda);
    EXPECT_EQ(res, res_vec({{7, 7}, {8, 8}, {9, 9}}));
  }
}

TEST(BinaryBuffer, RandomInsert) {
  test_buf buf;
  {
    int_vec l({1, 2, 3}), r({1, 2, 3, 4});
    auto res = buf.update_and_reduce(l, r, lambda);
    EXPECT_EQ(res, res_vec({{1, 1}, {2, 2}, {3, 3}}));
    // buffer has: right [4]
  }
  {
    int_vec l({4, 5, 6}), r({5, 6, 7, 8});
    auto res = buf.update_and_reduce(l, r, lambda);
    EXPECT_EQ(res, res_vec({{4, 4}, {5, 5}, {6, 6}}));
    // buffer has: right [7,8]
  }
  {
    int_vec l({7, 8, 9, 10}), r({});
    auto res = buf.update_and_reduce(l, r, lambda);
    EXPECT_EQ(res, res_vec({{7, 7}, {8, 8}}));
    // buffer has: left [9,10]
  }
  {
    int_vec l({}), r({9, 10});
    auto res = buf.update_and_reduce(l, r, lambda);
    EXPECT_EQ(res, res_vec({{9, 9}, {10, 10}}));
    // buffer is empty
  }
  {
    int_vec l({}), r({11, 12});
    auto res = buf.update_and_reduce(l, r, lambda);
    EXPECT_EQ(res, res_vec({}));
    // buffer has: right [11,12]
  }
  {
    int_vec l({}), r({13, 14, 15});
    auto res = buf.update_and_reduce(l, r, lambda);
    EXPECT_EQ(res, res_vec({}));
    // buffer has: right [11,12,13,14,15]
  }
  {
    int_vec l({11, 12, 13, 14, 15, 16, 17}), r({16, 17});
    auto res = buf.update_and_reduce(l, r, lambda);
    EXPECT_EQ(res, res_vec({{11, 11},
                            {12, 12},
                            {13, 13},
                            {14, 14},
                            {15, 15},
                            {16, 16},
                            {17, 17}}));
    // buffer is empty
  }
}