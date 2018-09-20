#ifndef MANTID_HISTOGRAMDATA_STANDARDDEVIATIONVECTOROF_H_
#define MANTID_HISTOGRAMDATA_STANDARDDEVIATIONVECTOROF_H_

#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/Iterable.h"
#include "MantidHistogramData/VectorOf.h"

#include <cmath>

namespace Mantid {
namespace HistogramData {
namespace detail {

/** StandardDeviationVectorOf

  This class is an implementation detail of classes like
  HistogramData::CountStandardDeviations and
  HistogramData::FrequencyStandardDeviations. It
  extends VectorOf with conversions (construction and assignment) from objects
  holding variances.

  The first template parameter is the type of the inheriting class. This is the
  CRTP (curiously recurring template pattern).

  The second template parameter is the type of the object to store.

  The third template parameter is the type of the associated variance object.

  @author Simon Heybrock
  @date 2016

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
template <class T, class CowType, class Variances>
class StandardDeviationVectorOf : public VectorOf<T, CowType>,
                                  public Iterable<T> {
public:
  using VectorOf<T, CowType>::VectorOf;
  using VectorOf<T, CowType>::operator=;

  StandardDeviationVectorOf() = default;
  // The copy and move constructor and assignment are not captured properly by
  // the using declaration above, so we need them here explicitly.
  StandardDeviationVectorOf(const StandardDeviationVectorOf &) = default;
  StandardDeviationVectorOf(StandardDeviationVectorOf &&) = default;
  StandardDeviationVectorOf &
  operator=(const StandardDeviationVectorOf &) & = default;
  StandardDeviationVectorOf &
  operator=(StandardDeviationVectorOf &&) & = default;

  /// Copy construct from variances, taking the square-root of each variance.
  StandardDeviationVectorOf(const Variances &variances);
  /// Move construct from variances, taking the square-root of each variance.
  StandardDeviationVectorOf(Variances &&variances);
  /// Copy assignment from variances, taking the square-root of each variance.
  StandardDeviationVectorOf &operator=(const Variances &variances) &;
  /// Move assignment from variances, taking the square-root of each variance.
  StandardDeviationVectorOf &operator=(Variances &&variances) &;

protected:
  // This is used as base class only, cannot delete polymorphically, so
  // destructor is protected.
  ~StandardDeviationVectorOf() = default;
};

template <class T, class CowType, class Variances>
StandardDeviationVectorOf<T, CowType, Variances>::StandardDeviationVectorOf(
    const Variances &variances) {
  if (!variances)
    return;
  auto &derived = static_cast<T &>(*this);
  derived.operator=(variances.cowData());
  std::transform(derived.cbegin(), derived.cend(), derived.begin(),
                 static_cast<double (*)(const double)>(sqrt));
}

template <class T, class CowType, class Variances>
StandardDeviationVectorOf<T, CowType, Variances>::StandardDeviationVectorOf(
    Variances &&variances) {
  if (!variances)
    return;
  auto &derived = static_cast<T &>(*this);
  derived.operator=(variances.cowData());
  // Data is now shared between variances and derived. We want to avoid
  // triggering a copy by the iterator access below, so we have to set variances
  // to null. We cannot directly null the cow_ptr in variances, since it is of a
  // different type and we do not have access to its private members.
  variances = Kernel::cow_ptr<CowType>(nullptr);
  std::transform(derived.cbegin(), derived.cend(), derived.begin(),
                 static_cast<double (*)(const double)>(sqrt));
}

template <class T, class CowType, class Variances>
StandardDeviationVectorOf<T, CowType, Variances> &
StandardDeviationVectorOf<T, CowType, Variances>::
operator=(const Variances &variances) & {
  StandardDeviationVectorOf<T, CowType, Variances> tmp(variances);
  auto &derived = static_cast<T &>(*this);
  derived.operator=(tmp.cowData());
  return *this;
}

template <class T, class CowType, class Variances>
StandardDeviationVectorOf<T, CowType, Variances> &
StandardDeviationVectorOf<T, CowType, Variances>::
operator=(Variances &&variances) & {
  StandardDeviationVectorOf<T, CowType, Variances> tmp(std::move(variances));
  auto &derived = static_cast<T &>(*this);
  derived.operator=(tmp.cowData());
  return *this;
}

} // namespace detail
} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_STANDARDDEVIATIONVECTOROF_H_ */
