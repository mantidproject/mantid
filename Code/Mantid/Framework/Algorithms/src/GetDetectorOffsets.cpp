/*WIKI* 

This algorithm requires a workspace that is both in d-spacing, but has also been preprocessed by the [[CrossCorrelate]] algorithm.  In this first step you select one spectrum to be the reference spectrum and all of the other spectrum are cross correlated against it.  Each output spectrum then contains a peak whose location defines the offset from the reference spectrum.

The algorithm iterates over each spectrum in the workspace and fits a [[Gaussian]] function to the reference peaks.  The fit is used to calculate the centre of the fitted peak, and the offset is then calculated as:

<math>-peakCentre*step/(dreference+PeakCentre*step)</math>

This is then written into a [[CalFile|.cal file]] for every detector that contributes to that spectrum.  All of the entries in the cal file are initially set to both be included, but also to all group into a single group on [[DiffractionFocussing]].  The [[CreateCalFileByNames]] algorithm can be used to alter the grouping in the cal file.

*WIKI*/
/*WIKI_USAGE*
'''Python'''

 GetDetOffsetsMultiPeaks("InputW","OutputW",0.01,2.0,1.8,2.2,"output.cal","offsets","mask")
*WIKI_USAGE*/
#include "MantidAlgorithms/GetDetectorOffsets.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/IBackgroundFunction.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include <fstream>
#include <iomanip>
#include <ostream>
#include <sstream>
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid
{
  namespace Algorithms
  {

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(GetDetectorOffsets)
    
    /// Sets documentation strings for this algorithm
    void GetDetectorOffsets::initDocs()
    {
      this->setWikiSummary("Creates an [[OffsetsWorkspace]] containing offsets for each detector. You can then save these to a .cal file using SaveCalFile.");
      this->setOptionalMessage("Creates an OffsetsWorkspace containing offsets for each detector. You can then save these to a .cal file using SaveCalFile.");
    }
    
    using namespace Kernel;
    using namespace API;
    using std::size_t;
    using namespace DataObjects;

    /// Constructor
    GetDetectorOffsets::GetDetectorOffsets() :
      API::Algorithm()
    {}

    /// Destructor
    GetDetectorOffsets::~GetDetectorOffsets()
    {}


    //-----------------------------------------------------------------------------------------
    /** Initialisation method. Declares properties to be used in algorithm.
     */
    void GetDetectorOffsets::init()
    {

      declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,
          boost::make_shared<WorkspaceUnitValidator>("dSpacing")),"A 2D workspace with X values of d-spacing");

      auto mustBePositive = boost::make_shared<BoundedValidator<double> >();
      mustBePositive->setLower(0);

      declareProperty("Step",0.001, mustBePositive,
        "Step size used to bin d-spacing data");
      declareProperty("DReference",2.0, mustBePositive,
         "Center of reference peak in d-space");
      declareProperty("XMin",0.0, "Minimum of CrossCorrelation data to search for peak, usually negative");
      declareProperty("XMax",0.0, "Maximum of CrossCorrelation data to search for peak, usually positive");

      declareProperty(new FileProperty("GroupingFileName","", FileProperty::OptionalSave, ".cal"),
          "Optional: The name of the output CalFile to save the generated OffsetsWorkspace." );
      declareProperty(new WorkspaceProperty<OffsetsWorkspace>("OutputWorkspace","",Direction::Output),
          "An output workspace containing the offsets.");
      declareProperty(new WorkspaceProperty<>("MaskWorkspace","Mask",Direction::Output),
          "An output workspace containing the mask.");
      // Only keep peaks
      std::vector<std::string> peakNames = FunctionFactory::Instance().getFunctionNames<IPeakFunction>();
      declareProperty("PeakFunction", "Gaussian", boost::make_shared<StringListValidator>(peakNames));
      declareProperty("MaxOffset", 1.0, "Maximum absolute value of offsets; default is 1");
    }

    //-----------------------------------------------------------------------------------------
    /** Executes the algorithm
     *
     *  @throw Exception::FileError If the grouping file cannot be opened or read successfully
     */
    void GetDetectorOffsets::exec()
    {
      inputW=getProperty("InputWorkspace");
      Xmin=getProperty("XMin");
      Xmax=getProperty("XMax");
      maxOffset=getProperty("MaxOffset");
      if (Xmin>=Xmax)
        throw std::runtime_error("Must specify Xmin<Xmax");
      dreference=getProperty("DReference");
      step=getProperty("Step");
      int nspec=static_cast<int>(inputW->getNumberHistograms());
      // Create the output OffsetsWorkspace
      OffsetsWorkspace_sptr outputW(new OffsetsWorkspace(inputW->getInstrument()));
      // Create the output MaskWorkspace
      MaskWorkspace_sptr maskWS(new MaskWorkspace(inputW->getInstrument()));
      //To get the workspace index from the detector ID
      const detid2index_map pixel_to_wi = maskWS->getDetectorIDToWorkspaceIndexMap(true);

      // Fit all the spectra with a gaussian
      Progress prog(this, 0, 1.0, nspec);
      PARALLEL_FOR1(inputW)
      for (int wi=0;wi<nspec;++wi)
      {
        PARALLEL_START_INTERUPT_REGION
        // Fit the peak
        double offset=fitSpectra(wi);
        double mask=0.0;
        if (std::abs(offset) > maxOffset)
        { 
          offset = 0.0;
          mask = 1.0;
        }

        // Get the list of detectors in this pixel
        const std::set<detid_t> & dets = inputW->getSpectrum(wi)->getDetectorIDs();

        // Most of the exec time is in FitSpectra, so this critical block should not be a problem.
        PARALLEL_CRITICAL(GetDetectorOffsets_setValue)
        {
          // Use the same offset for all detectors from this pixel
          std::set<detid_t>::iterator it;
          for (it = dets.begin(); it != dets.end(); ++it)
          {
            outputW->setValue(*it, offset);
            const auto mapEntry = pixel_to_wi.find(*it);
            if ( mapEntry == pixel_to_wi.end() ) continue;
            const size_t workspaceIndex = mapEntry->second;
            if (mask == 1.)
            {
              // Being masked
              maskWS->maskWorkspaceIndex(workspaceIndex);
              maskWS->dataY(workspaceIndex)[0] = mask;
            }
            else
            {
              // Using the detector
              maskWS->dataY(workspaceIndex)[0] = mask;
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
        IAlgorithm_sptr childAlg = createChildAlgorithm("SaveCalFile");
        childAlg->setProperty("OffsetsWorkspace", outputW);
        childAlg->setProperty("MaskWorkspace", maskWS);
        childAlg->setPropertyValue("Filename", filename);
        childAlg->executeAsChildAlg();
      }

    }


    //-----------------------------------------------------------------------------------------
   /** Calls Gaussian1D as a child algorithm to fit the offset peak in a spectrum
    *
    *  @param s :: The Workspace Index to fit
    *  @return The calculated offset value
    */
    double GetDetectorOffsets::fitSpectra(const int64_t s)
    {
      // Find point of peak centre
      const MantidVec & yValues = inputW->readY(s);
      MantidVec::const_iterator it = std::max_element(yValues.begin(), yValues.end());
      const double peakHeight = *it; 
      const double peakLoc = inputW->readX(s)[it - yValues.begin()];
      // Return if peak of Cross Correlation is nan (Happens when spectra is zero)
      //Pixel with large offset will be masked
      if ( boost::math::isnan(peakHeight) ) return (1000.);

      IAlgorithm_sptr fit_alg;
      try
      {
        //set the ChildAlgorithm no to log as this will be run once per spectra
        fit_alg = createChildAlgorithm("Fit",-1,-1,false);
      } catch (Exception::NotFoundError&)
      {
        g_log.error("Can't locate Fit algorithm");
        throw ;
      }
      auto fun = createFunction(peakHeight, peakLoc);
      fit_alg->setProperty("Function",fun);

      fit_alg->setProperty("InputWorkspace",inputW);
      fit_alg->setProperty<int>("WorkspaceIndex",static_cast<int>(s)); // TODO what is the right thing to do here?
      fit_alg->setProperty("StartX",Xmin);
      fit_alg->setProperty("EndX",Xmax);
      fit_alg->setProperty("MaxIterations",100);

      IFunction_sptr fun_ptr = createFunction(peakHeight, peakLoc);
      
      fit_alg->setProperty("Function",fun_ptr);
      fit_alg->executeAsChildAlg();
      std::string fitStatus = fit_alg->getProperty("OutputStatus");
      //Pixel with large offset will be masked
      if ( fitStatus.compare("success") ) return (1000.);

      //std::vector<double> params = fit_alg->getProperty("Parameters");
      API::IFunction_sptr function = fit_alg->getProperty("Function");
      double offset = function->getParameter(3);//params[3]; // f1.PeakCentre
      offset = -1.*offset*step/(dreference+offset*step);
      //factor := factor * (1+offset) for d-spacemap conversion so factor cannot be negative
      return offset;
    }

    /**
     * Create a function string from the given parameters and the algorithm inputs
     * @param peakHeight :: The height of the peak
     * @param peakLoc :: The location of the peak
     */
    IFunction_sptr GetDetectorOffsets::createFunction(const double peakHeight, const double peakLoc)
    {
      FunctionFactoryImpl & creator = FunctionFactory::Instance();
      auto background = creator.createFunction("LinearBackground");
      auto peak = 
        boost::dynamic_pointer_cast<IPeakFunction>(creator.createFunction(getProperty("PeakFunction")));
      peak->setHeight(peakHeight);
      peak->setCentre(peakLoc);
      const double sigma(10.0);
      peak->setFwhm(2.0*std::sqrt(2.0*std::log(2.0))*sigma);

      CompositeFunction* fitFunc = new CompositeFunction(); //Takes ownership of the functions
      fitFunc->addFunction(background);
      fitFunc->addFunction(peak);

      return boost::shared_ptr<IFunction>(fitFunc);
    }



  } // namespace Algorithm
} // namespace Mantid
