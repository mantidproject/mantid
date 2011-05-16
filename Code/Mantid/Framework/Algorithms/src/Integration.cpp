//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Integration.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/Progress.h"
#include <cmath>

#include "MantidAPI/TextAxis.h"

namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(Integration)

/// Sets documentation strings for this algorithm
void Integration::initDocs()
{
  this->setWikiSummary("Integration takes a 2D [[workspace]] or an [[EventWorkspace]] as input and sums the data values. Optionally, the range summed can be restricted in either dimension. ");
  this->setOptionalMessage("Integration takes a 2D workspace or an EventWorkspace as input and sums the data values. Optionally, the range summed can be restricted in either dimension.");
}


using namespace Kernel;
using namespace API;
using namespace DataObjects;


/** Initialisation method.
 *
 */
void Integration::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,new HistogramValidator<>));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));

  declareProperty("RangeLower",EMPTY_DBL());
  declareProperty("RangeUpper",EMPTY_DBL());
  BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
  mustBePositive->setLower(0);
  declareProperty("StartWorkspaceIndex", 0, mustBePositive);
  // As the property takes ownership of the validator pointer, have to take care to pass in a unique
  // pointer to each property.
  declareProperty("EndWorkspaceIndex", EMPTY_INT(), mustBePositive->clone());
  declareProperty("IncludePartialBins", false, "If true then partial bins from the beginning and end of the input range are also included in the integration.");
}

/** Executes the algorithm
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void Integration::exec()
{
  // Try and retrieve the optional properties
  m_MinRange = getProperty("RangeLower");
  m_MaxRange = getProperty("RangeUpper");
  m_MinSpec = getProperty("StartWorkspaceIndex");
  m_MaxSpec = getProperty("EndWorkspaceIndex");
  const bool incPartBins = getProperty("IncludePartialBins");

  // Get the input workspace
  MatrixWorkspace_const_sptr localworkspace = getProperty("InputWorkspace");

  const size_t numberOfSpectra = localworkspace->getNumberHistograms();

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


  // Can we keep events?
  inputEventWS = boost::dynamic_pointer_cast<const EventWorkspace>(localworkspace);
  if (inputEventWS)
  {
    execEvent();
    return;
  }

  // Create the 1D workspace for the output
  MatrixWorkspace_sptr outputWorkspace = API::WorkspaceFactory::Instance().create(localworkspace,m_MaxSpec-m_MinSpec+1,2,1);

  bool is_distrib=outputWorkspace->isDistribution();
  Progress progress(this,0,1,m_MaxSpec-m_MinSpec+1);

  const bool axisIsText = localworkspace->getAxis(1)->isText();

  // Loop over spectra
  PARALLEL_FOR2(localworkspace,outputWorkspace)
  for (int i = m_MinSpec; i <= m_MaxSpec; ++i)
  {
    PARALLEL_START_INTERUPT_REGION
    const int j = i - m_MinSpec;
    
    // Copy Axis values from previous workspace
    if ( axisIsText )
    {
      Mantid::API::TextAxis* newAxis = dynamic_cast<Mantid::API::TextAxis*>(outputWorkspace->getAxis(1));
      newAxis->setLabel(j, localworkspace->getAxis(1)->label(i));
    }
    else
    {
      outputWorkspace->getAxis(1)->setValue(j, localworkspace->getAxis(1)->operator()(i));
    }

    // Retrieve the spectrum into a vector
    const MantidVec& X = localworkspace->readX(i);
    const MantidVec& Y = localworkspace->readY(i);
    const MantidVec& E = localworkspace->readE(i);

    // If doing partial bins, we want to set the bin boundaries to the specified values
    // regardless of whether they're 'in range' for this spectrum
    // Have to do this here, ahead of the 'continue' a bit down from here.
    if ( incPartBins )
    {
      outputWorkspace->dataX(j)[0] = m_MinRange;
      outputWorkspace->dataX(j)[1] = m_MaxRange;
    }

    // Find the range [min,max]
    MantidVec::const_iterator lowit, highit;
    if (m_MinRange == EMPTY_DBL()) lowit=X.begin();
    else lowit=std::lower_bound(X.begin(),X.end(),m_MinRange);

    if (m_MaxRange == EMPTY_DBL()) highit=X.end();
    else highit=std::find_if(lowit,X.end(),std::bind2nd(std::greater<double>(),m_MaxRange));

    // If range specified doesn't overlap with this spectrum then bail out
    if ( lowit == X.end() || highit == X.begin() ) continue;
    
    highit--; // Upper limit is the bin before, i.e. the last value smaller than MaxRange
  
    MantidVec::difference_type distmin = std::distance(X.begin(),lowit);
    MantidVec::difference_type distmax = std::distance(X.begin(),highit);

    double sumY = 0.0;
    double sumE = 0.0;

    if (!is_distrib) //Sum the Y, and sum the E in quadrature
    {
      sumY = std::accumulate(Y.begin()+distmin,Y.begin()+distmax,0.0);
      sumE = std::accumulate(E.begin()+distmin,E.begin()+distmax,0.0,VectorHelper::SumSquares<double>());
    }
    else // Sum Y*binwidth and Sum the (E*binwidth)^2.
    {
      std::vector<double> widths(X.size());
      std::adjacent_difference(lowit,highit+1,widths.begin()); // highit+1 is safe while input workspace guaranteed to be histogram
      sumY = std::inner_product(Y.begin()+distmin,Y.begin()+distmax,widths.begin()+1,0.0);
      sumE = std::inner_product(E.begin()+distmin,E.begin()+distmax,widths.begin()+1,0.0,std::plus<double>(),VectorHelper::TimesSquares<double>());
    }
    // If partial bins are included, set integration range to exact range given and add on contributions from partial bins either side of range.
    if( incPartBins )
    {
      if( distmin > 0 )
      {
        const double lower_bin = *lowit;
        const double prev_bin = *(lowit - 1);
        double fraction = (lower_bin - m_MinRange);
        if( !is_distrib )
        {
          fraction /= (lower_bin - prev_bin);
        }
        const MantidVec::size_type val_index = distmin - 1;
        sumY += Y[val_index] * fraction;
        const double eval = E[val_index];
        sumE += eval * eval * fraction * fraction;
      }
      if( highit < X.end() - 1)
      {
        const double upper_bin = *highit;
        const double next_bin = *(highit + 1);
        double fraction = (m_MaxRange - upper_bin);
        if( !is_distrib )
        {
          fraction /= (next_bin - upper_bin);
        }
        sumY += Y[distmax] * fraction;
        const double eval = E[distmax];
        sumE += eval * eval * fraction * fraction;
      }
    }
    else
    {
      outputWorkspace->dataX(j)[0] = lowit==X.end() ? *(lowit-1) : *(lowit);
      outputWorkspace->dataX(j)[1] = *highit;
    }

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



/** Executes the algorithm on EventWorkspaces
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void Integration::execEvent()
{
  EventWorkspace_sptr outputWS;

  // Changing in place, and not changing the number of histograms?
  if (getPropertyValue("InputWorkspace") == getPropertyValue("OutputWorkspace") &&
      (m_MaxSpec == inputEventWS->getNumberHistograms()-1) && (m_MinSpec == 0))
  {
    // Then we don't need to copy the data.
    // OutputWorkspace property will point to input workspace, just need to cast.
    MatrixWorkspace_sptr outputWS_matrix = getProperty("OutputWorkspace");
    outputWS = boost::dynamic_pointer_cast<EventWorkspace>(outputWS_matrix);
  }
  else
  {
    //Make a brand new EventWorkspace
    outputWS = boost::dynamic_pointer_cast<EventWorkspace>(
        API::WorkspaceFactory::Instance().create("EventWorkspace", m_MaxSpec-m_MinSpec+1, 2, 1));
    //Copy geometry over.
    API::WorkspaceFactory::Instance().initializeFromParent(inputEventWS, outputWS, false);
    //You need to copy over the data as well.
    // -- we copy only a range, if specified.
    outputWS->copyDataFrom( (*inputEventWS), m_MinSpec, m_MaxSpec );
  }

  Progress * progress = new Progress(this,0,1,m_MaxSpec-m_MinSpec +1 + inputEventWS->getNumberHistograms());

  //Sort the input WS - this will allow parallel processing after
  inputEventWS->sortAll(TOF_SORT, progress);

  for (int i = 0; i < m_MaxSpec-m_MinSpec; ++i)
  {
    // Remove events before minRange
    outputWS->getEventListPtr(i)->maskTof( -std::numeric_limits<double>::max(), m_MinRange);
    // Remove events after maxRange
    outputWS->getEventListPtr(i)->maskTof(  m_MaxRange, std::numeric_limits<double>::max());
    // Progress report
    progress->report("Filtering");
  }

  // Make the X bin vector
  MantidVecPtr X;
  X.access().push_back(m_MinRange);
  X.access().push_back(m_MaxRange);

  // And set it for all workspaces
  outputWS->setAllX(X);

  // Assign it to the output workspace property
  setProperty("OutputWorkspace", boost::dynamic_pointer_cast<MatrixWorkspace>(outputWS));


//  // Loop over spectra
//  PARALLEL_FOR2(localworkspace,outputWorkspace)
  //  for (int i = m_MinSpec; i <= m_MaxSpec; ++i)
//  {
//    PARALLEL_START_INTERUPT_REGION
}


} // namespace Algorithms
} // namespace Mantid
