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

#include <boost/algorithm/string.hpp>
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
  declareParameter("DeltaRotationAngle", 0.0, "angle of relative rotation in degree");
  declareParameter("DeltaT0", 0.0, "delta of TOF");
}

void SCDCalibratePanels2ObjFunc::setPeakWorkspace(IPeaksWorkspace_sptr &pws, const std::string componentName) {
  m_pws = pws->clone();
  m_cmpt = componentName;

  // Special adjustment for CORELLI
  Instrument_sptr inst = std::const_pointer_cast<Instrument>(m_pws->getInstrument());
  if (inst->getName().compare("CORELLI") == 0 && m_cmpt != "moderator")
    // the second check is just to ensure that no accidental passing in
    // a bank name with sixteenpack already appended
    if (!boost::algorithm::ends_with(m_cmpt, "/sixteenpack"))
      m_cmpt.append("/sixteenpack");

  // Set the iteration count
  n_iter = 0;
}

/**
 * @brief Evalute the objective function with given feature vector X
 *
 * @param out     :: Q_calculated, which means yValues should be set to
 * Q_measured when setting up the Fit algorithm
 * @param xValues :: feature vector [shiftx3, rotx3, T0]
 * @param order   :: dimensionality of feature vector
 */
void SCDCalibratePanels2ObjFunc::function1D(double *out, const double *xValues, const size_t order) const {
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
  // const double dT0 = getParameter("DeltaT0");
  //-- NOTE: given that these components are never used as
  //         one vector, there is no need to construct a
  //         xValues
  UNUSED_ARG(xValues);
  UNUSED_ARG(order);

  // -- always working on a copy only
  IPeaksWorkspace_sptr pws = m_pws->clone();

  // Debugging related
  IPeaksWorkspace_sptr pws_ref = m_pws->clone();

  // NOTE: when optimizing T0, a none component will be passed in.
  if (m_cmpt != "none/sixteenpack") {
    // rotation
    pws = rotateInstrumentComponentBy(vx, vy, vz, drotang, m_cmpt, pws);

    // translation
    pws = moveInstruentComponentBy(dx, dy, dz, m_cmpt, pws);
  }

  // TODO:
  // need to do something with dT0

  // calculate residual
  // double residual = 0.0;
  for (int i = 0; i < pws->getNumberPeaks(); ++i) {
    // cache TOF
    const double tof = pws->getPeak(i).getTOF();

    Peak pk = Peak(pws->getPeak(i));
    // update instrument
    pk.setInstrument(pws->getInstrument());
    // update detector ID
    pk.setDetectorID(pk.getDetectorID());
    // calculate&set wavelength based on new instrument
    Units::Wavelength wl;
    wl.initialize(pk.getL1(), pk.getL2(), pk.getScattering(), 0, pk.getInitialEnergy(), 0.0);
    pk.setWavelength(wl.singleFromTOF(tof));

    V3D qv = pk.getQSampleFrame();
    for (int j = 0; j < 3; ++j)
      out[i * 3 + j] = qv[j];

    // check the difference between n and target
    // auto ubm = pws->sample().getOrientedLattice().getUB();
    // V3D qv_target = ubm * pws->getPeak(i).getIntHKL();
    // qv_target *= 2 * PI;
    // V3D delta_qv = qv - qv_target;
    // residual += delta_qv.norm2();
  }

  n_iter += 1;

  // V3D dtrans = V3D(dx, dy, dz);
  // V3D rotaxis = V3D(vx, vy, vz);
  // residual /= pws->getNumberPeaks();
  // std::ostringstream msgiter;
  // msgiter.precision(8);
  // msgiter << "residual@iter_" << n_iter << ": " << residual << "\n"
  //         << "-- (dx, dy, dz) = " << dtrans << "\n"
  //         << "-- ang@axis = " << drotang << "@" << rotaxis << "\n\n";
  // g_log.information() << msgiter.str();
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
 * @param pws  :: input workspace (mostly peaksworkspace)
 */
IPeaksWorkspace_sptr SCDCalibratePanels2ObjFunc::moveInstruentComponentBy(double deltaX, double deltaY, double deltaZ,
                                                                          std::string componentName,
                                                                          IPeaksWorkspace_sptr &pws) const {
  // Workspace_sptr inputws = std::dynamic_pointer_cast<Workspace>(pws);

  // move instrument is really fast, even with zero input
  IAlgorithm_sptr mv_alg = Mantid::API::AlgorithmFactory::Instance().create("MoveInstrumentComponent", -1);
  //
  mv_alg->initialize();
  mv_alg->setChild(true);
  mv_alg->setLogging(LOGCHILDALG);
  mv_alg->setProperty("Workspace", pws);
  mv_alg->setProperty("ComponentName", componentName);
  mv_alg->setProperty("X", deltaX);
  mv_alg->setProperty("Y", deltaY);
  mv_alg->setProperty("Z", deltaZ);
  mv_alg->setProperty("RelativePosition", true);
  mv_alg->executeAsChildAlg();

  return pws;
}

/**
 * @brief Rotate the instrument by angle axis
 *
 * @param rotVx  :: x of rotation axis
 * @param rotVy  :: y of rotation axis
 * @param rotVz  :: z of rotation axis
 * @param rotAng  :: rotation angle (in degree)
 * @param componentName  :: component name
 * @param pws  :: peak workspace
 * @return IPeaksWorkspace_sptr
 */
IPeaksWorkspace_sptr SCDCalibratePanels2ObjFunc::rotateInstrumentComponentBy(double rotVx, double rotVy, double rotVz,
                                                                             double rotAng, std::string componentName,
                                                                             IPeaksWorkspace_sptr &pws) const {
  // rotate
  IAlgorithm_sptr rot_alg = Mantid::API::AlgorithmFactory::Instance().create("RotateInstrumentComponent", -1);
  //
  rot_alg->initialize();
  rot_alg->setChild(true);
  rot_alg->setLogging(LOGCHILDALG);
  rot_alg->setProperty("Workspace", pws);
  rot_alg->setProperty("ComponentName", componentName);
  rot_alg->setProperty("X", rotVx);
  rot_alg->setProperty("Y", rotVy);
  rot_alg->setProperty("Z", rotVz);
  rot_alg->setProperty("Angle", rotAng);
  rot_alg->setProperty("RelativeRotation", true);
  rot_alg->executeAsChildAlg();

  return pws;
}

} // namespace Crystal
} // namespace Mantid
