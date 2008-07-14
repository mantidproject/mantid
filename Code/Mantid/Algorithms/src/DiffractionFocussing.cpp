//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/DiffractionFocussing.h"
#include "MantidAPI/Workspace.h"
#include "MantidKernel/FileValidator.h"

#include <sstream>
#include <numeric>
#include <math.h>

namespace Mantid
{
  namespace Algorithms
  {

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(DiffractionFocussing)

    using namespace Kernel;
    using API::WorkspaceProperty;
    using API::Workspace_sptr;
    using API::Workspace;

    // Get a reference to the logger
    Logger& DiffractionFocussing::g_log = Logger::get("DiffractionFocussing");

    /** Initialisation method. Declares properties to be used in algorithm.
    *
    */
    void DiffractionFocussing::init()
    {
      declareProperty(new WorkspaceProperty<Workspace>("InputWorkspace","",Direction::Input));
      declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output));

      //valid extensions, we will allow all, so just an empty array
      std::vector<std::string> exts;
      declareProperty("GroupingFileName","",new FileValidator(exts));
    }

    /** Executes the rebin algorithm
    *
    *  @throw runtime_error Thrown if
    */
    void DiffractionFocussing::exec()
    {
      // retrieve the properties
      std::string groupingFileName=getProperty("GroupingFileName");

      // Get the input workspace
      Workspace_sptr inputW = getProperty("InputWorkspace");
      std::string outputWorkspaceName=getProperty("OutputWorkspace");

      //do this first to check that a valid file is available before doing any work
      // std::map<int,int>readGroupingFile(groupingFileName);

      //Convert to d-spacing units
      API::Workspace_sptr outputW = convertUnitsToDSpacing(inputW,outputWorkspaceName);

      //Rebin to a common set of bins
      RebinWorkspace(outputW);

      //The spectra are grouped according to the grouping file

      // make output Workspace the same type is the input, but with new length of signal array
      //API::Workspace_sptr outputW = API::WorkspaceFactory::Instance().create(inputW,histnumber,nx,ny);


      //copy over the spectrum No and ErrorHelper
      //        outputW->getAxis()->spectraNo(hist)=inputW->getAxis()->spectraNo(hist);
      //outputW->setErrorHelper(hist,inputW->errorHelper(hist));

      //outputW->isDistribution(dist);
      // Assign it to the output workspace property
      setProperty("OutputWorkspace",outputW);

      return;
    }

    /// Run ConvertUnits as a sub-algorithm to convert to dSpacing
    Workspace_sptr DiffractionFocussing::convertUnitsToDSpacing(API::Workspace_sptr workspace, std::string outputWorkspaceName)
    {
      const std::string CONVERSION_UNIT = "dSpacing";

      boost::shared_ptr<Kernel::Unit>& xUnit = workspace->getAxis(0)->unit();

      g_log.information() << "Converting units from "<< xUnit->label() << " to " << CONVERSION_UNIT<<".\n";

      API::Algorithm_sptr childAlg = createSubAlgorithm("ConvertUnits");
      childAlg->setPropertyValue("InputWorkspace", getPropertyValue("InputWorkspace"));
      childAlg->setPropertyValue("OutputWorkspace", outputWorkspaceName);
      childAlg->setPropertyValue("Target",CONVERSION_UNIT);

      // Now execute the sub-algorithm. Catch and log any error
      try
      {
        childAlg->execute();
      }
      catch (std::runtime_error& err)
      {
        g_log.error("Unable to successfully run ConvertUnits sub-algorithm");
        throw;
      }

      if ( ! childAlg->isExecuted() ) g_log.error("Unable to successfully run ConvertUnits sub-algorithm");      

      return childAlg->getProperty("OutputWorkspace");
    }

    /// Run Rebin as a sub-algorithm to harmionise the bin boundaries
    void DiffractionFocussing::RebinWorkspace(API::Workspace_sptr workspace)
    {

      double min=0;
      double max=0;
      int step=0;

      calculateRebinParams(workspace,min,max,step);
      std::vector<double> paramArray;
      paramArray.push_back(min);
      paramArray.push_back(-step);
      paramArray.push_back(max);

      g_log.information() << "Rebinning from "<< min << 
        " to " << max <<
        " in "<< step <<" logaritmic steps.\n";

      API::Algorithm_sptr childAlg = createSubAlgorithm("Rebin");
      childAlg->setPropertyValue("InputWorkspace", "Anonymous");
      childAlg->setProperty<Workspace_sptr>("InputWorkspace", workspace);
      childAlg->setPropertyValue("OutputWorkspace", "Anonymous");
      childAlg->setProperty<Workspace_sptr>("OutputWorkspace", workspace);
      childAlg->setProperty<std::vector<double>>("params",paramArray);

      // Now execute the sub-algorithm. Catch and log any error
      try
      {
        childAlg->execute();
      }
      catch (std::runtime_error& err)
      {
        g_log.error("Unable to successfully run Rebinning sub-algorithm");
        throw;
      }

      if ( ! childAlg->isExecuted() ) g_log.error("Unable to successfully run Rebinning sub-algorithm");      

    }

    void DiffractionFocussing::calculateRebinParams(API::Workspace_sptr workspace,double& min,double& max,int& step)
    {
      //step is easy
      step = workspace->blocksize();
      
      min=999999999;
      //for min and max we need to iterate over the data block and investigate each one
      int length = workspace->getNumberHistograms();
      for (int i = 0; i < length; i++)
      {
        const std::vector<double>& xVec = workspace->readX(i);
        const double& localMin = xVec[0];
        const double& localMax = xVec[xVec.size()-1];
        if (localMin < min) min = localMin;
        if (localMax > max) max = localMax;
      }

    }


  } // namespace Algorithm
} // namespace Mantid




