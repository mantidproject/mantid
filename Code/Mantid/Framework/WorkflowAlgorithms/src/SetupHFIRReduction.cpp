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
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/RebinParamsValidator.h"
#include <boost/algorithm/string/predicate.hpp>
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
  // Load options
  std::string load_grp = "Load Options";

  declareProperty("SampleDetectorDistance", EMPTY_DBL(), "Sample to detector distance to use (overrides meta data), in mm");
  declareProperty("SampleDetectorDistanceOffset", EMPTY_DBL(), "Offset to the sample to detector distance (use only when using the distance found in the meta data), in mm");
  declareProperty("SolidAngleCorrection", true, "If true, the solide angle correction will be applied to the data");

  // Optionally, we can specify the wavelength and wavelength spread and overwrite
  // the value in the data file (used when the data file is not populated)
  auto mustBePositive = boost::make_shared<Kernel::BoundedValidator<double> >();
  mustBePositive->setLower(0.0);
  declareProperty("Wavelength", EMPTY_DBL(), mustBePositive,
      "Wavelength value to use when loading the data file (Angstrom).");
  declareProperty("WavelengthSpread", 0.1, mustBePositive,
    "Wavelength spread to use when loading the data file (default 0.0)" );

  setPropertyGroup("SampleDetectorDistance", load_grp);
  setPropertyGroup("SampleDetectorDistanceOffset", load_grp);
  setPropertyGroup("SolidAngleCorrection", load_grp);
  setPropertyGroup("Wavelength", load_grp);
  setPropertyGroup("WavelengthSpread", load_grp);

  // Beam center
  std::string center_grp = "Beam Center";
  std::vector<std::string> centerOptions;
  centerOptions.push_back("None");
  centerOptions.push_back("Value");
  centerOptions.push_back("DirectBeam");
  centerOptions.push_back("Scattering");

  declareProperty("BeamCenterMethod", "None",
      boost::make_shared<StringListValidator>(centerOptions),
      "Method for determining the transmission data beam center");

  //    Option 1: Set beam center by hand
  declareProperty("BeamCenterX", EMPTY_DBL(), "Position of the beam center, in pixel");
  declareProperty("BeamCenterY", EMPTY_DBL(), "Position of the beam center, in pixel");

  //    Option 2: Find it (expose properties from FindCenterOfMass)
  declareProperty(new API::FileProperty("BeamCenterFile", "", API::FileProperty::OptionalLoad, ".xml"),
      "The name of the input data file to load");
  //declareProperty("Tolerance", EMPTY_DBL(), "Tolerance on the center of mass position between each iteration [m]. Default: 0.00125");
  auto positiveDouble = boost::make_shared<BoundedValidator<double> >();
  positiveDouble->setLower(0);
  declareProperty("BeamRadius", EMPTY_DBL(),
      "Radius of the beam area used the exclude the beam when calculating "
      "the center of mass of the scattering pattern [pixels]. Default=3.0");

  setPropertyGroup("BeamCenterMethod", center_grp);
  setPropertyGroup("BeamCenterX", center_grp);
  setPropertyGroup("BeamCenterY", center_grp);
  setPropertyGroup("BeamCenterFile", center_grp);
  //setPropertyGroup("Tolerance", center_grp);
  setPropertyGroup("BeamRadius", center_grp);

  // Normalisation
  std::vector<std::string> incidentBeamNormOptions;
  incidentBeamNormOptions.push_back("None");
  incidentBeamNormOptions.push_back("Monitor");
  incidentBeamNormOptions.push_back("Timer");
  this->declareProperty("Normalisation", "Monitor",
      boost::make_shared<StringListValidator>(incidentBeamNormOptions),
      "Options for data normalisation");

  // Dark current
  declareProperty(new API::FileProperty("DarkCurrentFile", "", API::FileProperty::OptionalLoad, ".xml"),
      "The name of the input data file to load as dark current.");

  // Sensitivity
  std::string eff_grp = "Sensitivity";
  declareProperty(new API::FileProperty("SensitivityFile", "", API::FileProperty::OptionalLoad, ".xml"),
      "Flood field or sensitivity file.");
  declareProperty("MinEfficiency", EMPTY_DBL(), positiveDouble,
      "Minimum efficiency for a pixel to be considered (default: no minimum).");
  declareProperty("MaxEfficiency", EMPTY_DBL(), positiveDouble,
      "Maximum efficiency for a pixel to be considered (default: no maximum).");
  declareProperty("UseDefaultDC", true, "If true, the dark current subtracted from the sample data will also be subtracted from the flood field.");
  declareProperty(new API::FileProperty("SensitivityDarkCurrentFile", "", API::FileProperty::OptionalLoad, ".xml"),
      "The name of the input file to load as dark current.");
  declareProperty("SensitivityBeamCenterX", EMPTY_DBL(), "Position of the beam center for the sensitivity data, in pixel");
  declareProperty("SensitivityBeamCenterY", EMPTY_DBL(), "Position of the beam center for the sensitivity data, in pixel");
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputSensitivityWorkspace","", Direction::Output, PropertyMode::Optional));

  setPropertyGroup("SensitivityFile", eff_grp);
  setPropertyGroup("MinEfficiency", eff_grp);
  setPropertyGroup("MaxEfficiency", eff_grp);
  setPropertyGroup("UseDefaultDC", eff_grp);
  setPropertyGroup("SensitivityDarkCurrentFile", eff_grp);
  setPropertyGroup("SensitivityBeamCenterX", eff_grp);
  setPropertyGroup("SensitivityBeamCenterY", eff_grp);
  setPropertyGroup("OutputSensitivityWorkspace", eff_grp);

  // Transmission
  std::string trans_grp = "Transmission";
  std::vector<std::string> transOptions;
  transOptions.push_back("Value");
  transOptions.push_back("DirectBeam");
  //transOptions.push_back("BeamSpreader");
  declareProperty("TransmissionMethod", "Value",
      boost::make_shared<StringListValidator>(transOptions),
      "Transmission determination method");

  // - Transmission value entered by hand
  declareProperty("TransmissionValue", EMPTY_DBL(), positiveDouble,
      "Transmission value.");
  declareProperty("TransmissionError", EMPTY_DBL(), positiveDouble,
      "Transmission error.");

  // - Direct beam method transmission calculation
  declareProperty("TransmissionBeamRadius", 3.0,
      "Radius of the beam area used to compute the transmission [pixels]");
  declareProperty(new API::FileProperty("TransmissionSampleDataFile", "",
      API::FileProperty::OptionalLoad, ".xml"),
      "Sample data file for transmission calculation");
  declareProperty(new API::FileProperty("TransmissionEmptyDataFile", "",
      API::FileProperty::OptionalLoad, ".xml"),
      "Empty data file for transmission calculation");

  // - transmission beam center
  declareProperty("TransmissionBeamCenterMethod", "None",
      boost::make_shared<StringListValidator>(centerOptions),
      "Method for determining the transmission data beam center");

  //    Option 1: Set beam center by hand
  declareProperty("TransmissionBeamCenterX", EMPTY_DBL(),
      "Transmission beam center location in X [pixels]");
  declareProperty("TransmissionBeamCenterY", EMPTY_DBL(),
      "Transmission beam center location in Y [pixels]");

  //    Option 2: Find it (expose properties from FindCenterOfMass)
  declareProperty(new API::FileProperty("TransmissionBeamCenterFile", "",
      API::FileProperty::OptionalLoad, ".xml"),
      "The name of the input data file to load");
  declareProperty("ThetaDependentTransmission", true,
      "If true, a theta-dependent transmission correction will be applied.");

  setPropertyGroup("TransmissionMethod", trans_grp);
  setPropertyGroup("TransmissionValue", trans_grp);
  setPropertyGroup("TransmissionError", trans_grp);
  setPropertyGroup("TransmissionBeamRadius", trans_grp);
  setPropertyGroup("TransmissionSampleDataFile", trans_grp);
  setPropertyGroup("TransmissionEmptyDataFile", trans_grp);
  setPropertyGroup("TransmissionBeamCenterMethod", trans_grp);
  setPropertyGroup("TransmissionBeamCenterX", trans_grp);
  setPropertyGroup("TransmissionBeamCenterY", trans_grp);
  setPropertyGroup("TransmissionBeamCenterFile", trans_grp);
  setPropertyGroup("ThetaDependentTransmission", trans_grp);

  // Background options
  std::string bck_grp = "Background";
  declareProperty("BackgroundFiles", "", "Background data files");
  this->declareProperty("BckTransmissionMethod", "Value",
      boost::make_shared<StringListValidator>(transOptions),
      "Transmission determination method");

  // - Transmission value entered by hand
  declareProperty("BckTransmissionValue", EMPTY_DBL(), positiveDouble,
      "Transmission value.");
  declareProperty("BckTransmissionError", EMPTY_DBL(), positiveDouble,
      "Transmission error.");

  // - Direct beam method transmission calculation
  declareProperty("BckTransmissionBeamRadius", 3.0,
      "Radius of the beam area used to compute the transmission [pixels]");
  declareProperty(new API::FileProperty("BckTransmissionSampleDataFile", "",
      API::FileProperty::OptionalLoad, ".xml"),
      "Sample data file for transmission calculation");
  declareProperty(new API::FileProperty("BckTransmissionEmptyDataFile", "",
      API::FileProperty::OptionalLoad, ".xml"),
      "Empty data file for transmission calculation");

  // - transmission beam center
  declareProperty("BckTransmissionBeamCenterMethod", "None",
      boost::make_shared<StringListValidator>(centerOptions),
      "Method for determining the transmission data beam center");
  //    Option 1: Set beam center by hand
  declareProperty("BckTransmissionBeamCenterX", EMPTY_DBL(),
      "Transmission beam center location in X [pixels]");
  declareProperty("BckTransmissionBeamCenterY", EMPTY_DBL(),
      "Transmission beam center location in Y [pixels]");
  //    Option 2: Find it (expose properties from FindCenterOfMass)
  declareProperty(new API::FileProperty("BckTransmissionBeamCenterFile", "",
      API::FileProperty::OptionalLoad, ".xml"),
      "The name of the input data file to load");
  declareProperty("BckThetaDependentTransmission", true,
      "If true, a theta-dependent transmission correction will be applied.");

  setPropertyGroup("BackgroundFiles", bck_grp);
  setPropertyGroup("BckTransmissionMethod", bck_grp);
  setPropertyGroup("BckTransmissionValue", bck_grp);
  setPropertyGroup("BckTransmissionError", bck_grp);
  setPropertyGroup("BckTransmissionBeamRadius", bck_grp);
  setPropertyGroup("BckTransmissionSampleDataFile", bck_grp);
  setPropertyGroup("BckTransmissionEmptyDataFile", bck_grp);
  setPropertyGroup("BckTransmissionBeamCenterMethod", trans_grp);
  setPropertyGroup("BckTransmissionBeamCenterX", bck_grp);
  setPropertyGroup("BckTransmissionBeamCenterY", bck_grp);
  setPropertyGroup("BckTransmissionBeamCenterFile", trans_grp);
  setPropertyGroup("BckThetaDependentTransmission", bck_grp);

  // Geometry correction
  std::string geo_grp = "Geometry";
  declareProperty("SampleThickness", EMPTY_DBL(), "Sample thickness [cm]");

  // I(Q) calculation
  std::string iq1d_grp = "I(q) calculation";
  declareProperty("DoAzimuthalAverage", true);
  declareProperty(new ArrayProperty<double>("IQBinning", boost::make_shared<RebinParamsValidator>(true)));

  auto positiveInt = boost::make_shared<BoundedValidator<int> >();
  positiveInt->setLower(0);
  declareProperty("IQNumberOfBins", 100, positiveInt,
                  "Number of I(q) bins when binning is not specified.");
  declareProperty("IQLogBinning", false,
                  "I(q) log binning when binning is not specified.");

  declareProperty("NumberOfSubpixels", 1, positiveInt,
                  "Number of sub-pixels used for each detector pixel in each direction."
                  "The total number of sub-pixels will be NPixelDivision*NPixelDivision.");
  declareProperty("ErrorWeighting", false,
                  "Choose whether each pixel contribution will be weighted by 1/error^2.");
  setPropertyGroup("DoAzimuthalAverage", iq1d_grp);
  setPropertyGroup("IQBinning", iq1d_grp);
  setPropertyGroup("IQNumberOfBins", iq1d_grp);
  setPropertyGroup("IQLogBinning", iq1d_grp);
  setPropertyGroup("NumberOfSubpixels", iq1d_grp);
  setPropertyGroup("ErrorWeighting", iq1d_grp);

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
  AlgorithmProperty *algProp = new AlgorithmProperty("LoadAlgorithm");
  algProp->setValue(loadAlg->toString());
  reductionManager->declareProperty(algProp);

  // Beam center
  const double beamCenterX = getProperty("BeamCenterX");
  const double beamCenterY = getProperty("BeamCenterY");
  const std::string centerMethod = getPropertyValue("BeamCenterMethod");

  // Beam center option for transmission data
  if (boost::iequals(centerMethod, "Value") && !isEmpty(beamCenterX) && !isEmpty(beamCenterY))
  {
    reductionManager->declareProperty(new PropertyWithValue<double>("LatestBeamCenterX", beamCenterX) );
    reductionManager->declareProperty(new PropertyWithValue<double>("LatestBeamCenterY", beamCenterY) );
  }
  else if (!boost::iequals(centerMethod, "None"))
  {
    bool useDirectBeamMethod = true;
    if (!boost::iequals(centerMethod, "DirectBeam")) useDirectBeamMethod = false;
    const std::string beamCenterFile = getProperty("BeamCenterFile");
    if (beamCenterFile.size()>0)
    {
      const double beamRadius = getProperty("BeamRadius");

      IAlgorithm_sptr ctrAlg = createSubAlgorithm("SANSBeamFinder");
      ctrAlg->setProperty("Filename", beamCenterFile);
      ctrAlg->setProperty("UseDirectBeamMethod", useDirectBeamMethod);
      if (!isEmpty(beamRadius)) ctrAlg->setProperty("BeamRadius", beamRadius);
      ctrAlg->setPropertyValue("ReductionProperties", reductionManagerName);

      algProp = new AlgorithmProperty("SANSBeamFinderAlgorithm");
      algProp->setValue(ctrAlg->toString());
      reductionManager->declareProperty(algProp);
    } else {
      g_log.error() << "ERROR: Beam center determination was required"
          " but no file was provided" << std::endl;
    }
  }

  // Store dark current algorithm
  const std::string darkCurrentFile = getPropertyValue("DarkCurrentFile");
  if (darkCurrentFile.size() > 0)
  {
    IAlgorithm_sptr darkAlg = createSubAlgorithm("HFIRDarkCurrentSubtraction");
    darkAlg->setProperty("Filename", darkCurrentFile);
    darkAlg->setProperty("OutputDarkCurrentWorkspace", "");
    darkAlg->setPropertyValue("ReductionProperties", reductionManagerName);
    algProp = new AlgorithmProperty("DarkCurrentAlgorithm");
    algProp->setValue(darkAlg->toString());
    reductionManager->declareProperty(algProp);
  }

  // Store default dark current algorithm
  IAlgorithm_sptr darkDefaultAlg = createSubAlgorithm("HFIRDarkCurrentSubtraction");
  darkDefaultAlg->setProperty("OutputDarkCurrentWorkspace", "");
  darkDefaultAlg->setPropertyValue("ReductionProperties", reductionManagerName);
  algProp = new AlgorithmProperty("DefaultDarkCurrentAlgorithm");
  algProp->setValue(darkDefaultAlg->toString());
  reductionManager->declareProperty(algProp);

  // Solid angle correction
  const bool solidAngleCorrection = getProperty("SolidAngleCorrection");
  if (solidAngleCorrection)
  {
    IAlgorithm_sptr solidAlg = createSubAlgorithm("SANSSolidAngleCorrection");
    algProp = new AlgorithmProperty("SANSSolidAngleCorrection");
    algProp->setValue(solidAlg->toString());
    reductionManager->declareProperty(algProp);
  }

  // Normalization
  const std::string normalization = getProperty("Normalisation");
  if (!boost::contains(normalization, "None"))
  {
    IAlgorithm_sptr normAlg = createSubAlgorithm("HFIRSANSNormalise");
    normAlg->setProperty("NormalisationType", normalization);
    algProp = new AlgorithmProperty("NormaliseAlgorithm");
    algProp->setValue(normAlg->toString());
    reductionManager->declareProperty(algProp);
    reductionManager->declareProperty(new PropertyWithValue<std::string>("TransmissionNormalisation", normalization) );
  } else
    reductionManager->declareProperty(new PropertyWithValue<std::string>("TransmissionNormalisation", "Timer") );


  // Sensitivity correction
  const std::string sensitivityFile = getPropertyValue("SensitivityFile");
  if (sensitivityFile.size() > 0)
  {
    const bool useSampleDC = getProperty("UseDefaultDC");
    const std::string sensitivityDarkCurrentFile = getPropertyValue("SensitivityDarkCurrentFile");
    const std::string outputSensitivityWS = getPropertyValue("OutputSensitivityWorkspace");
    const double minEff = getProperty("MinEfficiency");
    const double maxEff = getProperty("MaxEfficiency");
    const double sensitivityBeamCenterX = getProperty("SensitivityBeamCenterX");
    const double sensitivityBeamCenterY = getProperty("SensitivityBeamCenterY");

    IAlgorithm_sptr effAlg = createSubAlgorithm("SANSSensitivityCorrection");
    effAlg->setProperty("Filename", sensitivityFile);
    effAlg->setProperty("UseSampleDC", useSampleDC);
    effAlg->setProperty("DarkCurrentFile", sensitivityDarkCurrentFile);
    effAlg->setProperty("MinEfficiency", minEff);
    effAlg->setProperty("MaxEfficiency", maxEff);
    if (!isEmpty(sensitivityBeamCenterX)) effAlg->setProperty("BeamCenterX", sensitivityBeamCenterX);
    if (!isEmpty(sensitivityBeamCenterY)) effAlg->setProperty("BeamCenterY", sensitivityBeamCenterY);
    effAlg->setProperty("OutputSensitivityWorkspace", outputSensitivityWS);
    effAlg->setPropertyValue("ReductionProperties", reductionManagerName);

    algProp = new AlgorithmProperty("SensitivityAlgorithm");
    algProp->setValue(effAlg->toString());
    reductionManager->declareProperty(algProp);
  }

  setupTransmission(reductionManager);

  setupBackground(reductionManager);

  // Geometry correction
  const double thickness = getProperty("SampleThickness");
  if (!isEmpty(thickness))
  {
    IAlgorithm_sptr thickAlg = createSubAlgorithm("NormaliseByThickness");
    thickAlg->setProperty("SampleThickness", thickness);

    algProp = new AlgorithmProperty("GeometryAlgorithm");
    algProp->setValue(thickAlg->toString());
    reductionManager->declareProperty(algProp);
  }

  // Azimuthal averaging
  const bool doAveraging = getProperty("DoAzimuthalAverage");
  if (doAveraging)
  {
    const std::string binning = getPropertyValue("IQBinning");
    const std::string n_bins = getPropertyValue("IQNumberOfBins");
    const bool log_binning = getProperty("IQLogBinning");
    const std::string n_subpix = getPropertyValue("NumberOfSubpixels");
    const bool err_weighting = getProperty("ErrorWeighting");

    IAlgorithm_sptr iqAlg = createSubAlgorithm("SANSAzimuthalAverage1D");
    iqAlg->setPropertyValue("Binning", binning);
    iqAlg->setPropertyValue("NumberOfBins", n_bins);
    iqAlg->setProperty("LogBinning", log_binning);
    iqAlg->setPropertyValue("NumberOfSubpixels", n_subpix);
    iqAlg->setProperty("ErrorWeighting", err_weighting);
    iqAlg->setPropertyValue("ReductionProperties", reductionManagerName);

    algProp = new AlgorithmProperty("IQAlgorithm");
    algProp->setValue(iqAlg->toString());
    g_log.information() << iqAlg->toString() << std::endl;
    reductionManager->declareProperty(algProp);
  }

  setPropertyValue("OutputMessage", "HFIR reduction options set");
}

void SetupHFIRReduction::setupBackground(boost::shared_ptr<PropertyManager> reductionManager)
{
  const std::string reductionManagerName = getProperty("ReductionProperties");
  // Background
  const std::string backgroundFile = getPropertyValue("BackgroundFiles");
  if (backgroundFile.size() > 0)
    reductionManager->declareProperty(new PropertyWithValue<std::string>("BackgroundFiles", backgroundFile) );
  const bool bckThetaDependentTrans = getProperty("BckThetaDependentTransmission");
  const std::string bckTransMethod = getProperty("BckTransmissionMethod");
  if (boost::iequals(bckTransMethod, "Value"))
  {
    const double transValue = getProperty("BckTransmissionValue");
    const double transError = getProperty("BckTransmissionError");
    if (!isEmpty(transValue) && !isEmpty(transError))
    {
      IAlgorithm_sptr transAlg = createSubAlgorithm("ApplyTransmissionCorrection");
      transAlg->setProperty("TransmissionValue", transValue);
      transAlg->setProperty("TransmissionError", transError);
      transAlg->setProperty("ThetaDependent", bckThetaDependentTrans);

      AlgorithmProperty *algProp = new AlgorithmProperty("BckTransmissionAlgorithm");
      algProp->setValue(transAlg->toString());
      reductionManager->declareProperty(algProp);
    } else {
      g_log.error("SetupHFIRReduction expected transmission/error values and got empty values");
    }
  }
  else if (boost::iequals(bckTransMethod, "DirectBeam"))
  {
    const std::string sampleFilename = getPropertyValue("BckTransmissionSampleDataFile");
    const std::string emptyFilename = getPropertyValue("BckTransmissionEmptyDataFile");
    const double beamRadius = getProperty("BckTransmissionBeamRadius");
    const double beamX = getProperty("BckTransmissionBeamCenterX");
    const double beamY = getProperty("BckTransmissionBeamCenterY");
    const bool thetaDependentTrans = getProperty("BckThetaDependentTransmission");

    IAlgorithm_sptr transAlg = createSubAlgorithm("SANSDirectBeamTransmission");
    transAlg->setProperty("SampleDataFilename", sampleFilename);
    transAlg->setProperty("EmptyDataFilename", emptyFilename);
    transAlg->setProperty("BeamRadius", beamRadius);

    // Beam center option for transmission data
    const std::string centerMethod = getPropertyValue("BckTransmissionBeamCenterMethod");
    if (boost::iequals(centerMethod, "Value") && !isEmpty(beamX) && !isEmpty(beamY))
    {
      transAlg->setProperty("BeamCenterX", beamX);
      transAlg->setProperty("BeamCenterY", beamY);
    }
    else if (boost::iequals(centerMethod, "DirectBeam"))
    {
      const std::string beamCenterFile = getProperty("BckTransmissionBeamCenterFile");
      if (beamCenterFile.size()>0)
       {
         IAlgorithm_sptr ctrAlg = createSubAlgorithm("SANSBeamFinder");
         ctrAlg->setProperty("Filename", beamCenterFile);
         ctrAlg->setProperty("UseDirectBeamMethod", true);
         ctrAlg->setProperty("PersistentCorrection", false);
         ctrAlg->setPropertyValue("ReductionProperties", reductionManagerName);

         AlgorithmProperty *algProp = new AlgorithmProperty("BckTransmissionBeamCenterAlgorithm");
         algProp->setValue(ctrAlg->toString());
         reductionManager->declareProperty(algProp);
       } else {
         g_log.error() << "ERROR: Beam center determination was required"
             " but no file was provided" << std::endl;
       }
    }
    transAlg->setProperty("ThetaDependent", thetaDependentTrans);
    AlgorithmProperty *algProp = new AlgorithmProperty("BckTransmissionAlgorithm");
    algProp->setValue(transAlg->toString());
    reductionManager->declareProperty(algProp);
  }
}

void SetupHFIRReduction::setupTransmission(boost::shared_ptr<PropertyManager> reductionManager)
{
  const std::string reductionManagerName = getProperty("ReductionProperties");
  // Transmission options
  const bool thetaDependentTrans = getProperty("ThetaDependentTransmission");
  const std::string transMethod = getProperty("TransmissionMethod");

  // Transmission is entered by hand
  if (boost::iequals(transMethod, "Value"))
  {
    const double transValue = getProperty("TransmissionValue");
    const double transError = getProperty("TransmissionError");
    if (!isEmpty(transValue) && !isEmpty(transError))
    {
      IAlgorithm_sptr transAlg = createSubAlgorithm("ApplyTransmissionCorrection");
      transAlg->setProperty("TransmissionValue", transValue);
      transAlg->setProperty("TransmissionError", transError);
      transAlg->setProperty("ThetaDependent", thetaDependentTrans);

      AlgorithmProperty *algProp = new AlgorithmProperty("TransmissionAlgorithm");
      algProp->setValue(transAlg->toString());
      reductionManager->declareProperty(algProp);
    } else {
      g_log.error("SetupHFIRReduction expected transmission/error values and got empty values");
    }
  }
  // Direct beam method for transmission determination
  else if (boost::iequals(transMethod, "DirectBeam"))
  {
    const std::string sampleFilename = getPropertyValue("TransmissionSampleDataFile");
    const std::string emptyFilename = getPropertyValue("TransmissionEmptyDataFile");
    const double beamRadius = getProperty("TransmissionBeamRadius");
    const double beamX = getProperty("TransmissionBeamCenterX");
    const double beamY = getProperty("TransmissionBeamCenterY");
    const std::string centerMethod = getPropertyValue("TransmissionBeamCenterMethod");

    IAlgorithm_sptr transAlg = createSubAlgorithm("SANSDirectBeamTransmission");
    transAlg->setProperty("SampleDataFilename", sampleFilename);
    transAlg->setProperty("EmptyDataFilename", emptyFilename);
    transAlg->setProperty("BeamRadius", beamRadius);

    // Beam center option for transmission data
    if (boost::iequals(centerMethod, "Value") && !isEmpty(beamX) && !isEmpty(beamY))
    {
      transAlg->setProperty("BeamCenterX", beamX);
      transAlg->setProperty("BeamCenterY", beamY);
    }
    else if (boost::iequals(centerMethod, "DirectBeam"))
    {
      const std::string beamCenterFile = getProperty("TransmissionBeamCenterFile");
      if (beamCenterFile.size()>0)
       {
         IAlgorithm_sptr ctrAlg = createSubAlgorithm("SANSBeamFinder");
         ctrAlg->setProperty("Filename", beamCenterFile);
         ctrAlg->setProperty("UseDirectBeamMethod", true);
         ctrAlg->setProperty("PersistentCorrection", false);
         ctrAlg->setPropertyValue("ReductionProperties", reductionManagerName);

         AlgorithmProperty *algProp = new AlgorithmProperty("TransmissionBeamCenterAlgorithm");
         algProp->setValue(ctrAlg->toString());
         reductionManager->declareProperty(algProp);
       } else {
         g_log.error() << "ERROR: Beam center determination was required"
             " but no file was provided" << std::endl;
       }
    }
    transAlg->setProperty("ThetaDependent", thetaDependentTrans);
    AlgorithmProperty *algProp = new AlgorithmProperty("TransmissionAlgorithm");
    algProp->setValue(transAlg->toString());
    reductionManager->declareProperty(algProp);
  }
}

} // namespace WorkflowAlgorithms
} // namespace Mantid

