/*WIKI* 

This algorithm requires a workspace that is both in d-spacing, but has also been preprocessed by the [[CrossCorrelate]] algorithm.  In this first step you select one spectrum to be the reference spectrum and all of the other spectrum are cross correlated against it.  Each output spectrum then contains a peak whose location defines the offset from the reference spectrum.

The algorithm iterates over each spectrum in the workspace and fits a [[Gaussian]] function to the reference peak.  The fit is used to calculate the centre of the fitted peak, and the offset is then calculated as:

This is then written into a [[CalFile|.cal file]] for every detector that contributes to that spectrum.  All of the entries in the cal file are initially set to both be included, but also to all group into a single group on [[DiffractionFocussing]].  The [[CreateCalFileByNames]] algorithm can be used to alter the grouping in the cal file.

== Usage ==
'''Python'''

GetDetOffsetsMultiPeaks("InputW","OutputW",0.01,2.0,1.8,2.2,"output.cal")


*WIKI*/
#include "MantidAlgorithms/GetDetOffsetsMultiPeaks.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/IBackgroundFunction.h"
#include "MantidAPI/CompositeFunctionMW.h"
#include "MantidKernel/VectorHelper.h"
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


    //-----------------------------------------------------------------------------------------
    /** Initialisation method. Declares properties to be used in algorithm.
     */
    void GetDetOffsetsMultiPeaks::init()
    {

      declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,
          new WorkspaceUnitValidator<>("dSpacing")),"A 2D workspace with X values of d-spacing");

      declareProperty("DReference","2.0","Optional: enter a comma-separated list of the expected X-position of the centre of the peaks. Only peaks near these positions will be fitted." );

      declareProperty(new FileProperty("GroupingFileName","", FileProperty::OptionalSave, ".cal"),
          "Optional: The name of the output CalFile to save the generated OffsetsWorkspace." );
      declareProperty(new WorkspaceProperty<OffsetsWorkspace>("OutputWorkspace","",Direction::Output),
          "An output workspace containing the offsets.");
      declareProperty(new WorkspaceProperty<>("MaskWorkspace","Mask",Direction::Output),
          "An output workspace containing the mask.");
      declareProperty("MaxOffset", 1.0, "Maximum absolute value of offsets; default is 1");
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

      // Fit all the spectra with a gaussian
      Progress prog(this, 0, 1.0, nspec);
      PARALLEL_FOR1(inputW)
      for (int wi=0;wi<nspec;++wi)
      {
        PARALLEL_START_INTERUPT_REGION
        // Fit the peak
        double offset=fitSpectra(wi);
        double mask=1.0;
        if (std::abs(offset) > maxOffset)
        { 
          offset = 0.0;
          mask = 0.0;
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
            if (mask == 0.) maskWS->maskWorkspaceIndex((*pixel_to_wi)[*it]);
            else maskWS->dataY((*pixel_to_wi)[*it])[0] = mask;
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
    double GetDetOffsetsMultiPeaks::fitSpectra(const int64_t s)
    {
       g_log.information("Calling FindPeaks as a sub-algorithm");
       double offset = 1000.0;

       API::IAlgorithm_sptr findpeaks = createSubAlgorithm("FindPeaks",0.0,0.2);
       findpeaks->setProperty("InputWorkspace", inputW);
       findpeaks->setProperty<int>("FWHM",7);
       findpeaks->setProperty<int>("Tolerance",4);
       // FindPeaks will do the checking on the validity of WorkspaceIndex
       findpeaks->setProperty("WorkspaceIndex",static_cast<int>(s));
  
      //Get the specified peak positions, which is optional
      std::string peakPositions = getProperty("DReference");
      findpeaks->setProperty("PeakPositions", peakPositions);
      findpeaks->setProperty<std::string>("BackgroundType", "Linear");
      findpeaks->setProperty<bool>("HighBackground", true);
      findpeaks->executeAsSubAlg();
      ITableWorkspace_sptr peakslist = findpeaks->getProperty("PeaksList");
      std::vector<double> peakPos = Kernel::VectorHelper::splitStringIntoVector<double>(peakPositions);
      double errsumold = 1000.0;
      for (int i = 0; i < peakslist->rowCount(); ++i)
      {
        double errsum = 0.0;
        // Get references to the data
        const double centre = peakslist->getRef<double>("centre",i);
        double tryoffset = 0;
        if(centre > 0) tryoffset = peakPos[i]-centre;
        for (int j = 0; j < peakslist->rowCount(); ++j)
        {
          const double centrej = peakslist->getRef<double>("centre",j);
          if(centrej > 0) errsum += std::fabs(peakPos[j]-(centrej+tryoffset));
        }
        if (errsum < errsumold)
        {
          //See formula in AlignDetectors
          offset = tryoffset/(peakPos[i]-tryoffset);
          errsumold = errsum;
        }
      }
      return offset;
    }


  } // namespace Algorithm
} // namespace Mantid
