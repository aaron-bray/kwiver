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

#ifndef VITAL_RANGE_SLIDING_H
#define VITAL_RANGE_SLIDING_H

#include <vital/range/defs.h>
#include <vital/range/integer_sequence.h>

#include <array>

namespace kwiver {
namespace vital {
namespace range {

// ----------------------------------------------------------------------------
/// Sliding window range adapter.
/**
 * This range adapter provides a sliding view on a range. Given a \c Size, the
 * first iteration will return that number of elements, in the same order as
 * the input range. Each subsequent step will shift this window by one. If the
 * input range has fewer than \c Size elements, the output range will be empty.
 */
template < size_t Size, typename Range >
class sliding_view
{
protected:
  using detail = range_detail< Range >;
  using range_iterator_t = typename detail::iterator_t;

public:
  using value_t = std::array< typename detail::value_t, Size >;

  class const_iterator
  {
  public:
    const_iterator( const_iterator const& ) = default;
    const_iterator& operator=( const_iterator const& ) = default;

    bool operator!=( const_iterator const& other ) const;

    value_t operator*() const;

    const_iterator& operator++();

  protected:
    friend class sliding_view;
    const_iterator( range_iterator_t iter, range_iterator_t const& end );

    struct dereference_helper
    {
      template < size_t... Indices >
      static value_t dereference(
        std::array< range_iterator_t, Size > const& iters,
        integer_sequence_t< size_t, Indices... > )
      {
        return {{ *( iters[ Indices ] )... }};
      }
    };

    std::array< range_iterator_t, Size > m_iter;
  };

  sliding_view( Range const& range ) : m_range( range ) {}

  const_iterator begin() const
  { return { detail::begin( m_range ), detail::end( m_range ) }; }

  const_iterator end() const
  { return { detail::end( m_range ), detail::end( m_range ) }; }

protected:
  Range const& m_range;
};

// ----------------------------------------------------------------------------
template < size_t Size, typename Range >
sliding_view< Size, Range >::const_iterator
::const_iterator( range_iterator_t iter, range_iterator_t const& end )
{
  for ( size_t i = 0; i < Size; ++i )
  {
    m_iter[ i ] = iter;
    if ( iter != end ) ++iter;
  }
}

// ----------------------------------------------------------------------------
template < size_t Size, typename Range >
typename sliding_view< Size, Range >::value_t
sliding_view< Size, Range >::const_iterator
::operator*() const
{
  auto const indices = make_integer_sequence< size_t, Size >();
  return dereference_helper::dereference( m_iter, indices );
}

// ----------------------------------------------------------------------------
template < size_t Size, typename Range >
bool
sliding_view< Size, Range >::const_iterator
::operator!=( const_iterator const& other ) const
{
  return m_iter.back() != other.m_iter.back();
}

// ----------------------------------------------------------------------------
template < size_t Size, typename Range >
typename sliding_view< Size, Range >::const_iterator&
sliding_view< Size, Range >::const_iterator
::operator++()
{
  for ( size_t i = 0; i < Size - 1; ++i )
  {
    m_iter[ i ] = m_iter[ i + 1 ];
  }
  ++m_iter.back();

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

KWIVER_RANGE_ADAPTER_TEMPLATE( sliding, ( size_t Size ), ( Size ) )

} } } // end namespace

#endif
