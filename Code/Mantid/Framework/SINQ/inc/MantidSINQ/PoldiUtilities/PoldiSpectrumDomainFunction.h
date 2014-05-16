#ifndef MANTID_SINQ_POLDISPECTRUMDOMAINFUNCTION_H_
#define MANTID_SINQ_POLDISPECTRUMDOMAINFUNCTION_H_

#include "MantidSINQ/DllConfig.h"
#include "MantidAPI/ParamFunction.h"
#include <string>

#include "MantidSINQ/PoldiUtilities/PoldiInstrumentAdapter.h"
#include "MantidSINQ/PoldiUtilities/PoldiSourceSpectrum.h"
#include "MantidKernel/PhysicalConstants.h"

#include "MantidDataObjects/Workspace2D.h"

namespace Mantid
{
namespace Poldi
{

/** PoldiSpectrumDomainFunction : TODO: DESCRIPTION

    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
using namespace API;
using namespace DataObjects;

struct DetectorElementCharacteristics {
    DetectorElementCharacteristics()
    { }

    DetectorElementCharacteristics(int element, PoldiAbstractDetector_sptr detector, PoldiAbstractChopper_sptr chopper)
    {
        distance = detector->distanceFromSample(element);
        totalDistance = detector->distanceFromSample(element) + chopper->distanceFromSample();
        twoTheta = detector->twoTheta(element);
        sinTheta = sin(twoTheta / 2.0);
        cosTheta = cos(twoTheta / 2.0);
        tof1A = 4947.990234375;//2.0 * totalDistance * sinTheta * PhysicalConstants::NeutronMass / (PhysicalConstants::h * 1e7);//4947.99013618171464192258; //063732507753820628;//4947.990; //Conversions::dtoTOF(1.0, totalDistance, sinTheta);
    }

    double distance;
    double totalDistance;
    double twoTheta;
    double sinTheta;
    double cosTheta;
    double tof1A;
};

class DetectorElementData
{
public:
    DetectorElementData(int element, DetectorElementCharacteristics center, PoldiAbstractDetector_sptr detector, PoldiAbstractChopper_sptr chopper)
    {
        DetectorElementCharacteristics current(element, detector, chopper);

        m_intensityFactor = pow(center.distance / current.distance, 2.0) * current.sinTheta / center.sinTheta;
        m_lambdaFactor = 2.0 * current.sinTheta / center.tof1A;
        m_timeFactor = current.sinTheta / center.sinTheta * current.totalDistance / center.totalDistance;
        m_widthFactor = current.cosTheta - center.cosTheta;
    }

    double intensityFactor() const { return m_intensityFactor; }
    double lambdaFactor() const { return m_lambdaFactor; }
    double timeFactor() const { return m_timeFactor; }
    double widthFactor() const { return m_widthFactor; }

protected:
    double m_intensityFactor;
    double m_lambdaFactor;
    double m_timeFactor;
    double m_widthFactor;
};

typedef boost::shared_ptr<const DetectorElementData> DetectorElementData_const_sptr;

class MANTID_SINQ_DLL PoldiSpectrumDomainFunction : public ParamFunction
{
public:
    PoldiSpectrumDomainFunction();
    virtual ~PoldiSpectrumDomainFunction()
    {}
    
    virtual std::string name() const { return "PoldiSpectrumDomainFunction"; }

    virtual void setWorkspace(boost::shared_ptr<const Workspace> ws);
    virtual void function(const FunctionDomain &domain, FunctionValues &values) const;


protected:
    virtual void init();

    double getArrivalTime(double tof);

    void initializeParametersFromWorkspace(Workspace2D_const_sptr workspace2D);
    void initializeFromInstrument(PoldiAbstractDetector_sptr detector, PoldiAbstractChopper_sptr chopper);

    std::vector<double> getChopperSlitOffsets(PoldiAbstractChopper_sptr chopper);
    std::vector<DetectorElementData_const_sptr> getDetectorElementData(PoldiAbstractDetector_sptr detector, PoldiAbstractChopper_sptr chopper);
    DetectorElementCharacteristics getDetectorCenterCharacteristics(PoldiAbstractDetector_sptr detector, PoldiAbstractChopper_sptr chopper);

    double dToTOF(double d) const;
    double timeTransformedWidth(double widthT, size_t detectorIndex) const;
    double timeTransformedCentre(double centreT, size_t detectorIndex) const;
    double timeTransformedIntensity(double areaD, double centreT, size_t detectorIndex) const;
    double detectorElementIntensity(double centreT, size_t detectorIndex) const;

    double actualFunction(double x, double x0, double sigma, double area) const;

    PoldiSourceSpectrum_const_sptr m_spectrum;
    DetectorElementCharacteristics m_detectorCenter;
    std::vector<DetectorElementData_const_sptr> m_detectorElementData;

    std::vector<double> m_chopperSlitOffsets;

    double m_detectorEfficiency;
    double m_deltaT;
};


} // namespace Poldi
} // namespace Mantid

#endif  /* MANTID_SINQ_POLDISPECTRUMDOMAINFUNCTION_H_ */
