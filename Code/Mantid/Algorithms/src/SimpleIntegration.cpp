//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SimpleIntegration.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidAPI/Progress.h"

namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(SimpleIntegration)

using namespace Kernel;
using namespace API;


/** Initialisation method.
 *
 */
void SimpleIntegration::init()
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
void SimpleIntegration::exec()
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

  bool is_distrib=outputWorkspace->isDistribution();

  Progress progress(this,0,1,m_MinSpec,m_MaxSpec,1);
  // Loop over spectra
  PARALLEL_FOR2(localworkspace,outputWorkspace)
  for (int i = m_MinSpec; i <= m_MaxSpec; ++i)
  {
    PARALLEL_START_INTERUPT_REGION
    const int j = i - m_MinSpec;
    if (localworkspace->axes() > 1)
    {
      outputWorkspace->getAxis(1)->spectraNo(j) = localworkspace->getAxis(1)->spectraNo(i);
    }

    // Retrieve the spectrum into a vector
    const MantidVec& X = localworkspace->readX(i);
    const MantidVec& Y = localworkspace->readY(i);
    const MantidVec& E = localworkspace->readE(i);

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

    double sumY, sumE;
    if (!is_distrib) //Sum the Y, and sum the E in quadrature
    {
      sumY=std::accumulate(Y.begin()+distmin,Y.begin()+distmax,0.0);
      sumE=std::accumulate(E.begin()+distmin,E.begin()+distmax,0.0,VectorHelper::SumSquares<double>());
    }
    else // Sum Y*binwidth and Sum the (E*binwidth)^2.
    {
      std::vector<double> widths(X.size());
      std::adjacent_difference(lowit,highit+1,widths.begin()); // highit+1 is safe while input workspace guaranteed to be histogram
      sumY=std::inner_product(Y.begin()+distmin,Y.begin()+distmax,widths.begin()+1,0.0);
      sumE=std::inner_product(E.begin()+distmin,E.begin()+distmax,widths.begin()+1,0.0,std::plus<double>(),VectorHelper::TimesSquares<double>());
    }

    //Set X-boundaries
    outputWorkspace->dataX(j)[0] = lowit==X.end() ? *(lowit-1) : *(lowit);
    outputWorkspace->dataX(j)[1] = *highit;
    outputWorkspace->dataY(j)[0] = sumY;
    outputWorkspace->dataE(j)[0] = sqrt(sumE); // Propagate Gaussian error

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
