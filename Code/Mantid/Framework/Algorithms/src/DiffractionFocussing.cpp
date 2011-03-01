//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/DiffractionFocussing.h"
#include "MantidAPI/FileProperty.h"

#include <map>
#include <fstream>

namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(DiffractionFocussing)

using namespace Kernel;
using API::WorkspaceProperty;
using API::MatrixWorkspace_sptr;
using API::MatrixWorkspace;
using API::FileProperty;

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void DiffractionFocussing::init()
{
  //this->setWikiSummary("Algorithm to focus powder diffraction data into a number of histograms according to a grouping scheme defined in a [[CalFile]].");
  //this->setOptionalMessage("Algorithm to focus powder diffraction data into a number of histograms according to a grouping scheme defined in a CalFile.");

  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input),
    "The input workspace" );
  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output),
    "The result of diffraction focussing of InputWorkspace" );
  declareProperty(new FileProperty("GroupingFileName", "", FileProperty::Load, ".cal"),
		  "The name of the CalFile with grouping data" );
}

/** Executes the algorithm
 *
 *  @throw Exception::FileError If the grouping file cannot be opened or read successfully
 *  @throw runtime_error If unable to run one of the sub-algorithms successfully
 */
void DiffractionFocussing::exec()
{
  // retrieve the properties
  std::string groupingFileName=getProperty("GroupingFileName");

  // Get the input workspace
  MatrixWorkspace_sptr inputW = getProperty("InputWorkspace");

  bool dist = inputW->isDistribution();

  //do this first to check that a valid file is available before doing any work
  std::multimap<int,int> detectorGroups;// <group, UDET>
  if (!readGroupingFile(groupingFileName, detectorGroups))
  {
    throw Exception::FileError("Error reading .cal file",groupingFileName);
  }

  //Convert to d-spacing units
  API::MatrixWorkspace_sptr tmpW = convertUnitsToDSpacing(inputW);

  //Rebin to a common set of bins
  RebinWorkspace(tmpW);

  std::set<int> groupNumbers;
  for(std::multimap<int,int>::const_iterator d = detectorGroups.begin();d!=detectorGroups.end();d++)
  {
    if (groupNumbers.find(d->first) == groupNumbers.end())
    {
      groupNumbers.insert(d->first);
    }
  }

  int iprogress = 0;
  int iprogress_count = groupNumbers.size();
  int iprogress_step = iprogress_count / 100;
  if (iprogress_step == 0) iprogress_step = 1;
  std::vector<int> resultIndeces;
  for(std::set<int>::const_iterator g = groupNumbers.begin();g!=groupNumbers.end();g++)
  {
    if (iprogress++ % iprogress_step == 0)
    {
      progress(0.68 + double(iprogress)/iprogress_count/3);
    }
    std::multimap<int,int>::const_iterator from = detectorGroups.lower_bound(*g);
    std::multimap<int,int>::const_iterator to =   detectorGroups.upper_bound(*g);
    std::vector<int> detectorList;
    for(std::multimap<int,int>::const_iterator d = from;d!=to;d++)
      detectorList.push_back(d->second);
    // Want version 1 of GroupDetectors here
    API::IAlgorithm_sptr childAlg = createSubAlgorithm("GroupDetectors",-1.0,-1.0,true,1);
    childAlg->setProperty("Workspace", tmpW);
    childAlg->setProperty< std::vector<int> >("DetectorList",detectorList);
    try
    {
      childAlg->execute();
      // get the index of the combined spectrum
      int ri = childAlg->getProperty("ResultIndex");
      if (ri >= 0)
      {
        resultIndeces.push_back(ri);
      }
    }
    catch(...)
    {
      g_log.error("Unable to successfully run GroupDetectors sub-algorithm");
      throw std::runtime_error("Unable to successfully run GroupDetectors sub-algorithm");
    }
  }

  // Discard left-over spectra, but print warning message giving number discarded
  int discarded = 0;
  const int oldHistNumber = tmpW->getNumberHistograms();
  API::Axis *spectraAxis = tmpW->getAxis(1);
  for(int i=0; i < oldHistNumber; i++)
    if ( spectraAxis->spectraNo(i) >= 0 && find(resultIndeces.begin(),resultIndeces.end(),i) == resultIndeces.end())
    {
      ++discarded;
    }
  g_log.warning() << "Discarded " << discarded << " spectra that were not assigned to any group" << std::endl;

  // Running GroupDetectors leads to a load of redundant spectra
  // Create a new workspace that's the right size for the meaningful spectra and copy them in
  int newSize = tmpW->blocksize();
  API::MatrixWorkspace_sptr outputW = API::WorkspaceFactory::Instance().create(tmpW,resultIndeces.size(),newSize+1,newSize);
  // Copy units
  outputW->getAxis(0)->unit() = tmpW->getAxis(0)->unit();
  outputW->getAxis(1)->unit() = tmpW->getAxis(1)->unit();

  API::Axis *spectraAxisNew = outputW->getAxis(1);

  for(int hist=0; hist < static_cast<int>(resultIndeces.size()); hist++)
  {
    int i = resultIndeces[hist];
    int spNo = spectraAxis->spectraNo(i);
    MantidVec &tmpE = tmpW->dataE(i);
    MantidVec &outE = outputW->dataE(hist);
    MantidVec &tmpY = tmpW->dataY(i);
    MantidVec &outY = outputW->dataY(hist);
    MantidVec &tmpX = tmpW->dataX(i);
    MantidVec &outX = outputW->dataX(hist);
    outE.assign(tmpE.begin(),tmpE.end());
    outY.assign(tmpY.begin(),tmpY.end());
    outX.assign(tmpX.begin(),tmpX.end());
    spectraAxisNew->setValue(hist,spNo);
    spectraAxis->setValue(i,-1);
  }

  progress(1.);

  outputW->isDistribution(dist);

  // Assign it to the output workspace property
  setProperty("OutputWorkspace",outputW);

  return;
}

/// Run ConvertUnits as a sub-algorithm to convert to dSpacing
MatrixWorkspace_sptr DiffractionFocussing::convertUnitsToDSpacing(const API::MatrixWorkspace_sptr& workspace)
{
  const std::string CONVERSION_UNIT = "dSpacing";

  Unit_const_sptr xUnit = workspace->getAxis(0)->unit();

  g_log.information() << "Converting units from "<< xUnit->label() << " to " << CONVERSION_UNIT<<".\n";

  API::IAlgorithm_sptr childAlg = createSubAlgorithm("ConvertUnits", 0.34, 0.66);
  childAlg->setProperty("InputWorkspace", workspace);
  childAlg->setPropertyValue("Target",CONVERSION_UNIT);

  // Now execute the sub-algorithm. Catch and log any error
  try
  {
    childAlg->execute();
  }
  catch (std::runtime_error&)
  {
    g_log.error("Unable to successfully run ConvertUnits sub-algorithm");
    throw;
  }

  if ( ! childAlg->isExecuted() ) g_log.error("Unable to successfully run ConvertUnits sub-algorithm");

  return childAlg->getProperty("OutputWorkspace");
}

/// Run Rebin as a sub-algorithm to harmonise the bin boundaries
void DiffractionFocussing::RebinWorkspace(API::MatrixWorkspace_sptr& workspace)
{

  double min=0;
  double max=0;
  double step=0;

  calculateRebinParams(workspace,min,max,step);
  std::vector<double> paramArray;
  paramArray.push_back(min);
  paramArray.push_back(-step);
  paramArray.push_back(max);

  g_log.information() << "Rebinning from "<< min << " to " << max <<
                         " in "<< step <<" logaritmic steps.\n";

  API::IAlgorithm_sptr childAlg = createSubAlgorithm("Rebin");
  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", workspace);
  childAlg->setProperty<std::vector<double> >("Params",paramArray);

  // Now execute the sub-algorithm. Catch and log any error
  try
  {
    childAlg->execute();
  }
  catch (std::runtime_error&)
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


/** Calculates rebin parameters: the min and max bin boundaries and the logarithmic step. The aim is to have approx.
    the same number of bins as in the input workspace.
    @param workspace :: The workspace being rebinned
    @param min ::       (return) The calculated frame starting point
    @param max ::       (return) The calculated frame ending point
    @param step ::      (return) The calculated bin width
 */
void DiffractionFocussing::calculateRebinParams(const API::MatrixWorkspace_const_sptr& workspace,double& min,double& max,double& step)
{

  min=999999999;
  //for min and max we need to iterate over the data block and investigate each one
  int length = workspace->getNumberHistograms();
  for (int i = 0; i < length; i++)
  {
    const MantidVec& xVec = workspace->readX(i);
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

/// Reads in the file with the grouping information
bool DiffractionFocussing::readGroupingFile(std::string groupingFileName, std::multimap<int,int>& detectorGroups)
{
  std::ifstream grFile(groupingFileName.c_str());
  if (!grFile)
  {
    g_log.error() << "Unable to open grouping file " << groupingFileName << std::endl;
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
    istr >> n >> udet >> offset >> sel >> group;
    // Check the line wasn't badly formatted - return a failure if it is
    //if ( ! istr.good() ) return false;
		// only allow groups with +ve ids
    if ((sel) && (group>0))
		{
			detectorGroups.insert(std::make_pair(group,udet));
		}
  }
  return true;
}

} // namespace Algorithm
} // namespace Mantid
