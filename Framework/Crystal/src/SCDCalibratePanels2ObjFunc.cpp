// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidCrystal/SCDCalibratePanels2ObjFunc.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument.h"

#include <boost/math/special_functions/round.hpp>
#include <cmath>

namespace Mantid {
namespace Crystal {

using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace {
// static logger
Logger g_log("SCDCalibratePanels2ObjFunc");
} // namespace

DECLARE_FUNCTION(SCDCalibratePanels2ObjFunc)

/// ---------------///
/// Core functions ///
/// ---------------///
SCDCalibratePanels2ObjFunc::SCDCalibratePanels2ObjFunc() {
  // parameters
  declareParameter("DeltaX", 0.0, "relative shift along X");
  declareParameter("DeltaY", 0.0, "relative shift along Y");
  declareParameter("DeltaZ", 0.0, "relative shift along Z");
  // rotation axis is defined as (1, theta, phi)
  // https://en.wikipedia.org/wiki/Spherical_coordinate_system
  declareParameter("Theta", PI / 4, "Polar coordinates theta in radians");
  declareParameter("Phi", PI / 4, "Polar coordinates phi in radians");
  // rotation angle
  declareParameter("DeltaRotationAngle", 0.0,
                   "angle of relative rotation in degree");
  declareParameter("DeltaT0", 0.0, "delta of TOF");

  // attributes
  // declareAttribute("Workspace", Attribute(""));
  // declareAttribute("ComponentName", Attribute(""));
}

void SCDCalibratePanels2ObjFunc::setPeakWorkspace(
    IPeaksWorkspace_sptr &pws, const std::string componentName) {
  m_pws = pws->clone();
  m_cmpt = componentName;
}

/**
 * @brief Evalute the objective function with given feature vector X
 *
 * @param out     :: Q_calculated, which means yValues should be set to
 * Q_measured when setting up the Fit algorithm
 * @param xValues :: feature vector [shiftx3, rotx3, T0]
 * @param order   :: dimensionality of feature vector
 */
void SCDCalibratePanels2ObjFunc::function1D(double *out, const double *xValues,
                                            const size_t order) const {
  // Get the feature vector component (numeric type)
  //-- delta in translation
  const double dx = getParameter("DeltaX");
  const double dy = getParameter("DeltaY");
  const double dz = getParameter("DeltaZ");
  //-- delta in rotation/orientation as angle axis pair
  //   using polar coordinates to ensure a unit vector
  //   (r, theta, phi) where r=1
  const double theta = getParameter("Theta");
  const double phi = getParameter("Phi");
  // compute the rotation axis
  double vx = sin(theta) * cos(phi);
  double vy = sin(theta) * sin(phi);
  double vz = cos(theta);
  //
  const double drotang = getParameter("DeltaRotationAngle");

  //-- delta in TOF
  const double dT0 = getParameter("DeltaT0");
  //-- NOTE: given that these components are never used as
  //         one vector, there is no need to construct a
  //         xValues
  UNUSED_ARG(xValues);
  UNUSED_ARG(order);

  // Get workspace and component name (string type)
  // m_ws = AnalysisDataService::Instance().retrieveWS<Workspace>(
  //     getAttribute("Workspace").asString());
  // m_cmpt = getAttribute("ComponentName").asString();

  // Special adjustment for CORELLI
  Instrument_sptr inst =
      std::const_pointer_cast<Instrument>(m_pws->getInstrument());
  if (inst->getName().compare("CORELLI") == 0 && m_cmpt != "moderator")
    m_cmpt.append("/sixteenpack");

  // NOTE: when optimizing T0, a none component will be passed in.
  if (m_cmpt != "none/sixteenpack") {
    // rotation
    // NOTE: moderator should not be reoriented
    rotateInstrumentComponentBy(vx, vy, vz, drotang, m_cmpt, m_pws);

    // translation
    moveInstruentComponentBy(dx, dy, dz, m_cmpt, m_pws);
  }

  // TODO:
  // need to do something with dT0

  // Now when we query qSample, it should be updated with the perturbed
  // detector positions
  for (int i = 0; i < m_pws->getNumberPeaks(); ++i) {
    // NOTE: we will skip over non-indexed peaks
    V3D hkl = V3D(boost::math::iround(m_pws->getPeak(i).getH()),
                  boost::math::iround(m_pws->getPeak(i).getK()),
                  boost::math::iround(m_pws->getPeak(i).getL()));

    if (hkl != UNSET_HKL) {
      V3D qv = m_pws->getPeak(i).getQSampleFrame();
      for (int j = 0; j < 3; ++j)
        out[i * 3 + j] = qv[j];
    }
  }
}

// -------///
// Helper ///
// -------///

/**
 * @brief Translate the component of given workspace by delta_(x, y, z)
 *
 * @param deltaX  :: The shift along the X-axis in m
 * @param deltaY  :: The shift along the Y-axis in m
 * @param deltaZ  :: The shift along the Z-axis in m
 * @param componentName  :: string representation of a component
 * @param ws  :: input workspace (mostly peaksworkspace)
 */
void SCDCalibratePanels2ObjFunc::moveInstruentComponentBy(
    double deltaX, double deltaY, double deltaZ, std::string componentName,
    const API::Workspace_sptr &ws) const {
  // move instrument is really fast, even with zero input
  IAlgorithm_sptr mv_alg = Mantid::API::AlgorithmFactory::Instance().create(
      "MoveInstrumentComponent", -1);
  mv_alg->initialize();
  mv_alg->setChild(true);
  mv_alg->setLogging(LOGCHILDALG);
  mv_alg->setProperty<Workspace_sptr>("Workspace", ws);
  mv_alg->setProperty("ComponentName", componentName);
  mv_alg->setProperty("X", deltaX);
  mv_alg->setProperty("Y", deltaY);
  mv_alg->setProperty("Z", deltaZ);
  mv_alg->setProperty("RelativePosition", true);
  mv_alg->executeAsChildAlg();
}

void SCDCalibratePanels2ObjFunc::rotateInstrumentComponentBy(
    double rotVx, double rotVy, double rotVz, double rotAng,
    std::string componentName, const API::Workspace_sptr &ws) const {
  // rotate
  IAlgorithm_sptr rot_alg = Mantid::API::AlgorithmFactory::Instance().create(
      "RotateInstrumentComponent", -1);
  //
  rot_alg->initialize();
  rot_alg->setChild(true);
  rot_alg->setLogging(LOGCHILDALG);
  rot_alg->setProperty<Workspace_sptr>("Workspace", ws);
  rot_alg->setProperty("ComponentName", componentName);
  rot_alg->setProperty("X", rotVx);
  rot_alg->setProperty("Y", rotVy);
  rot_alg->setProperty("Z", rotVz);
  rot_alg->setProperty("Angle", rotAng);
  rot_alg->setProperty("RelativeRotation", true);
  rot_alg->executeAsChildAlg();
}

} // namespace Crystal
} // namespace Mantid
