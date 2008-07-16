//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/DiffractionFocussing.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/FileValidator.h"

#include <set>
#include <map>
#include <fstream>
#include <sstream>
#include <numeric>
#include <math.h>

#include <iostream>

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
      std::multimap<int,int> detectorGroups;// <group, UDET>
      if (!readGroupingFile(groupingFileName, detectorGroups))
      {
          return;
      }

      //Convert to d-spacing units
      API::Workspace_sptr outputW = convertUnitsToDSpacing(inputW,outputWorkspaceName);

      //Rebin to a common set of bins
      RebinWorkspace(outputW);

      //The spectra are grouped according to the grouping file

      // make output Workspace the same type is the input, but with new length of signal array
      //API::Workspace_sptr outputW = API::WorkspaceFactory::Instance().create(inputW,histnumber,nx,ny);


      //copy over the spectrum No and ErrorHelper
      //boost::shared_ptr<Mantid::API::SpectraDetectorMap> spectraMap = inputW->getSpectraMap();
      /*for(int hist=0;hist<outputW->getNumberHistograms();hist++)
      {
          outputW->getAxis(1)->spectraNo(hist)=inputW->getAxis(1)->spectraNo(hist);
          outputW->setErrorHelper(hist,inputW->errorHelper(hist));
      }*/

      std::set<int> groupNumbers;
      for(std::multimap<int,int>::const_iterator d = detectorGroups.begin();d!=detectorGroups.end();d++)
      {
          //std::cerr<<"d "<<d->first<<' '<<d->second<<std::endl;
          if (groupNumbers.find(d->first) == groupNumbers.end())
          {
              std::cerr<<"    insert "<<d->first<<std::endl;
              groupNumbers.insert(d->first);
          }
      }
      std::cerr<<"OK "<<groupNumbers.size()<<std::endl;
      for(std::set<int>::const_iterator g = groupNumbers.begin();g!=groupNumbers.end();g++)
      {
          std::multimap<int,int>::const_iterator from = detectorGroups.lower_bound(*g);
          std::multimap<int,int>::const_iterator to =   detectorGroups.upper_bound(*g);
          std::vector<int> detectorList;
          for(std::multimap<int,int>::const_iterator d = from;d!=to;d++)
              detectorList.push_back(d->second);
          API::Algorithm_sptr childAlg = createSubAlgorithm("GroupDetectors");
          childAlg->setPropertyValue("Workspace", "Anonymous");
          DataObjects::Workspace2D_sptr outputW2D = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(outputW);
          childAlg->setProperty<DataObjects::Workspace2D_sptr>("Workspace", outputW2D);
          childAlg->setProperty< std::vector<int> >("DetectorList",detectorList);
          try
          {
              childAlg->execute();
          }
          catch(...)
          {
              g_log.error()<<"Unable to successfully run GroupDetectors sub-algorithm";
              throw std::runtime_error("Unable to successfully run GroupDetectors sub-algorithm");
          }
      }


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
    void DiffractionFocussing::RebinWorkspace(API::Workspace_sptr& workspace)
    {

      double min=0;
      double max=0;
      double step=0;

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
      childAlg->setProperty<std::vector<double> >("params",paramArray);

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
      else
      {
           workspace = childAlg->getProperty("OutputWorkspace");
      }

    }

    void DiffractionFocussing::calculateRebinParams(API::Workspace_sptr workspace,double& min,double& max,double& step)
    {
      
      min=999999999;
      //for min and max we need to iterate over the data block and investigate each one
      int length = workspace->getNumberHistograms();
      for (int i = 0; i < length; i++)
      {
        const std::vector<double>& xVec = workspace->readX(i);
        const double& localMin = xVec[0];
        const double& localMax = xVec[xVec.size()-1];
        if (localMin != std::numeric_limits<double>::infinity() &&
            localMax != std::numeric_limits<double>::infinity()) 
        {
            if (localMin < min) min = localMin;
            if (localMax > max) max = localMax;
        }
      }

      if (min <= 0.) min = 1e-6;

      //step is easy
      int n = workspace->blocksize();
      step = ( log(max) - log(min) )/n;
    }
    
    bool DiffractionFocussing::readGroupingFile(std::string groupingFileName, std::multimap<int,int>& detectorGroups)
    {
        std::ifstream grFile(groupingFileName.c_str());
        if (!grFile)
        {
            g_log.error()<<"Unable to open grouping file "<<groupingFileName;
            return false;
        }

        detectorGroups.clear();
        std::string str;
        while(getline(grFile,str))
        {
            if (str.empty() || str[0] == '#') continue;
            std::istringstream istr(str);
            int n,udet,sel,group;
            double offset;
            istr>>n>>udet>>offset>>sel>>group;
            detectorGroups.insert(std::make_pair(group,udet));
        }
        return true;
    }

  } // namespace Algorithm
} // namespace Mantid




