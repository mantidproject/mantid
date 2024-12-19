// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidSINQ/DllConfig.h"

#include "MantidSINQ/PoldiUtilities/PoldiAbstractChopper.h"
#include "MantidSINQ/PoldiUtilities/PoldiAbstractDetector.h"
#include "MantidSINQ/PoldiUtilities/UncertainValue.h"

#include "MantidDataObjects/Workspace2D.h"

namespace Mantid {
namespace Poldi {

/// Helper struct for inner correlation method.
struct CountLocator {
  int detectorElement;
  double arrivalWindowCenter;
  double arrivalWindowWidth;

  double cmin;
  double cmax;

  int icmin;
  int icmax;

  int iicmin;
  int iicmax;
};

/** PoldiAutoCorrelationCore :

    Implementation of the autocorrelation algorithm used for analysis of data
    acquired with POLDI.

        @author Michael Wedel, Paul Scherrer Institut - SINQ
        @date 10/02/2014
  */
class MANTID_SINQ_DLL PoldiAutoCorrelationCore {
public:
  PoldiAutoCorrelationCore(Kernel::Logger &g_log);
  virtual ~PoldiAutoCorrelationCore() = default;

  void setInstrument(const PoldiAbstractDetector_sptr &detector, const PoldiAbstractChopper_sptr &chopper);
  void setWavelengthRange(double lambdaMin, double lambdaMax);

  DataObjects::Workspace2D_sptr
  calculate(DataObjects::Workspace2D_sptr &countData,
            const DataObjects::Workspace2D_sptr &normCountData = DataObjects::Workspace2D_sptr());

protected:
  double getNormalizedTOFSum(const std::vector<double> &normalizedTofs) const;
  std::vector<double> calculateDWeights(const std::vector<double> &tofsFor1Angstrom, double deltaT, double deltaD,
                                        size_t nd) const;

  double getRawCorrelatedIntensity(double dValue, double weight) const;
  UncertainValue getCMessAndCSigma(double dValue, double slitTimeOffset, int index) const;
  CountLocator getCountLocator(double dValue, double slitTimeOffset, int index) const;
  virtual double reduceChopperSlitList(const std::vector<UncertainValue> &valuesWithSigma, double weight) const;

  std::vector<double> getDistances(const std::vector<int> &elements) const;
  std::vector<double> getTofsFor1Angstrom(const std::vector<int> &elements) const;

  double getCounts(int x, int y) const;
  virtual double getNormCounts(int x, int y) const;

  int getElementFromIndex(int index) const;
  double getTofFromIndex(int index) const;
  double getSumOfCounts(int timeBinCount, const std::vector<int> &detectorElements) const;

  int cleanIndex(int index, int maximum) const;

  void setCountData(const DataObjects::Workspace2D_sptr &countData);
  void setNormCountData(const DataObjects::Workspace2D_sptr &normCountData);

  double correctedIntensity(double intensity, double weight) const;
  virtual double calculateCorrelationBackground(double sumOfCorrelationCounts, double sumOfCounts) const;
  virtual DataObjects::Workspace2D_sptr finalizeCalculation(const std::vector<double> &correctedCorrelatedIntensities,
                                                            const std::vector<double> &dValues) const;

  std::shared_ptr<PoldiAbstractDetector> m_detector;
  std::shared_ptr<PoldiAbstractChopper> m_chopper;

  std::pair<double, double> m_wavelengthRange;

  double m_deltaT;
  double m_deltaD;
  int m_timeBinCount;
  std::vector<int> m_detectorElements;

  std::vector<double> m_weightsForD;
  std::vector<double> m_tofsFor1Angstrom;

  std::vector<int> m_indices;

  DataObjects::Workspace2D_sptr m_countData;
  DataObjects::Workspace2D_sptr m_normCountData;

  double m_sumOfWeights;
  double m_correlationBackground;

  double m_damp;
  Kernel::Logger &m_logger;
};

} // namespace Poldi
} // namespace Mantid
