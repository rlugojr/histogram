// Copyright 2015-2016 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef _BOOST_HISTOGRAM_DETAIL_META_HPP_
#define _BOOST_HISTOGRAM_DETAIL_META_HPP_

#include <boost/mpl/not.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/mpl/copy_if.hpp>
#include <boost/mpl/back_inserter.hpp>
#include <iterator>
#include <limits>
#include <type_traits>

namespace boost {
namespace histogram {
namespace detail {

template <typename T> struct has_variance {
  template <typename> static std::false_type test(...);

  template <typename C>
  static decltype(std::declval<C &>().variance(0), std::true_type{}) test(int);

  static bool const value = decltype(test<T>(0))::value;
};

template <typename T, typename = decltype(std::declval<T &>().size(),
                                          std::declval<T &>().increase(0),
                                          std::declval<T &>().value(0))>
struct is_storage {};

template <typename T,
          typename = decltype(*std::declval<T &>(), ++std::declval<T &>())>
struct is_iterator {};

template <typename T, typename = decltype(std::begin(std::declval<T &>()),
                                          std::end(std::declval<T &>()))>
struct is_sequence {};

template <typename MainVector, typename AuxVector> struct combine {
  using type = typename mpl::copy_if<
      AuxVector,
      mpl::not_<mpl::contains<MainVector, mpl::_1>>,
      mpl::back_inserter<MainVector>
    >::type;
};

} // namespace detail
} // namespace histogram
} // namespace boost

#endif
