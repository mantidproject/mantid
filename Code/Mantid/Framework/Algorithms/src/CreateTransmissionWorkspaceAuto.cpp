/*WIKI*
 Facade over [[CreateTransmissionWorkspace]]. Pull numeric parameters out of the instrument parameters where possible. You can override any of these automatically
 applied defaults by providing your own value for the input.

 See [[CreateTransmissionWorkspace]] for more information on the wrapped algorithm.
 *WIKI*/

#include "MantidAlgorithms/CreateTransmissionWorkspaceAuto.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidAPI/AlgorithmManager.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
  namespace Algorithms
  {

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(CreateTransmissionWorkspaceAuto)

    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    CreateTransmissionWorkspaceAuto::CreateTransmissionWorkspaceAuto()
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
     */
    CreateTransmissionWorkspaceAuto::~CreateTransmissionWorkspaceAuto()
    {
    }

    //----------------------------------------------------------------------------------------------
    /// Sets documentation strings for this algorithm
    const std::string CreateTransmissionWorkspaceAuto::summary() const
    {
      return "Creates a transmission run workspace in Wavelength from input TOF workspaces.";
    }

    //----------------------------------------------------------------------------------------------
    /** Initialize the algorithm's properties.
     */
    void CreateTransmissionWorkspaceAuto::init()
    {

      std::vector<std::string> analysis_modes;
      analysis_modes.push_back("PointDetectorAnalysis");
      analysis_modes.push_back("MultiDetectorAnalysis");
      declareProperty("AnalysisMode", analysis_modes.at(0),
          boost::make_shared<StringListValidator>(analysis_modes), "Analysis Mode to Choose",
          Direction::Input);

      declareProperty(
          new WorkspaceProperty<MatrixWorkspace>("FirstTransmissionRun", "", Direction::Input,
              boost::make_shared<WorkspaceUnitValidator>("TOF")), "Input workspace.");
      declareProperty(
          new WorkspaceProperty<MatrixWorkspace>("SecondTransmissionRun", "", Direction::Input,
              PropertyMode::Optional, boost::make_shared<WorkspaceUnitValidator>("TOF")),
          "Second transmission run workspace in TOF.");
      declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "", Direction::Output),
          "Output transmission workspace in wavelength.");

      declareProperty(
          new ArrayProperty<double>("Params", boost::make_shared<RebinParamsValidator>(true)),
          "A comma separated list of first bin boundary, width, last bin boundary. "
              "These parameters are used for stitching together transmission runs. "
              "Values are in wavelength (angstroms). This input is only needed if a SecondTransmission run is provided.");

      declareProperty(
          new PropertyWithValue<double>("StartOverlap", Mantid::EMPTY_DBL(), Direction::Input),
          "Start wavelength for stitching transmission runs together");

      declareProperty(new PropertyWithValue<double>("EndOverlap", Mantid::EMPTY_DBL(), Direction::Input),
          "End wavelength (angstroms) for stitching transmission runs together");

      auto boundedIndex = boost::make_shared<BoundedValidator<int> >();
      boundedIndex->setLower(0);

      declareProperty(new PropertyWithValue<int>("I0MonitorIndex", Mantid::EMPTY_INT(), boundedIndex),
          "I0 monitor index");

      declareProperty(new PropertyWithValue<std::string>("ProcessingInstructions", "", Direction::Input),
          "Processing instructions on workspace indexes to yield only the detectors of interest. See [[PerformIndexOperations]] for details.");

      declareProperty("WavelengthMin", Mantid::EMPTY_DBL(), "Wavelength Min in angstroms",
          Direction::Input);
      declareProperty("WavelengthMax", Mantid::EMPTY_DBL(), "Wavelength Max in angstroms",
          Direction::Input);
      declareProperty("WavelengthStep", Mantid::EMPTY_DBL(), "Wavelength step in angstroms",
          Direction::Input);
      declareProperty("MonitorBackgroundWavelengthMin", Mantid::EMPTY_DBL(),
          "Monitor wavelength background min in angstroms", Direction::Input);
      declareProperty("MonitorBackgroundWavelengthMax", Mantid::EMPTY_DBL(),
          "Monitor wavelength background max in angstroms", Direction::Input);
      declareProperty("MonitorIntegrationWavelengthMin", Mantid::EMPTY_DBL(),
          "Monitor integral min in angstroms", Direction::Input);
      declareProperty("MonitorIntegrationWavelengthMax", Mantid::EMPTY_DBL(),
          "Monitor integral max in angstroms", Direction::Input);
    }

    //----------------------------------------------------------------------------------------------
    /** Execute the algorithm.
     */
    void CreateTransmissionWorkspaceAuto::exec()
    {
      //auto firstWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(this->getPointerToProperty("FirstTransmissionRun")->value());
      MatrixWorkspace_sptr firstWS = this->getProperty("FirstTransmissionRun");
      auto instrument = firstWS->getInstrument();

      //Get all the inputs.

      std::string outputWorkspaceName = this->getPropertyValue("OutputWorkspace");
      std::string analysis_mode = this->getPropertyValue("AnalysisMode");

      MatrixWorkspace_sptr secondWS = this->getProperty("SecondTransmissionRun");

      auto start_overlap = isSet<double>("StartOverlap");
      auto end_overlap = isSet<double>("EndOverlap");
      auto params = isSet<std::vector<double>>("Params");
      auto i0_monitor_index = static_cast<int>(checkForDefault("I0MonitorIndex", instrument,
          "I0MonitorIndex"));

      std::string processing_commands;
      if (this->getPointerToProperty("ProcessingInstructions")->isDefault())
      {
        const int start = static_cast<int>(instrument->getNumberParameter("PointDetectorStart")[0]);
        const int stop = static_cast<int>(instrument->getNumberParameter("PointDetectorStop")[0]);
        if (analysis_mode == "PointDetectorAnalysis")
        {
          if (start == stop)
          {
            processing_commands = boost::lexical_cast<std::string>(start);
          }
          else
          {
            processing_commands = boost::lexical_cast<std::string>(start) + ","
                + boost::lexical_cast<std::string>(stop);
          }
        }
        else
        {
          processing_commands = boost::lexical_cast<std::string>(
              static_cast<int>(instrument->getNumberParameter("MultiDetectorStart")[0])) + ","
              + boost::lexical_cast<std::string>(firstWS->getNumberHistograms() - 1);
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
      double wavelength_back_min = checkForDefault("MonitorBackgroundWavelengthMin", instrument,
          "MonitorBackgroundMin");
      double wavelength_back_max = checkForDefault("MonitorBackgroundWavelengthMax", instrument,
          "MonitorBackgroundMax");
      double wavelength_integration_min = checkForDefault("MonitorIntegrationWavelengthMin", instrument,
          "MonitorIntegralMin");
      double wavelength_integration_max = checkForDefault("MonitorIntegrationWavelengthMax", instrument,
          "MonitorIntegralMax");

      //construct the algorithm

      IAlgorithm_sptr algCreateTransWS = createChildAlgorithm("CreateTransmissionWorkspace");
      algCreateTransWS->setRethrows(true);
      algCreateTransWS->initialize();

      if (algCreateTransWS->isInitialized())
      {

        algCreateTransWS->setProperty("FirstTransmissionRun", firstWS);

        if (secondWS)
        {
          algCreateTransWS->setProperty("SecondTransmissionRun", secondWS);
        }

        algCreateTransWS->setProperty("OutputWorkspace", outputWorkspaceName);

        if (start_overlap.is_initialized())
        {
          algCreateTransWS->setProperty("StartOverlap", start_overlap.get());
        }
        if (end_overlap.is_initialized())
        {
          algCreateTransWS->setProperty("EndOverlap", end_overlap.get());
        }
        if (params.is_initialized())
        {
          algCreateTransWS->setProperty("Params", params.get());
        }

        algCreateTransWS->setProperty("I0MonitorIndex", i0_monitor_index);
        algCreateTransWS->setProperty("ProcessingInstructions", processing_commands);
        algCreateTransWS->setProperty("WavelengthMin", wavelength_min);

        if (wavelength_step.is_initialized())
        {
          algCreateTransWS->setProperty("WavelengthStep", wavelength_step.get());
        }

        algCreateTransWS->setProperty("WavelengthMax", wavelength_max);
        algCreateTransWS->setProperty("MonitorBackgroundWavelengthMin", wavelength_back_min);
        algCreateTransWS->setProperty("MonitorBackgroundWavelengthMax", wavelength_back_max);
        algCreateTransWS->setProperty("MonitorIntegrationWavelengthMin", wavelength_integration_min);
        algCreateTransWS->setProperty("MonitorIntegrationWavelengthMax", wavelength_integration_max);

        algCreateTransWS->execute();
        if (!algCreateTransWS->isExecuted())
        {
          throw std::runtime_error("CreateTransmissionWorkspace did not execute sucessfully");
        }
        else
        {
          MatrixWorkspace_sptr outWS = algCreateTransWS->getProperty("OutputWorkspace");
          setProperty("OutputWorkspace", outWS);
        }
      }
      else
      {
        throw std::runtime_error("CreateTransmissionWorkspace could not be initialised");
      }

    }

    template<typename T>
    boost::optional<T> CreateTransmissionWorkspaceAuto::isSet(std::string propName) const
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

    double CreateTransmissionWorkspaceAuto::checkForDefault(std::string propName,
        Mantid::Geometry::Instrument_const_sptr instrument, std::string idf_name) const
    {
      auto algProperty = this->getPointerToProperty(propName);
      if (algProperty->isDefault())
      {
        auto defaults = instrument->getNumberParameter(idf_name);
        if (defaults.size() == 0)
        {
          throw std::runtime_error(
              "No data could be retrieved from the parameters and argument wasn't provided: "
                  + propName);
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
