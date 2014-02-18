#ifndef MANTID_SINQ_POLDIAUTOCORRELATIONCORE_H_
#define MANTID_SINQ_POLDIAUTOCORRELATIONCORE_H_

#include "MantidKernel/System.h"

#include "MantidSINQ/DllConfig.h"

#include "MantidSINQ/PoldiAbstractDetector.h"
#include "MantidSINQ/PoldiAbstractChopper.h"

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
    PoldiAutoCorrelationCore();
    virtual ~PoldiAutoCorrelationCore() { }

    void setInstrument(boost::shared_ptr<PoldiAbstractDetector> detector, boost::shared_ptr<PoldiAbstractChopper> chopper);
    void setWavelengthRange(double lambdaMin, double lambdaMax);

    void calculate(DataObjects::Workspace2D_sptr countData, DataObjects::Workspace2D_sptr outputWorkspace);

    // conversion between TOF (in musec) and d (in Angstrom), related through distance in mm and sin(theta)
    static double dtoTOF(double d, double distance, double sinTheta);
    static double TOFtod(double tof, double distance, double sinTheta);

protected:
    double getDeltaD(double deltaT);
    std::pair<int, int> getDRangeAsDeltaMultiples(double getDeltaD);
    std::vector<double> getDGrid(double deltaT);

    double getNormalizedTOFSum(std::vector<double> tofsForD1, double deltaT, size_t nd);
    void calculateDWeights(std::vector<double> tofsForD1, double deltaT, size_t nd);
    double getNormalizedTOFSumAlternative(std::vector<double> tofsForD1, double deltaT, size_t nd);

    double getRawCorrelatedIntensity(double dValue, double weight);
    std::pair<double, double> getCMessAndCSigma(double dValue, double slitTimeOffset, int index);
    double reduceChopperSlitList(std::vector<std::pair<double, double> > valuesWithSigma, double weight);

    std::vector<double> getDistances(std::vector<int> elements);
    std::vector<double> getTofsForD1(std::vector<int> elements);

    double getCounts(int x, int y);
    double getNormCounts(int x, int y);

    int getElement(int index);
    double getTof(int index);
    double getSumOfCounts(int timeElements, std::vector<int> detectorElements);

    int cleanIndex(int index, int maximum);


    boost::shared_ptr<PoldiAbstractDetector> m_detector;
    boost::shared_ptr<PoldiAbstractChopper> m_chopper;

    std::pair<double, double> m_wavelengthRange;

    double m_deltaT;
    double m_deltaD;
    int m_timeElements;
    std::vector<int> m_detectorElements;

    std::vector<double> m_weightsForD;
    std::vector<double> m_tofsFor1Angstrom;

    std::vector<int> m_indices;

    DataObjects::Workspace2D_sptr m_countData;

    double m_damp;
};


} // namespace Poldi
} // namespace Mantid

#endif  /* MANTID_SINQ_POLDIAUTOCORRELATIONCORE_H_ */
