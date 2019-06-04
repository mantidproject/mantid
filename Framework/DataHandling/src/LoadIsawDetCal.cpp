// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadIsawDetCal.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MultipleFileProperty.h"
#include "MantidAPI/Run.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"

#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ObjCompAssembly.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"

#include "MantidKernel/Strings.h"
#include "MantidKernel/V3D.h"

#include <algorithm>
#include <boost/algorithm/string/trim.hpp>
#include <fstream>
#include <numeric>
#include <sstream>

namespace Mantid {
namespace DataHandling {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(LoadIsawDetCal)

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

/** Initialisation method
 */
void LoadIsawDetCal::init() {
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(
                      "InputWorkspace", "", Direction::InOut,
                      boost::make_shared<InstrumentValidator>()),
                  "The workspace containing the geometry to be calibrated.");

  const auto exts =
      std::vector<std::string>({".DetCal", ".detcal", ".peaks", ".integrate"});
  declareProperty(
      std::make_unique<API::MultipleFileProperty>("Filename", exts),
      "The input filename of the ISAW DetCal file (Two files "
      "allowed for SNAP) ");

  declareProperty(std::make_unique<API::FileProperty>(
                      "Filename2", "", API::FileProperty::OptionalLoad, exts),
                  "The input filename of the second ISAW DetCal file (West "
                  "banks for SNAP) ");

  declareProperty("TimeOffset", 0.0, "Time Offset", Direction::Output);
}

namespace {
const constexpr double DegreesPerRadian = 180.0 / M_PI;

std::string getBankName(const std::string &bankPart, const int idnum) {
  if (bankPart == "WISHpanel" && idnum < 10) {
    return bankPart + "0" + std::to_string(idnum);
  } else {
    return bankPart + std::to_string(idnum);
  }
}

std::string getInstName(API::Workspace_const_sptr wksp) {
  MatrixWorkspace_const_sptr matrixWksp =
      boost::dynamic_pointer_cast<const MatrixWorkspace>(wksp);
  if (matrixWksp) {
    return matrixWksp->getInstrument()->getName();
  }

  PeaksWorkspace_const_sptr peaksWksp =
      boost::dynamic_pointer_cast<const PeaksWorkspace>(wksp);
  if (peaksWksp) {
    return peaksWksp->getInstrument()->getName();
  }

  throw std::runtime_error("Failed to determine instrument name");
}
} // namespace

std::map<std::string, std::string> LoadIsawDetCal::validateInputs() {
  std::map<std::string, std::string> result;

  // two detcal files is only valid for snap
  std::vector<std::string> filenames = getFilenames();
  if (filenames.empty()) {
    result["Filename"] = "Must supply .detcal file";
  } else if (filenames.size() == 2) {
    Workspace_const_sptr wksp = getProperty("InputWorkspace");
    const auto instname = getInstName(wksp);

    if (instname != "SNAP") {
      result["Filename"] = "Two files is only valid for SNAP";
    }
  } else if (filenames.size() > 2) {
    result["Filename"] = "Supply at most two .detcal files";
  }

  return result;
}

/** Executes the algorithm
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void LoadIsawDetCal::exec() {
  // Get the input workspace
  Workspace_sptr ws = getProperty("InputWorkspace");
  MatrixWorkspace_sptr inputW =
      boost::dynamic_pointer_cast<MatrixWorkspace>(ws);
  PeaksWorkspace_sptr inputP = boost::dynamic_pointer_cast<PeaksWorkspace>(ws);

  Instrument_sptr inst = getCheckInst(ws);

  std::string instname = inst->getName();

  const auto filenames = getFilenames();

  // Output summary to log file
  int count, id, nrows, ncols;
  double width, height, depth, detd, x, y, z, base_x, base_y, base_z, up_x,
      up_y, up_z;
  std::ifstream input(filenames[0].c_str(), std::ios_base::in);
  std::string line;
  std::string detname;
  // Build a list of Rectangular Detectors
  std::vector<boost::shared_ptr<RectangularDetector>> detList;
  for (int i = 0; i < inst->nelements(); i++) {
    boost::shared_ptr<RectangularDetector> det;
    boost::shared_ptr<ICompAssembly> assem;
    boost::shared_ptr<ICompAssembly> assem2;

    det = boost::dynamic_pointer_cast<RectangularDetector>((*inst)[i]);
    if (det) {
      detList.push_back(det);
    } else {
      // Also, look in the first sub-level for RectangularDetectors (e.g. PG3).
      // We are not doing a full recursive search since that will be very long
      // for lots of pixels.
      assem = boost::dynamic_pointer_cast<ICompAssembly>((*inst)[i]);
      if (assem) {
        for (int j = 0; j < assem->nelements(); j++) {
          det = boost::dynamic_pointer_cast<RectangularDetector>((*assem)[j]);
          if (det) {
            detList.push_back(det);

          } else {
            // Also, look in the second sub-level for RectangularDetectors (e.g.
            // PG3).
            // We are not doing a full recursive search since that will be very
            // long for lots of pixels.
            assem2 = boost::dynamic_pointer_cast<ICompAssembly>((*assem)[j]);
            if (assem2) {
              for (int k = 0; k < assem2->nelements(); k++) {
                det = boost::dynamic_pointer_cast<RectangularDetector>(
                    (*assem2)[k]);
                if (det) {
                  detList.push_back(det);
                }
              }
            }
          }
        }
      }
    }
  }
  std::unordered_set<int> uniqueBanks; // for CORELLI and WISH
  std::string bankPart = "bank";
  if (instname == "WISH")
    bankPart = "WISHpanel";
  if (detList.empty()) {
    // Get all children
    std::vector<IComponent_const_sptr> comps;
    inst->getChildren(comps, true);

    for (auto &comp : comps) {
      std::string bankName = comp->getName();
      boost::trim(bankName);
      boost::erase_all(bankName, bankPart);
      int bank = 0;
      Strings::convert(bankName, bank);
      if (bank == 0)
        continue;
      // Track unique bank numbers
      uniqueBanks.insert(bank);
    }
  }

  auto expInfoWS = boost::dynamic_pointer_cast<ExperimentInfo>(ws);
  auto &componentInfo = expInfoWS->mutableComponentInfo();
  std::vector<ComponentScaling> rectangularDetectorScalings;

  while (std::getline(input, line)) {
    if (line[0] == '7') {
      double mL1, mT0;
      std::stringstream(line) >> count >> mL1 >> mT0;
      setProperty("TimeOffset", mT0);
      // Convert from cm to m
      if (instname == "WISH")
        center(0.0, 0.0, -mL1, "undulator", ws, componentInfo);
      else
        center(0.0, 0.0, -mL1, "moderator", ws, componentInfo);
      // mT0 and time of flight are both in microsec
      if (mT0 != 0.0) {
        if (inputW) {
          API::Run &run = inputW->mutableRun();
          // Check to see if LoadEventNexus had T0 from TOPAZ Parameter file
          IAlgorithm_sptr alg1 = createChildAlgorithm("ChangeBinOffset");
          alg1->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputW);
          alg1->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", inputW);
          if (run.hasProperty("T0")) {
            double T0IDF = run.getPropertyValueAsType<double>("T0");
            alg1->setProperty("Offset", mT0 - T0IDF);
          } else {
            alg1->setProperty("Offset", mT0);
          }
          alg1->executeAsChildAlg();
          inputW = alg1->getProperty("OutputWorkspace");
          // set T0 in the run parameters
          run.addProperty<double>("T0", mT0, true);
        } else if (inputP) {
          // set T0 in the run parameters
          API::Run &run = inputP->mutableRun();
          run.addProperty<double>("T0", mT0, true);
        }
      }
    }

    if (line[0] != '5')
      continue;

    std::stringstream(line) >> count >> id >> nrows >> ncols >> width >>
        height >> depth >> detd >> x >> y >> z >> base_x >> base_y >> base_z >>
        up_x >> up_y >> up_z;
    if (id == 10 && filenames.size() == 2 && instname == "SNAP") {
      input.close();
      input.open(filenames[1].c_str());
      while (std::getline(input, line)) {
        if (line[0] != '5')
          continue;

        std::stringstream(line) >> count >> id >> nrows >> ncols >> width >>
            height >> depth >> detd >> x >> y >> z >> base_x >> base_y >>
            base_z >> up_x >> up_y >> up_z;
        if (id == 10)
          break;
      }
    }
    boost::shared_ptr<RectangularDetector> det;
    std::string bankName = getBankName(bankPart, id);
    auto matchingDetector = std::find_if(
        detList.begin(), detList.end(),
        [&bankName](const boost::shared_ptr<RectangularDetector> &detector) {
          return detector->getName() == bankName;
        });
    if (matchingDetector != detList.end()) {
      det = *matchingDetector;
    }

    V3D rX(base_x, base_y, base_z);
    V3D rY(up_x, up_y, up_z);

    if (det) {
      detname = det->getName();
      center(x, y, z, detname, ws, componentInfo);

      ComponentScaling detScaling;
      detScaling.scaleX = CM_TO_M * width / det->xsize();
      detScaling.scaleY = CM_TO_M * height / det->ysize();
      detScaling.componentName = detname;
      // Scaling will need both scale factors if LoadIsawPeaks or LoadIsawDetCal
      // has already
      // applied a calibration
      if (inputW) {
        Geometry::ParameterMap &pmap = inputW->instrumentParameters();
        auto oldscalex = pmap.getDouble(detname, std::string("scalex"));
        auto oldscaley = pmap.getDouble(detname, std::string("scaley"));
        if (!oldscalex.empty())
          detScaling.scaleX *= oldscalex[0];
        if (!oldscaley.empty())
          detScaling.scaleY *= oldscaley[0];
      }
      if (inputP) {
        Geometry::ParameterMap &pmap = inputP->instrumentParameters();
        auto oldscalex = pmap.getDouble(detname, std::string("scalex"));
        auto oldscaley = pmap.getDouble(detname, std::string("scaley"));
        if (!oldscalex.empty())
          detScaling.scaleX *= oldscalex[0];
        if (!oldscaley.empty())
          detScaling.scaleY *= oldscaley[0];
      }

      rectangularDetectorScalings.push_back(detScaling);

      doRotation(rX, rY, componentInfo, det);
    }
    auto bank = uniqueBanks.find(id);
    if (bank == uniqueBanks.end())
      continue;
    int idnum = *bank;

    bankName = getBankName(bankPart, idnum);
    // Retrieve it
    auto comp = inst->getComponentByName(bankName);
    // for Corelli with sixteenpack under bank
    if (instname == "CORELLI") {
      std::vector<Geometry::IComponent_const_sptr> children;
      boost::shared_ptr<const Geometry::ICompAssembly> asmb =
          boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(
              inst->getComponentByName(bankName));
      asmb->getChildren(children, false);
      comp = children[0];
    }
    if (comp) {
      // Omitted scaling tubes
      detname = comp->getFullName();
      center(x, y, z, detname, ws, componentInfo);

      bool doWishCorrection =
          (instname == "WISH"); // TODO: find out why this is needed for WISH
      doRotation(rX, rY, componentInfo, comp, doWishCorrection);
    }
  }

  // Do this last, to avoid the issue of invalidating DetectorInfo
  applyScalings(ws, rectangularDetectorScalings);

  setProperty("InputWorkspace", ws);
}

/**
 * The intensity function calculates the intensity as a function of detector
 * position and angles
 * @param x :: The shift along the X-axis
 * @param y :: The shift along the Y-axis
 * @param z :: The shift along the Z-axis
 * @param detname :: The detector name
 * @param ws :: The workspace
 * @param componentInfo :: The component info object for the workspace
 */
void LoadIsawDetCal::center(const double x, const double y, const double z,
                            const std::string &detname, API::Workspace_sptr ws,
                            Geometry::ComponentInfo &componentInfo) {

  Instrument_sptr inst = getCheckInst(ws);

  IComponent_const_sptr comp = inst->getComponentByName(detname);
  if (comp == nullptr) {
    throw std::runtime_error("Component with name " + detname +
                             " was not found.");
  }

  const V3D position(x * CM_TO_M, y * CM_TO_M, z * CM_TO_M);

  const auto componentIndex = componentInfo.indexOf(comp->getComponentID());
  componentInfo.setPosition(componentIndex, position);
}

/**
 * Gets the instrument of the workspace, checking that the workspace
 * and the instrument are as expected.
 *
 * @param ws workspace (expected Matrix or Peaks Workspace)
 *
 * @throw std::runtime_error if there's any problem with the workspace or it is
 * not possible to get an instrument object from it
 */
Instrument_sptr LoadIsawDetCal::getCheckInst(API::Workspace_sptr ws) {
  MatrixWorkspace_sptr inputW =
      boost::dynamic_pointer_cast<MatrixWorkspace>(ws);
  PeaksWorkspace_sptr inputP = boost::dynamic_pointer_cast<PeaksWorkspace>(ws);

  // Get some stuff from the input workspace
  Instrument_sptr inst;
  if (inputW) {
    inst = boost::const_pointer_cast<Instrument>(inputW->getInstrument());
    if (!inst)
      throw std::runtime_error("Could not get a valid instrument from the "
                               "MatrixWorkspace provided as input");
  } else if (inputP) {
    inst = boost::const_pointer_cast<Instrument>(inputP->getInstrument());
    if (!inst)
      throw std::runtime_error("Could not get a valid instrument from the "
                               "PeaksWorkspace provided as input");
  } else {
    throw std::runtime_error("Could not get a valid instrument from the "
                             "workspace which does not seem to be valid as "
                             "input (must be either MatrixWorkspace or "
                             "PeaksWorkspace");
  }

  return inst;
}

std::vector<std::string> LoadIsawDetCal::getFilenames() {
  std::vector<std::string> filenamesFromPropertyUnraveld;
  std::vector<std::vector<std::string>> filenamesFromProperty =
      this->getProperty("Filename");
  for (const auto &outer : filenamesFromProperty) {
    std::copy(outer.begin(), outer.end(),
              std::back_inserter(filenamesFromPropertyUnraveld));
  }

  // shouldn't be used except for legacy cases
  const std::string filename2 = this->getProperty("Filename2");
  if (!filename2.empty())
    filenamesFromPropertyUnraveld.push_back(filename2);

  return filenamesFromPropertyUnraveld;
}

/**
 * Perform the rotation for the calibration
 *
 * @param rX the vector of (base_x, base_y, base_z) from the calibration file
 * @param rY the vector of (up_x, up_y, up_z) from the calibration file
 * @param componentInfo the ComponentInfo object from the workspace
 * @param comp the component to rotate
 * @param doWishCorrection if true apply a special correction for WISH
 */
void LoadIsawDetCal::doRotation(V3D rX, V3D rY, ComponentInfo &componentInfo,
                                boost::shared_ptr<const IComponent> comp,
                                bool doWishCorrection) {
  // These are the ISAW axes
  rX.normalize();
  rY.normalize();

  // These are the original axes
  constexpr V3D oX(1., 0., 0.);
  constexpr V3D oY(0., 1., 0.);

  // Axis that rotates X
  const V3D ax1 = oX.cross_prod(rX);
  Quat Q1;
  if (!ax1.nullVector(1e-12)) {
    // Rotation angle from oX to rX
    double angle1 = oX.angle(rX) * DegreesPerRadian;
    if (doWishCorrection)
      angle1 += 180.0;
    // Create the first quaternion
    Q1.setAngleAxis(angle1, ax1);
  }

  // Now we rotate the original Y using Q1
  V3D roY = oY;
  Q1.rotate(roY);
  // Find the axis that rotates oYr onto rY
  const V3D ax2 = roY.cross_prod(rY);
  Quat Q2;
  if (!ax2.nullVector(1e-12)) {
    const double angle2 = roY.angle(rY) * DegreesPerRadian;
    Q2.setAngleAxis(angle2, ax2);
  }
  // Final = those two rotations in succession; Q1 is done first.
  const Quat Rot = Q2 * Q1;

  // Then find the corresponding relative position
  const auto componentIndex = componentInfo.indexOf(comp->getComponentID());

  componentInfo.setRotation(componentIndex, Rot);
}

/**
 * Apply the scalings from the calibration file. This is called after doing the
 *moves and rotations associated with the calibration, to avoid the problem of
 *invalidation DetectorInfo after writing to the parameter map.
 *
 * @param ws the input workspace
 * @param rectangularDetectorScalings a vector containing a component ID, and
 *values for scalex and scaley
 */
void LoadIsawDetCal::applyScalings(
    Workspace_sptr &ws,
    const std::vector<ComponentScaling> &rectangularDetectorScalings) {

  for (const auto &scaling : rectangularDetectorScalings) {
    IAlgorithm_sptr alg1 = createChildAlgorithm("ResizeRectangularDetector");
    alg1->setProperty<Workspace_sptr>("Workspace", ws);
    alg1->setProperty("ComponentName", scaling.componentName);
    alg1->setProperty("ScaleX", scaling.scaleX);
    alg1->setProperty("ScaleY", scaling.scaleY);
    alg1->executeAsChildAlg();
  }
}

} // namespace DataHandling
} // namespace Mantid
