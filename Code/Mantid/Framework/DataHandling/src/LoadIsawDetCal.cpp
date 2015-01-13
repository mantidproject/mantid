//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadIsawDetCal.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/EventList.h"
#include "MantidAPI/TextAxis.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Instrument/ObjCompAssembly.h"
#include "MantidKernel/V3D.h"
#include <Poco/File.h>
#include <sstream>
#include <fstream>
#include <numeric>
#include <cmath>
#include <iomanip>
#include "MantidAPI/WorkspaceValidators.h"

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
LoadIsawDetCal::~LoadIsawDetCal() {}

/**
 * The intensity function calculates the intensity as a function of detector
 * position and angles
 * @param x :: The shift along the X-axis
 * @param y :: The shift along the Y-axis
 * @param z :: The shift along the Z-axis
 * @param detname :: The detector name
 * @param inname :: The workspace name
 */

void LoadIsawDetCal::center(double x, double y, double z, std::string detname,
                            std::string inname) {
  IAlgorithm_sptr alg1 = createChildAlgorithm("MoveInstrumentComponent");
  alg1->setProperty("Workspace", inname);
  alg1->setPropertyValue("ComponentName", detname);
  alg1->setProperty("X", x);
  alg1->setProperty("Y", y);
  alg1->setProperty("Z", z);
  alg1->setPropertyValue("RelativePosition", "0");
  alg1->executeAsChildAlg();
}

/** Initialisation method
*/
void LoadIsawDetCal::init() {
  declareProperty(new WorkspaceProperty<MatrixWorkspace>(
                      "InputWorkspace", "", Direction::InOut,
                      boost::make_shared<InstrumentValidator>()),
                  "The workspace containing the geometry to be calibrated.");

  declareProperty(
      new API::FileProperty("Filename", "", API::FileProperty::Load, ".DetCal"),
      "The input filename of the ISAW DetCal file (East banks for SNAP) ");

  declareProperty(new API::FileProperty("Filename2", "",
                                        API::FileProperty::OptionalLoad,
                                        ".DetCal"),
                  "The input filename of the second ISAW DetCal file (West "
                  "banks for SNAP) ");

  declareProperty("TimeOffset", 0.0, "Time Offset", Direction::Output);
}

/** Executes the algorithm
*
*  @throw runtime_error Thrown if algorithm cannot execute
*/
void LoadIsawDetCal::exec() {

  // Get the input workspace
  MatrixWorkspace_sptr inputW = getProperty("InputWorkspace");

  // Get some stuff from the input workspace
  Instrument_const_sptr inst = inputW->getInstrument();
  std::string instname = inst->getName();
  Geometry::Instrument_sptr instrument(new Geometry::Instrument(instname));

  // set-up minimizer

  std::string inname = getProperty("InputWorkspace");
  std::string filename = getProperty("Filename");
  std::string filename2 = getProperty("Filename2");

  // Output summary to log file
  int idnum = 0, count, id, nrows, ncols;
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

  if (detList.empty())
    throw std::runtime_error("This instrument does not have any "
                             "RectangularDetector's. LoadIsawDetCal cannot "
                             "operate on this instrument at this time.");

  while (std::getline(input, line)) {
    if (line[0] == '7') {
      double mL1, mT0;
      std::stringstream(line) >> count >> mL1 >> mT0;
      setProperty("TimeOffset", mT0);
      // Convert from cm to m
      center(0.0, 0.0, -0.01 * mL1, "moderator", inname);
      // mT0 and time of flight are both in microsec
      IAlgorithm_sptr alg1 = createChildAlgorithm("ChangeBinOffset");
      alg1->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputW);
      alg1->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", inputW);
      alg1->setProperty("Offset", mT0);
      alg1->executeAsChildAlg();
      inputW = alg1->getProperty("OutputWorkspace");
      // set T0 in the run parameters
      API::Run &run = inputW->mutableRun();
      run.addProperty<double>("T0", mT0, true);
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
    std::ostringstream Detbank;
    Detbank << "bank" << id;
    // Loop through detectors to match names with number from DetCal file
    for (int i = 0; i < static_cast<int>(detList.size()); i++)
      if (detList[i]->getName().compare(Detbank.str()) == 0)
        idnum = i;
    det = detList[idnum];
    if (det) {
      IAlgorithm_sptr alg1 = createChildAlgorithm("ResizeRectangularDetector");
      alg1->setProperty<MatrixWorkspace_sptr>("Workspace", inputW);
      alg1->setProperty("ComponentName", det->getName());
      // Convert from cm to m
      alg1->setProperty("ScaleX", 0.01 * width / det->xsize());
      alg1->setProperty("ScaleY", 0.01 * height / det->ysize());
      alg1->executeAsChildAlg();

      // Convert from cm to m
      x *= 0.01;
      y *= 0.01;
      z *= 0.01;
      detname = det->getName();
      center(x, y, z, detname, inname);

      // These are the ISAW axes
      V3D rX = V3D(base_x, base_y, base_z);
      rX.normalize();
      V3D rY = V3D(up_x, up_y, up_z);
      rY.normalize();
      // V3D rZ=rX.cross_prod(rY);

      // These are the original axes
      V3D oX = V3D(1., 0., 0.);
      V3D oY = V3D(0., 1., 0.);
      V3D oZ = V3D(0., 0., 1.);

      // Axis that rotates X
      V3D ax1 = oX.cross_prod(rX);
      // Rotation angle from oX to rX
      double angle1 = oX.angle(rX);
      angle1 *= 180.0 / M_PI;
      // Create the first quaternion
      Quat Q1(angle1, ax1);

      // Now we rotate the original Y using Q1
      V3D roY = oY;
      Q1.rotate(roY);
      // Find the axis that rotates oYr onto rY
      V3D ax2 = roY.cross_prod(rY);
      double angle2 = roY.angle(rY);
      angle2 *= 180.0 / M_PI;
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
        Rot = Rot * rot0;
      }
      boost::shared_ptr<const IComponent> grandparent = parent->getParent();
      if (grandparent) {
        Quat rot0 = grandparent->getRelativeRot();
        rot0.inverse();
        Rot = Rot * rot0;
      }

      // Need to get the address to the base instrument component
      Geometry::ParameterMap &pmap = inputW->instrumentParameters();

      // Set or overwrite "rot" instrument parameter.
      pmap.addQuat(comp.get(), "rot", Rot);
    }
  }
  setProperty("InputWorkspace", inputW);

  return;
}

} // namespace Algorithm
} // namespace Mantid
