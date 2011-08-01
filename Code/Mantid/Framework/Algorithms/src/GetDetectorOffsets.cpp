#include "MantidAlgorithms/GetDetectorOffsets.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/IBackgroundFunction.h"
#include "MantidAPI/CompositeFunctionMW.h"
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
          new WorkspaceUnitValidator<>("dSpacing")),"A 2D workspace with X values of d-spacing");

      BoundedValidator<double> *mustBePositive = new BoundedValidator<double>();
      mustBePositive->setLower(0);

      declareProperty("Step",0.001, mustBePositive,
        "Step size used to bin d-spacing data");
      declareProperty("DReference",2.0, mustBePositive->clone(),
         "Center of reference peak in d-space");
      declareProperty("XMin",0.0, "Minimum of CrossCorrelation data to search for peak, usually negative");
      declareProperty("XMax",0.0, "Maximum of CrossCorrelation data to search for peak, usually positive");

      declareProperty(new FileProperty("GroupingFileName","", FileProperty::OptionalSave, ".cal"),
          "Optional: The name of the output CalFile to save the generated OffsetsWorkspace." );
      declareProperty(new WorkspaceProperty<OffsetsWorkspace>("OutputWorkspace","",Direction::Output),
          "An output workspace containing the offsets.");
      // Only keep peaks
      std::vector<std::string> peakNames = FunctionFactory::Instance().getFunctionNames<IPeakFunction>();
      declareProperty("PeakFunction", "Gaussian", new ListValidator(peakNames));
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
      if (Xmin>=Xmax)
        throw std::runtime_error("Must specify Xmin<Xmax");
      dreference=getProperty("DReference");
      step=getProperty("Step");
      int nspec=static_cast<int>(inputW->getNumberHistograms());
      // Create the output OffsetsWorkspace
      OffsetsWorkspace_sptr outputW(new OffsetsWorkspace(inputW->getInstrument()));

      // Fit all the spectra with a gaussian
      Progress prog(this, 0, 1.0, nspec);
      PARALLEL_FOR1(inputW)
      for (int wi=0;wi<nspec;++wi)
      {
        PARALLEL_START_INTERUPT_REGION
        // Fit the peak
        double offset=fitSpectra(wi);

        // Get the list of detectors in this pixel
        const std::set<detid_t> & dets = inputW->getSpectrum(wi)->getDetectorIDs();

        // Most of the exec time is in FitSpectra, so this critical block should not be a problem.
        PARALLEL_CRITICAL(GetDetectorOffsets_setValue)
        {
          // Use the same offset for all detectors from this pixel
          std::set<detid_t>::iterator it;
          for (it = dets.begin(); it != dets.end(); it++)
          {
            outputW->setValue(*it, offset);
          }
        }
        prog.report();
        PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION

      // Return the output
      setProperty("OutputWorkspace",outputW);

      // Also save to .cal file, if requested
      std::string filename=getProperty("GroupingFileName");
      if (!filename.empty())
      {
        progress(0.9, "Saving .cal file");
        IAlgorithm_sptr childAlg = createSubAlgorithm("SaveCalFile");
        childAlg->setProperty("OffsetsWorkspace", outputW);
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
    double GetDetectorOffsets::fitSpectra(const int64_t s)
    {
      // Find point of peak centre
      const MantidVec & yValues = inputW->readY(s);
      MantidVec::const_iterator it = std::max_element(yValues.begin(), yValues.end());
      const double peakHeight = *it; 
      const double peakLoc = inputW->readX(s)[it - yValues.begin()];
      // Return offset of 0 if peak of Cross Correlation is nan (Happens when spectra is zero)
      if ( boost::math::isnan(peakHeight) ) return (0.);

      IAlgorithm_sptr fit_alg;
      try
      {
        //set the subalgorithm no to log as this will be run once per spectra
        fit_alg = createSubAlgorithm("Fit",-1,-1,false);
      } catch (Exception::NotFoundError&)
      {
        g_log.error("Can't locate Fit algorithm");
        throw ;
      }
      fit_alg->setProperty("InputWorkspace",inputW);
      fit_alg->setProperty<int>("WorkspaceIndex",static_cast<int>(s)); // TODO what is the right thing to do here?
      fit_alg->setProperty("StartX",Xmin);
      fit_alg->setProperty("EndX",Xmax);
      fit_alg->setProperty("MaxIterations",100);

      std::string fun_str = createFunctionString(peakHeight, peakLoc);
      
      fit_alg->setProperty("Function",fun_str);
      fit_alg->executeAsSubAlg();
      std::string fitStatus = fit_alg->getProperty("OutputStatus");
      if ( fitStatus.compare("success") ) return (0.);

      std::vector<double> params = fit_alg->getProperty("Parameters");
      double offset = params[3]; // f1.PeakCentre
      offset = -1.*offset*step/(dreference+offset*step);
      //factor := factor * (1+offset) for d-spacemap conversion so factor cannot be negative
      if (offset < -1.) offset = -1.;
      return offset;
    }

    /**
     * Create a function string from the given parameters and the algorithm inputs
     * @param peakHeight :: The height of the peak
     * @param peakLoc :: The location of the peak
     */
    std::string GetDetectorOffsets::createFunctionString(const double peakHeight, const double peakLoc) 
    {
      FunctionFactoryImpl & creator = FunctionFactory::Instance();
      IBackgroundFunction *background = 
        dynamic_cast<IBackgroundFunction*>(creator.createFunction("LinearBackground"));
      IPeakFunction *peak =
        dynamic_cast<IPeakFunction*>(creator.createFunction(getProperty("PeakFunction")));
      peak->setHeight(peakHeight);
      peak->setCentre(peakLoc);
      const double sigma(10.0);
      peak->setWidth(2.0*std::sqrt(2.0*std::log(2.0))*sigma);

      CompositeFunctionMW fitFunc; //Takes ownership of the functions
      fitFunc.addFunction(background);
      fitFunc.addFunction(peak);

      return fitFunc.asString();
    }



  } // namespace Algorithm
} // namespace Mantid
