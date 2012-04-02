/*WIKI*

This algorithm requires a workspace that is both in d-spacing, but has also been preprocessed by the [[CrossCorrelate]] algorithm.  In this first step you select one spectrum to be the reference spectrum and all of the other spectrum are cross correlated against it.  Each output spectrum then contains a peak whose location defines the offset from the reference spectrum.

The algorithm iterates over each spectrum in the workspace and fits a [[Gaussian]] function to the reference peak.  The fit is used to calculate the centre of the fitted peak, and the offset is then calculated as:

This is then written into a [[CalFile|.cal file]] for every detector that contributes to that spectrum.  All of the entries in the cal file are initially set to both be included, but also to all group into a single group on [[DiffractionFocussing]].  The [[CreateCalFileByNames]] algorithm can be used to alter the grouping in the cal file.

== Usage ==
'''Python'''

GetDetOffsetsMultiPeaks("InputW","OutputW",0.01,2.0,1.8,2.2,"output.cal")


*WIKI*/
#include "MantidAlgorithms/GetDetOffsetsMultiPeaks.h"
#include "MantidAlgorithms/GSLFunctions.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/IBackgroundFunction.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include <fstream>
#include <iomanip>
#include <ostream>
#include <sstream>

namespace Mantid
{
  namespace Algorithms
  {

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(GetDetOffsetsMultiPeaks)
    
    /// Sets documentation strings for this algorithm
    void GetDetOffsetsMultiPeaks::initDocs()
    {
      this->setWikiSummary("Creates an [[OffsetsWorkspace]] containing offsets for each detector. You can then save these to a .cal file using SaveCalFile.");
      this->setOptionalMessage("Creates an OffsetsWorkspace containing offsets for each detector. You can then save these to a .cal file using SaveCalFile.");
    }
    
    using namespace Kernel;
    using namespace API;
    using std::size_t;
    using namespace DataObjects;

    /// Constructor
    GetDetOffsetsMultiPeaks::GetDetOffsetsMultiPeaks() :
      API::Algorithm()
    {}

    /// Destructor
    GetDetOffsetsMultiPeaks::~GetDetOffsetsMultiPeaks()
    {}
  
    static double gsl_costFunction(const gsl_vector *v, void *params)
    {
      double *p = (double *)params;
      size_t n = static_cast<size_t>(p[0]);
      std::vector<double> peakPosToFit(n);
      std::vector<double> peakPosFitted(n);
      std::vector<double> chisq(n);
      double minD = p[1];
      double maxD = p[2];
      for (size_t i = 0; i < n; i++)
      {
        peakPosToFit[i] = p[i+3];
      }
      for (size_t i = 0; i < n; i++)
      {
        peakPosFitted[i] = p[i+n+3];
      }
      for (size_t i = 0; i < n; i++)
      {
        chisq[i] = p[i+2*n+3];
      }
    
      double offset = gsl_vector_get(v, 0);
      double errsum = 0.0;
      for (size_t i = 0; i < n; ++i)
      {
        // Get references to the data
        //See formula in AlignDetectors
        double offsetAD = offset*peakPosToFit[i]/(1.+offset);
        if(peakPosFitted[i] > minD && peakPosFitted[i] < maxD)
          errsum += std::fabs(peakPosToFit[i]-(peakPosFitted[i]+offsetAD))/chisq[i];
      }
      return errsum;
    }
  
    //-----------------------------------------------------------------------------------------
    /** Initialisation method. Declares properties to be used in algorithm.
     */
    void GetDetOffsetsMultiPeaks::init()
    {

      declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,
          boost::make_shared<WorkspaceUnitValidator>("dSpacing")),"A 2D workspace with X values of d-spacing");

      declareProperty(new ArrayProperty<double>("DReference"),"Enter a comma-separated list of the expected X-position of the centre of the peaks. Only peaks near these positions will be fitted." );
      declareProperty("FitWindowMaxWidth", 0.,
        "Optional: The maximum width of the fitting window. If this is <=0 the windows is not specified to FindPeaks" );

      std::vector<std::string> peaktypes;
      peaktypes.push_back("BackToBackExponential");
      peaktypes.push_back("Gaussian");
      peaktypes.push_back("Lorentzian");
      declareProperty("PeakFunction", "Gaussian", boost::make_shared<StringListValidator>(peaktypes),
                      "Type of peak to fit");

      std::vector<std::string> bkgdtypes;
      bkgdtypes.push_back("Flat");
      bkgdtypes.push_back("Linear");
      bkgdtypes.push_back("Quadratic");
      declareProperty("BackgroundType", "Linear", boost::make_shared<StringListValidator>(bkgdtypes),
          "Type of Background. The choice can be either Linear or Quadratic");

      declareProperty("HighBackground", true,
          "Relatively weak peak in high background");
      declareProperty(new FileProperty("GroupingFileName","", FileProperty::OptionalSave, ".cal"),
          "Optional: The name of the output CalFile to save the generated OffsetsWorkspace." );
      declareProperty(new WorkspaceProperty<OffsetsWorkspace>("OutputWorkspace","",Direction::Output),
          "An output workspace containing the offsets.");
      declareProperty(new WorkspaceProperty<>("MaskWorkspace","Mask",Direction::Output),
          "An output workspace containing the mask.");
      declareProperty("MaxOffset", 1.0, "Maximum absolute value of offsets; default is 1");
      declareProperty("MaxChiSq", 100., "Maximum chisq value for individual peak fit allowed. (Default: 100)");
      //Disable default gsl error handler (which is to call abort!)
      gsl_set_error_handler_off();
    }

    /**
     * The windows should be half of the distance between the peaks of maxWidth, whichever is smaller.
     * @param dmin The minimum d-spacing for the workspace
     * @param dmax The maximum d-spacing for the workspace
     * @param peaks The list of peaks to generate windows for
     * @param maxWidth The maximum width of a window
     * @return The list of windows for each peak
     */
    std::vector<double> generateWindows(const double dmin, const double dmax,
                                        const std::vector<double> &peaks, const double maxWidth)
    {
      if (maxWidth <= 0.)
      {
        return std::vector<double>(); // empty vector because this is turned off
      }

      std::size_t numPeaks = peaks.size();
      std::vector<double> windows(2*numPeaks);
      double widthLeft;
      double widthRight;
      for (std::size_t i = 0; i < numPeaks; i++)
      {
        if (i == 0)
          widthLeft = peaks[i] - dmin;
        else
          widthLeft = .5 * (peaks[i] - peaks[i-1]);
        if (i + 1 == numPeaks)
          widthRight = dmax - peaks[i];
        else
          widthRight = .5 * (peaks[i+1] - peaks[i]);

        widthLeft  = std::min(widthLeft, maxWidth);
        widthRight = std::min(widthRight, maxWidth);

        windows[2*i]   = peaks[i] - widthLeft;
        windows[2*i+1] = peaks[i] + widthRight;
      }

      return windows;
    }

    //-----------------------------------------------------------------------------------------
    /** Executes the algorithm
     *
     *  @throw Exception::FileError If the grouping file cannot be opened or read successfully
     */
    void GetDetOffsetsMultiPeaks::exec()
    {
      const double BAD_OFFSET(1000.); // mark things that didn't work with this

      MatrixWorkspace_sptr inputW=getProperty("InputWorkspace");
      double maxOffset=getProperty("MaxOffset");
      int nspec=static_cast<int>(inputW->getNumberHistograms());
      // Create the output OffsetsWorkspace
      OffsetsWorkspace_sptr outputW(new OffsetsWorkspace(inputW->getInstrument()));
      // determine min/max d-spacing of the workspace
      double wkspDmin, wkspDmax;
      inputW->getXMinMax(wkspDmin, wkspDmax);
      // Create the output MaskWorkspace
      MatrixWorkspace_sptr maskWS(new SpecialWorkspace2D(inputW->getInstrument()));
      //To get the workspace index from the detector ID
      detid2index_map * pixel_to_wi = maskWS->getDetectorIDToWorkspaceIndexMap(true);
      // the peak positions and where to fit
      std::vector<double> peakPositions = getProperty("DReference");
      std::vector<double> fitWindows = generateWindows(wkspDmin, wkspDmax, peakPositions, this->getProperty("FitWindowMaxWidth"));
      g_log.information() << "windows : ";
      if (fitWindows.empty())
      {
        g_log.information() << "empty\n";
      }
      else
      {
        for (std::vector<double>::const_iterator it = fitWindows.begin(); it != fitWindows.end(); ++it)
          g_log.information() << *it << " ";
        g_log.information() << "\n";
      }

      // some shortcuts for event workspaces
      EventWorkspace_const_sptr eventW = boost::dynamic_pointer_cast<const EventWorkspace>( inputW );
      bool isEvent = false;
      if (eventW)
        isEvent = true;

      // cache the peak and background function names
      m_peakType = this->getPropertyValue("PeakFunction");
      m_backType = this->getPropertyValue("BackgroundType");
      // the maximum allowable chisq value for an individual peak fit
      m_maxChiSq = this->getProperty("MaxChiSq");

      // Fit all the spectra with a gaussian
      Progress prog(this, 0, 1.0, nspec);
      // cppcheck-suppress syntaxError
      PRAGMA_OMP(parallel for schedule(dynamic, 1) )
      for (int wi=0;wi<nspec;++wi)
      {
        PARALLEL_START_INTERUPT_REGION
        double offset = 0.0;
        double fitSum = 0.0;
        // checks for dead detectors
        if ((isEvent) && (eventW->getEventList(wi).empty()))
        {
          // dead detector will be masked
          offset = BAD_OFFSET;
        }
        else {
          const MantidVec& Y = inputW->readY(wi);
          const int YLength = static_cast<int>(Y.size());
          double sumY = 0.0;
          for (int i = 0; i < YLength; i++) sumY += Y[i];
          if (sumY < 1.e-30)
          {
            // Dead detector will be masked
            offset=BAD_OFFSET;
          }
        }
        if (offset < 10.)
        {
          // Fit the peak
          std::vector<double> peakPosToFit, peakPosFitted, chisq;
          size_t nparams;
          double minD, maxD;
          fitSpectra(wi, inputW, peakPositions, fitWindows, nparams, minD, maxD, peakPosToFit, peakPosFitted, chisq);
          if (nparams > 0)
          {
          //double * params = new double[2*nparams+1];
          double params[153];
          if(nparams > 50) nparams = 50;
          params[0] = static_cast<double>(nparams);
          params[1] = minD;
          params[2] = maxD;
          for (size_t i = 0; i < nparams; i++)
          {
            params[i+3] = peakPosToFit[i];
          }
          for (size_t i = 0; i < nparams; i++)
          {
            params[i+3+nparams] = peakPosFitted[i];
          }
          for (size_t i = 0; i < nparams; i++)
          {
            params[i+3+2*nparams] = chisq[i];
          }
    
          const gsl_multimin_fminimizer_type *T =
          gsl_multimin_fminimizer_nmsimplex;
          gsl_multimin_fminimizer *s = NULL;
          gsl_vector *ss, *x;
          gsl_multimin_function minex_func;
    
          // finally do the fitting
    
          size_t nopt = 1;
          size_t iter = 0;
          int status = 0;
          double size;
     
          /* Starting point */
          x = gsl_vector_alloc (nopt);
          gsl_vector_set_all (x, 0.0);
    
          /* Set initial step sizes to 0.001 */
          ss = gsl_vector_alloc (nopt);
          gsl_vector_set_all (ss, 0.001);
    
          /* Initialize method and iterate */
          minex_func.n = nopt;
          minex_func.f = &Mantid::Algorithms::gsl_costFunction;
          minex_func.params = &params;
    
          s = gsl_multimin_fminimizer_alloc (T, nopt);
          gsl_multimin_fminimizer_set (s, &minex_func, x, ss);
    
          do
          {
            iter++;
            status = gsl_multimin_fminimizer_iterate(s);
            if (status)
              break;
    
            size = gsl_multimin_fminimizer_size (s);
            status = gsl_multimin_test_size (size, 1e-4);
    
          }
          while (status == GSL_CONTINUE && iter < 50);
    
          // Output summary to log file
          std::string reportOfDiffractionEventCalibrateDetectors = gsl_strerror(status);
          g_log.debug() << " Workspace Index = " << wi << 
            " Method used = " << " Simplex" << 
            " Iteration = " << iter << 
            " Status = " << reportOfDiffractionEventCalibrateDetectors << 
            " Minimize Sum = " << s->fval << 
            " Offset   = " << gsl_vector_get (s->x, 0) << "  \n";
          offset = gsl_vector_get (s->x, 0);
          fitSum = s->fval;
          gsl_vector_free(x);
          gsl_vector_free(ss);
          gsl_multimin_fminimizer_free (s);
          //delete [] params;
          }
          else
          {
              offset = BAD_OFFSET;
          }
        }
        double mask=0.0;
        if (std::abs(offset) > maxOffset)
        { 
          offset = 0.0;
          mask = 1.0;
        }

        // Get the list of detectors in this pixel
        const std::set<detid_t> & dets = inputW->getSpectrum(wi)->getDetectorIDs();

        // Most of the exec time is in FitSpectra, so this critical block should not be a problem.
        PARALLEL_CRITICAL(GetDetOffsetsMultiPeaks_setValue)
        {
          // Use the same offset for all detectors from this pixel
          std::set<detid_t>::iterator it;
          for (it = dets.begin(); it != dets.end(); ++it)
          {
            outputW->setValue(*it, offset, fitSum);
            if (mask == 1.)
            {
              // Being masked
              maskWS->maskWorkspaceIndex((*pixel_to_wi)[*it]);
              maskWS->dataY((*pixel_to_wi)[*it])[0] = mask;
            }
            else
            {
              // Using the detector
               maskWS->dataY((*pixel_to_wi)[*it])[0] = mask;
            }
          }
        }
        prog.report();
        PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION

      // Return the output
      setProperty("OutputWorkspace",outputW);
      setProperty("MaskWorkspace",maskWS);

      // Also save to .cal file, if requested
      std::string filename=getProperty("GroupingFileName");
      if (!filename.empty())
      {
        progress(0.9, "Saving .cal file");
        IAlgorithm_sptr childAlg = createSubAlgorithm("SaveCalFile");
        childAlg->setProperty("OffsetsWorkspace", outputW);
        childAlg->setProperty("MaskWorkspace", maskWS);
        childAlg->setPropertyValue("Filename", filename);
        childAlg->executeAsSubAlg();
      }

    }


    //-----------------------------------------------------------------------------------------
   /** Calls Gaussian1D as a child algorithm to fit the offset peak in a spectrum
    *
    *  @param s :: The Workspace Index to fit
    *  @return The calculated offset value
    */

    void GetDetOffsetsMultiPeaks::fitSpectra(const int64_t s, MatrixWorkspace_sptr inputW, const std::vector<double> &peakPositions,
                                             const std::vector<double> &fitWindows, size_t &nparams, double &minD, double &maxD,
                                             std::vector<double>&peakPosToFit, std::vector<double>&peakPosFitted,
                                             std::vector<double> &chisq)
    {
      const MantidVec & X = inputW->readX(s);
      minD = X.front();
      maxD = X.back();
      bool useFitWindows = (!fitWindows.empty());
      std::vector<double> fitWindowsToUse;
      for (int i = 0; i < static_cast<int>(peakPositions.size()); ++i)
      {
        if((peakPositions[i] > minD) && (peakPositions[i] < maxD))
        {
          peakPosToFit.push_back(peakPositions[i]);
          if (useFitWindows)
          {
            fitWindowsToUse.push_back(std::max(fitWindows[2*i], minD));
            fitWindowsToUse.push_back(std::min(fitWindows[2*i+1], maxD));
          }
        }
      }

      API::IAlgorithm_sptr findpeaks = createSubAlgorithm("FindPeaks", -1, -1, false);
      findpeaks->setProperty("InputWorkspace", inputW);
      findpeaks->setProperty<int>("FWHM",7);
      findpeaks->setProperty<int>("Tolerance",4);
      // FindPeaks will do the checking on the validity of WorkspaceIndex
      findpeaks->setProperty("WorkspaceIndex",static_cast<int>(s));
  
      //Get the specified peak positions, which is optional
      findpeaks->setProperty("PeakPositions", peakPosToFit);
      if (useFitWindows)
        findpeaks->setProperty("FitWindows", fitWindowsToUse);
      findpeaks->setProperty<std::string>("PeakFunction", m_peakType);
      findpeaks->setProperty<std::string>("BackgroundType", m_backType);
      findpeaks->setProperty<bool>("HighBackground", this->getProperty("HighBackground"));
      findpeaks->setProperty<int>("MinGuessedPeakWidth",4);
      findpeaks->setProperty<int>("MaxGuessedPeakWidth",4);
      findpeaks->executeAsSubAlg();
      ITableWorkspace_sptr peakslist = findpeaks->getProperty("PeaksList");
      std::vector<size_t> banned;
      for (size_t i = 0; i < peakslist->rowCount(); ++i)
      {
        double centre = peakslist->getRef<double>("centre",i);
        if (centre <= minD || centre >= maxD)
        {
            banned.push_back(i);
            continue;
        }
        double chi2 = peakslist->getRef<double>("chi2",i);
        if (chi2 > m_maxChiSq)
        {
          banned.push_back(i);
          continue;
        }

        // Get references to the data
        peakPosFitted.push_back(centre);
        chisq.push_back(chi2);
      }
      // delete banned peaks
      for (std::vector<size_t>::const_reverse_iterator it = banned.rbegin(); it != banned.rend(); ++it)
          peakPosToFit.erase(peakPosToFit.begin() + (*it));
      nparams = peakPosFitted.size();
      return;
    }

  } // namespace Algorithm
} // namespace Mantid
