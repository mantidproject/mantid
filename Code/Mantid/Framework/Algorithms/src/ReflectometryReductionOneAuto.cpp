#include "MantidAlgorithms/ReflectometryReductionOneAuto.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/RebinParamsValidator.h"
#include <boost/optional.hpp>

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
    int ReflectometryReductionOneAuto::version() const { return 1;};

    /// Algorithm's category for identification. @see Algorithm::category
    const std::string ReflectometryReductionOneAuto::category() const { return "Reflectometry\\ISIS";}

    /// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
    const std::string ReflectometryReductionOneAuto::summary() const { return "Reduces a single TOF/Lambda reflectometry run into a mod Q vs I/I0 workspace. Performs transmission corrections.";};

    //----------------------------------------------------------------------------------------------
    /** Initialize the algorithm's properties.
    */
    void ReflectometryReductionOneAuto::init()
    {
      declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace", "", Direction::Input, PropertyMode::Mandatory), "Input run in TOF or Lambda");

      std::vector<std::string> analysis_modes;
      analysis_modes.push_back("PointDetectorAnalysis");
      analysis_modes.push_back("MultiDetectorAnalysis");
      auto analysis_mode_validator = boost::make_shared<StringListValidator>(analysis_modes);

      declareProperty(new ArrayProperty<int>("RegionOfDirectBeam", Direction::Input), "Indices of the spectra a pair (lower, upper) that mark the ranges that correspond to the direct beam in multi-detector mode.");

      declareProperty("AnalysisMode", analysis_modes[0], analysis_mode_validator, "Analysis Mode to Choose", Direction::Input);

      declareProperty(new WorkspaceProperty<MatrixWorkspace>("FirstTransmissionRun", "", Direction::Input, PropertyMode::Optional), "First transmission run workspace in TOF or Wavelength");

      auto tof_validator = boost::make_shared<WorkspaceUnitValidator>("TOF");
      declareProperty(new WorkspaceProperty<MatrixWorkspace>("SecondTransmissionRun", "", Direction::Input, PropertyMode::Optional, tof_validator), "Second transmission run workspace in TOF");
      declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "", Direction::Output), "Output workspace in wavelength q");
      declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspaceWavelength", "", Direction::Output), "Output workspace in wavelength");

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
      MatrixWorkspace_sptr in_ws = getProperty("InputWorkspace");
      auto instrument = in_ws->getInstrument();

      //Get all the inputs.

      std::string output_workspace_name = getPropertyValue("OutputWorkspace");
      std::string output_workspace_lam_name = getPropertyValue("OutputWorkspaceWavelength");
      std::string analysis_mode = getPropertyValue("AnalysisMode");
      MatrixWorkspace_sptr first_ws = getProperty("FirstTransmissionRun");
      MatrixWorkspace_sptr second_ws = getProperty("SecondTransmissionRun");
      auto start_overlap = isSet<double>("StartOverlap");
      auto end_overlap = isSet<double>("EndOverlap");
      auto params = isSet<MantidVec>("Params");
      auto i0_monitor_index = static_cast<int>(checkForDefault("I0MonitorIndex", instrument, "I0MonitorIndex"));

      std::string processing_commands;
      if (this->getPointerToProperty("ProcessingInstructions")->isDefault())
      {
        if (analysis_mode == "PointDetectorAnalysis")
        {
          const int detStart = static_cast<int>(instrument->getNumberParameter("PointDetectorStart")[0]);
          const int detStop  = static_cast<int>(instrument->getNumberParameter("PointDetectorStop")[0]);

          if(detStart == detStop)
          {
            //If the range given only specifies one detector, we pass along just that one detector
            processing_commands = boost::lexical_cast<std::string>(detStart);
          }
          else
          {
            //Otherwise, we create a range.
            processing_commands = boost::lexical_cast<std::string>(detStart) + ":" + boost::lexical_cast<std::string>(detStop);
          }
        }
        else
        {
          processing_commands = boost::lexical_cast<std::string>(static_cast<int>(instrument->getNumberParameter("MultiDetectorStart")[0]))
            + ":" + boost::lexical_cast<std::string>(in_ws->getNumberHistograms() - 1);
        }
      }
      else
      {
        std::string processing_commands_temp = this->getProperty("ProcessingInstructions");
        processing_commands = processing_commands_temp;
      }

      double wavelength_min = checkForDefault("WavelengthMin", instrument, "LambdaMin");
      double wavelength_max = checkForDefault("WavelengthMax", instrument, "LambdaMax");
      auto wavelength_step = isSet<double>("WavelengthStep");
      double wavelength_back_min = checkForDefault("MonitorBackgroundWavelengthMin", instrument, "MonitorBackgroundMin");
      double wavelength_back_max = checkForDefault("MonitorBackgroundWavelengthMax", instrument, "MonitorBackgroundMax");
      double wavelength_integration_min = checkForDefault("MonitorIntegrationWavelengthMin", instrument, "MonitorIntegralMin");
      double wavelength_integration_max = checkForDefault("MonitorIntegrationWavelengthMax", instrument, "MonitorIntegralMax");

      auto detector_component_name = isSet<std::string>("DetectorComponentName");
      auto sample_component_name = isSet<std::string>("SampleComponentName");
      auto theta_in = isSet<double>("ThetaIn");
      auto region_of_direct_beam = isSet< std::vector<int> >("RegionOfDirectBeam");

      bool correct_positions = this->getProperty("CorrectDetectorPositions");
      bool strict_spectrum_checking = this->getProperty("StrictSpectrumChecking");

      //Pass the arguments and execute the main algorithm.

      IAlgorithm_sptr refRedOne = createChildAlgorithm("ReflectometryReductionOne");
      refRedOne->initialize();
      if (refRedOne->isInitialized())
      {
        refRedOne->setProperty("InputWorkspace",in_ws);
        refRedOne->setProperty("AnalysisMode",analysis_mode);
        refRedOne->setProperty("OutputWorkspace",output_workspace_name);
        refRedOne->setProperty("OutputWorkspaceWavelength",output_workspace_lam_name);
        refRedOne->setProperty("I0MonitorIndex",i0_monitor_index);
        refRedOne->setProperty("ProcessingInstructions",processing_commands);
        refRedOne->setProperty("WavelengthMin",wavelength_min);
        refRedOne->setProperty("WavelengthMax",wavelength_max);
        refRedOne->setProperty("MonitorBackgroundWavelengthMin",wavelength_back_min);
        refRedOne->setProperty("MonitorBackgroundWavelengthMax",wavelength_back_max);
        refRedOne->setProperty("MonitorIntegrationWavelengthMin",wavelength_integration_min);
        refRedOne->setProperty("MonitorIntegrationWavelengthMax",wavelength_integration_max);
        refRedOne->setProperty("CorrectDetectorPositions",correct_positions);
        refRedOne->setProperty("StrictSpectrumChecking",strict_spectrum_checking);

        if ( first_ws)
        {
          refRedOne->setProperty("FirstTransmissionRun",first_ws);
        }

        if ( second_ws)
        {
          refRedOne->setProperty("SecondTransmissionRun",second_ws);
        }

        if ( start_overlap.is_initialized())
        {
          refRedOne->setProperty("StartOverlap",start_overlap.get());
        }

        if ( end_overlap.is_initialized())
        {
          refRedOne->setProperty("EndOverlap",end_overlap.get());
        }

        if ( params.is_initialized())
        {
          refRedOne->setProperty("Params",params.get());
        }

        if ( wavelength_step.is_initialized())
        {
          refRedOne->setProperty("WavelengthStep",wavelength_step.get());
        }

        if ( region_of_direct_beam.is_initialized())
        {
          refRedOne->setProperty("RegionOfDirectBeam",region_of_direct_beam.get());
        }

        if ( detector_component_name.is_initialized())
        {
          refRedOne->setProperty("DetectorComponentName",detector_component_name.get());
        }

        if ( sample_component_name.is_initialized())
        {
          refRedOne->setProperty("SampleComponentName",sample_component_name.get());
        }

        if ( theta_in.is_initialized())
        {
          refRedOne->setProperty("ThetaIn",theta_in.get());
        }

        refRedOne->execute();
        if (!refRedOne->isExecuted())
        {
          throw std::runtime_error("ReflectometryReductionOne did not execute sucessfully");
        }
        else
        {
          MatrixWorkspace_sptr new_IvsQ1 = refRedOne->getProperty("OutputWorkspace");
          MatrixWorkspace_sptr new_IvsLam1 = refRedOne->getProperty("OutputWorkspaceWavelength");
          double thetaOut1 = refRedOne->getProperty("ThetaOut");
          setProperty("OutputWorkspace", new_IvsQ1);
          setProperty("OutputWorkspaceWavelength", new_IvsLam1);
          setProperty("ThetaOut", thetaOut1);
        }
      }
      else
      {
        throw std::runtime_error("ReflectometryReductionOne could not be initialised");
      }

    }

    template
      <typename T>
      boost::optional<T> ReflectometryReductionOneAuto::isSet(std::string propName) const
    {
      auto algProperty = this->getPointerToProperty(propName);
      if (algProperty->isDefault())
      {
        return boost::optional<T>();
      }
      else
      {
        T value = this->getProperty(propName);
        return boost::optional<T>(value);
      }
    }

    double ReflectometryReductionOneAuto::checkForDefault(std::string propName, Mantid::Geometry::Instrument_const_sptr instrument, std::string idf_name) const
    {
      auto algProperty = this->getPointerToProperty(propName);
      if (algProperty->isDefault())
      {
        auto defaults = instrument->getNumberParameter(idf_name);
        if (defaults.size() == 0)
        {
          throw std::runtime_error("No data could be retrieved from the parameters and argument wasn't provided: " + propName);
        }
        return defaults[0];
      }
      else
      {
        return boost::lexical_cast<double, std::string>(algProperty->value());
      }
    }

  } // namespace Algorithms
} // namespace Mantid
