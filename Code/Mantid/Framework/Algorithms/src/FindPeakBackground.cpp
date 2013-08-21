/*WIKI*

Separates background from signal for spectra of a workspace.

*WIKI*/

#include "MantidAlgorithms/FindPeakBackground.h"
#include "MantidAlgorithms/FindPeaks.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Statistics.h"
#include "MantidDataObjects/Workspace2D.h"

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
    declareProperty(inwsprop, "Name of input MatrixWorkspace to have signal and background separated.");

    declareProperty("WorkspaceIndex", EMPTY_INT(), "Index of the spectrum to have peak and background separated. "
                    "Default is to calculate for all spectra.");

    declareProperty(new ArrayProperty<double>("FitWindow"),
                    "Optional: enter a comma-separated list of the expected X-position of window to fit. The number of values must be exactly two.");

    auto outwsprop = new WorkspaceProperty<Workspace2D>("OutputWorkspace", "", Direction::Output);
    declareProperty(outwsprop, "Name of the output Workspace2D containing the peak.");

  }
  
  //----------------------------------------------------------------------------------------------
  /** Execute body
    */
  void FindPeakBackground::exec()
  {
    // 1. Get input and validate
    MatrixWorkspace_const_sptr inpWS = getProperty("InputWorkspace");
    int inpwsindex = getProperty("WorkspaceIndex");
    std::vector<double> m_vecFitWindows = getProperty("FitWindow");

    bool separateall = false;
    if (inpwsindex == EMPTY_INT())
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
      numspec = 1;
    }
    size_t sizex = inpWS->readX(0).size();
    size_t sizey = inpWS->readY(0).size();
    size_t n = sizey;
    size_t l0 = 0;
    const MantidVec& inpX = inpWS->readX(0);

    if (m_vecFitWindows.size() > 0)
    {
  	  Mantid::Algorithms::FindPeaks fp;
  	  l0 = fp.getVectorIndex(inpX, m_vecFitWindows[0]);
      n = fp.getVectorIndex(inpX, m_vecFitWindows[1]);
      if (n < sizey) n++;
    }

    Workspace2D_sptr outWS = boost::dynamic_pointer_cast<Workspace2D>(WorkspaceFactory::Instance().create(
          "Workspace2D", numspec, 2, 2));

    // 3. Get Y values
    Progress prog(this, 0, 1.0, numspec);
    //PARALLEL_FOR2(inpWS, outWS)
    for (size_t i = 0; i < numspec; ++i)
    {
      //PARALLEL_START_INTERUPT_REGION
      // a) figure out wsindex
      size_t wsindex;
      if (separateall)
      {
        // Update wsindex to index in input workspace
        wsindex = i;
      }
      else
      {
        // Use the wsindex as the input
        wsindex = static_cast<size_t>(inpwsindex);
        if (wsindex >= inpWS->getNumberHistograms())
        {
          stringstream errmsg;
          errmsg << "Input workspace index " << inpwsindex << " is out of input workspace range = "
                 << inpWS->getNumberHistograms() << endl;
        }
      }

      // Find background
      const MantidVec& inpX = inpWS->readX(wsindex);
      const MantidVec& inpY = inpWS->readY(wsindex);
      const MantidVec& inpE = inpWS->readE(wsindex);

      MantidVec& vecX = outWS->dataX(i);
      MantidVec& vecY = outWS->dataY(i);
      MantidVec& vecE = outWS->dataE(i);

	  // save output of mask * Y
	  vecX[0] = inpX[l0];
	  vecX[1] = inpX[n-1];
	  vecY[0] = inpY[l0];
	  vecY[1] = inpY[n-1];
	  vecE[0] = inpE[l0];
	  vecE[1] = inpE[n-1];

      double Ymean, Yvariance, Ysigma;
      MantidVec maskedY;
      for (size_t l = l0; l < n; ++l)maskedY.push_back(inpY[l]);
      MantidVec mask(n-l0,0.0);
      double xn = static_cast<double>(n-l0);
      double k = 1.0;
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
			  if (peaks.size() > 0)
			  {
				  size_t ipeak = peaks.size()-1;
				  if (mask[l] != mask[l-1] && mask[l] == 0)
				  {
					  peaks[ipeak].stop = l+l0-1;
				  }
				  if (inpY[l+l0] > peaks[ipeak].maxY) peaks[ipeak].maxY = inpY[l+l0];
			  }
		  }
		  if(peaks.size()> 0)
		  {
			  if(peaks[peaks.size()-1].stop == 0) peaks[peaks.size()-1].stop = n-1;
			  std::sort(peaks.begin(), peaks.end(), by_len());

			  // save endpoints
			  vecX[0] = inpX[peaks[0].start];
			  // extra point for histogram input
			  vecX[1] = inpX[peaks[0].stop + sizex - sizey];
			  vecY[0] = inpY[peaks[0].start];
			  vecY[1] = inpY[peaks[0].stop];
			  vecE[0] = inpE[peaks[0].start];
			  vecE[1] = inpE[peaks[0].stop];
		  }
      }

	  prog.report();
	  //PARALLEL_END_INTERUPT_REGION
    } // ENDFOR
    //PARALLEL_CHECK_INTERUPT_REGION

    // 4. Set the output
    setProperty("OutputWorkspace", outWS);

    return;
  }
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
