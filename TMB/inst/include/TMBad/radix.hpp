#ifndef HAVE_RADIX_HPP
#define HAVE_RADIX_HPP
// Autogenerated - do not edit by hand !
#define GLOBAL_HASH_TYPE unsigned int
#define GLOBAL_COMPRESS_TOL 16
#define GLOBAL_UNION_OR_STRUCT union
#define stringify(s) #s
#define xstringify(s) stringify(s)
#define THREAD_NUM 0
#define GLOBAL_INDEX_VECTOR std::vector<GLOBAL_INDEX_TYPE>
#define GLOBAL_INDEX_TYPE unsigned int
#define GLOBAL_MAX_NUM_THREADS 48
#define INDEX_OVERFLOW(x) \
  ((size_t)(x) >= (size_t)std::numeric_limits<GLOBAL_INDEX_TYPE>::max())
#define ASSERT(x)                                \
  if (!(x)) {                                    \
    Rcout << "ASSERTION FAILED: " << #x << "\n"; \
    abort();                                     \
  }
#define GLOBAL_REPLAY_TYPE ad_aug
#define INHERIT_CTOR(A, B)                          \
  A() {}                                            \
  template <class T1>                               \
  A(T1 x1) : B(x1) {}                               \
  template <class T1, class T2>                     \
  A(T1 x1, T2 x2) : B(x1, x2) {}                    \
  template <class T1, class T2, class T3>           \
  A(T1 x1, T2 x2, T3 x3) : B(x1, x2, x3) {}         \
  template <class T1, class T2, class T3, class T4> \
  A(T1 x1, T2 x2, T3 x3, T4 x4) : B(x1, x2, x3, x4) {}
#define GLOBAL_SCALAR_TYPE double
#include <algorithm>
#include <vector>
#include "global.hpp"  // ASSERT

namespace radix {

/** \brief Simple radix sort implementation
    \details Beats `std::sort` for *relevant workloads* i.e. random integer
   sequences (hash codes). \tparam T **Unsigned integer** value type of vector
   to be sorted. \tparam I Integer type of the index type.
*/
template <class T, class I>
struct radix {
  /** \brief Reference to the input vector */
  const std::vector<T>& x;
  /** \brief Output: `sort(x)` */
  std::vector<T> x_sort;
  /** \brief Output: `order(x)` permutation */
  std::vector<I> x_order;
  /** \brief Radix width in bits */
  static const int radix_width = 8;
  /** \brief Total width in bits */
  static const int total_width = sizeof(T) * 8;
  /** \brief Number of unique keys used per radix pass */
  static const int num_keys = (1 << radix_width);
  /** \brief Mask to extract keys - E.g 0xFF by default */
  static const int mask = num_keys - 1;
  size_t key(T x, int k) { return (x >> k) & mask; }

  radix(const std::vector<T>& x) : x(x) {
    ASSERT(T(-1) > T(0));
    ASSERT(total_width % radix_width == 0);
  }
  template <bool get_order>
  void run_sort() {
    T bitwise_min = ~0, bitwise_max = 0;
    for (size_t i = 0; i < x.size(); i++) {
      bitwise_min &= x[i];
      bitwise_max |= x[i];
    }

    x_sort = x;
    x_order.resize(x.size() * get_order);
    for (size_t i = 0; i < x_order.size(); i++) x_order[i] = i;

    std::vector<size_t> count(num_keys);
    std::vector<size_t> pos(num_keys);

    std::vector<I> y_order(x.size() * get_order);
    std::vector<T> y_sort(x.size());
    for (size_t k = 0; k < total_width; k += radix_width) {
      if (key(bitwise_min, k) == key(bitwise_max, k)) continue;

      std::fill(count.begin(), count.end(), 0);
      for (size_t i = 0; i < x.size(); i++) {
        count[key(x[i], k)]++;
      }

      std::fill(pos.begin(), pos.end(), 0);
      for (size_t i = 1; i < pos.size(); i++) {
        pos[i] = pos[i - 1] + count[i - 1];
      }
      for (size_t i = 0; i < x.size(); i++) {
        T x_sort_i = x_sort[i];
        size_t& j = pos[key(x_sort_i, k)];

        y_sort[j] = x_sort_i;
        if (get_order) y_order[j] = x_order[i];
        j++;
      }
      std::swap(x_sort, y_sort);
      std::swap(x_order, y_order);
    }
  }
  std::vector<T> sort() {
    run_sort<false>();
    return x_sort;
  }
  std::vector<I> order() {
    run_sort<true>();
    return x_order;
  }
  std::vector<I> first_occurance() {
    run_sort<true>();
    std::vector<I> ans(x_order.size());
    for (size_t i = 0; i < ans.size(); i++) ans[i] = i;
    for (size_t i = 1; i < x_sort.size(); i++) {
      if (x_sort[i - 1] == x_sort[i]) {
        ans[x_order[i]] = ans[x_order[i - 1]];
      }
    }
    return ans;
  }
};

template <class I, class T>
std::vector<I> order(const std::vector<T>& x) {
  return radix<T, I>(x).order();
}

template <class I, class T>
std::vector<I> first_occurance(const std::vector<T>& x) {
  return radix<T, I>(x).first_occurance();
}

}  // namespace radix
#endif  // HAVE_RADIX_HPP
