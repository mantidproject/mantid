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

    void setInstrument(boost::shared_ptr<PoldiAbstractDetector> detector, boost::shared_ptr<PoldiAbstractChopper> chopper);
    void setWavelengthRange(double lambdaMin, double lambdaMax);

    DataObjects::Workspace2D_sptr calculate(DataObjects::Workspace2D_sptr countData);

    // conversion between TOF (in musec) and d (in Angstrom), related through distance in mm and sin(theta)
    static double dtoTOF(double d, double distance, double sinTheta);
    static double TOFtod(double tof, double distance, double sinTheta);

protected:
    virtual double getDeltaD(double deltaT);
    std::pair<int, int> getDRangeAsDeltaMultiples(double getDeltaD);
    virtual std::vector<double> getDGrid(double deltaD);

    double getNormalizedTOFSum(std::vector<double> normalizedTofs);
    std::vector<double> calculateDWeights(std::vector<double> tofsFor1Angstrom, double deltaT, double deltaD, size_t nd);

    double getRawCorrelatedIntensity(double dValue, double weight);
    virtual UncertainValue getCMessAndCSigma(double dValue, double slitTimeOffset, int index);
    double reduceChopperSlitList(std::vector<UncertainValue> valuesWithSigma, double weight);
    double sumIOverSigmaInverse(std::vector<double> &iOverSigmas);

    std::vector<double> getDistances(std::vector<int> elements);
    std::vector<double> getTofsFor1Angstrom(std::vector<int> elements);

    virtual double getCounts(int x, int y);
    virtual double getNormCounts(int x, int y);

    virtual int getElementFromIndex(int index);
    virtual double getTofFromIndex(int index);
    double getSumOfCounts(int timeBinCount, std::vector<int> detectorElements);

    virtual int cleanIndex(int index, int maximum);

    void setCountData(DataObjects::Workspace2D_sptr countData);

    double correctedIntensity(double intensity, double weight);


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
    int m_elementsMaxIndex;

    double m_sumOfWeights;
    double m_correlationBackground;

    double m_damp;
    Kernel::Logger& m_logger;
};


} // namespace Poldi
} // namespace Mantid

#endif  /* MANTID_SINQ_POLDIAUTOCORRELATIONCORE_H_ */
