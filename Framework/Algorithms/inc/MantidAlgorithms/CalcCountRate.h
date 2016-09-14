#ifndef MANTID_ALGORITHMS_CALCCOUNTRATE_H_
#define MANTID_ALGORITHMS_CALCCOUNTRATE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid {
namespace Algorithms {

/**  In normal circumstances an instrument in event mode counts neutrons with
  constant steady rate which depends on beam intensity, instrument settings and
  sample.
  Sometimes hardware issues cause it to count much faster or slower. This
  appears as spurious signals on the final neutron images and users want to
  filter these signals.

  The algorithm calculates neutrons counting rate as the function of the
  experiment's time
  and adds appropriate logs to the event workspace
  for further event filtering on the basis of these logs, if the log values in
  some parts differ strongly from average values.


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
class DLLExport CalcCountRate : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;

protected: // for testing, actually private
  /// pointer to the log used to normalize results or NULL if no such log
  /// present on input workspace.
  Kernel::TimeSeriesProperty<double> const * m_pNormalizationLog{nullptr};
  /// default number of points in the target log
  size_t m_numLogSteps{100};

  /// specifies if rate is calculated in selected frame interval (range defined)
  /// or all frame should be used
  bool m_rangeExplicit{false};
  /// spurion search ranges
  double m_XRangeMin{0}, m_XRangeMax{0};

  DataObjects::EventWorkspace_sptr m_workingWS;

  void setWSDataRanges(DataObjects::EventWorkspace_sptr &InputWorkspace);

  void
  setOutLogParameters(const DataObjects::EventWorkspace_sptr &InputWorkspace);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CALCCOUNTRATE_H_ */
