//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidWorkflowAlgorithms/SANSSensitivityCorrection.h"
#include "MantidWorkflowAlgorithms/ReductionTableHandler.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/TableRow.h"
#include "Poco/File.h"
#include "Poco/Path.h"
#include "Poco/String.h"
#include "MantidAPI/AlgorithmManager.h"

namespace Mantid
{
namespace WorkflowAlgorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SANSSensitivityCorrection)

/// Sets documentation strings for this algorithm
void SANSSensitivityCorrection::initDocs()
{
  this->setWikiSummary("Perform SANS sensitivity correction.");
  this->setOptionalMessage("Perform SANS sensitivity correction.");
}

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

void SANSSensitivityCorrection::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input));

  declareProperty(new API::FileProperty("Filename", "", API::FileProperty::Load, ".nxs"),
      "Flood field or sensitivity file.");
  declareProperty("UseSampleDC", true, "If true, the dark current subtracted from the sample data will also be subtracted from the flood field.");
  declareProperty(new API::FileProperty("DarkCurrentFile", "", API::FileProperty::OptionalLoad, ".nxs"),
      "The name of the input file to load as dark current.");

  BoundedValidator<double> *positiveDouble = new BoundedValidator<double>();
  positiveDouble->setLower(0);
  declareProperty("MinEfficiency", EMPTY_DBL(), positiveDouble,
      "Minimum efficiency for a pixel to be considered (default: no minimum).");
  declareProperty("MaxEfficiency", EMPTY_DBL(), positiveDouble->clone(),
      "Maximum efficiency for a pixel to be considered (default: no maximum).");

  declareProperty("BeamCenterX", EMPTY_DBL(), "Beam position in X pixel coordinates (optional: otherwise sample beam center is used)");
  declareProperty("BeamCenterY", EMPTY_DBL(), "Beam position in Y pixel coordinates (optional: otherwise sample beam center is used)");

  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));
  declareProperty(new WorkspaceProperty<TableWorkspace>("ReductionTableWorkspace","", Direction::Output, true));
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputSensitivityWorkspace","", Direction::Output, true));
  declareProperty("OutputMessage","",Direction::Output);

}

void SANSSensitivityCorrection::exec()
{
  // Output log
  m_output_message = "";

  Progress progress(this,0.0,1.0,10);

  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  const std::string fileName = getPropertyValue("Filename");

  // Get the reduction table workspace or create one
  TableWorkspace_sptr reductionTable = getProperty("ReductionTableWorkspace");
  ReductionTableHandler reductionHandler(reductionTable);
  if (!reductionTable)
  {
    const std::string reductionTableName = getPropertyValue("ReductionTableWorkspace");
    if (reductionTableName.size()>0) setProperty("ReductionTableWorkspace", reductionHandler.getTable());
  }
  if (reductionHandler.findStringEntry("SensitivityAlgorithm").size()==0)
    reductionHandler.addEntry("SensitivityAlgorithm", toString());

  progress.report("Loading sensitivity file");

  // Look for an entry for the dark current in the reduction table
  Poco::Path path(fileName);
  const std::string entryName = "Sensitivity"+path.getBaseName();
  MatrixWorkspace_sptr floodWS = reductionHandler.findWorkspaceEntry(entryName);
  std::string floodWSName = reductionHandler.findStringEntry(entryName);

  if (floodWSName.size()==0) {
    floodWSName = getPropertyValue("OutputSensitivityWorkspace");
    if (floodWSName.size()==0)
      floodWSName = "__sensitivity_"+path.getBaseName();
    setPropertyValue("OutputSensitivityWorkspace", floodWSName);
    reductionHandler.addEntry(entryName, floodWSName);
  }

  // Load the flood field if we don't have it already
  if (!floodWS)
  {
    std::string loader = reductionHandler.findStringEntry("LoadAlgorithm");
    // Read in default beam center
    double center_x = getProperty("BeamCenterX");
    double center_y = getProperty("BeamCenterY");
    if (isEmpty(center_x) || isEmpty(center_y))
    {
      center_x = reductionHandler.findDoubleEntry("LatestBeamCenterX");
      center_y = reductionHandler.findDoubleEntry("LatestBeamCenterY");
    }

    const std::string rawFloodWSName = "__flood_data_"+path.getBaseName();
    MatrixWorkspace_sptr rawFloodWS;
    if (loader.size()==0)
    {
      IAlgorithm_sptr loadAlg = createSubAlgorithm("Load", 0.1, 0.3);
      loadAlg->setProperty("Filename", fileName);
      if (!isEmpty(center_x)) loadAlg->setProperty("BeamCenterX", center_x);
      if (!isEmpty(center_y)) loadAlg->setProperty("BeamCenterY", center_y);
      loadAlg->executeAsSubAlg();
      rawFloodWS = loadAlg->getProperty("OutputWorkspace");
      m_output_message += "   | Loaded " + fileName + "\n";
    } else {
      IAlgorithm_sptr loadAlg = Algorithm::fromString(loader);
      loadAlg->setChild(true);
      loadAlg->setProperty("Filename", fileName);
      loadAlg->setPropertyValue("OutputWorkspace", rawFloodWSName);
      if (!isEmpty(center_x) && loadAlg->existsProperty("BeamCenterX")) loadAlg->setProperty("BeamCenterX", center_x);
      if (!isEmpty(center_y) && loadAlg->existsProperty("BeamCenterY")) loadAlg->setProperty("BeamCenterY", center_y);
      loadAlg->execute();
      rawFloodWS = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(rawFloodWSName));
      m_output_message += "   |Loaded " + fileName + "\n";
      if (loadAlg->existsProperty("OutputMessage")) 
      {
	      std::string msg = loadAlg->getPropertyValue("OutputMessage");
  	    m_output_message += "   |" + Poco::replace(msg, "\n", "\n   |") + "\n";
  	  }
    }

    // Check whether we just loaded a flood field data set, or the actual sensitivity
    if (!rawFloodWS->run().hasProperty("is_sensitivity"))
    {
      const std::string darkCurrentFile = getPropertyValue("DarkCurrentFile");

      // Look for a dark current subtraction algorithm
      std::string dark_current = reductionHandler.findStringEntry("DarkCurrentAlgorithm");
      std::string dark_result = "";
      if (dark_current.size()>0)
      {
        IAlgorithm_sptr darkAlg = Algorithm::fromString(dark_current);
        darkAlg->setChild(true);
        darkAlg->setProperty("InputWorkspace", rawFloodWS);
        darkAlg->setProperty("OutputWorkspace", rawFloodWS);

        // Execute as-is if we use the sample dark current, otherwise check
        // whether a dark current file was provided.
        // Otherwise do nothing
        if (getProperty("UseSampleDC"))
        {
          darkAlg->execute();
          if (darkAlg->existsProperty("OutputMessage")) dark_result = darkAlg->getPropertyValue("OutputMessage");
        }
        else if (darkCurrentFile.size()>0)
        {
          darkAlg->setProperty("Filename", darkCurrentFile);
          darkAlg->execute();
          if (darkAlg->existsProperty("OutputMessage")) dark_result = darkAlg->getPropertyValue("OutputMessage");
        }
      } else if (darkCurrentFile.size()>0)
      {
        g_log.error() << "No dark current algorithm provided to load ["
            << getPropertyValue("DarkCurrentFile") << "]: skipped!" << std::endl;
        dark_result = "   No dark current algorithm provided: skipped\n";
      }
      m_output_message += "   |" + Poco::replace(dark_result, "\n", "\n   |") + "\n";

      // Look for solid angle correction algorithm
      std::string solid_angle = reductionHandler.findStringEntry("SolidAngleAlgorithm");
      if (solid_angle.size()>0)
      {
        IAlgorithm_sptr solidAlg = Algorithm::fromString(solid_angle);
        solidAlg->setChild(true);
        solidAlg->setProperty("InputWorkspace", rawFloodWS);
        solidAlg->setProperty("OutputWorkspace", rawFloodWS);
        solidAlg->execute();
        std::string msg = "Solid angle correction applied\n";
        if (solidAlg->existsProperty("OutputMessage")) msg = solidAlg->getPropertyValue("OutputMessage");
        m_output_message += "   |" + Poco::replace(msg, "\n", "\n   |") + "\n";
      }

      // Calculate detector sensitivity
      IAlgorithm_sptr effAlg = createSubAlgorithm("CalculateEfficiency");
      effAlg->setProperty("InputWorkspace", rawFloodWS);

      const double minEff = getProperty("MinEfficiency");
      const double maxEff = getProperty("MaxEfficiency");
      effAlg->setProperty("MinEfficiency", minEff);
      effAlg->setProperty("MaxEfficiency", maxEff);
      effAlg->execute();
      floodWS = effAlg->getProperty("OutputWorkspace");
    } else {
      floodWS = rawFloodWS;
    }

    floodWS->mutableRun().addProperty("is_sensitivity", 1, "", true);
    setProperty("OutputSensitivityWorkspace", floodWS);
  }
  progress.report(3, "Loaded flood field");

  // Divide sample data by detector efficiency
  IAlgorithm_sptr minusAlg = createSubAlgorithm("Divide", 0.6, 0.7);
  minusAlg->setProperty("LHSWorkspace", inputWS);
  minusAlg->setProperty("RHSWorkspace", floodWS);
  minusAlg->executeAsSubAlg();
  MatrixWorkspace_sptr outputWS = minusAlg->getProperty("OutputWorkspace");

  // Copy over the efficiency's masked pixels to the reduced workspace
  IAlgorithm_sptr getMaskAlg = createSubAlgorithm("GetMaskedDetectors", 0.7, 0.75);
  getMaskAlg->setProperty("InputWorkspace", floodWS);
  getMaskAlg->executeAsSubAlg();

  IAlgorithm_sptr maskAlg = createSubAlgorithm("MaskDetectors", 0.75, 0.85);
  maskAlg->setProperty("Workspace", outputWS);
  maskAlg->setPropertyValue("DetectorList", getMaskAlg->getPropertyValue("DetectorList"));
  maskAlg->executeAsSubAlg();

  setProperty("OutputWorkspace", outputWS);
  setProperty("OutputMessage", "Sensitivity correction applied\n"+m_output_message);

  progress.report("Performed sensitivity correction");
}

} // namespace WorkflowAlgorithms
} // namespace Mantid

