/*WIKI* 
Create a PropertyManager object setting the reduction options for HFIR SANS.
The property manager object is then added to the PropertyManagerDataService.

See [http://www.mantidproject.org/Reduction_for_HFIR_SANS SANS Reduction] documentation for details.

*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidWorkflowAlgorithms/SetupHFIRReduction.h"
#include "MantidAPI/AlgorithmProperty.h"
#include "MantidAPI/PropertyManagerDataService.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid
{
namespace WorkflowAlgorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SetupHFIRReduction)

/// Sets documentation strings for this algorithm
void SetupHFIRReduction::initDocs()
{
  this->setWikiSummary("Set up HFIR SANS reduction options.");
  this->setOptionalMessage("Set up HFIR SANS reduction options.");
}

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

void SetupHFIRReduction::init()
{
  declareProperty("SampleDetectorDistance", EMPTY_DBL(), "Sample to detector distance to use (overrides meta data), in mm");
  declareProperty("SampleDetectorDistanceOffset", EMPTY_DBL(), "Offset to the sample to detector distance (use only when using the distance found in the meta data), in mm");

  // Optionally, we can specify the wavelength and wavelength spread and overwrite
  // the value in the data file (used when the data file is not populated)
  auto mustBePositive = boost::make_shared<Kernel::BoundedValidator<double> >();
  mustBePositive->setLower(0.0);
  declareProperty("Wavelength", EMPTY_DBL(), mustBePositive,
      "Wavelength value to use when loading the data file (Angstrom).");
  declareProperty("WavelengthSpread", 0.1, mustBePositive,
    "Wavelength spread to use when loading the data file (default 0.0)" );

  // Beam center
  std::string center_grp = "Beam Center";
  declareProperty("FindBeamCenter", false, "If True, the beam center will be calculated");
  declareProperty("BeamCenterX", EMPTY_DBL(), "Position of the beam center, in pixel");
  declareProperty("BeamCenterY", EMPTY_DBL(), "Position of the beam center, in pixel");
  setPropertyGroup("FindBeamCenter", center_grp);
  setPropertyGroup("BeamCenterX", center_grp);
  setPropertyGroup("BeamCenterY", center_grp);

  declareProperty("OutputMessage","",Direction::Output);
  declareProperty("ReductionProperties","__sans_reduction_properties", Direction::Input);
}

void SetupHFIRReduction::exec()
{
  // Reduction property manager
  const std::string reductionManagerName = getProperty("ReductionProperties");
  if (reductionManagerName.size()==0)
  {
    g_log.error() << "ERROR: Reduction Property Manager name is empty" << std::endl;
    return;
  }
  boost::shared_ptr<PropertyManager> reductionManager = boost::make_shared<PropertyManager>();
  PropertyManagerDataService::Instance().addOrReplace(reductionManagerName, reductionManager);

  // Store name of the instrument
  reductionManager->declareProperty(new PropertyWithValue<std::string>("InstrumentName", "HFIRSANS") );

  // Load algorithm
  const double sdd = getProperty("SampleDetectorDistance");
  const double sddOffset = getProperty("SampleDetectorDistanceOffset");
  const double wavelength = getProperty("Wavelength");
  const double wavelengthSpread = getProperty("WavelengthSpread");

  IAlgorithm_sptr loadAlg = createSubAlgorithm("HFIRLoad");
  if (!isEmpty(sdd)) loadAlg->setProperty("SampleDetectorDistance", sdd);
  if (!isEmpty(sddOffset)) loadAlg->setProperty("SampleDetectorDistanceOffset", sddOffset);
  if (!isEmpty(wavelength))
  {
    loadAlg->setProperty("Wavelength", wavelength);
    loadAlg->setProperty("WavelengthSpread", wavelengthSpread);
  }
  reductionManager->declareProperty(new AlgorithmProperty("LoadAlgorithm"));
  reductionManager->setProperty("LoadAlgorithm", loadAlg);

  // Beam center
  const double beamCenterX = getProperty("BeamCenterX");
  const double beamCenterY = getProperty("BeamCenterY");
  const bool calcBeamCenter = getProperty("FindBeamCenter");
  if (!calcBeamCenter)
  {
    reductionManager->declareProperty(new PropertyWithValue<double>("LatestBeamCenterX", beamCenterX) );
    reductionManager->declareProperty(new PropertyWithValue<double>("LatestBeamCenterY", beamCenterY) );
  }

  // Store default dark current algorithm
  IAlgorithm_sptr darkAlg = createSubAlgorithm("HFIRDarkCurrentSubtraction");
  darkAlg->setProperty("OutputDarkCurrentWorkspace", "");
  darkAlg->setPropertyValue("ReductionProperties", reductionManagerName);
  reductionManager->declareProperty(new AlgorithmProperty("DefaultDarkCurrentAlgorithm"));
  reductionManager->setProperty("DefaultDarkCurrentAlgorithm", darkAlg);

  setPropertyValue("OutputMessage", "HFIR reduction options set");
}

} // namespace WorkflowAlgorithms
} // namespace Mantid

