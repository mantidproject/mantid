/*WIKI*
 Creates a transmission run workspace given one or more TOF workspaces and the original run Workspace. If two workspaces are provided, then
 the workspaces are stitched together using [[Stitch1D]]. InputWorkspaces must be in TOF. A single output workspace is generated with x-units of Wavlength in angstroms.
 *WIKI*/

#include "MantidAlgorithms/CreateTransmissionWorkspace.h"

#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include <boost/make_shared.hpp>

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
      return "Reflectometry\\ISIS";
    }

    //----------------------------------------------------------------------------------------------
    /// Sets documentation strings for this algorithm
    void CreateTransmissionWorkspace::initDocs()
    {
      this->setWikiSummary(
          "Creates a transmission run workspace in Wavelength from input TOF workspaces.");
      this->setOptionalMessage(this->getWikiSummary());
    }

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

      setPropertySettings("StartOverlapQ",
          new Kernel::EnabledWhenProperty("SecondTransmissionWorkspace", IS_NOT_DEFAULT));

      setPropertySettings("EndOverlapQ",
          new Kernel::EnabledWhenProperty("SecondTransmissionWorkspace", IS_NOT_DEFAULT));

    }

    //----------------------------------------------------------------------------------------------
    /** Execute the algorithm.
     */
    void CreateTransmissionWorkspace::exec()
    {
      OptionalMatrixWorkspace_sptr firstTransmissionRun;
      OptionalMatrixWorkspace_sptr secondTransmissionRun;
      OptionalDouble stitchingStartQ;
      OptionalDouble stitchingDeltaQ;
      OptionalDouble stitchingEndQ;
      OptionalDouble stitchingStartOverlapQ;
      OptionalDouble stitchingEndOverlapQ;

      // Get the transmission run property information.
      getTransmissionRunInfo(firstTransmissionRun, secondTransmissionRun, stitchingStartQ,
          stitchingDeltaQ, stitchingEndQ, stitchingStartOverlapQ, stitchingEndOverlapQ);

      // Get wavelength intervals.
      const MinMax wavelengthInterval = this->getMinMax("WavelengthMin", "WavelengthMax");
      const double wavelengthStep = getProperty("WavelengthStep");
      const MinMax monitorBackgroundWavelengthInterval = getMinMax("MonitorBackgroundWavelengthMin",
          "MonitorBackgroundWavelengthMax");
      const MinMax monitorIntegrationWavelengthInterval = getMinMax("MonitorIntegrationWavelengthMin",
          "MonitorIntegrationWavelengthMax");

      // Get the index list
      const WorkspaceIndexList indexList = getWorkspaceIndexList();

      // Get the monitor i0 index
      const int i0MonitorIndex = getProperty("I0MonitorIndex");

      // Create the transmission workspace.
      MatrixWorkspace_sptr outWS = this->makeTransmissionCorrection(indexList, wavelengthInterval,
          monitorBackgroundWavelengthInterval, monitorIntegrationWavelengthInterval, i0MonitorIndex,
          firstTransmissionRun.get(), secondTransmissionRun, stitchingStartQ, stitchingDeltaQ, stitchingEndQ,
          stitchingStartOverlapQ, stitchingEndOverlapQ, wavelengthStep);


      setProperty("OutputWorkspace", outWS);
    }

  } // namespace Algorithms
} // namespace Mantid
