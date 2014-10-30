#ifndef MANTID_SINQ_POLDIINSTRUMENTADAPTER_H_
#define MANTID_SINQ_POLDIINSTRUMENTADAPTER_H_

#include "MantidKernel/System.h"
#include "MantidSINQ/DllConfig.h"
#include "MantidGeometry/Instrument.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/MatrixWorkspace.h"

#include <map>

#include "MantidSINQ/PoldiUtilities/PoldiAbstractDetector.h"
#include "MantidSINQ/PoldiUtilities/PoldiAbstractChopper.h"
#include "MantidSINQ/PoldiUtilities/PoldiSourceSpectrum.h"

namespace Mantid
{
namespace Poldi
{

/** PoldiInstrumentAdapter :
    Adapter for constructing POLDI objects on the basis
    of Mantid's instrument-tools.

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

class AbstractDoubleValueExtractor
{
public:
    AbstractDoubleValueExtractor() { }

    virtual ~AbstractDoubleValueExtractor() { }

    virtual double operator()(const API::Run &runInformation, const std::string &propertyName) const = 0;
};

typedef boost::shared_ptr<AbstractDoubleValueExtractor> AbstractDoubleValueExtractor_sptr;

class NumberDoubleValueExtractor : public AbstractDoubleValueExtractor
{
public:
    NumberDoubleValueExtractor() :
        AbstractDoubleValueExtractor()
    { }
    virtual ~NumberDoubleValueExtractor() { }

    virtual double operator()(const API::Run &runInformation, const std::string &propertyName) const {
        return runInformation.getPropertyValueAsType<double>(propertyName);
    }
};

class VectorDoubleValueExtractor : public AbstractDoubleValueExtractor
{
public:
    VectorDoubleValueExtractor() :
        AbstractDoubleValueExtractor()
    { }
    virtual ~VectorDoubleValueExtractor() { }

    virtual double operator()(const API::Run &runInformation, const std::string &propertyName) const {
        return runInformation.getPropertyValueAsType<std::vector<double> >(propertyName).front();
    }
};

class VectorIntValueExtractor : public AbstractDoubleValueExtractor
{
public:
    VectorIntValueExtractor() :
        AbstractDoubleValueExtractor()
    { }
    virtual ~VectorIntValueExtractor() { }

    virtual double operator()(const API::Run &runInformation, const std::string &propertyName) const {
        return static_cast<double>(runInformation.getPropertyValueAsType<std::vector<int> >(propertyName).front());
    }
};

class MANTID_SINQ_DLL PoldiInstrumentAdapter
{
public:
    PoldiInstrumentAdapter(const API::MatrixWorkspace_const_sptr &matrixWorkspace);
    PoldiInstrumentAdapter(const Geometry::Instrument_const_sptr &mantidInstrument, const API::Run &runInformation);
    virtual ~PoldiInstrumentAdapter();

    PoldiAbstractChopper_sptr chopper() const;
    PoldiAbstractDetector_sptr detector() const;
    PoldiSourceSpectrum_sptr spectrum() const;

protected:
    PoldiInstrumentAdapter() { }

    void initializeFromInstrumentAndRun(const Geometry::Instrument_const_sptr &mantidInstrument, const API::Run &runInformation);

    void setDetector(const Geometry::Instrument_const_sptr &mantidInstrument);

    void setChopper(const Geometry::Instrument_const_sptr &mantidInstrument, const API::Run &runInformation);
    double getCleanChopperSpeed(double rawChopperSpeed) const;
    double getChopperSpeedFromRun(const API::Run &runInformation) const;
    double getChopperSpeedTargetFromRun(const API::Run &runInformation) const;
    bool chopperSpeedMatchesTarget(const API::Run &runInformation, double chopperSpeed) const;

    double extractPropertyFromRun(const API::Run &runInformation, const std::string &propertyName) const;
    AbstractDoubleValueExtractor_sptr getExtractorForProperty(Kernel::Property *chopperSpeedProperty) const;

    void setSpectrum(const Geometry::Instrument_const_sptr &mantidInstrument);


    PoldiAbstractChopper_sptr m_chopper;
    PoldiAbstractDetector_sptr m_detector;
    PoldiSourceSpectrum_sptr m_spectrum;
    
    static const std::string m_chopperSpeedPropertyName;
    static const std::string m_chopperSpeedTargetPropertyName;

    static std::map<std::string, AbstractDoubleValueExtractor_sptr> m_extractors;
};

typedef boost::shared_ptr<PoldiInstrumentAdapter> PoldiInstrumentAdapter_sptr;

} // namespace Poldi
} // namespace Mantid

#endif  /* MANTID_SINQ_POLDIINSTRUMENTADAPTER_H_ */
