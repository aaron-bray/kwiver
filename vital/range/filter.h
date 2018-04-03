/*ckwg +29
 * Copyright 2018 by Kitware, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of Kitware, Inc. nor the names of any contributors may be used
 *    to endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef VITAL_RANGE_FILTER_H
#define VITAL_RANGE_FILTER_H

#include <vital/range/defs.h>

#include <functional>

namespace kwiver {
namespace vital {
namespace range {

// ----------------------------------------------------------------------------
/// Filtering range adapter.
/**
 * This range adapter applies a filter to the elements of a range. When
 * iterating over the range, only elements which pass the filter (that is, the
 * filter functor returns \c true) will be seen.
 */
template < typename FilterFunction, typename Range >
class filter_view
{
protected:
  using detail = range_detail< Range >;
  using range_iterator_t = typename detail::iterator_t;

public:
  using value_t = typename detail::value_t;
  using filter_function_t = FilterFunction;

  class const_iterator
  {
  public:
    const_iterator( const_iterator const& ) = default;
    const_iterator& operator=( const_iterator const& ) = default;

    bool operator!=( const_iterator const& other ) const
    { return m_iter != other.m_iter; }

    value_t operator*() const { return *m_iter; }

    const_iterator& operator++();

  protected:
    friend class filter_view;
    const_iterator( range_iterator_t const& iter,
                    range_iterator_t const& end,
                    filter_function_t const& func )
      : m_iter{ iter }, m_end{ end }, m_func( func ) {}

    range_iterator_t m_iter, m_end;
    filter_function_t const& m_func;
  };

  filter_view( Range const& range, filter_function_t func )
    : m_range( range ), m_func{ func } {}

  const_iterator begin() const;

  const_iterator end() const
  { return { detail::end( m_range ), detail::end( m_range ), m_func }; }

protected:
  Range const& m_range;
  filter_function_t m_func;
};

// ----------------------------------------------------------------------------
template < typename FilterFunction, typename Range >
typename filter_view< FilterFunction, Range >::const_iterator
filter_view< FilterFunction, Range >
::begin() const
{
  auto iter =
    const_iterator{ detail::begin( m_range ), detail::end( m_range ), m_func };

  return ( m_func( *iter ) ? iter : ++iter );
}

// ----------------------------------------------------------------------------
template < typename FilterFunction, typename Range >
typename filter_view< FilterFunction, Range >::const_iterator&
filter_view< FilterFunction, Range >::const_iterator
::operator++()
{
  while ( m_iter != m_end )
  {
    ++m_iter;
    if ( m_iter != m_end && m_func( *m_iter ) ) break;
  }
  return *this;
}

///////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------------
template < typename Functor >
struct filter_view_adapter_t
{
  template < typename Range >
  filter_view< Functor, Range >
  adapt( Range const& range ) const
  { return { range, m_func }; }

  Functor m_func;
};

KWIVER_RANGE_ADAPTER_FUNCTION( filter )

} } } // end namespace

#endif
