/*WIKI*

Integration sums up spectra in a [[Workspace]] and outputs a [[Workspace]] that contains only 1 value per spectrum (i.e. the sum). The associated errors are added in quadrature.
The two X values per spectrum are set to the limits of the range over which the spectrum has been integrated. By default, the entire range is integrated and all spectra are included.

=== Optional properties ===
If only a portion of the workspace should be integrated then the optional parameters may be used to restrict the range. StartWorkspaceIndex & EndWorkspaceIndex may be used to select a contiguous range of spectra in the workspace (note that these parameters refer to the workspace index value rather than spectrum numbers as taken from the raw file).
If only a certain range of each spectrum should be summed (which must be the same for all spectra being integrated) then the Range_lower and Range_upper properties should be used. No rebinning takes place as part of this algorithm: if the values given do not coincide with a bin boundary then the first bin boundary within the range is used. If a value is given that is beyond the limit covered by the spectrum then it will be integrated up to its limit.
The data that falls outside any values set will not contribute to the output workspace.

=== EventWorkspaces ===
If an [[EventWorkspace]] is used as the input, the output will be a [[MatrixWorkspace]]. [[Rebin]] is recommended if you want to keep the workspace as an EventWorkspace.


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Integration.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/RebinnedOutput.h"
#include "MantidAPI/Progress.h"
#include <cmath>

#include "MantidAPI/TextAxis.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(Integration)

/// Sets documentation strings for this algorithm
void Integration::initDocs()
{
  this->setWikiSummary("Integration takes a 2D [[workspace]] or an [[EventWorkspace]] as input and sums the data values. Optionally, the range summed can be restricted in either dimension. The output will always be a [[MatrixWorkspace]] even when inputting an EventWorkspace, if you wish to keep this as the output then you should use [[Rebin]].");
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
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input, boost::make_shared<HistogramValidator>()));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));

  declareProperty("RangeLower",EMPTY_DBL());
  declareProperty("RangeUpper",EMPTY_DBL());

  auto mustBePositive = boost::make_shared<BoundedValidator<int> >();
  mustBePositive->setLower(0);
  declareProperty("StartWorkspaceIndex", 0, mustBePositive);
  declareProperty("EndWorkspaceIndex", EMPTY_INT(), mustBePositive);
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
  m_IncPartBins = getProperty("IncludePartialBins");

  // Get the input workspace
  MatrixWorkspace_const_sptr localworkspace = this->getInputWorkspace();

  const int numberOfSpectra = static_cast<int>(localworkspace->getNumberHistograms());

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

  // Create the 2D workspace (with 1 bin) for the output
  MatrixWorkspace_sptr outputWorkspace = this->getOutputWorkspace(localworkspace);

  bool is_distrib=outputWorkspace->isDistribution();
  Progress progress(this, 0, 1, m_MaxSpec-m_MinSpec+1);

  const bool axisIsText = localworkspace->getAxis(1)->isText();

  // Loop over spectra
  PARALLEL_FOR2(localworkspace,outputWorkspace)
  for (int i = m_MinSpec; i <= m_MaxSpec; ++i)
  {
    PARALLEL_START_INTERUPT_REGION
    // Workspace index on the output
    const int outWI = i - m_MinSpec;

    // Copy Axis values from previous workspace
    if ( axisIsText )
    {
      Mantid::API::TextAxis* newAxis = dynamic_cast<Mantid::API::TextAxis*>(outputWorkspace->getAxis(1));
      newAxis->setLabel(outWI, localworkspace->getAxis(1)->label(i));
    }

    // This is the output
    ISpectrum * outSpec = outputWorkspace->getSpectrum(outWI);
    // This is the input
    const ISpectrum * inSpec = localworkspace->getSpectrum(i);

    // Copy spectrum number, detector IDs
    outSpec->copyInfoFrom(*inSpec);

    // Retrieve the spectrum into a vector
    const MantidVec& X = inSpec->readX();
    const MantidVec& Y = inSpec->readY();
    const MantidVec& E = inSpec->readE();

    // If doing partial bins, we want to set the bin boundaries to the specified values
    // regardless of whether they're 'in range' for this spectrum
    // Have to do this here, ahead of the 'continue' a bit down from here.
    if ( m_IncPartBins )
    {
      outSpec->dataX()[0] = m_MinRange;
      outSpec->dataX()[1] = m_MaxRange;
    }

    // Find the range [min,max]
    MantidVec::const_iterator lowit, highit;
    if (m_MinRange == EMPTY_DBL()) lowit=X.begin();
    else lowit=std::lower_bound(X.begin(),X.end(),m_MinRange);

    if (m_MaxRange == EMPTY_DBL()) highit=X.end();
    else highit=std::find_if(lowit,X.end(),std::bind2nd(std::greater<double>(),m_MaxRange));

    // If range specified doesn't overlap with this spectrum then bail out
    if ( lowit == X.end() || highit == X.begin() ) continue;

    // Upper limit is the bin before, i.e. the last value smaller than MaxRange
    --highit;

    MantidVec::difference_type distmin = std::distance(X.begin(),lowit);
    MantidVec::difference_type distmax = std::distance(X.begin(),highit);

    double sumY = 0.0;
    double sumE = 0.0;
    if (distmax<=distmin)
    {
      sumY=0.;
      sumE=0.;
    }
    else
    {
        if (!is_distrib)
        {
        //Sum the Y, and sum the E in quadrature
          {
            sumY = std::accumulate(Y.begin()+distmin, Y.begin()+distmax, 0.0);
            sumE = std::accumulate(E.begin()+distmin, E.begin()+distmax, 0.0,
                             VectorHelper::SumSquares<double>());
          }
        }
        else
        {
            // Sum Y*binwidth and Sum the (E*binwidth)^2.
            std::vector<double> widths(X.size());
            // highit+1 is safe while input workspace guaranteed to be histogram
            std::adjacent_difference(lowit,highit+1,widths.begin());
            sumY = std::inner_product(Y.begin()+distmin, Y.begin()+distmax,
                                widths.begin()+1, 0.0);
            sumE = std::inner_product(E.begin()+distmin, E.begin()+distmax,
                                widths.begin()+1, 0.0, std::plus<double>(),
                                VectorHelper::TimesSquares<double>());
        }
    }
    // If partial bins are included, set integration range to exact range
    // given and add on contributions from partial bins either side of range.
    if( m_IncPartBins )
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
      outSpec->dataX()[0] = lowit==X.end() ? *(lowit-1) : *(lowit);
      outSpec->dataX()[1] = *highit;
    }

    outSpec->dataY()[0] = sumY;
    outSpec->dataE()[0] = sqrt(sumE); // Propagate Gaussian error

    progress.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // Assign it to the output workspace property
  setProperty("OutputWorkspace", outputWorkspace);

  return;
}

/**
 * This function gets the input workspace. In the case for a RebinnedOutput
 * workspace, it must be cleaned before proceeding. Other workspaces are
 * untouched.
 * @return the input workspace, cleaned if necessary
 */
MatrixWorkspace_const_sptr Integration::getInputWorkspace()
{
  MatrixWorkspace_sptr temp = getProperty("InputWorkspace");
  if (temp->id() == "RebinnedOutput")
  {
    // Clean the input workspace in the RebinnedOutput case for nan's and
    // inf's in order to treat the data correctly later.
    IAlgorithm_sptr alg = this->createChildAlgorithm("ReplaceSpecialValues");
    alg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", temp);
    std::string outName = "_"+temp->getName()+"_clean";
    alg->setProperty("OutputWorkspace", outName);
    alg->setProperty("NaNValue", 0.0);
    alg->setProperty("NaNError", 0.0);
    alg->setProperty("InfinityValue", 0.0);
    alg->setProperty("InfinityError", 0.0);
    alg->executeAsChildAlg();
    temp = alg->getProperty("OutputWorkspace");
  }
  return temp;
}

/**
 * This function creates the output workspace. In the case of a RebinnedOutput
 * workspace, the resulting workspace only needs to be a Workspace2D to handle
 * the integration. Other workspaces are handled normally.
 * @return the output workspace
 */
MatrixWorkspace_sptr Integration::getOutputWorkspace(MatrixWorkspace_const_sptr inWS)
{
  if (inWS->id() == "RebinnedOutput")
  {
    MatrixWorkspace_sptr outWS = API::WorkspaceFactory::Instance().create("Workspace2D",
                                                                          m_MaxSpec-m_MinSpec+1,2,1);
    API::WorkspaceFactory::Instance().initializeFromParent(inWS, outWS, true);
    return outWS;
  }
  else
  {
    return API::WorkspaceFactory::Instance().create(inWS, m_MaxSpec-m_MinSpec+1,
                                                    2, 1);
  }
}

} // namespace Algorithms
} // namespace Mantid
