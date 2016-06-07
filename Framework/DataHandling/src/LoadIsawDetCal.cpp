//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadIsawDetCal.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/InstrumentValidator.h"

#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/PeaksWorkspace.h"

#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Instrument/ObjCompAssembly.h"
#include "MantidGeometry/Instrument/ComponentHelper.h"

#include "MantidKernel/V3D.h"

#include <Poco/File.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <numeric>
#include <cmath>

namespace Mantid {
namespace DataHandling {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(LoadIsawDetCal)

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

/// Constructor
LoadIsawDetCal::LoadIsawDetCal() : API::Algorithm() {}

/// Destructor
LoadIsawDetCal::~LoadIsawDetCal() = default;

/** Initialisation method
*/
void LoadIsawDetCal::init() {
  declareProperty(make_unique<WorkspaceProperty<Workspace>>(
                      "InputWorkspace", "", Direction::InOut,
                      boost::make_shared<InstrumentValidator>()),
                  "The workspace containing the geometry to be calibrated.");

  declareProperty(
      make_unique<API::FileProperty>("Filename", "", API::FileProperty::Load,
                                     ".DetCal"),
      "The input filename of the ISAW DetCal file (East banks for SNAP) ");

  declareProperty(
      make_unique<API::FileProperty>(
          "Filename2", "", API::FileProperty::OptionalLoad, ".DetCal"),
      "The input filename of the second ISAW DetCal file (West "
      "banks for SNAP) ");

  declareProperty("TimeOffset", 0.0, "Time Offset", Direction::Output);
}

namespace {
const constexpr double DegreesPerRadian = 180.0 / M_PI;

std::string getBankName(const std::string &bankPart, int idnum) {
  if (bankPart == "WISHpanel" && idnum < 10) {
    return bankPart + "0" + std::to_string(idnum);
  } else {
    return bankPart + std::to_string(idnum);
  }
}
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

  // set-up minimizer

  std::string filename = getProperty("Filename");
  std::string filename2 = getProperty("Filename2");

  // Output summary to log file
  int count, id, nrows, ncols;
  double width, height, depth, detd, x, y, z, base_x, base_y, base_z, up_x,
      up_y, up_z;
  std::ifstream input(filename.c_str(), std::ios_base::in);
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
  if (instname.compare("WISH") == 0)
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

  while (std::getline(input, line)) {
    if (line[0] == '7') {
      double mL1, mT0;
      std::stringstream(line) >> count >> mL1 >> mT0;
      setProperty("TimeOffset", mT0);
      // Convert from cm to m
      if (instname.compare("WISH") == 0)
        center(0.0, 0.0, -0.01 * mL1, "undulator", ws);
      else
        center(0.0, 0.0, -0.01 * mL1, "moderator", ws);
      // mT0 and time of flight are both in microsec
      if (inputW) {
        API::Run &run = inputW->mutableRun();
        // Check to see if LoadEventNexus had T0 from TOPAZ Parameter file
        if (run.hasProperty("T0")) {
          double T0IDF = run.getPropertyValueAsType<double>("T0");
          IAlgorithm_sptr alg1 = createChildAlgorithm("ChangeBinOffset");
          alg1->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputW);
          alg1->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", inputW);
          alg1->setProperty("Offset", mT0 - T0IDF);
          alg1->executeAsChildAlg();
          inputW = alg1->getProperty("OutputWorkspace");
          // set T0 in the run parameters
          run.addProperty<double>("T0", mT0, true);
        } else {
          IAlgorithm_sptr alg1 = createChildAlgorithm("ChangeBinOffset");
          alg1->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputW);
          alg1->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", inputW);
          alg1->setProperty("Offset", mT0);
          alg1->executeAsChildAlg();
          inputW = alg1->getProperty("OutputWorkspace");
          // set T0 in the run parameters
          run.addProperty<double>("T0", mT0, true);
        }
      }
    }

    if (line[0] != '5')
      continue;

    std::stringstream(line) >> count >> id >> nrows >> ncols >> width >>
        height >> depth >> detd >> x >> y >> z >> base_x >> base_y >> base_z >>
        up_x >> up_y >> up_z;
    if (id == 10 && instname == "SNAP" && filename2 != "") {
      input.close();
      input.open(filename2.c_str());
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
    if (det) {
      detname = det->getName();
      IAlgorithm_sptr alg1 = createChildAlgorithm("ResizeRectangularDetector");
      alg1->setProperty<Workspace_sptr>("Workspace", ws);
      alg1->setProperty("ComponentName", detname);
      // Convert from cm to m
      alg1->setProperty("ScaleX", 0.01 * width / det->xsize());
      alg1->setProperty("ScaleY", 0.01 * height / det->ysize());
      alg1->executeAsChildAlg();

      // Convert from cm to m
      x *= 0.01;
      y *= 0.01;
      z *= 0.01;
      center(x, y, z, detname, ws);

      // These are the ISAW axes
      V3D rX = V3D(base_x, base_y, base_z);
      rX.normalize();
      V3D rY = V3D(up_x, up_y, up_z);
      rY.normalize();
      // V3D rZ=rX.cross_prod(rY);

      // These are the original axes
      V3D oX = V3D(1., 0., 0.);
      V3D oY = V3D(0., 1., 0.);

      // Axis that rotates X
      V3D ax1 = oX.cross_prod(rX);
      // Rotation angle from oX to rX
      double angle1 = oX.angle(rX);
      angle1 *= DegreesPerRadian;
      // Create the first quaternion
      Quat Q1(angle1, ax1);

      // Now we rotate the original Y using Q1
      V3D roY = oY;
      Q1.rotate(roY);
      // Find the axis that rotates oYr onto rY
      V3D ax2 = roY.cross_prod(rY);
      double angle2 = roY.angle(rY);
      angle2 *= DegreesPerRadian;
      Quat Q2(angle2, ax2);

      // Final = those two rotations in succession; Q1 is done first.
      Quat Rot = Q2 * Q1;

      // Then find the corresponding relative position
      boost::shared_ptr<const IComponent> comp =
          inst->getComponentByName(detname);
      boost::shared_ptr<const IComponent> parent = comp->getParent();
      if (parent) {
        Quat rot0 = parent->getRelativeRot();
        rot0.inverse();
        Rot *= rot0;
      }
      boost::shared_ptr<const IComponent> grandparent = parent->getParent();
      if (grandparent) {
        Quat rot0 = grandparent->getRelativeRot();
        rot0.inverse();
        Rot *= rot0;
      }

      if (inputW) {
        Geometry::ParameterMap &pmap = inputW->instrumentParameters();
        // Set or overwrite "rot" instrument parameter.
        pmap.addQuat(comp.get(), "rot", Rot);
      } else if (inputP) {
        Geometry::ParameterMap &pmap = inputP->instrumentParameters();
        // Set or overwrite "rot" instrument parameter.
        pmap.addQuat(comp.get(), "rot", Rot);
      }
    }
    auto bank = uniqueBanks.find(id);
    if (bank == uniqueBanks.end())
      continue;
    int idnum = *bank;

    bankName = getBankName(bankPart, idnum);
    // Retrieve it
    boost::shared_ptr<const IComponent> comp =
        inst->getComponentByName(bankName);
    if (instname.compare("CORELLI") ==
        0) // for Corelli with sixteenpack under bank
    {
      std::vector<Geometry::IComponent_const_sptr> children;
      boost::shared_ptr<const Geometry::ICompAssembly> asmb =
          boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(
              inst->getComponentByName(bankName));
      asmb->getChildren(children, false);
      comp = children[0];
    }
    if (comp) {
      // Omitted resizing tubes

      // Convert from cm to m
      x *= 0.01;
      y *= 0.01;
      z *= 0.01;
      detname = comp->getFullName();
      center(x, y, z, detname, ws);

      // These are the ISAW axes
      V3D rX = V3D(base_x, base_y, base_z);
      rX.normalize();
      V3D rY = V3D(up_x, up_y, up_z);
      rY.normalize();
      // V3D rZ=rX.cross_prod(rY);

      // These are the original axes
      V3D oX = V3D(1., 0., 0.);
      V3D oY = V3D(0., 1., 0.);

      // Axis that rotates X
      V3D ax1 = oX.cross_prod(rX);
      // Rotation angle from oX to rX
      double angle1 = oX.angle(rX);
      angle1 *= DegreesPerRadian;
      // TODO: find out why this is needed for WISH
      if (instname == "WISH")
        angle1 += 180.0;
      // Create the first quaternion
      Quat Q1(angle1, ax1);

      // Now we rotate the original Y using Q1
      V3D roY = oY;
      Q1.rotate(roY);
      // Find the axis that rotates oYr onto rY
      V3D ax2 = roY.cross_prod(rY);
      double angle2 = roY.angle(rY);
      angle2 *= DegreesPerRadian;
      Quat Q2(angle2, ax2);

      // Final = those two rotations in succession; Q1 is done first.
      Quat Rot = Q2 * Q1;

      boost::shared_ptr<const IComponent> parent = comp->getParent();
      if (parent) {
        Quat rot0 = parent->getRelativeRot();
        rot0.inverse();
        Rot = Rot * rot0;
      }
      boost::shared_ptr<const IComponent> grandparent = parent->getParent();
      if (grandparent) {
        Quat rot0 = grandparent->getRelativeRot();
        rot0.inverse();
        Rot = Rot * rot0;
      }

      if (inputW) {
        Geometry::ParameterMap &pmap = inputW->instrumentParameters();
        // Set or overwrite "rot" instrument parameter.
        pmap.addQuat(comp.get(), "rot", Rot);
      } else if (inputP) {
        Geometry::ParameterMap &pmap = inputP->instrumentParameters();
        // Set or overwrite "rot" instrument parameter.
        pmap.addQuat(comp.get(), "rot", Rot);
      }
    }
  }

  setProperty("InputWorkspace", ws);

  return;
}

/**
 * The intensity function calculates the intensity as a function of detector
 * position and angles
 * @param x :: The shift along the X-axis
 * @param y :: The shift along the Y-axis
 * @param z :: The shift along the Z-axis
 * @param detname :: The detector name
 * @param ws :: The workspace
 */

void LoadIsawDetCal::center(double x, double y, double z,
                            const std::string &detname,
                            API::Workspace_sptr ws) {

  Instrument_sptr inst = getCheckInst(ws);

  IComponent_const_sptr comp = inst->getComponentByName(detname);
  if (comp == nullptr) {
    std::ostringstream mess;
    mess << "Component with name " << detname << " was not found.";
    g_log.error(mess.str());
    throw std::runtime_error(mess.str());
  }

  // Do the move
  using namespace Geometry::ComponentHelper;
  TransformType positionType = Absolute;
  MatrixWorkspace_sptr inputW =
      boost::dynamic_pointer_cast<MatrixWorkspace>(ws);
  PeaksWorkspace_sptr inputP = boost::dynamic_pointer_cast<PeaksWorkspace>(ws);
  if (inputW) {
    Geometry::ParameterMap &pmap = inputW->instrumentParameters();
    Geometry::ComponentHelper::moveComponent(*comp, pmap, V3D(x, y, z),
                                             positionType);
  } else if (inputP) {
    Geometry::ParameterMap &pmap = inputP->instrumentParameters();
    Geometry::ComponentHelper::moveComponent(*comp, pmap, V3D(x, y, z),
                                             positionType);
  }
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
    if (!inst)
      throw std::runtime_error("Could not get a valid instrument from the "
                               "workspace which does not seem to be valid as "
                               "input (must be either MatrixWorkspace or "
                               "PeaksWorkspace");
  }

  return inst;
}

} // namespace Algorithm
} // namespace Mantid
