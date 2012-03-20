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
#include "MantidAPI/CompositeFunctionMW.h"
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
      std::vector<double> peakPosToFit, peakPosFitted;
      size_t n = static_cast<size_t>(p[0]);
      double minD = p[1];
      double maxD = p[2];
      for (size_t i = 0; i < n; i++)
      {
        peakPosToFit.push_back(p[i+3]);
      }
      for (size_t i = 0; i < n; i++)
      {
        peakPosFitted.push_back(p[i+n+3]);
      }
    
      double offset = gsl_vector_get(v, 0);
      double errsum = 0.0;
      for (size_t i = 0; i < n; ++i)
      {
        // Get references to the data
        //See formula in AlignDetectors
        double offsetAD = offset*peakPosToFit[i]/(1.+offset);
        if(peakPosFitted[i] > minD && peakPosFitted[i] < maxD)
          errsum += std::fabs(peakPosToFit[i]-(peakPosFitted[i]+offsetAD));
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
      //Disable default gsl error handler (which is to call abort!)
      gsl_set_error_handler_off();
    }

    //-----------------------------------------------------------------------------------------
    /** Executes the algorithm
     *
     *  @throw Exception::FileError If the grouping file cannot be opened or read successfully
     */
    void GetDetOffsetsMultiPeaks::exec()
    {
      inputW=getProperty("InputWorkspace");
      maxOffset=getProperty("MaxOffset");
      int nspec=static_cast<int>(inputW->getNumberHistograms());
      // Create the output OffsetsWorkspace
      OffsetsWorkspace_sptr outputW(new OffsetsWorkspace(inputW->getInstrument()));
      // Create the output MaskWorkspace
      MatrixWorkspace_sptr maskWS(new SpecialWorkspace2D(inputW->getInstrument()));
      //To get the workspace index from the detector ID
      detid2index_map * pixel_to_wi = maskWS->getDetectorIDToWorkspaceIndexMap(true);

      // some shortcuts for event workspaces
      EventWorkspace_const_sptr eventW = boost::dynamic_pointer_cast<const EventWorkspace>( inputW );
      bool isEvent = false;
      if (eventW)
        isEvent = true;

      // Fit all the spectra with a gaussian
      Progress prog(this, 0, 1.0, nspec);
      // cppcheck-suppress syntaxError
      PRAGMA_OMP(parallel for schedule(dynamic, 1) )
      for (int wi=0;wi<nspec;++wi)
      {
        PARALLEL_START_INTERUPT_REGION
        double offset = 0.0;
        // checks for dead detectors
        if ((isEvent) && (eventW->getEventList(wi).empty()))
        {
          // dead detector will be masked
          offset = 1000.;
        }
        else {
          const MantidVec& Y = inputW->readY(wi);
          const int YLength = static_cast<int>(Y.size());
          double sumY = 0.0;
          for (int i = 0; i < YLength; i++) sumY += Y[i];
          if (sumY < 1.e-30)
          {
            // Dead detector will be masked
            offset=1000.;
          }
        }
        if (offset < 10.)
        {
          // Fit the peak
          MatrixWorkspace_sptr inputW = getProperty("InputWorkspace");
          std::string peakPositions = getProperty("DReference");
          std::vector<double> peakPosToFit, peakPosFitted;
          size_t nparams;
          double minD, maxD;
          fitSpectra(wi, inputW, peakPositions, nparams, minD, maxD, peakPosToFit, peakPosFitted);
          if (nparams > 0)
          {
          //double * params = new double[2*nparams+1];
          double params[103];
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
          gsl_vector_free(x);
          gsl_vector_free(ss);
          gsl_multimin_fminimizer_free (s);
          //delete [] params;
          }
          else
          {
              offset = 1000.;
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
            outputW->setValue(*it, offset);
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

    void GetDetOffsetsMultiPeaks::fitSpectra(const int64_t s, MatrixWorkspace_sptr inputW, const std::string &peakPositions, size_t &nparams, double &minD, double &maxD, std::vector<double>&peakPosToFit, std::vector<double>&peakPosFitted)
    {
      const MantidVec & X = inputW->readX(s);
      minD = X.front();
      maxD = X.back();
      std::vector<double> peakPos = Kernel::VectorHelper::splitStringIntoVector<double>(peakPositions);
      for (int i = 0; i < static_cast<int>(peakPos.size()); ++i)
      {
        if((peakPos[i] > minD) && (peakPos[i] < maxD))
          peakPosToFit.push_back(peakPos[i]);
      }

      API::IAlgorithm_sptr findpeaks = createSubAlgorithm("FindPeaks",0.0,0.2, false);
      findpeaks->setProperty("InputWorkspace", inputW);
      findpeaks->setProperty<int>("FWHM",7);
      findpeaks->setProperty<int>("Tolerance",4);
      // FindPeaks will do the checking on the validity of WorkspaceIndex
      findpeaks->setProperty("WorkspaceIndex",static_cast<int>(s));
  
      //Get the specified peak positions, which is optional
      findpeaks->setProperty("PeakPositions", peakPosToFit);
      findpeaks->setProperty<std::string>("BackgroundType", this->getPropertyValue("BackgroundType"));
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
        // Get references to the data
        peakPosFitted.push_back(centre);
      }
      // delete banned peaks
      for (std::vector<size_t>::const_reverse_iterator it = banned.rbegin(); it != banned.rend(); ++it)
          peakPosToFit.erase(peakPosToFit.begin() + (*it));
      nparams = peakPosFitted.size();
      peakPos.clear();
      return;
    }

  } // namespace Algorithm
} // namespace Mantid
