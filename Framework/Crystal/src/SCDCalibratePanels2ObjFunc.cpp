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
  declareAttribute("Workspace", Attribute(""));
  declareAttribute("ComponentName", Attribute(""));
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
  m_ws = AnalysisDataService::Instance().retrieveWS<Workspace>(
      getAttribute("Workspace").asString());
  m_cmpt = getAttribute("ComponentName").asString();

  // Special adjustment for CORELLI
  PeaksWorkspace_sptr pws = std::dynamic_pointer_cast<PeaksWorkspace>(m_ws);
  Instrument_sptr inst =
      std::const_pointer_cast<Instrument>(pws->getInstrument());
  if (inst->getName().compare("CORELLI") == 0 && m_cmpt != "moderator")
    m_cmpt.append("/sixteenpack");

  // NOTE: Since the feature vectors are all deltas with respect to the starting
  // position,
  //       we need to only operate on a copy instead of the original to avoid
  //       changing the base value
  std::shared_ptr<API::Workspace> calc_ws = m_ws->clone();

  // NOTE: when optimizing T0, a none component will be passed in.
  if (m_cmpt != "none/sixteenpack") {
    // rotation
    // NOTE: moderator should not be reoriented
    rotateInstrumentComponentBy(vx, vy, vz, drotang, m_cmpt, calc_ws);

    // translation
    moveInstruentComponentBy(dx, dy, dz, m_cmpt, calc_ws);
  }

  // generate a flatten Q_sampleframe from calculated ws (by moving instrument
  // component) so that a direct comparison can be performed between measured
  // and calculated
  PeaksWorkspace_sptr calc_pws =
      std::dynamic_pointer_cast<PeaksWorkspace>(calc_ws);
  Instrument_sptr calc_inst =
      std::const_pointer_cast<Instrument>(calc_pws->getInstrument());

  // NOTE: We are not sure if the PeaksWorkspace level T0
  //       if going go affect the peak.getTOF
  Mantid::API::Run &run = calc_pws->mutableRun();
  double T0 = 0.0;
  if (run.hasProperty("T0")) {
    T0 = run.getPropertyValueAsType<double>("T0");
  }
  T0 += dT0;
  run.addProperty<double>("T0", T0, true);

  for (int i = 0; i < calc_pws->getNumberPeaks(); ++i) {
    const Peak pk = calc_pws->getPeak(i);

    V3D hkl =
        V3D(boost::math::iround(pk.getH()), boost::math::iround(pk.getK()),
            boost::math::iround(pk.getL()));
    if (hkl == UNSET_HKL)
      throw std::runtime_error("Found unindexed peak in input workspace!");

    // construct the out vector (Qvectors)
    Units::Wavelength wl;
    V3D calc_qv;
    // somehow calibration results works better with direct method
    // but moderator requires the strange in-and-out way
    if (m_cmpt != "moderator") {
      wl.initialize(pk.getL1(), pk.getL2(), pk.getScattering(), 0,
                    pk.getInitialEnergy(), 0.0);
      // create a peak with shifted wavelength
      Peak calc_pk(calc_inst, pk.getDetectorID(),
                   wl.singleFromTOF(pk.getTOF() + dT0), hkl,
                   pk.getGoniometerMatrix());
      calc_qv = calc_pk.getQSampleFrame();
    } else {
      Peak calc_pk(calc_inst, pk.getDetectorID(), pk.getWavelength(), hkl,
                   pk.getGoniometerMatrix());
      wl.initialize(calc_pk.getL1(), calc_pk.getL2(), calc_pk.getScattering(),
                    0, calc_pk.getInitialEnergy(), 0.0);
      // adding the TOF shift here
      calc_pk.setWavelength(wl.singleFromTOF(pk.getTOF() + dT0));
      calc_qv = calc_pk.getQSampleFrame();
    }

    // get the updated/calculated q vector in sample frame and set it to out
    // V3D calc_qv = calc_pk.getQSampleFrame();
    for (int j = 0; j < 3; ++j)
      out[i * 3 + j] = calc_qv[j];
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
