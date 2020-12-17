// This file is part of KWIVER, and is distributed under the
// OSI-approved BSD 3-Clause License. See top-level LICENSE file or
// https://github.com/Kitware/kwiver/blob/master/LICENSE for details.

/**
 * \file
 * \brief extract_descriptors algorithm instantiation
 */

#include <vital/algo/extract_descriptors.h>
#include <vital/algo/algorithm.txx>

namespace kwiver {
namespace vital {
namespace algo {

extract_descriptors
::extract_descriptors()
{
  attach_logger( "algo.extract_descriptors" );
}

} } }

/// \cond DoxygenSuppress
INSTANTIATE_ALGORITHM_DEF(kwiver::vital::algo::extract_descriptors);
/// \endcond
