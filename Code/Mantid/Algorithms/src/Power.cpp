//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Power.h"
#include "MantidKernel/Exception.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid {
namespace Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM( Power)

///////////////////////////////////


void Power::defineProperties() {
	BoundedValidator<double> *mustBePositive = new BoundedValidator<double> ();
	mustBePositive->setLower(0.0);
	declareProperty("Exponent", 1.0, mustBePositive);
}

void Power::retrieveProperties() {
	m_exponent = getProperty("Exponent");
}

void Power::performUnaryOperation(const double& XIn, const double& YIn,
		const double& EIn, double& YOut, double& EOut) {
		YOut = CalculatePower(YIn, m_exponent);
		EOut = m_exponent * YOut * (EIn / YIn);
}

inline double Power::CalculatePower(const double base, const double exponent)
{
return std::pow(base, exponent);
}

}
}

