// Copyright 2015-2017 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef _BOOST_HISTOGRAM_HISTOGRAM_IMPL_STATIC_HPP_
#define _BOOST_HISTOGRAM_HISTOGRAM_IMPL_STATIC_HPP_

#include <boost/call_traits.hpp>
#include <boost/config.hpp>
#include <boost/fusion/adapted/mpl.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <boost/fusion/container/vector/convert.hpp>
#include <boost/fusion/include/algorithm.hpp>
#include <boost/fusion/include/as_vector.hpp>
#include <boost/fusion/include/comparison.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/fusion/include/mpl.hpp>
#include <boost/fusion/include/sequence.hpp>
#include <boost/fusion/sequence.hpp>
#include <boost/fusion/sequence/comparison.hpp>
#include <boost/histogram/axis.hpp>
#include <boost/histogram/detail/axis_visitor.hpp>
#include <boost/histogram/detail/meta.hpp>
#include <boost/histogram/detail/utility.hpp>
#include <boost/histogram/histogram_fwd.hpp>
#include <boost/mpl/count.hpp>
#include <boost/mpl/empty.hpp>
#include <boost/mpl/vector.hpp>
#include <type_traits>

// forward declaration for serialization
namespace boost {
namespace serialization {
class access;
}
} // namespace boost

namespace boost {
namespace histogram {

template <typename Axes, typename Storage>
class histogram<Static, Axes, Storage> {
  static_assert(!mpl::empty<Axes>::value, "at least one axis required");
  using size_pair = std::pair<std::size_t, std::size_t>;
  using axes_size = typename fusion::result_of::size<Axes>::type;
  using axes_type = typename fusion::result_of::as_vector<Axes>::type;

public:
  using value_type = typename Storage::value_type;

public:
  histogram() = default;
  histogram(const histogram &rhs) = default;
  histogram(histogram &&rhs) = default;
  histogram &operator=(const histogram &rhs) = default;
  histogram &operator=(histogram &&rhs) = default;

  template <typename... Axes1>
  explicit histogram(const Axes1 &... axes) : axes_(axes...) {
    storage_ = Storage(field_count());
  }

  // template <typename... Axes1>
  // explicit histogram(Axes1 &&... axes) : axes_(std::move(axes)...) {
  //   storage_ = Storage(field_count());
  // }

  template <typename D, typename A, typename S>
  explicit histogram(const histogram<D, A, S> &rhs) : storage_(rhs.storage_) {
    detail::axes_assign(axes_, rhs.axes_);
  }

  template <typename D, typename A, typename S>
  histogram &operator=(const histogram<D, A, S> &rhs) {
    if (static_cast<const void *>(this) != static_cast<const void *>(&rhs)) {
      detail::axes_assign(axes_, rhs.axes_);
      storage_ = rhs.storage_;
    }
    return *this;
  }

  template <typename D, typename A, typename S>
  bool operator==(const histogram<D, A, S> &rhs) const {
    return detail::axes_equal(axes_, rhs.axes_) && storage_ == rhs.storage_;
  }

  template <typename D, typename A, typename S>
  bool operator!=(const histogram<D, A, S> &rhs) const {
    return !operator==(rhs);
  }

  template <typename D, typename A, typename S>
  histogram &operator+=(const histogram<D, A, S> &rhs) {
    if (!detail::axes_equal(axes_, rhs.axes_)) {
      throw std::logic_error("axes of histograms differ");
    }
    storage_ += rhs.storage_;
    return *this;
  }

  template <typename... Args> void fill(const Args &... args) {
    using n_weight = typename mpl::count<mpl::vector<Args...>, weight>;
    static_assert(n_weight::value <= 1,
                  "arguments may contain at most one instance of type weight");
    static_assert(sizeof...(args) == axes_size::value + n_weight::value,
                  "number of arguments does not match histogram dimension");
    std::size_t idx = 0, stride = 1;
    double w = 0.0;
    apply_lin<detail::xlin, 0, Args...>(idx, stride, w, args...);
    if (stride) {
      if (n_weight::value)
        storage_.increase(idx, w);
      else
        storage_.increase(idx);
    }
  }

  template <typename... Indices>
  value_type value(const Indices &... indices) const {
    static_assert(sizeof...(indices) == axes_size::value,
                  "number of arguments does not match histogram dimension");
    std::size_t idx = 0, stride = 1;
    apply_lin<detail::lin, 0, Indices...>(idx, stride, indices...);
    if (stride == 0) {
      throw std::out_of_range("invalid index");
    }
    return storage_.value(idx);
  }

  template <typename... Indices>
  value_type variance(const Indices &... indices) const {
    static_assert(detail::has_variance<Storage>::value,
                  "Storage lacks variance support");
    static_assert(sizeof...(indices) == axes_size::value,
                  "number of arguments does not match histogram dimension");
    std::size_t idx = 0, stride = 1;
    apply_lin<detail::lin, 0, Indices...>(idx, stride, indices...);
    if (stride == 0) {
      throw std::out_of_range("invalid index");
    }
    return storage_.variance(idx);
  }

  /// Number of axes (dimensions) of histogram
  constexpr unsigned dim() const { return axes_size::value; }

  /// Total number of bins in the histogram (including underflow/overflow)
  std::size_t size() const { return storage_.size(); }

  /// Sum of all counts in the histogram
  double sum() const {
    double result = 0.0;
    for (std::size_t i = 0, n = size(); i < n; ++i) {
      result += storage_.value(i);
    }
    return result;
  }

  /// Reset bin counters to zero
  void reset() { storage_ = std::move(Storage(storage_.size())); }

  /// Get N-th axis
  template <unsigned N>
  constexpr typename std::add_const<
      typename fusion::result_of::value_at_c<axes_type, N>::type>::type &
  axis(std::integral_constant<unsigned, N>) const {
    static_assert(N < axes_size::value, "axis index out of range");
    return fusion::at_c<N>(axes_);
  }

  // Get first axis (convenience for 1-d histograms)
  constexpr typename std::add_const<
      typename fusion::result_of::value_at_c<axes_type, 0>::type>::type &
  axis() const {
    return fusion::at_c<0>(axes_);
  }

  /// Apply unary functor/function to each axis
  template <typename Unary> void for_each_axis(Unary &unary) const {
    fusion::for_each(axes_, unary);
  }

private:
  axes_type axes_;
  Storage storage_;

  std::size_t field_count() const {
    detail::field_count fc;
    fusion::for_each(axes_, fc);
    return fc.value;
  }

  template <template <class, class> class Lin, unsigned D, typename First,
            typename... Rest>
  void apply_lin(std::size_t &idx, std::size_t &stride, const First &x,
                 const Rest &... rest) const {
    Lin<typename fusion::result_of::value_at_c<axes_type, D>::type,
        First>::apply(idx, stride, fusion::at_c<D>(axes_), x);
    return apply_lin<Lin, D + 1, Rest...>(idx, stride, rest...);
  }

  template <template <class, class> class Lin, unsigned D>
  void apply_lin(std::size_t &idx, std::size_t &stride) const {}

  template <template <class, class> class Lin, unsigned D, typename First,
            typename... Rest>
  void apply_lin(std::size_t &idx, std::size_t &stride, double &w,
                 const First &x, const Rest &... rest) const {
    Lin<typename fusion::result_of::value_at_c<axes_type, D>::type,
        First>::apply(idx, stride, fusion::at_c<D>(axes_), x);
    return apply_lin<Lin, D + 1, Rest...>(idx, stride, w, rest...);
  }

  template <template <class, class> class Lin, unsigned D, typename,
            typename... Rest>
  void apply_lin(std::size_t &idx, std::size_t &stride, double &w,
                 const weight &x, const Rest &... rest) const {
    w = static_cast<double>(x);
    return apply_lin<Lin, D, Rest...>(idx, stride, w, rest...);
  }

  template <template <class, class> class Lin, unsigned D>
  void apply_lin(std::size_t &idx, std::size_t &stride, double &w) const {}

  template <typename D, typename A, typename S> friend class histogram;

  friend class ::boost::serialization::access;
  template <typename Archive> void serialize(Archive &, unsigned);
};

/// default static type factory
template <typename... Axes>
inline histogram<Static, mpl::vector<Axes...>>
make_static_histogram(Axes &&... axes) {
  return histogram<Static, mpl::vector<Axes...>>(std::forward<Axes>(axes)...);
}

/// static type factory with variable storage type
template <typename Storage, typename... Axes>
inline histogram<Static, mpl::vector<Axes...>, Storage>
make_static_histogram_with(Axes &&... axes) {
  return histogram<Static, mpl::vector<Axes...>, Storage>(
      std::forward<Axes>(axes)...);
}

} // namespace histogram
} // namespace boost

#endif