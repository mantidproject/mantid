#include "MantidAlgorithms/CreateTransmissionWorkspace.h"

#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include <boost/make_shared.hpp>
#include <boost/assign/list_of.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
  namespace Algorithms
  {

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(CreateTransmissionWorkspace)

    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    CreateTransmissionWorkspace::CreateTransmissionWorkspace()
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
     */
    CreateTransmissionWorkspace::~CreateTransmissionWorkspace()
    {
    }

    //----------------------------------------------------------------------------------------------
    /// Algorithm's name for identification. @see Algorithm::name
    const std::string CreateTransmissionWorkspace::name() const
    {
      return "CreateTransmissionWorkspace";
    }
    ;

    /// Algorithm's version for identification. @see Algorithm::version
    int CreateTransmissionWorkspace::version() const
    {
      return 1;
    }
    ;

    /// Algorithm's category for identification. @see Algorithm::category
    const std::string CreateTransmissionWorkspace::category() const
    {
      return "Reflectometry";
    }

    //----------------------------------------------------------------------------------------------

    //----------------------------------------------------------------------------------------------
    /** Initialize the algorithm's properties.
     */
    void CreateTransmissionWorkspace::init()
    {
      boost::shared_ptr<CompositeValidator> inputValidator = boost::make_shared<CompositeValidator>();
      inputValidator->add(boost::make_shared<WorkspaceUnitValidator>("TOF"));

      declareProperty(
          new WorkspaceProperty<MatrixWorkspace>("FirstTransmissionRun", "", Direction::Input,
              PropertyMode::Mandatory, inputValidator->clone()),
          "First transmission run, or the low wavelength transmision run if SecondTransmissionRun is also provided.");

      declareProperty(
          new WorkspaceProperty<MatrixWorkspace>("SecondTransmissionRun", "", Direction::Input,
              PropertyMode::Optional, inputValidator->clone()),
          "Second, high wavelength transmission run. Optional. Causes the InputWorkspace to be treated as the low wavelength transmission run.");

      this->initStitchingInputs();
      this->initIndexInputs();
      this->initWavelengthInputs();

      declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "", Direction::Output),
          "Output Workspace IvsQ.");

      setPropertySettings("Params",
          new Kernel::EnabledWhenProperty("SecondTransmissionWorkspace", IS_NOT_DEFAULT));

      setPropertySettings("StartOverlap",
          new Kernel::EnabledWhenProperty("SecondTransmissionWorkspace", IS_NOT_DEFAULT));

      setPropertySettings("EndOverlap",
          new Kernel::EnabledWhenProperty("SecondTransmissionWorkspace", IS_NOT_DEFAULT));

    }

    //----------------------------------------------------------------------------------------------
    /** Execute the algorithm.
     */
    void CreateTransmissionWorkspace::exec()
    {
      OptionalMatrixWorkspace_sptr firstTransmissionRun;
      OptionalMatrixWorkspace_sptr secondTransmissionRun;
      OptionalDouble stitchingStart;
      OptionalDouble stitchingDelta;
      OptionalDouble stitchingEnd;
      OptionalDouble stitchingStartOverlap;
      OptionalDouble stitchingEndOverlap;

      // Get the transmission run property information.
      getTransmissionRunInfo(firstTransmissionRun, secondTransmissionRun, stitchingStart, stitchingDelta,
          stitchingEnd, stitchingStartOverlap, stitchingEndOverlap);

      // Get wavelength intervals.
      const MinMax wavelengthInterval = this->getMinMax("WavelengthMin", "WavelengthMax");
      const double wavelengthStep = getProperty("WavelengthStep");
      const MinMax monitorBackgroundWavelengthInterval = getMinMax("MonitorBackgroundWavelengthMin",
          "MonitorBackgroundWavelengthMax");
      const MinMax monitorIntegrationWavelengthInterval = getMinMax("MonitorIntegrationWavelengthMin",
          "MonitorIntegrationWavelengthMax");

      const std::string processingCommands = getWorkspaceIndexList();

      // Get the monitor i0 index
      const int i0MonitorIndex = getProperty("I0MonitorIndex");

      // Create the transmission workspace.
      MatrixWorkspace_sptr outWS = this->makeTransmissionCorrection(processingCommands,
          wavelengthInterval, monitorBackgroundWavelengthInterval, monitorIntegrationWavelengthInterval,
          i0MonitorIndex, firstTransmissionRun.get(), secondTransmissionRun, stitchingStart,
          stitchingDelta, stitchingEnd, stitchingStartOverlap, stitchingEndOverlap, wavelengthStep);

      setProperty("OutputWorkspace", outWS);
    }

    /**
     * Create a transmission corrections workspace utilising one or two workspaces.
     *
     * Input workspaces are in TOF. These are converted to lambda, normalized and stitched together (if two given).
     *
     * @param processingCommands : Processing instructions. Usually a list of detector indexes to keep.
     * @param wavelengthInterval : Wavelength interval for the run workspace.
     * @param wavelengthMonitorBackgroundInterval : Wavelength interval for the monitor background
     * @param wavelengthMonitorIntegrationInterval : Wavelength interval for the monitor integration
     * @param i0MonitorIndex : Monitor index for the I0 monitor
     * @param firstTransmissionRun : The first transmission run
     * @param secondTransmissionRun : The second transmission run (optional)
     * @param stitchingStart : Stitching start (optional but dependent on secondTransmissionRun)
     * @param stitchingDelta : Stitching delta (optional but dependent on secondTransmissionRun)
     * @param stitchingEnd : Stitching end (optional but dependent on secondTransmissionRun)
     * @param stitchingStartOverlap : Stitching start overlap (optional but dependent on secondTransmissionRun)
     * @param stitchingEndOverlap : Stitching end overlap (optional but dependent on secondTransmissionRun)
     * @param wavelengthStep : Step in angstroms for rebinning for workspaces converted into wavelength.
     * @return A transmission workspace in Wavelength units.
     */
    MatrixWorkspace_sptr CreateTransmissionWorkspace::makeTransmissionCorrection(
        const std::string& processingCommands, const MinMax& wavelengthInterval,
        const MinMax& wavelengthMonitorBackgroundInterval,
        const MinMax& wavelengthMonitorIntegrationInterval, const int& i0MonitorIndex,
        MatrixWorkspace_sptr firstTransmissionRun, OptionalMatrixWorkspace_sptr secondTransmissionRun,
        const OptionalDouble& stitchingStart, const OptionalDouble& stitchingDelta,
        const OptionalDouble& stitchingEnd, const OptionalDouble& stitchingStartOverlap,
        const OptionalDouble& stitchingEndOverlap, const double& wavelengthStep)
    {
      auto trans1InLam = toLam(firstTransmissionRun, processingCommands, i0MonitorIndex,
          wavelengthInterval, wavelengthMonitorBackgroundInterval, wavelengthStep);
      MatrixWorkspace_sptr trans1Detector = trans1InLam.get<0>();
      MatrixWorkspace_sptr trans1Monitor = trans1InLam.get<1>();

      // Monitor integration ... can this happen inside the toLam routine?
      auto integrationAlg = this->createChildAlgorithm("Integration");
      integrationAlg->initialize();
      integrationAlg->setProperty("InputWorkspace", trans1Monitor);
      integrationAlg->setProperty("RangeLower", wavelengthMonitorIntegrationInterval.get<0>());
      integrationAlg->setProperty("RangeUpper", wavelengthMonitorIntegrationInterval.get<1>());
      integrationAlg->execute();
      trans1Monitor = integrationAlg->getProperty("OutputWorkspace");

      MatrixWorkspace_sptr transmissionWS = divide(trans1Detector, trans1Monitor);

      if (secondTransmissionRun.is_initialized())
      {
        auto transRun2 = secondTransmissionRun.get();
        g_log.debug("Extracting second transmission run workspace indexes from spectra");

        auto trans2InLam = toLam(transRun2, processingCommands, i0MonitorIndex, wavelengthInterval,
            wavelengthMonitorBackgroundInterval, wavelengthStep);

        // Unpack the conversion results.
        MatrixWorkspace_sptr trans2Detector = trans2InLam.get<0>();
        MatrixWorkspace_sptr trans2Monitor = trans2InLam.get<1>();

        // Monitor integration ... can this happen inside the toLam routine?
        auto integrationAlg = this->createChildAlgorithm("Integration");
        integrationAlg->initialize();
        integrationAlg->setProperty("InputWorkspace", trans2Monitor);
        integrationAlg->setProperty("RangeLower", wavelengthMonitorIntegrationInterval.get<0>());
        integrationAlg->setProperty("RangeUpper", wavelengthMonitorIntegrationInterval.get<1>());
        integrationAlg->execute();
        trans2Monitor = integrationAlg->getProperty("OutputWorkspace");

        MatrixWorkspace_sptr normalizedTrans2 = divide(trans2Detector, trans2Monitor);

        // Stitch the results.
        auto stitch1DAlg = this->createChildAlgorithm("Stitch1D");
        stitch1DAlg->initialize();
        AnalysisDataService::Instance().addOrReplace("transmissionWS", transmissionWS);
        AnalysisDataService::Instance().addOrReplace("normalizedTrans2", normalizedTrans2);
        stitch1DAlg->setProperty("LHSWorkspace", transmissionWS);
        stitch1DAlg->setProperty("RHSWorkspace", normalizedTrans2);
        if (stitchingStartOverlap.is_initialized())
        {
          stitch1DAlg->setProperty("StartOverlap", stitchingStartOverlap.get());
        }
        if (stitchingEndOverlap.is_initialized())
        {
          stitch1DAlg->setProperty("EndOverlap", stitchingEndOverlap.get());
        }
        if (stitchingStart.is_initialized() && stitchingEnd.is_initialized()
            && stitchingDelta.is_initialized())
        {
          const std::vector<double> params = boost::assign::list_of(stitchingStart.get())(
              stitchingDelta.get())(stitchingEnd.get()).convert_to_container<std::vector<double> >();
          stitch1DAlg->setProperty("Params", params);
        }
        else if (stitchingDelta.is_initialized())
        {
          const double delta = stitchingDelta.get();
          stitch1DAlg->setProperty("Params", std::vector<double>(1, delta));
        }
        stitch1DAlg->execute();
        transmissionWS = stitch1DAlg->getProperty("OutputWorkspace");
        AnalysisDataService::Instance().remove("transmissionWS");
        AnalysisDataService::Instance().remove("normalizedTrans2");
      }

      return transmissionWS;
    }

  } // namespace Algorithms
} // namespace Mantid
