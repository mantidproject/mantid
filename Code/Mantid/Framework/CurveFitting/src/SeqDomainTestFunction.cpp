#include "MantidCurveFitting/SeqDomainTestFunction.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "boost/lexical_cast.hpp"
#include "MantidAPI/FunctionFactory.h"

namespace Mantid
{
namespace CurveFitting
{

DECLARE_FUNCTION(SeqDomainTestFunction)

SeqDomainTestFunction::SeqDomainTestFunction() :
    ParamFunction()
{

}

void SeqDomainTestFunction::function(const FunctionDomain &domain, FunctionValues &values) const
{
    const FunctionDomain1DSpectrum &spectrumDomain = dynamic_cast<const FunctionDomain1DSpectrum &>(domain);

    double wsIndex = static_cast<double>(spectrumDomain.getWorkspaceIndex());
    double slope = getParameter(spectrumDomain.getWorkspaceIndex() % 40);

    for(size_t j = 0; j < spectrumDomain.size(); ++j) {
        values.addToCalculated(j, wsIndex + slope * spectrumDomain[j]);
    }
}

void SeqDomainTestFunction::functionDeriv(const FunctionDomain &domain, Jacobian &jacobian)
{
    const FunctionDomain1DSpectrum &spectrumDomain = dynamic_cast<const FunctionDomain1DSpectrum &>(domain);

    for(size_t j = 0; j < spectrumDomain.size(); ++j) {
        jacobian.set(j, spectrumDomain.getWorkspaceIndex() % 40, spectrumDomain[j]);
    }
}

void SeqDomainTestFunction::init()
{
    for(size_t i = 0; i < 40; ++ i) {
      declareParameter("Slope" + boost::lexical_cast<std::string>(i), 4.0);
    }
}


} // namespace CurveFitting
} // namespace Mantid
