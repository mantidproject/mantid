/*WIKI* 


Returns the relative efficiency of the forward detector group compared to the backward detector group. If Alpha is larger than 1 more counts has been collected in the forward group.

This algorithm leave the input workspace unchanged. To group detectors in a workspace use [[GroupDetectors]].


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <cmath>
#include <vector>

#include "MantidKernel/ArrayProperty.h"
#include "MantidAlgorithms/CalMuonDeadTime.h"
#include "MantidAPI/TableRow.h"
//#include "MantidCurveFitting/Fit.h"
//#include "MantidCurveFitting/LinearBackground.h"

namespace Mantid
{
namespace Algorithms
{

using namespace Kernel;
using API::Progress;
//using Mantid::CurveFitting::LinearBackground;
//using Mantid::CurveFitting::Fit;

// Register the class into the algorithm factory
DECLARE_ALGORITHM( CalMuonDeadTime)

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void CalMuonDeadTime::init()
{
  declareProperty(new API::WorkspaceProperty<>("InputWorkspace", "",
      Direction::Input), "Name of the input workspace");

  declareProperty(new API::WorkspaceProperty<API::ITableWorkspace>("DeadTimeTable","",Direction::Output),
    "The name of the TableWorkspace in which to store the list of deadtimes for each spectrum" );  
}

/** Executes the algorithm
 *
 */
void CalMuonDeadTime::exec()
{
  API::MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");

  // set up table output workspace
  API::ITableWorkspace_sptr outTable = API::WorkspaceFactory::Instance().createTable("TableWorkspace");
  outTable->addColumn("int","spectrum");
  outTable->addColumn("double","dead-time");

  size_t numSpec = inputWS->getNumberHistograms();

  // cal deadtime for each spectrum
  for (size_t i = 0; i < numSpec; i++)
  {
    // create data to fit against

//    std::string wsName = "TempForMuonCalDeadTime";
    
/*
    int histogramNumber = 1;
    int timechannels = inputWS->blocksize();
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",histogramNumber,timechannels,timechannels);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
	  Mantid::MantidVec& x = ws2D->dataX(0); // x-values (time-of-flight)
    Mantid::MantidVec& y = ws2D->dataY(0); // y-values (counts)
    Mantid::MantidVec& e = ws2D->dataE(0); // error values of counts
*/

      // Do linear fit 

      const double in_bg0 = inputWS->dataY(i)[0];
      const double in_bg1 = 0.0;

      API::IAlgorithm_sptr fit;
      fit = createSubAlgorithm("Fit", -1, -1, true);

      const int wsindex = static_cast<int>(i);
      fit->setProperty("InputWorkspace", inputWS);
      fit->setProperty("WorkspaceIndex", wsindex);
      
      std::stringstream ss;
      ss << "name=LinearBackground,A0=" << in_bg0 << ",A1=" << in_bg1;
      std::string function = ss.str();

      fit->setProperty("Function", function);
      fit->executeAsSubAlg();

      std::string fitStatus = fit->getProperty("OutputStatus");
      std::vector<double> params = fit->getProperty("Parameters");
      std::vector<std::string> paramnames = fit->getProperty("ParameterNames");

      // Check order of names
      if (paramnames[0].compare("A0") != 0)
      {
        g_log.error() << "Parameter 0 should be A0, but is " << paramnames[0] << std::endl;
        throw std::invalid_argument("Parameters are out of order @ 0, should be A0");
      }
      if (paramnames[1].compare("A1") != 0)
      {
        g_log.error() << "Parameter 1 should be A1, but is " << paramnames[1]
            << std::endl;
        throw std::invalid_argument("Parameters are out of order @ 0, should be A1");
      }

      //
      const double time_bin = inputWS->dataX(i)[1]-inputWS->dataX(i)[0];

      if (!fitStatus.compare("success"))
      {
        const double A0 = params[0];
        const double A1 = params[1];


//        g_log.debug() << "Peak Fitted. Centre=" << centre << ", Sigma=" << width
//              << ", Height=" << height << ", Background slope=" << bgslope
//              << ", Background intercept=" << bgintercept << std::endl;
        API::TableRow t = outTable->appendRow();
        t << wsindex+1 << A1; //-(A1/A0)*time_bin;
      } 
      else
      {
        g_log.warning() << "Fit falled. Status = " << fitStatus << std::endl
                        << "For workspace index " << i << std::endl;
      } 

  } 


  // finally calculate alpha

  setProperty("DeadTimeTable", outTable);
}

} // namespace Algorithm
} // namespace Mantid


