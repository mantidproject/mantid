//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/FindPeaks.h"
#include "MantidAPI/TableRow.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FindPeaks)

using namespace Kernel;
using namespace API;

// Set the number of smoothing iterations to 5, the optimum value according to Mariscotti
int FindPeaks::g_z = 5;

/// Constructor
FindPeaks::FindPeaks() : API::Algorithm(),m_progress(NULL) {}

void FindPeaks::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input),
    "Name of the workspace to search" );
  BoundedValidator<int> *range = new BoundedValidator<int>(1,32);
  // The estimated width of a peak in terms of number of channels
  declareProperty("FWHM",7,range,
    "Estimated number of points covered by the fwhm of a peak (default 7)" );
  // The tolerance allowed in meeting the conditions
  BoundedValidator<int> *min = new BoundedValidator<int>();
  min->setLower(1);
  declareProperty("Tolerance", 4, min,
    "A measure of the strictness desired in meeting the condition on peak candidates,\n"
    "Mariscotti recommends 2 (default 4)");
  
  // Temporary so that I can look at the smoothed data
  declareProperty(new WorkspaceProperty<>("SmoothedData","",Direction::Output));

  // The found peaks in a table
  declareProperty(
    new WorkspaceProperty<API::ITableWorkspace>("PeaksList","",Direction::Output),
    "The name of the TableWorkspace in which to store the list of peaks found" );
  
  // Set up the columns for the TableWorkspace holding the peak information
  m_peaks = WorkspaceFactory::Instance().createTable("TableWorkspace");
  m_peaks->addColumn("int","spectrum");
  m_peaks->addColumn("double","centre");
  m_peaks->addColumn("double","width");
  m_peaks->addColumn("double","height");
  m_peaks->addColumn("double","backgroundintercept");
  m_peaks->addColumn("double","backgroundslope");
}

void FindPeaks::exec()
{
  // Retrieve the input workspace
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");

  MatrixWorkspace_sptr smoothedData = this->calculateSecondDifference(inputWS);

  // The optimum number of points in the smoothing, according to Mariscotti, is 0.6*fwhm
  const int fwhm = getProperty("FWHM");
  int w = static_cast<int>(0.6 * fwhm);
  // w must be odd
  if (!(w%2)) ++w;

  // Carry out the number of smoothing steps given by g_z (should be 5)
  for (int i = 0; i < g_z; ++i)
  {
    this->smoothData(smoothedData,w);
  }
  // Now calculate the errors on the smoothed data
  this->calculateStandardDeviation(inputWS,smoothedData,w);

  // Calculate n1 (Mariscotti eqn. 18)
  const double kz = 1.22; // This kz corresponds to z=5 & w=0.6*fwhm - see Mariscotti Fig. 8
  const int n1 = static_cast<int>(kz * fwhm + 0.5);
  // Can't calculate n2 or n3 yet because they need i0
  const int tolerance = getProperty("Tolerance");

  // Temporary - to allow me to look at smoothed data
  setProperty("SmoothedData",smoothedData);

  // Loop over the spectra searching for peaks
  const int numHists = smoothedData->getNumberHistograms();
  const int blocksize = smoothedData->blocksize();
  m_progress = new Progress(this,0.0,1.0,numHists);

  for (int k = 0; k < numHists; ++k)
  {
    const std::vector<double> &S = smoothedData->readY(k);
    const std::vector<double> &F = smoothedData->readE(k);
     //progress(double(k/numHists));
	
    // This implements the flow chart given on page 320 of Mariscotti
    int i0 = 0, i1 = 0, i2 = 0, i3 = 0, i4 = 0, i5 = 0;
    for ( int i = 1; i < blocksize; ++i)
    {
		
      int M = 0;
      if ( S[i] > F[i] ) M = 1;
      else { S[i] > 0 ? M = 2 : M = 3; }

      if ( S[i-1] > F[i-1] )
      {
        switch (M)
        {
        case 3:
          i3 = i;
          // intentional fall-through
        case 2:
          i2 = i-1;
          break;
        case 1:
          // do nothing
          break;
        default:
          assert( false ); // should never happen
        }
      }
      else if ( S[i-1] > 0 )
      {
        switch (M)
        {
        case 3:
          i3 = i;
          break;
        case 2:
          // do nothing
          break;
        case 1:
          i1 = i;
          break;
        default:
          assert( false ); // should never happen
        }
      }
      else
      {
        switch (M)
        {
        case 3:
          // do nothing
          break;
        case 2: // fall through (i.e. same action if M = 1 or 2)
        case 1:
          i5 = i-1;
          break;
        default:
          assert( false ); // should never happen
        }
      }

      if ( i5 && i1 && i2 && i3 ) // If i5 has been set then we should have the full set and can check conditions
      {
        i4 = i3; // Starting point for finding i4 - calculated below
        double num = 0.0, denom = 0.0;
        for ( int j = i3; j <= i5; ++j )
        {
          // Calculate i4 - it's at the minimum value of Si between i3 & i5
          if ( S[j] <= S[i4] ) i4 = j;
          // Calculate sums for i0 (Mariscotti eqn. 27)
          num += j * S[j];
          denom += S[j];
        }
        i0 = static_cast<int>(num/denom);

        // Check we have a correctly ordered set of points. If not, reset and continue
        if ( i1>i2 || i2>i3 || i3>i4 || i5<=i4 )
        {
          i5 = 0;
          continue;
        }

        // Check if conditions are fulfilled - if any are not, loop onto the next i in the spectrum
        // Mariscotti eqn. (14)
        if ( std::abs(S[i4]) < 2*F[i4] )
        {
          i5 = 0;
          continue;
        }
        // Mariscotti eqn. (19)
        if ( abs( i5-i3+1-n1 ) > tolerance )
        {
          i5 = 0;
          continue;
        }
        // Calculate n2 (Mariscotti eqn. 20)
        int n2 = abs( static_cast<int>(0.5*(F[i0]/S[i0])*(n1+tolerance)+0.5) );
        const int n2b = abs( static_cast<int>(0.5*(F[i0]/S[i0])*(n1-tolerance)+0.5) );
        if (n2b > n2) n2 = n2b;
        // Mariscotti eqn. (21)
        const int testVal = n2 ? n2 : 1;
        if ( i3-i2-1 > testVal )
        {
          i5 = 0;
          continue;
        }
        // Calculate n3 (Mariscotti eqn. 22)
        int n3 = abs( static_cast<int>((n1+tolerance)*(1-2*(F[i0]/S[i0])) + 0.5) );
        const int n3b = abs( static_cast<int>((n1-tolerance)*(1-2*(F[i0]/S[i0])) + 0.5) );
        if ( n3b < n3 ) n3 = n3b;
        // Mariscotti eqn. (23)
        if ( i2-i1+1 < n3 )
        {
          i5 = 0;
          continue;
        }

        // If we get to here then we've identified a peak
        g_log.debug() << "Spectrum=" << k << " i0=" << inputWS->readX(k)[i0] << " i1=" << i1 << " i2=" << i2 << " i3=" << i3 << " i4=" << i4 << " i5=" << i5 << std::endl;

        this->fitPeak(inputWS,k,i0,i4);
        
        // reset and go searching for the next peak
        i1 = 0, i2 = 0, i3 = 0, i4 = 0, i5 = 0;
      }

    } // loop through a single spectrum

	m_progress->report();
	
  } // loop over spectra

  g_log.information() << "Total of " << m_peaks->rowCount() << " peaks found and successfully fitted." << std::endl;
  setProperty("PeaksList",m_peaks);
}

/** Calculates the second difference of the data (Y values) in a workspace.
 *  Done according to equation (3) in Mariscotti: \f$ S_i = N_{i+1} - 2N_i + N_{i+1} \f$.
 *  In the output workspace, the 2nd difference is in Y, X is unchanged and E is zero.
 *  @param input The workspace to calculate the second difference of
 *  @return A workspace containing the second difference
 */
API::MatrixWorkspace_sptr FindPeaks::calculateSecondDifference(const API::MatrixWorkspace_const_sptr &input)
{
  // We need a new workspace the same size as the input ont
  MatrixWorkspace_sptr diffed = WorkspaceFactory::Instance().create(input);

  const int numHists = input->getNumberHistograms();
  const int blocksize = input->blocksize();

  // Loop over spectra
  for (int i = 0; i < numHists; ++i)
  {
    // Copy over the X values
    diffed->dataX(i) = input->readX(i);
    
    const std::vector<double> &Y = input->readY(i);
    std::vector<double> &S = diffed->dataY(i);
    // Go through each spectrum calculating the second difference at each point
    // First and last points in each spectrum left as zero (you'd never be able to find peaks that close to the edge anyway)
    for (int j = 1; j < blocksize-1; ++j)
    {
      S[j] = Y[j-1] - 2*Y[j] + Y[j+1];
    }
  }

  return diffed;
}

/** Calls the SmoothData algorithm as a sub-algorithm on a workspace
 *  @param WS The workspace containing the data to be smoothed. The smoothed result will be stored in this pointer.
 *  @param w  The number of data points which should contribute to each smoothed point
 */
void FindPeaks::smoothData(API::MatrixWorkspace_sptr &WS, const int &w)
{
  g_log.information("Smoothing the input data");
  IAlgorithm_sptr smooth = createSubAlgorithm("SmoothData");
  smooth->setProperty("InputWorkspace", WS);
  // The number of points which contribute to each smoothed point
  smooth->setProperty("NPoints",w);
  try {
    smooth->execute();
  } catch (std::runtime_error) {
    g_log.error("Unable to successfully run SmoothData sub-algorithm");
    throw;
  }

  if ( ! smooth->isExecuted() )
  {
    g_log.error("Unable to successfully run SmoothData sub-algorithm");
    throw std::runtime_error("Unable to successfully run SmoothData sub-algorithm");
  }
  // Get back the result
  WS = smooth->getProperty("OutputWorkspace");
}

/** Calculates the statistical error on the smoothed data.
 *  Uses Mariscotti equation (11), amended to use errors of input data rather than sqrt(Y).
 *  @param input    The input data to the algorithm
 *  @param smoothed The smoothed data
 *  @param w        The value of w (the size of the smoothing 'window')
 *  @throw std::invalid_argument if w is greater than 19
 */
void FindPeaks::calculateStandardDeviation(const API::MatrixWorkspace_const_sptr &input, const API::MatrixWorkspace_sptr &smoothed, const int &w)
{
  // Guard against anyone changing the value of z, which would mean different phi values were needed (see Marriscotti p.312)
  assert( g_z == 5 );
  // Have to adjust for fact that I normalise Si (unlike the paper)
  const int factor = static_cast<int>(std::pow(static_cast<double>(w),g_z));

  // Check value of w is valid
  if (w > 19)
  {
    std::string s("Invalid value of w - must not be greater than 19 (corresponding to a max fwhm of 32)");
    g_log.error(s);
    throw std::invalid_argument(s);
  }
  // These are the values of phi as a function of w for z=5. See Mariscotti p.312.
  std::map<int,int> phi;
  phi[1] = 6; phi[3] = 448; phi[5] = 5220; phi[7] = 27342; phi[9] = 95034; phi[11] = 257796; phi[13] = 592488;
  phi[15] = 1209410; phi[17] = 2258382; phi[19] = 3934824;
  
  const double constant = sqrt(static_cast<double>(phi[w])) / factor;
  
  const int numHists = smoothed->getNumberHistograms();
  const int blocksize = smoothed->blocksize();
  for (int i = 0; i < numHists; ++i)
  {
    const std::vector<double> &E = input->readE(i);
    std::vector<double> &Fi = smoothed->dataE(i);

    for (int j = 0; j < blocksize; ++j)
    {
      Fi[j] = constant * E[j];
    }
  }
}

/** Attempts to fit a candidate peak
 * 
 *  @param input    The input workspace
 *  @param spectrum The spectrum index of the peak
 *  @param i0       Channel number of peak i0
 *  @param i4       Channel number of peak i4
 */
void FindPeaks::fitPeak(const API::MatrixWorkspace_sptr &input, const int spectrum, const int i0, const int i4)
{
  IAlgorithm_sptr fit;
  try
  {
    // Fitting the candidate peaks to a Gaussian
    fit = createSubAlgorithm("Gaussian1D");
  }
  catch (Exception::NotFoundError)
  {
    g_log.error("The StripPeaks algorithm requires the CurveFitting library");
    throw;
  }
  fit->setProperty("InputWorkspace",input);
  fit->setProperty("SpectrumIndex",spectrum);

  const std::vector<double> &X = input->readX(spectrum);
  const std::vector<double> &Y = input->readY(spectrum);
  
  for (unsigned int width = 2; width <= 10; width +=2)
  {
    // See Mariscotti eqn. 20. Using l=1 for bg0/bg1 - correspond to p6 & p7 in paper.
    unsigned int i_min = 1;
    if (i0 > static_cast<int>(5*width)) i_min = i0 - 5*width;
    unsigned int i_max = i0 + 5*width;
    // Bounds checks
    if (i_min<1) i_min=1;
    if (i_max>=Y.size()-1) i_max=Y.size()-2;
    const double bg_lowerSum = Y[i_min-1] + Y[i_min] + Y[i_min+1];
    const double bg_upperSum = Y[i_max-1] + Y[i_max] + Y[i_max+1];

    const double in_bg0 = (bg_lowerSum + bg_upperSum) / 6.0;
    const double in_bg1 = (bg_upperSum - bg_lowerSum) / (3.0*(i_max-i_min+1));
    const double in_height = Y[i4] - in_bg0;
    const double in_centre = input->isHistogramData() ? 0.5*(X[i0]+X[i0+1]) : X[i0];
    const double in_sigma = (i0+width < X.size())? X[i0+width] - X[i0] : 0.;
  
    fit->setProperty("bg0",in_bg0);
    fit->setProperty("bg1",in_bg1);
    fit->setProperty("peakCentre",in_centre);
    fit->setProperty("sigma",in_sigma);
    fit->setProperty("height",in_height);
    fit->setProperty("StartX",X[i_min]);
    fit->setProperty("EndX",X[i_max]);
  
    try {
      fit->execute();
    } catch (std::runtime_error) {
      g_log.error("Unable to successfully run Gaussian Fit sub-algorithm");
      throw;
    }

    if ( ! fit->isExecuted() )
    {
      g_log.error("Unable to successfully run Gaussian Fit sub-algorithm");
      throw std::runtime_error("Unable to successfully run Gaussian Fit sub-algorithm");
    }
 
    std::string fitStatus = fit->getProperty("Output Status");
    const double height = fit->getProperty("height");
    if ( height < 0 ) fitStatus.clear();              // Height must be positive
    if ( ! fitStatus.compare("success") ) 
    {
      const double centre = fit->getProperty("peakCentre");
      const double width = fit->getProperty("sigma");
      const double bgintercept = fit->getProperty("bg0");
      const double bgslope = fit->getProperty("bg1");
      g_log.information() << "Peak Fitted. Centre=" << centre << ", Sigma=" << width << ", Height=" << height 
                    << ", Background slope=" << bgslope << ", Background intercept=" << bgintercept << std::endl;
      API::TableRow t = m_peaks->appendRow();
      t << spectrum << centre << width << height << bgintercept << bgslope;
      break;
    }
  }

}

} // namespace Algorithms
} // namespace Mantid
