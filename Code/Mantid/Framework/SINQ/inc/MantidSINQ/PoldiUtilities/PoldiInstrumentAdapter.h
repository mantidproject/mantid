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
    AbstractDoubleValueExtractor(std::string doubleValueKey) :
        m_doubleValueKey(doubleValueKey)
    { }

    virtual ~AbstractDoubleValueExtractor() { }

    virtual double operator()(const API::Run &runInformation) const = 0;

protected:
    std::string m_doubleValueKey;
};

typedef boost::shared_ptr<AbstractDoubleValueExtractor> AbstractDoubleValueExtractor_sptr;

class NumberDoubleValueExtractor : public AbstractDoubleValueExtractor
{
public:
    NumberDoubleValueExtractor(std::string doubleValueKey) :
        AbstractDoubleValueExtractor(doubleValueKey)
    { }
    virtual ~NumberDoubleValueExtractor() { }

    virtual double operator()(const API::Run &runInformation) const {
        return runInformation.getPropertyValueAsType<double>(m_doubleValueKey);
    }
};

class VectorDoubleValueExtractor : public AbstractDoubleValueExtractor
{
public:
    VectorDoubleValueExtractor(std::string doubleValueKey) :
        AbstractDoubleValueExtractor(doubleValueKey)
    { }
    virtual ~VectorDoubleValueExtractor() { }

    virtual double operator()(const API::Run &runInformation) const {
        return runInformation.getPropertyValueAsType<std::vector<double> >(m_doubleValueKey).front();
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
    
    static const std::string getChopperSpeedPropertyName();
    
protected:
    PoldiInstrumentAdapter() { }

    void initializeFromInstrumentAndRun(const Geometry::Instrument_const_sptr &mantidInstrument, const API::Run &runInformation);

    void setDetector(const Geometry::Instrument_const_sptr &mantidInstrument);
    void setChopper(const Geometry::Instrument_const_sptr &mantidInstrument, const API::Run &runInformation);
    void setSpectrum(const Geometry::Instrument_const_sptr &mantidInstrument);

    double getChopperSpeedFromRun(const API::Run &runInformation);
    AbstractDoubleValueExtractor_sptr getExtractorForProperty(Kernel::Property *chopperSpeedProperty);

    PoldiAbstractChopper_sptr m_chopper;
    PoldiAbstractDetector_sptr m_detector;
    PoldiSourceSpectrum_sptr m_spectrum;
    
    static const std::string m_chopperSpeedPropertyName;
    static std::map<std::string, AbstractDoubleValueExtractor_sptr> m_extractors;
};

typedef boost::shared_ptr<PoldiInstrumentAdapter> PoldiInstrumentAdapter_sptr;

} // namespace Poldi
} // namespace Mantid

#endif  /* MANTID_SINQ_POLDIINSTRUMENTADAPTER_H_ */
