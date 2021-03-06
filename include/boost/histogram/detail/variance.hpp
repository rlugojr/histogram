// Copyright 2015-2017 Hans Dembinski
//
// Distributed under the Boost Software License, version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef _BOOST_HISTOGRAM_DETAIL_VARIANCE_HPP_
#define _BOOST_HISTOGRAM_DETAIL_VARIANCE_HPP_

#include <boost/histogram/detail/meta.hpp>

namespace boost {
namespace histogram {
namespace detail {

namespace {
template <typename Storage>
typename std::enable_if<has_variance<Storage>::value,
                        typename Storage::value_type>::type
variance_impl(const Storage &s, std::size_t i) {
  return s.variance(i);
} // delegate to Storage implementation

template <typename Storage>
typename std::enable_if<!(has_variance<Storage>::value),
                        typename Storage::value_type>::type
variance_impl(const Storage &s, std::size_t i) {
  return s.value(i);
} // standard Poisson estimate
} // namespace

template <typename Storage>
typename Storage::value_type variance(const Storage &s, std::size_t i) {
  return variance_impl<Storage>(s, i);
}
} // namespace detail
} // namespace histogram
} // namespace boost

#endif
