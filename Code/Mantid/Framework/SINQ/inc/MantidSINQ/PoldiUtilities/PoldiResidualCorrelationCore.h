#ifndef MANTID_SINQ_POLDIRESIDUALCORRELATIONCORE_H_
#define MANTID_SINQ_POLDIRESIDUALCORRELATIONCORE_H_

#include "MantidSINQ/DllConfig.h"
#include "MantidSINQ/PoldiUtilities/PoldiAutoCorrelationCore.h"

namespace Mantid {
namespace Poldi {

/** PoldiResidualCorrelationCore

    This class is closely related to PoldiAutoCorrelationCore, which it
    inherits from. It performs the same analysis as the base class, just
    on different data - the residuals of a POLDI 2D fit.

    Please note that this class modifies the count data passed to
    PoldiAutoCorrelationCore::calculate().

        @author Michael Wedel, Paul Scherrer Institut - SINQ
        @date 20/11/2014

    Copyright © 2014 PSI-MSS

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
class MANTID_SINQ_DLL PoldiResidualCorrelationCore
    : public PoldiAutoCorrelationCore {
public:
  PoldiResidualCorrelationCore(Kernel::Logger &g_log, double weight = 0.0);
  virtual ~PoldiResidualCorrelationCore() {}

  double getWeight() const;
  void setWeight(double newWeight);

protected:
  double getNormCounts(int x, int y) const;
  double
  reduceChopperSlitList(const std::vector<UncertainValue> &valuesWithSigma,
                        double weight) const;
  double calculateAverage(const std::vector<double> &values) const;
  double calculateAverageDeviationFromValue(const std::vector<double> &values,
                                            double value) const;
  double calculateCorrelationBackground(double sumOfCorrelationCounts,
                                        double sumOfCounts) const;

  DataObjects::Workspace2D_sptr
  finalizeCalculation(const std::vector<double> &correctedCorrelatedIntensities,
                      const std::vector<double> &dValues) const;
  void distributeCorrelationCounts(
      const std::vector<double> &correctedCorrelatedIntensities,
      const std::vector<double> &dValues) const;
  void correctCountData() const;

  void addToCountData(int x, int y, double delta) const;

  double m_weight;
};

} // namespace Poldi
} // namespace Mantid

#endif /* MANTID_SINQ_POLDIRESIDUALCORRELATIONCORE_H_ */
