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
  // parameters for translation
  declareParameter("DeltaX", 0.0, "relative shift along X in meter");
  declareParameter("DeltaY", 0.0, "relative shift along Y in meter");
  declareParameter("DeltaZ", 0.0, "relative shift along Z in meter");
  // parameters for rotation
  declareParameter("RotX", 0.0, "relative rotation around X in degree");
  declareParameter("RotY", 0.0, "relative rotation around Y in degree");
  declareParameter("RotZ", 0.0, "relative rotation around Z in degree");
  // TOF offset for all peaks
  // NOTE: need to have a non-zero value here
  declareParameter("DeltaT0", 0.1, "delta of TOF");
  // This part is for fine tuning the sample position
  declareParameter("DeltaSampleX", 0.0, "relative shift of sample position along X.");
  declareParameter("DeltaSampleY", 0.0, "relative shift of sample position along Y.");
  declareParameter("DeltaSampleZ", 0.0, "relative shift of sample position along Z.");
}

void SCDCalibratePanels2ObjFunc::setPeakWorkspace(IPeaksWorkspace_sptr &pws, const std::string componentName,
                                                  const std::vector<double> tofs) {
  m_pws = pws->clone();
  m_cmpt = componentName;

  // Special adjustment for CORELLI
  Instrument_sptr inst = std::const_pointer_cast<Instrument>(m_pws->getInstrument());
  if (inst->getName().compare("CORELLI") == 0 && m_cmpt != "moderator")
    // the second check is just to ensure that no accidental passing in
    // a bank name with sixteenpack already appended
    if (!boost::algorithm::ends_with(m_cmpt, "/sixteenpack"))
      m_cmpt.append("/sixteenpack");

  // Get the experimentally measured TOFs
  m_tofs = tofs;

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
  //-- delta in rotation
  const double drx = getParameter("RotX");
  const double dry = getParameter("RotY");
  const double drz = getParameter("RotZ");
  //-- delta in TOF
  //  NOTE: The T0 here is a universal offset for all peaks
  double dT0 = getParameter("DeltaT0");
  //-- delta of sample position
  const double dsx = getParameter("DeltaSampleX");
  const double dsy = getParameter("DeltaSampleY");
  const double dsz = getParameter("DeltaSampleZ");

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
  //       -- For Corelli, this will be none/sixteenpack
  //       -- For others, this will be none
  bool calibrateT0 = (m_cmpt == "none/sixteenpack") || (m_cmpt == "none");
  // we don't need to move the instrument if we are calibrating T0
  if (!calibrateT0) {
    // translation
    pws = moveInstruentComponentBy(dx, dy, dz, m_cmpt, pws);

    // rotation
    pws = rotateInstrumentComponentBy(drx, dry, drz, m_cmpt, pws);
  }

  // tweak sample position
  pws = moveInstruentComponentBy(dsx, dsy, dsz, "sample-position", pws);

  // calculate residual
  // double residual = 0.0;
  for (int i = 0; i < pws->getNumberPeaks(); ++i) {
    // use the provided cached tofs
    const double tof = m_tofs[i];

    Peak pk = Peak(pws->getPeak(i));
    // update instrument
    // - this will update the instrument position attached to the peak
    // - this will update the sample position attached to the peak
    pk.setInstrument(pws->getInstrument());
    // update detector ID
    pk.setDetectorID(pk.getDetectorID());
    // calculate&set wavelength based on new instrument
    Units::Wavelength wl;
    wl.initialize(pk.getL1(), 0,
                  {{UnitParams::l2, pk.getL2()},
                   {UnitParams::twoTheta, pk.getScattering()},
                   {UnitParams::efixed, pk.getInitialEnergy()}});
    pk.setWavelength(wl.singleFromTOF(tof + dT0));

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
  // V3D drots = V3D(drx, dry, drz);
  // residual /= pws->getNumberPeaks();
  // std::ostringstream msgiter;
  // msgiter.precision(8);
  // msgiter << "residual@iter_" << n_iter << ": " << residual << "\n"
  //         << "-- (dx, dy, dz) = " << dtrans << "\n"
  //         << "-- (drx, dry, drz) = " << drots << "\n"
  //         << "-- dT0 = " << dT0 << "\n\n";
  // g_log.notice() << msgiter.str();
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
  auto mv_alg = Mantid::API::AlgorithmFactory::Instance().create("MoveInstrumentComponent", -1);
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
 * @param rotX  :: rotate around X
 * @param rotY  :: rotate around Y
 * @param rotZ  :: rotate around Z
 * @param componentName  :: component name
 * @param pws  :: peak workspace
 * @return IPeaksWorkspace_sptr
 */
IPeaksWorkspace_sptr SCDCalibratePanels2ObjFunc::rotateInstrumentComponentBy(double rotX, double rotY, double rotZ,
                                                                             std::string componentName,
                                                                             IPeaksWorkspace_sptr &pws) const {
  // rotate
  auto rot_alg = Mantid::API::AlgorithmFactory::Instance().create("RotateInstrumentComponent", -1);
  // around X
  rot_alg->initialize();
  rot_alg->setChild(true);
  rot_alg->setLogging(LOGCHILDALG);
  rot_alg->setProperty("Workspace", pws);
  rot_alg->setProperty("ComponentName", componentName);
  rot_alg->setProperty("X", 1.0);
  rot_alg->setProperty("Y", 0.0);
  rot_alg->setProperty("Z", 0.0);
  rot_alg->setProperty("Angle", rotX);
  rot_alg->setProperty("RelativeRotation", true);
  rot_alg->executeAsChildAlg();
  // around Y
  rot_alg->initialize();
  rot_alg->setChild(true);
  rot_alg->setLogging(LOGCHILDALG);
  rot_alg->setProperty("Workspace", pws);
  rot_alg->setProperty("ComponentName", componentName);
  rot_alg->setProperty("X", 0.0);
  rot_alg->setProperty("Y", 1.0);
  rot_alg->setProperty("Z", 0.0);
  rot_alg->setProperty("Angle", rotY);
  rot_alg->setProperty("RelativeRotation", true);
  rot_alg->executeAsChildAlg();
  // around Z
  rot_alg->initialize();
  rot_alg->setChild(true);
  rot_alg->setLogging(LOGCHILDALG);
  rot_alg->setProperty("Workspace", pws);
  rot_alg->setProperty("ComponentName", componentName);
  rot_alg->setProperty("X", 0.0);
  rot_alg->setProperty("Y", 0.0);
  rot_alg->setProperty("Z", 1.0);
  rot_alg->setProperty("Angle", rotZ);
  rot_alg->setProperty("RelativeRotation", true);
  rot_alg->executeAsChildAlg();

  return pws;
}

} // namespace Crystal
} // namespace Mantid
