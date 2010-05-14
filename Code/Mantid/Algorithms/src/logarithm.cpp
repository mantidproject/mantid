#include "MantidAlgorithms/logarithm.h"
#include <cmath>
namespace Mantid
{
namespace Algorithms
{
// Register the class into the algorithm factory
DECLARE_ALGORITHM(logarithm)



void logarithm::init(void)
{
	UnaryOperation::init();
	declareProperty("filler", 0.0,
    "Some values in a workspace can normally be zeros or may have negative values\n"
    "log(x) is not defined for such values, so here is the value, that will be placed as the result of ln(x<=0) operation\n"
	"Default value is 0");
	declareProperty("natural",true,"switch to choose between natural or base 10 logarithm");

}
void logarithm::retrieveProperties()
{
	this->log_Min   = getProperty("filler");
	this->is_natural= getProperty("natural");
}

void logarithm::performUnaryOperation(const double& XIn, const double& YIn, const double& EIn, double& YOut, double& EOut)
{
	if (YIn<=0){  
		YOut = this->log_Min;
		EOut = 0;
	}else{
		if (this->is_natural){
			YOut = std::log(YIn);
			EOut = EIn/YIn;
		}else{
			YOut = std::log10(YIn);
			EOut = 0.434*EIn/YIn;
		}
	}
}
} // End Namespace Algorithms
} // End Namespace Mantid