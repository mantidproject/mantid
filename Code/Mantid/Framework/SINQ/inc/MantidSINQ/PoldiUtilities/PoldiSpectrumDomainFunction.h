#ifndef MANTID_SINQ_POLDISPECTRUMDOMAINFUNCTION_H_
#define MANTID_SINQ_POLDISPECTRUMDOMAINFUNCTION_H_

#include "MantidSINQ/DllConfig.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IFunction1DSpectrum.h"
#include "MantidAPI/FunctionDomain1D.h"
#include <string>

#include "MantidSINQ/PoldiUtilities/PoldiInstrumentAdapter.h"
#include "MantidSINQ/PoldiUtilities/PoldiTimeTransformer.h"

#include "MantidDataObjects/Workspace2D.h"
#include "MantidSINQ/PoldiUtilities/PoldiConversions.h"

namespace Mantid
{
namespace Poldi
{

/** PoldiSpectrumDomainFunction : TODO: DESCRIPTION

    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

class MANTID_SINQ_DLL PoldiSpectrumDomainFunction : virtual public API::ParamFunction, virtual public API::IFunction1DSpectrum
{
public:
    PoldiSpectrumDomainFunction();
    virtual ~PoldiSpectrumDomainFunction()
    {}
    
    virtual std::string name() const { return "PoldiSpectrumDomainFunction"; }

    virtual void setWorkspace(boost::shared_ptr<const API::Workspace> ws);
    virtual void function1DSpectrum(const API::FunctionDomain1DSpectrum &domain, API::FunctionValues &values) const;

protected:
    virtual void init();
    void initializeParametersFromWorkspace(const DataObjects::Workspace2D_const_sptr &workspace2D);
    void initializeInstrumentParameters(const PoldiInstrumentAdapter_sptr &poldiInstrument);
    std::vector<double> getChopperSlitOffsets(const PoldiAbstractChopper_sptr &chopper);

    double actualFunction(double x, double x0, double sigma, double area) const;

    std::vector<double> m_chopperSlitOffsets;
    double m_deltaT;

    PoldiTimeTransformer_sptr m_timeTransformer;
};


} // namespace Poldi
} // namespace Mantid

#endif  /* MANTID_SINQ_POLDISPECTRUMDOMAINFUNCTION_H_ */
