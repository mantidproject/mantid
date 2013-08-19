/*WIKI*

Separates background from signal for spectra of a workspace.

*WIKI*/

#include "MantidAlgorithms/SeparateBackgroundFromSignal.h"

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

  DECLARE_ALGORITHM(SeparateBackgroundFromSignal)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  SeparateBackgroundFromSignal::SeparateBackgroundFromSignal()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  SeparateBackgroundFromSignal::~SeparateBackgroundFromSignal()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** WIKI:
   */
  void SeparateBackgroundFromSignal::initDocs()
  {
    setWikiSummary("Separates background from signal for spectra of a workspace.");
    setOptionalMessage("Separates background from signal for spectra of a workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Define properties
    */
  void SeparateBackgroundFromSignal::init()
  {
    auto inwsprop = new WorkspaceProperty<MatrixWorkspace>("InputWorkspace", "Anonymous", Direction::Input);
    declareProperty(inwsprop, "Name of input MatrixWorkspace to have Z-score calculated.");

    auto outwsprop = new WorkspaceProperty<Workspace2D>("OutputWorkspace", "", Direction::Output);
    declareProperty(outwsprop, "Name of the output Workspace2D containing the Z-scores.");

    declareProperty("WorkspaceIndex", EMPTY_INT(), "Index of the spectrum to have Z-score calculated. "
                    "Default is to calculate for all spectra.");

  }
  
  //----------------------------------------------------------------------------------------------
  /** Execute body
    */
  void SeparateBackgroundFromSignal::exec()
  {
    // 1. Get input and validate
    MatrixWorkspace_const_sptr inpWS = getProperty("InputWorkspace");
    int inpwsindex = getProperty("WorkspaceIndex");

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

    Workspace2D_sptr outWS = boost::dynamic_pointer_cast<Workspace2D>(WorkspaceFactory::Instance().create(
          "Workspace2D", numspec, sizex, sizey));

    // 3. Get Z values
    for (size_t i = 0; i < numspec; ++i)
    {
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
      double Ymean, Yvariance, Ysigma;
      MantidVec maskedY = inpWS->readY(wsindex);
      size_t n = maskedY.size();
      MantidVec mask(n,0.0);
      double xn = static_cast<double>(n);
      double k = 1.0;
      do
      {
    	  Statistics stats = getStatistics(maskedY);
    	  Ymean = stats.mean;
    	  Yvariance = stats.standard_deviation * stats.standard_deviation;
    	  Ysigma = std::sqrt((moment(maskedY,n,Ymean,4)-(xn-3.0)/(xn-1.0) * moment(maskedY,n,Ymean,2))/xn);
	      MantidVec::const_iterator it = std::max_element(maskedY.begin(), maskedY.end());
	      const size_t pos = it - maskedY.begin();
		  maskedY[pos] = 0;
		  mask[pos] = 1.0;
      }
      while (std::abs(Ymean-Yvariance) > k * Ysigma);

      if(n > 5)
      {
    	  // remove single outliers
		  if (mask[1] == mask[2] && mask[2] == mask[3])
			  mask[0] = mask[1];
		  if (mask[0] == mask[2] && mask[2] == mask[3])
			  mask[1] = mask[2];
		  for (size_t l = 2; l < n-3; ++l)
		  {
			  if (mask[l-1] == mask[l+1] && (mask[l-1] == mask[l-2] || mask[l+1] == mask[l+2]))
			  {
				  mask[l] = mask[l+1];
			  }
		  }
		  if (mask[n-2] == mask[n-3] && mask[n-3] == mask[n-4])
			  mask[n-1] = mask[n-2];
		  if (mask[n-1] == mask[n-3] && mask[n-3] == mask[n-4])
			  mask[n-2] = mask[n-1];

		  // mask regions not connected to largest region
		  // for loop can start > 1 for multiple peaks
		  vector<cont_peak> peaks;
		  for (size_t l = 1; l < n; ++l)
		  {
			  if (mask[l] != mask[l-1] && mask[l] == 1)
			  {
				  peaks.push_back(cont_peak());
				  peaks[peaks.size()-1].start = l;
			  }
			  if (peaks.size() > 0)
			  {
				  size_t ipeak = peaks.size()-1;
				  if (mask[l] != mask[l-1] && mask[l] == 0)
				  {
					  peaks[ipeak].stop = l-1;
				  }
				  if (inpY[l] > peaks[ipeak].maxY) peaks[ipeak].maxY = inpY[l];
			  }
		  }
		  if(peaks.size()> 0)
		  {
			  if(peaks[peaks.size()-1].stop == 0) peaks[peaks.size()-1].stop = n-1;
			  std::sort(peaks.begin(), peaks.end(), by_len());
			  for (size_t l = 1; l < peaks.size(); ++l)
			  {
				  for (size_t j = peaks[l].start; j <= peaks[l].stop; ++j)
				  {
					  mask[j] = 0;
				  }
			  }
		  }
      }
	  // save output of mask * Y
      vecX = inpX;
      std::transform(mask.begin(), mask.end(), inpY.begin(), vecY.begin(), std::multiplies<double>());
      std::transform(mask.begin(), mask.end(), inpE.begin(), vecE.begin(), std::multiplies<double>());
    } // ENDFOR

    // 4. Set the output
    setProperty("OutputWorkspace", outWS);

    return;
  }
  double SeparateBackgroundFromSignal::moment(MantidVec& X, size_t n, double mean, int k)
  {
	  double sum=0.0;
	  for (size_t i = 0; i < n; ++i)
	  {
		  sum += std::pow(X[i]-mean, k);
	  }
	  sum /= static_cast<double>(n);
          return sum;
  }
} // namespace Algorithms
} // namespace Mantid
