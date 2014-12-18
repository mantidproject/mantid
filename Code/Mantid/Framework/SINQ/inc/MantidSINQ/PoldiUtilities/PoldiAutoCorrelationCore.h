#ifndef MANTID_SINQ_POLDIAUTOCORRELATIONCORE_H_
#define MANTID_SINQ_POLDIAUTOCORRELATIONCORE_H_

#include "MantidKernel/System.h"

#include "MantidSINQ/DllConfig.h"

#include "MantidSINQ/PoldiUtilities/PoldiAbstractDetector.h"
#include "MantidSINQ/PoldiUtilities/PoldiAbstractChopper.h"
#include "MantidSINQ/PoldiUtilities/UncertainValue.h"

#include "MantidDataObjects/Workspace2D.h"


namespace Mantid
{
namespace Poldi
{

/// Helper struct for inner correlation method.
struct CountLocator
{
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
class MANTID_SINQ_DLL PoldiAutoCorrelationCore
{
public:
    PoldiAutoCorrelationCore(Kernel::Logger& g_log);
    virtual ~PoldiAutoCorrelationCore() { }

    void setInstrument(const PoldiAbstractDetector_sptr &detector, const PoldiAbstractChopper_sptr &chopper);
    void setWavelengthRange(double lambdaMin, double lambdaMax);

    DataObjects::Workspace2D_sptr calculate(DataObjects::Workspace2D_sptr &countData, const DataObjects::Workspace2D_sptr &normCountData = DataObjects::Workspace2D_sptr());

protected:
    double getNormalizedTOFSum(const std::vector<double> &normalizedTofs) const;
    std::vector<double> calculateDWeights(const std::vector<double> &tofsFor1Angstrom, double deltaT, double deltaD, size_t nd) const;

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
    virtual DataObjects::Workspace2D_sptr finalizeCalculation(const std::vector<double> &correctedCorrelatedIntensities, const std::vector<double> &dValues) const;

    boost::shared_ptr<PoldiAbstractDetector> m_detector;
    boost::shared_ptr<PoldiAbstractChopper> m_chopper;

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
    Kernel::Logger& m_logger;
};


} // namespace Poldi
} // namespace Mantid

#endif  /* MANTID_SINQ_POLDIAUTOCORRELATIONCORE_H_ */
