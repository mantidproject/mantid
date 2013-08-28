/*WIKI*


Algorithm written using this paper:
J. Appl. Cryst. (2013). 46, 663-671

Objective algorithm to separate signal from noise in a Poisson-distributed pixel data set

T. Straaso/, D. Mueter, H. O. So/rensen and J. Als-Nielsen

Synopsis: A method is described for the estimation of background level and separation
of background pixels from signal pixels in a Poisson-distributed data set by statistical analysis.
For each iteration, the pixel with the highest intensity value is eliminated from the
data set and the sample mean and the unbiased variance estimator are calculated. Convergence is reached when the
absolute difference between the sample mean and the sample variance of the data set is within k standard deviations of the
variance, the default value of k being 1.  The k value is called SigmaConstant in the algorithm input.


*WIKI*/

#include "MantidAlgorithms/FindPeakBackground.h"
#include "MantidAlgorithms/FindPeaks.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Statistics.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ListValidator.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"

#include <sstream>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace std;

namespace Mantid
{
namespace Algorithms
{

  DECLARE_ALGORITHM(FindPeakBackground)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  FindPeakBackground::FindPeakBackground()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  FindPeakBackground::~FindPeakBackground()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** WIKI:
   */
  void FindPeakBackground::initDocs()
  {
    setWikiSummary("Separates background from signal for spectra of a workspace.");
    setOptionalMessage("Separates background from signal for spectra of a workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Define properties
    */
  void FindPeakBackground::init()
  {
    auto inwsprop = new WorkspaceProperty<MatrixWorkspace>("InputWorkspace", "Anonymous", Direction::Input);
    declareProperty(inwsprop, "Name of input MatrixWorkspace that contains peaks.");

    declareProperty(new ArrayProperty<int>("WorkspaceIndices"), "Optional: enter a comma-separated list of the "
    		        "workspace indices to have peak and background separated. "
                    "Default is to calculate for all spectra.");

    declareProperty("SigmaConstant", 1.0, "Multiplier of standard deviations of the variance for convergence of "
    		        "peak elimination.  Default is 1.0. ");

    declareProperty(new ArrayProperty<double>("FitWindow"),
                    "Optional: enter a comma-separated list of the minimum and maximum X-positions of window to fit.  "
    		        "The window is the same for all indices in workspace. The length must be exactly two.");

    std::vector<std::string> bkgdtypes;
    bkgdtypes.push_back("Flat");
    bkgdtypes.push_back("Linear");
    //bkgdtypes.push_back("Quadratic");
    declareProperty("BackgroundType", "Linear", boost::make_shared<StringListValidator>(bkgdtypes),
                    "Type of Background.");

    // The found peak in a table
    declareProperty(new WorkspaceProperty<API::ITableWorkspace>("OutputWorkspace", "", Direction::Output),
                    "The name of the TableWorkspace in which to store the background found for each index.  "
    		        "Table contains the indices of the beginning and ending of peak "
    		        "and the estimated background coefficients for the constant, linear, and quadratic terms.");

  }
  
  //----------------------------------------------------------------------------------------------
  /** Execute body
    */
  void FindPeakBackground::exec()
  {
    // 1. Get input and validate
    MatrixWorkspace_const_sptr inpWS = getProperty("InputWorkspace");
    std::vector<int> inpwsindex = getProperty("WorkspaceIndices");
    std::vector<double> m_vecFitWindows = getProperty("FitWindow");
    m_backgroundType = getPropertyValue("BackgroundType");
    double k = getProperty("SigmaConstant");

    bool separateall = false;
    if (inpwsindex.size() == 0)
    {
      separateall = true;
    }

    // 2. Generate output
    size_t numspec;
    if (separateall)
    {
      numspec = inpWS->getNumberHistograms();
    }
    else
    {
      numspec = inpwsindex.size();
    }
    size_t sizex = inpWS->readX(0).size();
    size_t sizey = inpWS->readY(0).size();
    size_t n = sizey;
    size_t l0 = 0;
    const MantidVec& inpX = inpWS->readX(0);

    if (m_vecFitWindows.size() > 1)
    {
  	  Mantid::Algorithms::FindPeaks fp;
  	  l0 = fp.getVectorIndex(inpX, m_vecFitWindows[0]);
      n = fp.getVectorIndex(inpX, m_vecFitWindows[1]);
      if (n < sizey) n++;
    }

    // Set up output table workspace
    API::ITableWorkspace_sptr m_outPeakTableWS = WorkspaceFactory::Instance().createTable("TableWorkspace");
    m_outPeakTableWS->addColumn("int", "wksp_index");
    m_outPeakTableWS->addColumn("int", "peak_min_index");
    m_outPeakTableWS->addColumn("int", "peak_max_index");
    m_outPeakTableWS->addColumn("double", "bkg0");
    m_outPeakTableWS->addColumn("double", "bkg1");
    m_outPeakTableWS->addColumn("double", "bkg2");
    for( size_t i = 0; i < numspec; ++i )
      m_outPeakTableWS->appendRow();

    // 3. Get Y values
    Progress prog(this, 0, 1.0, numspec);
    PARALLEL_FOR2(inpWS, m_outPeakTableWS)
    for (int i = 0; i < static_cast<int>(numspec); ++i)
    {
      PARALLEL_START_INTERUPT_REGION
      // a) figure out wsindex
      size_t wsindex;
      if (separateall)
      {
        // Update wsindex to index in input workspace
        wsindex = static_cast<size_t>(i);
      }
      else
      {
        // Use the wsindex as the input
        wsindex = static_cast<size_t>(inpwsindex[i]);
        if (wsindex >= inpWS->getNumberHistograms())
        {
          stringstream errmsg;
          errmsg << "Input workspace index " << inpwsindex[i] << " is out of input workspace range = "
                 << inpWS->getNumberHistograms() << endl;
        }
      }

      // Find background
      const MantidVec& inpX = inpWS->readX(wsindex);
      const MantidVec& inpY = inpWS->readY(wsindex);

      double Ymean, Yvariance, Ysigma;
      MantidVec maskedY;
      MantidVec::const_iterator in = std::min_element(inpY.begin(), inpY.end());
      double bkg0 = inpY[in - inpY.begin()];
      for (size_t l = l0; l < n; ++l)
      {
          maskedY.push_back(inpY[l]-bkg0);
      }
      MantidVec mask(n-l0,0.0);
      double xn = static_cast<double>(n-l0);
      do
      {
    	  Statistics stats = getStatistics(maskedY);
    	  Ymean = stats.mean;
    	  Yvariance = stats.standard_deviation * stats.standard_deviation;
    	  Ysigma = std::sqrt((moment4(maskedY,n-l0,Ymean)-(xn-3.0)/(xn-1.0) * Yvariance)/xn);
	      MantidVec::const_iterator it = std::max_element(maskedY.begin(), maskedY.end());
	      const size_t pos = it - maskedY.begin();
		  maskedY[pos] = 0;
		  mask[pos] = 1.0;
      }
      while (std::abs(Ymean-Yvariance) > k * Ysigma);

      if(n-l0 > 5)
      {
    	  // remove single outliers
		  if (mask[1] == mask[2] && mask[2] == mask[3])
			  mask[0] = mask[1];
		  if (mask[0] == mask[2] && mask[2] == mask[3])
			  mask[1] = mask[2];
		  for (size_t l = 2; l < n-l0-3; ++l)
		  {
			  if (mask[l-1] == mask[l+1] && (mask[l-1] == mask[l-2] || mask[l+1] == mask[l+2]))
			  {
				  mask[l] = mask[l+1];
			  }
		  }
		  if (mask[n-l0-2] == mask[n-l0-3] && mask[n-l0-3] == mask[n-l0-4])
			  mask[n-l0-1] = mask[n-l0-2];
		  if (mask[n-l0-1] == mask[n-l0-3] && mask[n-l0-3] == mask[n-l0-4])
			  mask[n-l0-2] = mask[n-l0-1];

		  // mask regions not connected to largest region
		  // for loop can start > 1 for multiple peaks
		  vector<cont_peak> peaks;
		  if (mask[0] == 1)
		  {
			  peaks.push_back(cont_peak());
			  peaks[peaks.size()-1].start = l0;
                  }
		  for (size_t l = 1; l < n-l0; ++l)
		  {
			  if (mask[l] != mask[l-1] && mask[l] == 1)
			  {
				  peaks.push_back(cont_peak());
				  peaks[peaks.size()-1].start = l+l0;
			  }
			  else if (peaks.size() > 0)
			  {
				  size_t ipeak = peaks.size()-1;
				  if (mask[l] != mask[l-1] && mask[l] == 0)
				  {
					  peaks[ipeak].stop = l+l0;
				  }
				  if (inpY[l+l0] > peaks[ipeak].maxY) peaks[ipeak].maxY = inpY[l+l0];
			  }
		  }
		  size_t min_peak, max_peak;
		  double a0,a1,a2;
		  if(peaks.size()> 0)
		  {
			  if(peaks[peaks.size()-1].stop == 0) peaks[peaks.size()-1].stop = n-1;
			  std::sort(peaks.begin(), peaks.end(), by_len());

			  // save endpoints
			  min_peak = peaks[0].start;
			  // extra point for histogram input
			  max_peak = peaks[0].stop + sizex - sizey;
			  estimateBackground(inpX, inpY, l0, n,
			      peaks[0].start, peaks[0].stop, a0, a1, a2);
		  }
		  else
		  {
			  // assume peak is larger than window so no background
			  min_peak = l0;
			  max_peak = n-1;
			  a0 = 0.0;
			  a1 = 0.0;
			  a2 = 0.0;
		  }

		  // Add a new row
		  API::TableRow t = m_outPeakTableWS->getRow(i);
		  t << static_cast<int>(wsindex) << static_cast<int>(min_peak) << static_cast<int>(max_peak) << a0 << a1 <<a2;
      }

	  prog.report();
	  PARALLEL_END_INTERUPT_REGION
    } // ENDFOR
    PARALLEL_CHECK_INTERUPT_REGION

    // 4. Set the output
    setProperty("OutputWorkspace", m_outPeakTableWS);

    return;
  }
  //----------------------------------------------------------------------------------------------
  /** Estimate background
* @param X :: vec for X
* @param Y :: vec for Y
* @param i_min :: index of minimum in X to estimate background
* @param i_max :: index of maximum in X to estimate background
* @param p_min :: index of peak min in X to estimate background
* @param p_max :: index of peak max in X to estimate background
* @param out_bg0 :: interception
* @param out_bg1 :: slope
* @param out_bg2 :: a2 = 0
*/
  void FindPeakBackground::estimateBackground(const MantidVec& X, const MantidVec& Y, const size_t i_min, const size_t i_max,
		  const size_t p_min, const size_t p_max,double& out_bg0, double& out_bg1, double& out_bg2)
  {
    // Validate input
    if (i_min >= i_max)
      throw std::runtime_error("i_min cannot larger or equal to i_max");
    if (p_min >= p_max)
      throw std::runtime_error("p_min cannot larger or equal to p_max");
    double sum = 0.0;
    double sumX = 0.0;
    double sumY = 0.0;
    double sumX2 = 0.0;
    double sumXY = 0.0;
    for (size_t i = i_min; i < i_max; ++i)
    {
		  if(i >= p_min && i < p_max) continue;
		  sum += 1.0;
		  sumX += X[i];
		  sumX2 += X[i]*X[i];
		  sumY += Y[i];
		  sumXY += X[i]*Y[i];
    }

    // Estimate
    out_bg0 = 0.;
    out_bg1 = 0.;
    out_bg2 = 0.;
    if (m_backgroundType.compare("Linear") == 0) // linear background
    {
      // Cramer's rule for 2 x 2 matrix
      double divisor = sum*sumX2-sumX*sumX;
      if (divisor != 0)
      {
		  out_bg0 = (sumY*sumX2-sumX*sumXY)/divisor;
		  out_bg1 = (sum*sumXY-sumY*sumX)/divisor;
      }
    }
    else // flat background
    {     if(sum != 0) out_bg0 = sumY/sum;
    }

    g_log.debug() << "Estimated background: A0 = " << out_bg0 << ", A1 = "
                        << out_bg1 << ", A2 = " << out_bg2 << "\n";

    return;
  }
  //----------------------------------------------------------------------------------------------
  /** Calculate 4th moment
* @param X :: vec for X
* @param n :: length of vector
* @param mean :: mean of X
*/
  double FindPeakBackground::moment4(MantidVec& X, size_t n, double mean)
  {
	  double sum=0.0;
	  for (size_t i = 0; i < n; ++i)
	  {
		  sum += (X[i]-mean)*(X[i]-mean)*(X[i]-mean)*(X[i]-mean);
	  }
	  sum /= static_cast<double>(n);
          return sum;
  }
} // namespace Algorithms
} // namespace Mantid
