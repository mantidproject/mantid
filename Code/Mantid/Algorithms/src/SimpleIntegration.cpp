//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SimpleIntegration.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/VectorHelper.h"

namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(SimpleIntegration)

using namespace Kernel;
using namespace API;

// Get a reference to the logger
Logger& SimpleIntegration::g_log = Logger::get("SimpleIntegration");

/** Initialisation method.
 *
 */
void SimpleIntegration::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,new HistogramValidator<>));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));

  declareProperty("Range_lower",0.0);
  declareProperty("Range_upper",0.0);
  BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
  mustBePositive->setLower(0);
  declareProperty("StartSpectrum",0, mustBePositive);
  // As the property takes ownership of the validator pointer, have to take care to pass in a unique
  // pointer to each property.
  declareProperty("EndSpectrum",0, mustBePositive->clone());
}

/** Executes the algorithm
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void SimpleIntegration::exec()
{
  // Try and retrieve the optional properties
  m_MinRange = getProperty("Range_lower");
  m_MaxRange = getProperty("Range_upper");
  m_MinSpec = getProperty("StartSpectrum");
  m_MaxSpec = getProperty("EndSpectrum");

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

  int progress_step = (m_MaxSpec-m_MinSpec+1) / 100;
  if (progress_step == 0) progress_step = 1;

  std::vector<double> widths(0);

  bool is_distrib=outputWorkspace->isDistribution();
  if (is_distrib)
	  widths.resize(localworkspace->blocksize()); // Will contain the bin widths of distribution

  double sumY, sumE;
  // Loop over spectra
  for (int i = m_MinSpec, j=0; i <= m_MaxSpec; ++i, ++j)
  {
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
    if (std::abs(m_MinRange)<1e-7) lowit=X.begin();
    else lowit=std::lower_bound(X.begin(),X.end(),m_MinRange);

    if (std::abs(m_MaxRange)<1e-7) highit=X.end();
    else highit=std::find_if(lowit,X.end(),std::bind2nd(std::greater<double>(),m_MaxRange));

    // If range specified doesn't overlap with this spectrum then bail out
    if ( lowit == X.end() || highit == X.begin() ) continue;
    
    highit--; // Upper limit is the bin before, i.e. the last value smaller than MaxRange
  
    MantidVec::difference_type distmin=std::distance(X.begin(),lowit);
    MantidVec::difference_type distmax=std::distance(X.begin(),highit);

    if (!is_distrib) //Sum the Y, and sum the E in quadrature
    {
      sumY=std::accumulate(Y.begin()+distmin,Y.begin()+distmax,0.0);
      sumE=std::accumulate(E.begin()+distmin,E.begin()+distmax,0.0,VectorHelper::SumSquares<double>());
    }
    else // Sum Y*binwidth and Sum the (E*binwidth)^2.
    {
      std::adjacent_difference(lowit,highit,widths.begin());
      sumY=std::inner_product(Y.begin()+distmin,Y.begin()+distmax,widths.begin()+1,0.0);
      sumE=std::inner_product(E.begin()+distmin,E.begin()+distmax,widths.begin()+1,0.0,std::plus<double>(),VectorHelper::TimesSquares<double>());
    }

    //Set X-boundaries
    outputWorkspace->dataX(j)[0] = lowit==X.end() ? *(lowit-1) : *(lowit);
    outputWorkspace->dataX(j)[1] = *highit;
    outputWorkspace->dataY(j)[0] = sumY;
    outputWorkspace->dataE(j)[0] = sqrt(sumE); // Propagate Gaussian error

    if (j % progress_step == 0)
    {
        interruption_point();
        progress( double(j)/(m_MaxSpec-m_MinSpec+1) );
    }
  }

  // Assign it to the output workspace property
  setProperty("OutputWorkspace",outputWorkspace);

  return;
}

} // namespace Algorithms
} // namespace Mantid
