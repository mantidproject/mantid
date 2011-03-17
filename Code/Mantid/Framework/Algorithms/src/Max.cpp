//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Max.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidAPI/Progress.h"

namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(Max)

using namespace Kernel;
using namespace API;


/** Initialisation method.
 *
 */
void Max::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,new HistogramValidator<>));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));

  declareProperty("RangeLower",EMPTY_DBL());
  declareProperty("RangeUpper",EMPTY_DBL());
  BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
  mustBePositive->setLower(0);
  declareProperty("StartWorkspaceIndex",0, mustBePositive);
  // As the property takes ownership of the validator pointer, have to take care to pass in a unique
  // pointer to each property.
  declareProperty("EndWorkspaceIndex",EMPTY_INT(), mustBePositive->clone());
}

/** Executes the algorithm
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void Max::exec()
{
  // Try and retrieve the optional properties
  m_MinRange = getProperty("RangeLower");
  m_MaxRange = getProperty("RangeUpper");
  m_MinSpec = getProperty("StartWorkspaceIndex");
  m_MaxSpec = getProperty("EndWorkspaceIndex");


  // Get the input workspace
  MatrixWorkspace_const_sptr localworkspace = getProperty("InputWorkspace");

  const int numberOfSpectra = localworkspace->getNumberHistograms();

  // Check 'StartSpectrum' is in range 0-numberOfSpectra
  if ( m_MinSpec > numberOfSpectra )
  {
    g_log.warning("StartSpectrum out of range! Set to 0.");
    m_MinSpec = 0;
  }
  if ( isEmpty(m_MaxSpec) ) m_MaxSpec = numberOfSpectra-1;
  if ( m_MaxSpec > numberOfSpectra-1 || m_MaxSpec < m_MinSpec )
  {
    g_log.warning("EndSpectrum out of range! Set to max detector number");
    m_MaxSpec = numberOfSpectra;
  }
  if ( m_MinRange > m_MaxRange )
  {
    g_log.warning("Range_upper is less than Range_lower. Will integrate up to frame maximum.");
    m_MaxRange = 0.0;
  }

  // Create the 1D workspace for the output
  MatrixWorkspace_sptr outputWorkspace = API::WorkspaceFactory::Instance().create(localworkspace,m_MaxSpec-m_MinSpec+1,2,1);


  Progress progress(this,0,1,(m_MaxSpec-m_MinSpec+1));
  PARALLEL_FOR2(localworkspace,outputWorkspace)
  // Loop over spectra
  for (int i = m_MinSpec; i <= m_MaxSpec; ++i)
  {
    PARALLEL_START_INTERUPT_REGION
    int newindex=i-m_MinSpec;
    if (localworkspace->axes() > 1)
    {
      outputWorkspace->getAxis(1)->spectraNo(newindex) = localworkspace->getAxis(1)->spectraNo(i);
    }

    // Retrieve the spectrum into a vector
    const MantidVec& X = localworkspace->readX(i);
    const MantidVec& Y = localworkspace->readY(i);

    // Find the range [min,max]
    MantidVec::const_iterator lowit, highit;
    if (m_MinRange == EMPTY_DBL()) lowit=X.begin();
    else lowit=std::lower_bound(X.begin(),X.end(),m_MinRange);

    if (m_MaxRange == EMPTY_DBL()) highit=X.end();
    else highit=std::find_if(lowit,X.end(),std::bind2nd(std::greater<double>(),m_MaxRange));

    // If range specified doesn't overlap with this spectrum then bail out
    if ( lowit == X.end() || highit == X.begin() ) continue;

    highit--; // Upper limit is the bin before, i.e. the last value smaller than MaxRange

    MantidVec::difference_type distmin=std::distance(X.begin(),lowit);
    MantidVec::difference_type distmax=std::distance(X.begin(),highit);

    // Find the max element
    MantidVec::const_iterator maxY=std::max_element(Y.begin()+distmin,Y.begin()+distmax);
    MantidVec::difference_type d=std::distance(Y.begin(),maxY);
    // X boundaries for the max element
    outputWorkspace->dataX(newindex)[0]=*(X.begin()+d);
    outputWorkspace->dataX(newindex)[1]=*(X.begin()+d+1); //This is safe since X is of dimension Y+1
    outputWorkspace->dataY(newindex)[0]=*maxY;
    progress.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // Assign it to the output workspace property
  setProperty("OutputWorkspace",outputWorkspace);

  return;
}

} // namespace Algorithms
} // namespace Mantid
