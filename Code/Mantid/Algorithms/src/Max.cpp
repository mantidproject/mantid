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
  declareProperty("EndWorkspaceIndex",0, mustBePositive->clone());
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
  //m_MinSpec = getProperty("StartSpectrum");
  //m_MaxSpec = getProperty("EndSpectrum");
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
  if ( !m_MaxSpec ) m_MaxSpec = numberOfSpectra-1;
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

  MantidVec::difference_type distmin0 = 1,distmax0 = 0;
  Progress progress(this,0,1,m_MinSpec,m_MaxSpec,1);
  int j=0;
  // Loop over spectra
  for (int i = m_MinSpec; i <= m_MaxSpec; ++i)
  {
    if (localworkspace->axes() > 1)
    {
      outputWorkspace->getAxis(1)->spectraNo(j++) = localworkspace->getAxis(1)->spectraNo(i);
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

    if (distmin0 != distmin || distmax0 != distmax)
        g_log.debug()<<"Starting with spectrum "<<i<<" bins selected: from "<<distmin<<" ("<<*(X.begin()+distmin)
        <<") to "<<distmax<<" ("<<*(X.begin()+distmax)<<")\n";

    // Find the max element
    MantidVec::const_iterator maxY=std::max_element(Y.begin()+distmin,Y.begin()+distmax);
    MantidVec::difference_type d=std::distance(Y.begin(),maxY);
    // X boundaries for the max element
    outputWorkspace->dataX(i)[0]=*(X.begin()+d);
    outputWorkspace->dataX(i)[1]=*(X.begin()+d+1); //This is safe since X is of dimension Y+1
    outputWorkspace->dataY(i)[0]=*maxY;
    progress.report();
  }

  // Assign it to the output workspace property
  setProperty("OutputWorkspace",outputWorkspace);

  return;
}

} // namespace Algorithms
} // namespace Mantid
