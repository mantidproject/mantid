// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidHistogramData/Addable.h"
#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/HistogramY.h"
#include "MantidHistogramData/Iterable.h"
#include "MantidHistogramData/Offsetable.h"
#include "MantidHistogramData/Scalable.h"
#include "MantidHistogramData/VectorOf.h"

namespace Mantid {
namespace HistogramData {
class BinEdges;
class Counts;

/** Frequencies

  Container for the frequencies in a histogram. A copy-on-write mechanism saves
  memory and makes copying cheap. The implementation is based on
  detail::VectorOf, a wrapper around a cow_ptr. Mixins such as detail::Iterable
  provide iterators and other operations.

  @author Simon Heybrock
  @date 2016
*/
class MANTID_HISTOGRAMDATA_DLL Frequencies : public detail::VectorOf<Frequencies, HistogramY>,
                                             public detail::Addable<Frequencies>,
                                             public detail::Iterable<Frequencies>,
                                             public detail::Offsetable<Frequencies>,
                                             public detail::Scalable<Frequencies> {
public:
  using VectorOf<Frequencies, HistogramY>::VectorOf;
  using VectorOf<Frequencies, HistogramY>::operator=;
  // Multiple inheritance causes ambiguous overload, bring operators into scope.
  using detail::Addable<Frequencies>::operator+;
  using detail::Addable<Frequencies>::operator+=;
  using detail::Addable<Frequencies>::operator-;
  using detail::Addable<Frequencies>::operator-=;
  using detail::Offsetable<Frequencies>::operator+;
  using detail::Offsetable<Frequencies>::operator+=;
  using detail::Offsetable<Frequencies>::operator-;
  using detail::Offsetable<Frequencies>::operator-=;
  /// Default constructor, creates a NULL object.
  Frequencies() = default;
  // The copy and move constructor and assignment are not captured properly by
  // the using declaration above, so we need them here explicitly.
  /// Copy constructor. Lightweight, internal data will be shared.
  Frequencies(const Frequencies &) = default;
  /// Move constructor.
  Frequencies(Frequencies &&) = default;
  /// Copy assignment. Lightweight, internal data will be shared.
  Frequencies &operator=(const Frequencies &) & = default;
  /// Move assignment.
  Frequencies &operator=(Frequencies &&) & = default;

  Frequencies(const Counts &counts, const BinEdges &edges);
  Frequencies(Counts &&counts, const BinEdges &edges);
};

} // namespace HistogramData
} // namespace Mantid
