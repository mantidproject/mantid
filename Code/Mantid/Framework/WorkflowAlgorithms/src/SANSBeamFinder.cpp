/*WIKI* 
Beam finder workflow algorithm for SANS instruments.

See [http://www.mantidproject.org/Reduction_for_HFIR_SANS SANS Reduction] documentation for details.

*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidWorkflowAlgorithms/SANSBeamFinder.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "Poco/Path.h"
#include "Poco/String.h"
#include "Poco/NumberFormatter.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/AlgorithmProperty.h"
#include "MantidAPI/PropertyManagerDataService.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidWorkflowAlgorithms/EQSANSInstrument.h"

namespace Mantid
{
namespace WorkflowAlgorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SANSBeamFinder)

/// Sets documentation strings for this algorithm
void SANSBeamFinder::initDocs()
{
  this->setWikiSummary("Beam finder workflow algorithm for SANS instruments.");
  this->setOptionalMessage("Beam finder workflow algorithm for SANS instruments.");
}

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

void SANSBeamFinder::init()
{
  std::vector<std::string> exts;
  exts.push_back("_event.nxs");
  exts.push_back(".xml");
  declareProperty(new API::FileProperty("Filename", "", API::FileProperty::Load, exts),
      "Data filed used to find beam center");

  declareProperty("BeamCenterX", EMPTY_DBL(), "Beam position in X pixel coordinates");
  declareProperty("BeamCenterY", EMPTY_DBL(), "Beam position in Y pixel coordinates");

  declareProperty("UseDirectBeamMethod", true, "If true, the direct beam method will be used");
  declareProperty("BeamRadius", 3.0, "Beam radius in pixels, used with the scattered beam method");

  declareProperty("ReductionProperties","__sans_reduction_properties", Direction::Input);
  declareProperty("OutputMessage","",Direction::Output);
}

MatrixWorkspace_sptr SANSBeamFinder::loadBeamFinderFile(const std::string& beamCenterFile)
{
  Poco::Path path(beamCenterFile);
  const std::string entryName = "SANSBeamFinder"+path.getBaseName();

  MatrixWorkspace_sptr finderWS;

  if (m_reductionManager->existsProperty(entryName))
  {
    finderWS = m_reductionManager->getProperty(entryName);
    m_output_message += "   |Using existing workspace: " + finderWS->name() + '\n';
  } else {
    // Load the dark current if we don't have it already
    std::string finderWSName = "__beam_finder_"+path.getBaseName();

    if (!m_reductionManager->existsProperty("LoadAlgorithm"))
    {
      IAlgorithm_sptr loadAlg = createSubAlgorithm("EQSANSLoad", 0.1, 0.3);
      loadAlg->setProperty("Filename", beamCenterFile);
      loadAlg->setProperty("NoBeamCenter", true);
      loadAlg->setProperty("BeamCenterX", EMPTY_DBL());
      loadAlg->setProperty("BeamCenterY", EMPTY_DBL());
      loadAlg->executeAsSubAlg();
      finderWS = loadAlg->getProperty("OutputWorkspace");
      const std::string outMsg = loadAlg->getProperty("OutputMessage");
      m_output_message += "   |Loaded " + beamCenterFile + "\n";
      std::string msg = loadAlg->getPropertyValue("OutputMessage");
      m_output_message += "   |" + Poco::replace(msg, "\n", "\n   |") + "\n";
    } else {
      IAlgorithm_sptr loadAlg = m_reductionManager->getProperty("LoadAlgorithm");
      loadAlg->setChild(true);
      loadAlg->setChildStartProgress(0.1);
      loadAlg->setChildEndProgress(0.3);
      loadAlg->setProperty("Filename", beamCenterFile);
      if (loadAlg->existsProperty("NoBeamCenter")) loadAlg->setProperty("NoBeamCenter", true);
      if (loadAlg->existsProperty("BeamCenterX")) loadAlg->setProperty("BeamCenterX", EMPTY_DBL());
      if (loadAlg->existsProperty("BeamCenterY")) loadAlg->setProperty("BeamCenterY", EMPTY_DBL());
      loadAlg->setPropertyValue("OutputWorkspace", finderWSName);
      loadAlg->execute();
      finderWS = loadAlg->getProperty("OutputWorkspace");

      m_output_message += "   |Loaded " + beamCenterFile + "\n";
      if (loadAlg->existsProperty("OutputMessage"))
      {
        std::string msg = loadAlg->getPropertyValue("OutputMessage");
        m_output_message += "   |" + Poco::replace(msg, "\n", "\n   |") + "\n";
      }
    }
    m_reductionManager->declareProperty(new WorkspaceProperty<>(entryName,"",Direction::Output));
    m_reductionManager->setPropertyValue(entryName, finderWSName);
    m_reductionManager->setProperty(entryName, finderWS);
  }
  return finderWS;
}

void SANSBeamFinder::exec()
{
  // Reduction property manager
  const std::string reductionManagerName = getProperty("ReductionProperties");
  if (PropertyManagerDataService::Instance().doesExist(reductionManagerName))
  {
    m_reductionManager = PropertyManagerDataService::Instance().retrieve(reductionManagerName);
  }
  else
  {
    m_reductionManager = boost::make_shared<PropertyManager>();
    PropertyManagerDataService::Instance().addOrReplace(reductionManagerName, m_reductionManager);
  }

  if (!m_reductionManager->existsProperty("SANSBeamFinderAlgorithm"))
  {
    AlgorithmProperty *algProp = new AlgorithmProperty("SANSBeamFinderAlgorithm");
    algProp->setValue(toString());
    m_reductionManager->declareProperty(algProp);
  }

  m_output_message = "Beam center determination\n";

  // Find beam center
  double center_x = getProperty("BeamCenterX");
  double center_y = getProperty("BeamCenterY");

  // If the beam enter was provided, simply add it to the reduction
  // property manager for other algorithms to find it
  if (!isEmpty(center_x) && !isEmpty(center_y))
  {
    m_output_message += "   |Using supplied beam center: ";
  }
  else
  {
    // Load direct beam file
    const std::string beamCenterFile = getProperty("Filename");
    g_log.information() << "beam center file: " << beamCenterFile << std::endl;
    MatrixWorkspace_sptr beamCenterWS = loadBeamFinderFile(beamCenterFile);

    IAlgorithm_sptr ctrAlg = createSubAlgorithm("FindCenterOfMassPosition");
    ctrAlg->setProperty("InputWorkspace", beamCenterWS);

    const bool directBeam = getProperty("UseDirectBeamMethod");
    ctrAlg->setProperty("DirectBeam", directBeam);

    double beamRadius = getProperty("BeamRadius");
    if (!directBeam && !isEmpty(beamRadius))
    {
      std::vector<double> pars = beamCenterWS->getInstrument()->getNumberParameter("x-pixel-size");
      if (pars.empty())
      {
        g_log.error() << "Could not read pixel size from instrument parameters: using default" << std::endl;
      }
      else
      {
        ctrAlg->setProperty("BeamRadius", beamRadius * pars[0] / 1000.0);
      }
    }
    ctrAlg->execute();
    std::vector<double> centerOfMass = ctrAlg->getProperty("CenterOfMass");
    EQSANSInstrument::getPixelFromCoordinate(centerOfMass[0], centerOfMass[1], beamCenterWS, center_x, center_y);
    m_output_message += "   |Found beam center: ";
  }

  if (false && m_reductionManager->existsProperty("BeamCenterAlgorithm") &&
      m_reductionManager->existsProperty("BeamCenterFile"))
  {
    // Load direct beam file
    const std::string beamCenterFile = m_reductionManager->getProperty("BeamCenterFile");
    MatrixWorkspace_sptr beamCenterWS = loadBeamFinderFile(beamCenterFile);

    IAlgorithm_sptr centerAlg = m_reductionManager->getProperty("BeamCenterAlgorithm");
    centerAlg->setChild(true);
    centerAlg->setChildStartProgress(0.4);
    centerAlg->setChildEndProgress(0.8);
    centerAlg->setProperty("InputWorkspace", beamCenterWS);
    centerAlg->execute();
    std::vector<double> centerOfMass = centerAlg->getProperty("CenterOfMass");
    EQSANSInstrument::getPixelFromCoordinate(centerOfMass[0], centerOfMass[1], beamCenterWS, center_x, center_y);
  }


  // Check the validity of the output
  if (isEmpty(center_x) || isEmpty(center_y) ||
      center_x != center_x || center_y != center_y)
  {
    throw std::logic_error("No valid beam center could be determined!");
  }

  // Store for later use
  if (!m_reductionManager->existsProperty("LatestBeamCenterX"))
    m_reductionManager->declareProperty(new PropertyWithValue<double>("LatestBeamCenterX", center_x) );
  else
    m_reductionManager->setProperty("LatestBeamCenterX", center_x);

  if (!m_reductionManager->existsProperty("LatestBeamCenterY"))
    m_reductionManager->declareProperty(new PropertyWithValue<double>("LatestBeamCenterY", center_y) );
  else
    m_reductionManager->setProperty("LatestBeamCenterY", center_y);

  m_output_message += "[" + Poco::NumberFormatter::format(center_x, 1) + ", "
      + Poco::NumberFormatter::format(center_y, 1) + "]\n";

  setProperty("OutputMessage", m_output_message);
}

} // namespace WorkflowAlgorithms
} // namespace Mantid

