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
#include "MantidWorkflowAlgorithms/HFIRInstrument.h"

namespace Mantid {
namespace WorkflowAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SANSBeamFinder)

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

void SANSBeamFinder::init() {
  std::vector<std::string> exts;
  exts.push_back("_event.nxs");
  exts.push_back(".xml");
  declareProperty(
      new API::FileProperty("Filename", "", API::FileProperty::Load, exts),
      "Data filed used to find beam center");

  declareProperty("BeamCenterX", EMPTY_DBL(),
                  "Beam position in X pixel coordinates");
  declareProperty("BeamCenterY", EMPTY_DBL(),
                  "Beam position in Y pixel coordinates");

  declareProperty("UseDirectBeamMethod", true,
                  "If true, the direct beam method will be used");
  declareProperty("BeamRadius", 3.0,
                  "Beam radius in pixels, used with the scattered beam method");

  declareProperty("FoundBeamCenterX", EMPTY_DBL(), Direction::Output);
  declareProperty("FoundBeamCenterY", EMPTY_DBL(), Direction::Output);

  declareProperty("PersistentCorrection", true,
                  "If true, the algorithm will be persistent and re-used when "
                  "other data sets are processed");
  declareProperty("ReductionProperties", "__sans_reduction_properties",
                  Direction::Input);
  declareProperty("OutputMessage", "", Direction::Output);
}

MatrixWorkspace_sptr
SANSBeamFinder::loadBeamFinderFile(const std::string &beamCenterFile) {
  Poco::Path path(beamCenterFile);
  const std::string entryName = "SANSBeamFinder" + path.getBaseName();
  const std::string reductionManagerName = getProperty("ReductionProperties");

  MatrixWorkspace_sptr finderWS;

  if (m_reductionManager->existsProperty(entryName)) {
    finderWS = m_reductionManager->getProperty(entryName);
    m_output_message +=
        "   |Using existing workspace: " + finderWS->name() + '\n';
  } else {
    // Load the dark current if we don't have it already
    std::string finderWSName = "__beam_finder_" + path.getBaseName();

    if (!m_reductionManager->existsProperty("LoadAlgorithm")) {
      IAlgorithm_sptr loadAlg = createChildAlgorithm("EQSANSLoad", 0.1, 0.3);
      loadAlg->setProperty("Filename", beamCenterFile);
      loadAlg->setProperty("NoBeamCenter", true);
      loadAlg->setProperty("BeamCenterX", EMPTY_DBL());
      loadAlg->setProperty("BeamCenterY", EMPTY_DBL());
      loadAlg->setProperty("ReductionProperties", reductionManagerName);
      loadAlg->executeAsChildAlg();
      finderWS = loadAlg->getProperty("OutputWorkspace");
      m_output_message += "   |Loaded " + beamCenterFile + "\n";
      std::string msg = loadAlg->getPropertyValue("OutputMessage");
      m_output_message += "   |" + Poco::replace(msg, "\n", "\n   |") + "\n";
    } else {
      // Get load algorithm as a string so that we can create a completely
      // new proxy and ensure that we don't overwrite existing properties
      IAlgorithm_sptr loadAlg0 =
          m_reductionManager->getProperty("LoadAlgorithm");
      const std::string loadString = loadAlg0->toString();
      IAlgorithm_sptr loadAlg = Algorithm::fromString(loadString);

      loadAlg->setProperty("Filename", beamCenterFile);
      if (loadAlg->existsProperty("NoBeamCenter"))
        loadAlg->setProperty("NoBeamCenter", true);
      if (loadAlg->existsProperty("BeamCenterX"))
        loadAlg->setProperty("BeamCenterX", EMPTY_DBL());
      if (loadAlg->existsProperty("BeamCenterY"))
        loadAlg->setProperty("BeamCenterY", EMPTY_DBL());
      if (loadAlg->existsProperty("ReductionProperties"))
        loadAlg->setProperty("ReductionProperties", reductionManagerName);
      loadAlg->setPropertyValue("OutputWorkspace", finderWSName);
      loadAlg->execute();
      boost::shared_ptr<Workspace> wks =
          AnalysisDataService::Instance().retrieve(finderWSName);
      finderWS = boost::dynamic_pointer_cast<MatrixWorkspace>(wks);

      m_output_message += "   |Loaded " + beamCenterFile + "\n";
      if (loadAlg->existsProperty("OutputMessage")) {
        std::string msg = loadAlg->getPropertyValue("OutputMessage");
        m_output_message += "   |" + Poco::replace(msg, "\n", "\n   |") + "\n";
      }
    }
    m_reductionManager->declareProperty(
        new WorkspaceProperty<>(entryName, "", Direction::Output));
    m_reductionManager->setPropertyValue(entryName, finderWSName);
    m_reductionManager->setProperty(entryName, finderWS);
  }
  return finderWS;
}

void SANSBeamFinder::exec() {
  // Reduction property manager
  const std::string reductionManagerName = getProperty("ReductionProperties");
  if (PropertyManagerDataService::Instance().doesExist(reductionManagerName)) {
    m_reductionManager =
        PropertyManagerDataService::Instance().retrieve(reductionManagerName);
  } else {
    m_reductionManager = boost::make_shared<PropertyManager>();
    PropertyManagerDataService::Instance().addOrReplace(reductionManagerName,
                                                        m_reductionManager);
  }

  const bool persistent = getProperty("PersistentCorrection");
  if (!m_reductionManager->existsProperty("SANSBeamFinderAlgorithm") &&
      persistent) {
    AlgorithmProperty *algProp =
        new AlgorithmProperty("SANSBeamFinderAlgorithm");
    algProp->setValue(toString());
    m_reductionManager->declareProperty(algProp);
  }

  m_output_message = "Beam center determination\n";

  // Pixel coordinate to real-space coordinate mapping scheme
  bool specialMapping = false;
  if (m_reductionManager->existsProperty("InstrumentName")) {
    const std::string instrumentName =
        m_reductionManager->getPropertyValue("InstrumentName");
    specialMapping = instrumentName.compare("HFIRSANS") == 0;
  }

  // Find beam center
  double center_x = getProperty("BeamCenterX");
  double center_y = getProperty("BeamCenterY");

  // Check whether we already know the position
  const std::string beamCenterFile = getProperty("Filename");
  Poco::Path path(beamCenterFile);
  const std::string entryNameX = "SANSBeamFinder_X_" + path.getBaseName();
  const std::string entryNameY = "SANSBeamFinder_Y_" + path.getBaseName();

  // If the beam enter was provided, simply add it to the reduction
  // property manager for other algorithms to find it
  if (!isEmpty(center_x) && !isEmpty(center_y)) {
    m_output_message += "   |Using supplied beam center: ";
  } else if (m_reductionManager->existsProperty(entryNameX) &&
             m_reductionManager->existsProperty(entryNameY)) {
    center_x = m_reductionManager->getProperty(entryNameX);
    center_y = m_reductionManager->getProperty(entryNameY);
  } else {
    // Load the beam center file
    MatrixWorkspace_sptr beamCenterWS = loadBeamFinderFile(beamCenterFile);

    // HFIR reduction masks the first pixels on each edge of the detector
    if (specialMapping)
      maskEdges(beamCenterWS, 1, 1, 1, 1);

    IAlgorithm_sptr ctrAlg = createChildAlgorithm("FindCenterOfMassPosition");
    ctrAlg->setProperty("InputWorkspace", beamCenterWS);

    const bool directBeam = getProperty("UseDirectBeamMethod");
    ctrAlg->setProperty("DirectBeam", directBeam);

    double beamRadius = getProperty("BeamRadius");
    if (!directBeam && !isEmpty(beamRadius)) {
      std::vector<double> pars =
          beamCenterWS->getInstrument()->getNumberParameter("x-pixel-size");
      if (pars.empty()) {
        g_log.error() << "Could not read pixel size from instrument "
                         "parameters: using default" << std::endl;
      } else {
        ctrAlg->setProperty("BeamRadius", beamRadius * pars[0] / 1000.0);
      }
    }
    ctrAlg->execute();
    std::vector<double> centerOfMass = ctrAlg->getProperty("CenterOfMass");

    if (specialMapping) {
      HFIRInstrument::getPixelFromCoordinate(centerOfMass[0], centerOfMass[1],
                                             beamCenterWS, center_x, center_y);
    } else {
      EQSANSInstrument::getPixelFromCoordinate(
          centerOfMass[0], centerOfMass[1], beamCenterWS, center_x, center_y);
    }

    m_output_message += "   |Found beam center: ";
  }

  // Store for later use
  if (persistent) {
    if (!m_reductionManager->existsProperty("LatestBeamCenterX"))
      m_reductionManager->declareProperty(
          new PropertyWithValue<double>("LatestBeamCenterX", center_x));
    if (!m_reductionManager->existsProperty("LatestBeamCenterY"))
      m_reductionManager->declareProperty(
          new PropertyWithValue<double>("LatestBeamCenterY", center_y));

    m_reductionManager->setProperty("LatestBeamCenterX", center_x);
    m_reductionManager->setProperty("LatestBeamCenterY", center_y);
  }

  m_output_message += "[" + Poco::NumberFormatter::format(center_x, 3) + ", " +
                      Poco::NumberFormatter::format(center_y, 3) + "]\n";

  // Workflow algorithms can use the LatestBeamCenterX/Y entries, but to be
  // compatible with the old ReductionSteps we also set output properties
  // with the beam center position
  setProperty("FoundBeamCenterX", center_x);
  setProperty("FoundBeamCenterY", center_y);
  setProperty("OutputMessage", m_output_message);
}

/*
 * The standard HFIR reduction masks the edges of the detector
 * This is here mostly to allow a direct comparison with old HFIR code
 * and ensure that we reproduce the same results
 */
void SANSBeamFinder::maskEdges(MatrixWorkspace_sptr beamCenterWS, int high,
                               int low, int left, int right) {
  const int nx_pixels = (int)(HFIRInstrument::readInstrumentParameter(
      "number-of-x-pixels", beamCenterWS));
  const int ny_pixels = (int)(HFIRInstrument::readInstrumentParameter(
      "number-of-y-pixels", beamCenterWS));
  std::vector<int> IDs;

  // Lower edge
  for (int iy = 0; iy < low; iy++) {
    for (int ix = 0; ix < nx_pixels; ix++) {
      // Note that ix and iy are inverted. The HFIR reference frame is flipped
      // relative to Mantid.
      int id = HFIRInstrument::getDetectorFromPixel(iy, ix, beamCenterWS);
      IDs.push_back(id);
    }
  }

  // Upper edge
  for (int iy = ny_pixels - high; iy < ny_pixels; iy++) {
    for (int ix = 0; ix < nx_pixels; ix++) {
      int id = HFIRInstrument::getDetectorFromPixel(iy, ix, beamCenterWS);
      IDs.push_back(id);
    }
  }

  // Left edge
  for (int iy = 0; iy < ny_pixels; iy++) {
    for (int ix = 0; ix < left; ix++) {
      int id = HFIRInstrument::getDetectorFromPixel(iy, ix, beamCenterWS);
      IDs.push_back(id);
    }
  }

  // Right edge
  for (int iy = 0; iy < ny_pixels; iy++) {
    for (int ix = nx_pixels - right; ix < nx_pixels; ix++) {
      int id = HFIRInstrument::getDetectorFromPixel(iy, ix, beamCenterWS);
      IDs.push_back(id);
    }
  }

  IAlgorithm_sptr maskAlg = createChildAlgorithm("MaskDetectors");
  maskAlg->setProperty("Workspace", beamCenterWS);
  maskAlg->setProperty("DetectorList", IDs);
  maskAlg->execute();
}

} // namespace WorkflowAlgorithms
} // namespace Mantid
