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
          new WorkspaceProperty<MatrixWorkspace>("InputWorkspace", "", Direction::Input,
              PropertyMode::Mandatory, inputValidator->clone()),
          "First transmission run, or the low wavelength transmision run if SecondTransmissionRun is also provided.");

      declareProperty(
          new WorkspaceProperty<MatrixWorkspace>("SecondTransmissionWorkspace", "", Direction::Input,
              PropertyMode::Optional, inputValidator->clone()),
          "Second, high wavelength transmission run. Optional. Causes the InputWorkspace to be treated as the low wavelength transmission run.");

      declareProperty(
          new PropertyWithValue<double>("WavelengthMin", Mantid::EMPTY_DBL(),
              boost::make_shared<MandatoryValidator<double> >(), Direction::Input),
          "Wavelength minimum in angstroms");

      declareProperty(
          new PropertyWithValue<double>("WavelengthMax", Mantid::EMPTY_DBL(),
              boost::make_shared<MandatoryValidator<double> >(), Direction::Input),
          "Wavelength maximum in angstroms");

      declareProperty(
          new PropertyWithValue<double>("WavelengthStep", 0.05,
              boost::make_shared<MandatoryValidator<double> >(), Direction::Input),
          "Wavelength rebinning step in angstroms. Defaults to 0.05. Used for rebinning intermediate workspaces converted into wavelength.");

      boost::shared_ptr<CompositeValidator> mandatoryWorkspaceIndex = boost::make_shared<
          CompositeValidator>();
      mandatoryWorkspaceIndex->add(boost::make_shared<MandatoryValidator<int> >());
      auto boundedIndex = boost::make_shared<BoundedValidator<int> >();
      boundedIndex->setLower(0);
      mandatoryWorkspaceIndex->add(boundedIndex);

      declareProperty(
          new PropertyWithValue<int>("I0MonitorIndex", Mantid::EMPTY_INT(), mandatoryWorkspaceIndex),
          "I0 monitor index");

      declareProperty(
          new PropertyWithValue<double>("MonitorBackgroundWavelengthMin", Mantid::EMPTY_DBL(),
              boost::make_shared<MandatoryValidator<double> >(), Direction::Input),
          "Wavelength minimum for monitor background in angstroms.");

      declareProperty(
          new PropertyWithValue<double>("MonitorBackgroundWavelengthMax", Mantid::EMPTY_DBL(),
              boost::make_shared<MandatoryValidator<double> >(), Direction::Input),
          "Wavelength maximum for monitor background in angstroms.");

      declareProperty(
          new PropertyWithValue<double>("MonitorIntegrationWavelengthMin", Mantid::EMPTY_DBL(),
              boost::make_shared<MandatoryValidator<double> >(), Direction::Input),
          "Wavelength minimum for integration in angstroms.");
      declareProperty(
          new PropertyWithValue<double>("MonitorIntegrationWavelengthMax", Mantid::EMPTY_DBL(),
              boost::make_shared<MandatoryValidator<double> >(), Direction::Input),
          "Wavelength maximum for integration in angstroms.");

      declareProperty(new ArrayProperty<int>("WorkspaceIndexList"),
          "Indices of the spectra in pairs (lower, upper) that mark the ranges that correspond to detectors of interest.");

      declareProperty(
          new ArrayProperty<double>("Params", boost::make_shared<RebinParamsValidator>(true)),
          "A comma separated list of first bin boundary, width, last bin boundary. "
              "These parameters are used for stitching together transmission runs. "
              "Values are in q. This input is only needed if a SecondTransmission run is provided.");

      declareProperty(
          new PropertyWithValue<double>("StartOverlapQ", Mantid::EMPTY_DBL(), Direction::Input),
          "Start Q for stitching transmission runs together");

      declareProperty(
          new PropertyWithValue<double>("EndOverlapQ", Mantid::EMPTY_DBL(), Direction::Input),
          "End Q for stitching transmission runs together");

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
      MatrixWorkspace_sptr firstTranmsission = getProperty("InputWorkspace");
      setProperty("OutputWorkspace",firstTranmsission);
    }

  } // namespace Algorithms
} // namespace Mantid
