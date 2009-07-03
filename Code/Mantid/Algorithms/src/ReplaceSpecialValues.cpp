//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ReplaceSpecialValues.h"
#include "MantidKernel/Exception.h"
#include <limits>
#include <math.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid
{
	namespace Algorithms
	{
		// Register the class into the algorithm factory
		DECLARE_ALGORITHM(ReplaceSpecialValues)

		void ReplaceSpecialValues::defineProperties()
		{
			declareProperty("NaNValue", Mantid::EMPTY_DBL(),
				"The value used to replace occurances of NaN\n"
				"(default do not check)");			
			declareProperty("NaNError",0.0,
				"The error value used when replacing a value of NaN\n"
				"(default 0)");		
			declareProperty("InfinityValue",Mantid::EMPTY_DBL(),
				"The value used to replace occurances of positive or negative infinity\n"
				"(default do not check)");
			declareProperty("InfinityError",0.0,
				"The error value used when replacing a value of infinity\n"
				"(default 0)");
		}

		void ReplaceSpecialValues::retrieveProperties()
		{
			m_NaNValue = getProperty("NaNValue");
			m_NaNError = getProperty("NaNError");
			m_InfiniteValue = getProperty("InfinityValue");
			m_InfiniteError = getProperty("InfinityError");
			m_performNaNCheck = !checkifPropertyEmpty(m_NaNValue);
			m_performInfiniteCheck = !checkifPropertyEmpty(m_InfiniteValue);
			if (!(m_performNaNCheck || m_performInfiniteCheck))
			{
				throw std::invalid_argument("No value was defined for NaN or inifinity");
			}
		}

		void ReplaceSpecialValues::performUnaryOperation(const double& XIn, const double& YIn, const double& EIn, double& YOut, double& EOut)
		{
			YOut = YIn;
			EOut = EIn;

			if (m_performNaNCheck && checkIfNan(YIn))
			{
				YOut = m_NaNValue;
				EOut = m_NaNError;
			} else if (m_performInfiniteCheck && checkIfInfinite(YIn))
			{
				YOut = m_InfiniteValue;
				EOut = m_InfiniteError;
			}
		}


		bool ReplaceSpecialValues::checkIfNan(const double& value) const
		{
			return (value != value);
		}

		bool ReplaceSpecialValues::checkIfInfinite(const double& value) const
		{
			return (std::abs(value) == std::numeric_limits<double>::infinity());
		}

		bool ReplaceSpecialValues::checkifPropertyEmpty(const double& value) const
		{
			return ( std::abs(value - Mantid::EMPTY_DBL()) < 1e-08 );
		}

	} // namespace Algorithms
} // namespace Mantid
