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
  boost::shared_ptr<PropertyManager> m_reductionManager;
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

  // Find beam center
  double center_x = EMPTY_DBL();
  double center_y = EMPTY_DBL();

  if (m_reductionManager->existsProperty("BeamCenterAlgorithm") &&
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

     // Check the validity of the output
    if (isEmpty(center_x) || isEmpty(center_y) ||
        center_x != center_x || center_y != center_y)
    {
      throw std::logic_error("No valid beam center could be determined!");
    }

    // Store for later use
    m_reductionManager->declareProperty(new PropertyWithValue<double>("LatestBeamCenterX", center_x) );
    m_reductionManager->declareProperty(new PropertyWithValue<double>("LatestBeamCenterY", center_y) );
  }
  else
  {
    g_log.information() << "Beam center finding algorithm was not set: skipping" << std::endl;
  }

  setProperty("OutputMessage", "Beam finder completed");
}

} // namespace WorkflowAlgorithms
} // namespace Mantid

