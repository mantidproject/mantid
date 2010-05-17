//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Power.h"
#include "MantidKernel/Exception.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM( Power)

///////////////////////////////////


void Power::performBinaryOperation(const MantidVec& lhsX,
		const MantidVec& lhsY, const MantidVec& lhsE, const MantidVec& rhsY,
		const MantidVec& rhsE, MantidVec& YOut, MantidVec& EOut)
{

	if (rhsY.size() > 1)
	{
		throw std::invalid_argument("RHSWorkspace must be single valued.");
	}

	const int bins = lhsE.size();
	for (int j = 0; j < bins; ++j)
	{
		// Get references to the input Y's
		const double& leftY = lhsY[j];
		const double& rightY = rhsY[j];
		const double& leftE = lhsE[j];
		CheckExponent(rightY);
		double yOut = CalculatePower(leftY, rightY);

		EOut[j] = rightY * yOut * (leftE / leftY);

		YOut[j] = yOut;
	}
}

void Power::performBinaryOperation(const MantidVec& lhsX,
		const MantidVec& lhsY, const MantidVec& lhsE, const double& rhsY,
		const double& rhsE, MantidVec& YOut, MantidVec& EOut)
{
	CheckExponent(rhsY);
	const int bins = lhsE.size();
	for (int j = 0; j < bins; ++j)
	{
		// Get reference to input Y
		const double& leftY = lhsY[j];
		const double& leftE = lhsE[j];

		double yOut = CalculatePower(leftY, rhsY);

		EOut[j] = rhsY * yOut * (leftE / leftY);

		YOut[j] = yOut;
	}
}

inline void Power::CheckExponent(double exponent) {
	if (exponent < 0)
	{
		throw std::invalid_argument("Cannot have exponent < 0");
	}
}

inline double Power::CalculatePower(double base, double exponent)
{
	return std::pow(base, exponent);
}

void Power::setOutputUnits(const API::MatrixWorkspace_const_sptr lhs,
		const API::MatrixWorkspace_const_sptr rhs,
		API::MatrixWorkspace_sptr out)
{
	//TODO
}

}
}

