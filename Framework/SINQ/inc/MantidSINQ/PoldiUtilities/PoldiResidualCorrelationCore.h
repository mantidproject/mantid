// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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
  */
class MANTID_SINQ_DLL PoldiResidualCorrelationCore : public PoldiAutoCorrelationCore {
public:
  PoldiResidualCorrelationCore(Kernel::Logger &g_log, double weight = 0.0);
  double getWeight() const;
  void setWeight(double newWeight);

protected:
  double getNormCounts(int x, int y) const override;
  double reduceChopperSlitList(const std::vector<UncertainValue> &valuesWithSigma, double weight) const override;
  double calculateAverage(const std::vector<double> &values) const;
  double calculateAverageDeviationFromValue(const std::vector<double> &values, double value) const;
  double calculateCorrelationBackground(double sumOfCorrelationCounts, double sumOfCounts) const override;

  DataObjects::Workspace2D_sptr finalizeCalculation(const std::vector<double> &correctedCorrelatedIntensities,
                                                    const std::vector<double> &dValues) const override;
  void distributeCorrelationCounts(const std::vector<double> &correctedCorrelatedIntensities,
                                   const std::vector<double> &dValues) const;
  void correctCountData() const;

  void addToCountData(int x, int y, double delta) const;

  double m_weight;
};

} // namespace Poldi
} // namespace Mantid
