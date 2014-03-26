/*WIKI* 


The algorithm will raise the InputWorkspace to the power of the Exponent.
When acting on an event workspace, the output will be a Workspace2D, with the default binning from the original workspace.

== Errors ==

Defining the power algorithm as: <math>  y = \left ( a^b \right )  </math>, we can describe the error as: 
<math>s_{y} = by\left ( s_{a}/a \right )</math>, where <math>s_{y}</math> is the error in the result ''y'' and <math>s_{a}</math> is the error in the input ''a''.

*WIKI*/
/*WIKI_USAGE*
'''Python'''
 outputW = Power("inputW",3)

*WIKI_USAGE*/
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
DECLARE_ALGORITHM(Power)

Power::Power():UnaryOperation()
{
  this->useHistogram=true;
}
/// Sets documentation strings for this algorithm
void Power::initDocs()
{
  this->setWikiSummary("The Power algorithm will raise the base workspace to a particular power. Corresponding [[Error Values|error values]] will be created. ");
  this->setOptionalMessage("The Power algorithm will raise the base workspace to a particular power. Corresponding error values will be created.");
}


///////////////////////////////////


void Power::defineProperties()
{
  declareProperty("Exponent", 1.0,"The exponent with which to raise base values in the base workspace to.");
}

void Power::retrieveProperties()
{
  m_exponent = getProperty("Exponent");
}

void Power::performUnaryOperation(const double XIn, const double YIn, const double EIn, double& YOut,
    double& EOut)
{
  (void) XIn; //Avoid compiler warning
  YOut = calculatePower(YIn, m_exponent);
  EOut = std::fabs(m_exponent * YOut * (EIn / YIn));
}

inline double Power::calculatePower(const double base, const double exponent)
{
  return std::pow(base, exponent);
}

}
}

