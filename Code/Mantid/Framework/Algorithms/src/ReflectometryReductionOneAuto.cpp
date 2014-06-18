#include "MantidAlgorithms/ReflectometryReductionOneAuto.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/RebinParamsValidator.h"

namespace Mantid
{
  namespace Algorithms
  {

    using namespace Mantid::Kernel;
    using namespace Mantid::API;

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(ReflectometryReductionOneAuto)



    //----------------------------------------------------------------------------------------------
    /** Constructor
    */
    ReflectometryReductionOneAuto::ReflectometryReductionOneAuto()
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
    */
    ReflectometryReductionOneAuto::~ReflectometryReductionOneAuto()
    {
    }


    //----------------------------------------------------------------------------------------------


    /// Algorithm's name for identification. @see Algorithm::name
    const std::string ReflectometryReductionOneAuto::name() const { return "ReflectometryReductionOneAuto";};

    /// Algorithm's version for identification. @see Algorithm::version
    int ReflectometryReductionOneAuto::version() const { return 2;};

    /// Algorithm's category for identification. @see Algorithm::category
    const std::string ReflectometryReductionOneAuto::category() const { return "Reflectometry\\ISIS";}

    /// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
    const std::string ReflectometryReductionOneAuto::summary() const { return "Reduces a single TOF reflectometry run into a mod Q vs I/I0 workspace. Performs transmission corrections.";};

    //----------------------------------------------------------------------------------------------
    /** Initialize the algorithm's properties.
    */
    void ReflectometryReductionOneAuto::init()
    {
      auto input_validator = boost::make_shared<WorkspaceUnitValidator>("TOF");
      declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace", "", Direction::Input, PropertyMode::Mandatory, input_validator), "Input run in TOF");

      std::vector<std::string> analysis_modes;
      analysis_modes.push_back("PointDetectorAnalysis");
      analysis_modes.push_back("MultiDetectorAnalysis");
      auto analysis_mode_validator = boost::make_shared<StringListValidator>(analysis_modes);

      declareProperty(new ArrayProperty<int>("RegionOfDirectBeam", Direction::Input), "Indices of the spectra a pair (lower, upper) that mark the ranges that correspond to the direct beam in multi-detector mode.");

      declareProperty("AnalysisMode", analysis_modes[0],  analysis_mode_validator, "Analysis Mode to Choose", Direction::Input);

      declareProperty(new WorkspaceProperty<MatrixWorkspace>("FirstTransmissionRun", "", Direction::Input, PropertyMode::Optional), "First transmission run workspace in TOF or Wavelength");
      declareProperty(new WorkspaceProperty<MatrixWorkspace>("SecondTransmissionRun", "", Direction::Input, PropertyMode::Optional,  input_validator), "Second transmission run workspace in TOF");
      declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "", Direction::Output), "Output workspace in wavelength q");
      declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspaceWavelength", "", Direction::Output), "Output workspace in wavelength");

      //declareProperty(ArrayProperty<double>("Params", values=[sys.maxint]), "Rebinning Parameters. See Rebin for format.");
      declareProperty(new ArrayProperty<double>("Params", boost::make_shared<RebinParamsValidator>(true)),"A comma separated list of first bin boundary, width, last bin boundary. "
        "These parameters are used for stitching together transmission runs. "
        "Values are in wavelength (angstroms). This input is only needed if a SecondTransmission run is provided.");

      declareProperty("StartOverlap", Mantid::EMPTY_DBL(), "Overlap in Q.", Direction::Input);

      declareProperty("EndOverlap", Mantid::EMPTY_DBL(), "End overlap in Q.", Direction::Input);

      auto index_bounds = boost::make_shared<BoundedValidator<int> >();
      index_bounds->setLower(0);

      declareProperty(new PropertyWithValue<int>("I0MonitorIndex", Mantid::EMPTY_INT(), index_bounds), "I0 monitor index");
      declareProperty(new PropertyWithValue<std::string>("ProcessingInstructions", "", Direction::Input), "Processing commands to select and add spectrum to make a detector workspace. See [[PeformIndexOperations]] for syntax.");
      declareProperty("WavelengthMin", Mantid::EMPTY_DBL(), "Wavelength Min in angstroms", Direction::Input);
      declareProperty("WavelengthMax", Mantid::EMPTY_DBL(), "Wavelength Max in angstroms", Direction::Input);
      declareProperty("WavelengthStep", Mantid::EMPTY_DBL(), "Wavelength step in angstroms", Direction::Input);
      declareProperty("MonitorBackgroundWavelengthMin", Mantid::EMPTY_DBL(), "Monitor wavelength background min in angstroms", Direction::Input);
      declareProperty("MonitorBackgroundWavelengthMax", Mantid::EMPTY_DBL(), "Monitor wavelength background max in angstroms", Direction::Input);
      declareProperty("MonitorIntegrationWavelengthMin", Mantid::EMPTY_DBL(), "Monitor integral min in angstroms", Direction::Input);
      declareProperty("MonitorIntegrationWavelengthMax", Mantid::EMPTY_DBL(), "Monitor integral max in angstroms", Direction::Input);

      declareProperty(new PropertyWithValue<std::string>("DetectorComponentName", "", Direction::Input), "Name of the detector component i.e. point-detector. If these are not specified, the algorithm will attempt lookup using a standard naming convention.");
      declareProperty(new PropertyWithValue<std::string>("SampleComponentName", "", Direction::Input), "Name of the sample component i.e. some-surface-holder. If these are not specified, the algorithm will attempt lookup using a standard naming convention.");

      declareProperty("ThetaIn", Mantid::EMPTY_DBL(), "Final theta in degrees", Direction::Input);
      declareProperty("ThetaOut", Mantid::EMPTY_DBL(), "Calculated final theta in degrees.", Direction::Output);

      declareProperty("CorrectDetectorPositions", true, "Correct detector positions using ThetaIn (if given)");

      declareProperty("StrictSpectrumChecking", true, "Strict checking between spectrum numbers in input workspaces and transmission workspaces.");

    }

    //----------------------------------------------------------------------------------------------
    /** Execute the algorithm.
    */
    void ReflectometryReductionOneAuto::exec()
    {
      // TODO Auto-generated execute stub
    }



  } // namespace Algorithms
} // namespace Mantid
