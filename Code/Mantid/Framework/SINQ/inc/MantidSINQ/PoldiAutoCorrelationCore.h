#ifndef MANTID_SINQ_POLDIAUTOCORRELATIONCORE_H_
#define MANTID_SINQ_POLDIAUTOCORRELATIONCORE_H_

#include "MantidKernel/System.h"

#include "MantidSINQ/PoldiAbstractDetector.h"
#include "MantidSINQ/PoldiAbstractChopper.h"


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
class DLLExport PoldiAutoCorrelationCore
{
public:
    PoldiAutoCorrelationCore();
    virtual ~PoldiAutoCorrelationCore() { }

    void setInstrument(boost::shared_ptr<PoldiAbstractDetector> detector, boost::shared_ptr<PoldiAbstractChopper> chopper);
    void setWavelengthRange(double lambdaMin, double lambdaMax);

    std::pair<std::vector<double>, std::vector<double> > calculate(std::vector<double> timeData, std::vector<double> countData);
    
protected:
    virtual double getDeltaD(double deltaT);
    virtual std::pair<int, int> getDRangeAsDeltaMultiples(double getDeltaD);
    virtual std::vector<double> getDGrid(double deltaT);

    virtual double getTOFForD1(double distance, double sinTheta);

    boost::shared_ptr<PoldiAbstractDetector> m_detector;
    boost::shared_ptr<PoldiAbstractChopper> m_chopper;

    std::set<int> m_deadWires;

    std::pair<double, double> m_wavelengthRange;

    double m_deltaT;
    size_t m_timeElements;
    size_t m_detectorElements;
};


} // namespace Poldi
} // namespace Mantid

#endif  /* MANTID_SINQ_POLDIAUTOCORRELATIONCORE_H_ */
