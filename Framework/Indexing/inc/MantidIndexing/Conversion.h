// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidIndexing/DllConfig.h"
#include "MantidIndexing/GlobalSpectrumIndex.h"
#include "MantidIndexing/SpectrumNumber.h"

#include <algorithm>
#include <iterator>

namespace Mantid {
namespace Indexing {

/** Conversion helpers from and to (vectors of) integer types such as
  SpectrumNumber and GlobalSpectrumIndex.

  @author Simon Heybrock
  @date 2017
*/

/** Convert std::vector<In> to std::vector<Out> by static-casting all elements.
 *
 * `In` must be an integral type and `Out` can be any type inheriting
 * Indexing::IndexType, such as SpectrumNumber or GlobalSpectrumIndex. The
 * conversion is done using static_cast without any range check. It is the
 * responsibility of the caller to not pass any vectors containing elements that
 * suffer from loss of data in a static_cast. */
template <class Out, class In, typename std::enable_if<std::is_integral<In>::value>::type * = nullptr>
std::vector<Out> castVector(const std::vector<In> &indices) {
  std::vector<Out> converted;
  converted.reserve(indices.size());
  std::transform(indices.cbegin(), indices.cend(), std::back_inserter(converted),
                 [](const auto index) { return static_cast<typename Out::underlying_type>(index); });
  return converted;
}

/** Convert std::vector<In> to std::vector<Out> by static-casting all elements.
 *
 * `Out` must be an integral type and `In` can be any type inheriting
 * Indexing::IndexType, such as SpectrumNumber or GlobalSpectrumIndex. The
 * conversion is done using static_cast without any range check. It is the
 * responsibility of the caller to not pass any vectors containing elements that
 * suffer from loss of data in a static_cast. */
template <class Out, class In, typename std::enable_if<!std::is_integral<In>::value>::type * = nullptr>
std::vector<Out> castVector(const std::vector<In> &indices) {
  std::vector<Out> converted;
  converted.reserve(indices.size());
  std::transform(indices.cbegin(), indices.cend(), std::back_inserter(converted),
                 [](const auto &index) { return static_cast<Out>(static_cast<typename In::underlying_type>(index)); });
  return converted;
}

} // namespace Indexing
} // namespace Mantid
