/*WIKI* 

''Logarithm'' function calculates the logarithm of the data, held in a workspace and tries to estimate the errors of this data, by calculating logarithmic transformation of the errors. The errors are assumed to be small and Gaussian so they are calculated on the basis of Tailor decomposition e.g. if <math>S</math> and <math>Err</math> are the signal and errors for the initial signal, the logarithm would provide <math>S_{ln}=ln(S)</math> and <math>Err_{ln}=Err/S</math> accordingly. If the base 10 logarithm is used the errors are calculated as <math>Err_{log10}=0.434Err/S</math>

Some values in a workspace can normally be equal to zero. Logarithm is not calculated for values which are less or equal to 0, but the value of ''Filler'' is used instead. The errors for such cells set to zeros

When acting on an event workspace, the output will be a Workspace2D, with the default binning from the original workspace.

*WIKI*/
#include "MantidAlgorithms/Logarithm.h"
#include "MantidAPI/WorkspaceProperty.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

#include <cmath>
namespace Mantid
{
namespace Algorithms
{
// Register the class into the algorithm factory
DECLARE_ALGORITHM(Logarithm)

void Logarithm::initDocs(){
  this->setWikiSummary("''Logarithm'' function calculates the logarithm of the data, held in a workspace. A user can choose between natural (default) or base 10 logarithm");
  this->setOptionalMessage("Logarithm function calculates the logarithm of the data, held in a workspace. A user can choose between natural (default) or base 10 logarithm");
}

Logarithm::Logarithm():UnaryOperation(),log_Min(0),is_natural(true)
{
  this->useHistogram=true;
}

void Logarithm::defineProperties()
{
  declareProperty("Filler", 0.0,
"The value that will be placed into the output workspace if an input value is equal or less than 0. Default value is 0");
  declareProperty("Natural",true,"Logical value which specifies if user wands to calculate natural or base 10 logarithm.");
}

void Logarithm::retrieveProperties()
{
  this->log_Min   = getProperty("Filler");
  this->is_natural= getProperty("Natural");
}

void Logarithm::performUnaryOperation(const double XIn, const double YIn, const double EIn, double& YOut, double& EOut)
{
  (void) XIn; //Avoid compiler warning
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
