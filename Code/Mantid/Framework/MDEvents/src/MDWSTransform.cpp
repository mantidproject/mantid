#include "MantidMDEvents/MDWSTransform.h"
#include "MantidMDEvents/MDTransfAxisNames.h"
#include "MantidKernel/Strings.h"
#include <float.h>

namespace Mantid {
namespace MDEvents {
namespace {
// logger for the algorithm workspaces
Kernel::Logger g_Log("MDWSTransform");
}

using namespace CnvrtToMD;

/** method to build the Q-coordinates transformation.
 *
 * @param TargWSDescription -- the class which describes target MD workspace. In
 Q3D case this description is modified by the method
                               with default Q-axis labels and Q-axis units
 * @param FrameRequested    -- the string which describes the target
 transformation frame in Q3D case. If the string value is '''Auto'''
 *   the frame is selected depending on the presence of UB matrix and goniometer
 settings, namely it can be:
 * a) the laboratory -- (no UB matrix, goniometer angles set to 0)
   b) Q (sample frame)''': the goniometer rotation of the sample is taken out,
 to give Q in the frame of the sample. See [[SetGoniometer]] to specify the
 goniometer used in the experiment.
   c) Crystal or crystal Cartesian (C)- Busing, Levi 1967 coordinate system --
 depending on Q-scale requested
 *  one of the target frames above can be requested explicitly. In this case the
 method throws invalid argument if necessary parameters (UB matrix) is not
 attached to the workspace

 * @param QScaleRequested   -- Q-transformation needed
 *
 * @return the linear representation for the transformation matrix, which
 translate momentums from laboratory to the requested
 *   coordinate system.
*/
std::vector<double>
MDWSTransform::getTransfMatrix(MDEvents::MDWSDescription &TargWSDescription,
                               const std::string &FrameRequested,
                               const std::string &QScaleRequested) const {
  CoordScaling ScaleID = getQScaling(QScaleRequested);
  TargetFrame FrameID = getTargetFrame(FrameRequested);
  std::vector<double> transf =
      getTransfMatrix(TargWSDescription, FrameID, ScaleID);

  if (TargWSDescription.AlgID.compare("Q3D") == 0) {
    this->setQ3DDimensionsNames(TargWSDescription, FrameID, ScaleID);
  }

  return transf;
}
/** Method analyzes the state of UB matrix and goniometer attached to the
  *workspace and decides, which target
  * coordinate system these variables identify.
  *Crystal Frame decided in case if there is UB matrix is present and is not
  *unit matrix
  *Lab frame -- if goniometer is Unit and UB is unit matrix or not present
  *Sample frame -- otherwise
  */
CnvrtToMD::TargetFrame MDWSTransform::findTargetFrame(
    MDEvents::MDWSDescription &TargWSDescription) const {

  bool hasGoniometer = TargWSDescription.hasGoniometer();
  bool hasLattice = TargWSDescription.hasLattice();

  if (!hasGoniometer) {
    return LabFrame;
  } else {
    if (hasLattice)
      return HKLFrame;
    else
      return SampleFrame;
  }
}
/** Method verifies if the information available on the source workspace is
 *sufficient to build appropriate frame
 *@param TargWSDescription -- the class which contains the information about the
 *target workspace
 *@param CoordFrameID     -- the ID of the target frame requested
 *
 * method throws invalid argument if the information on the workspace is
 *insufficient to define the frame requested
*/
void MDWSTransform::checkTargetFrame(
    const MDEvents::MDWSDescription &TargWSDescription,
    const CnvrtToMD::TargetFrame CoordFrameID) const {
  switch (CoordFrameID) {
  case (LabFrame): // nothing needed for lab frame
    return;
  case (SampleFrame):
    if (!TargWSDescription.hasGoniometer())
      throw std::invalid_argument(
          " Sample frame needs goniometer to be defined on the workspace ");
    return;
  case (HKLFrame): // ubMatrix has to be present
    if (!TargWSDescription.hasLattice())
      throw std::invalid_argument(
          " HKL frame needs UB matrix defined on the workspace ");
    if (!TargWSDescription.hasGoniometer())
      g_Log.warning() << "  HKL frame does not have goniometer defined on the "
                         "workspace. Assuming unit goniometer matrix\n";
    return;
  default:
    throw std::runtime_error(
        " Unexpected argument in MDWSTransform::checkTargetFrame");
  }
}

/** The matrix to convert neutron momentums into the target coordinate system */
std::vector<double>
MDWSTransform::getTransfMatrix(MDEvents::MDWSDescription &TargWSDescription,
                               CnvrtToMD::TargetFrame FrameID,
                               CoordScaling &ScaleID) const {

  Kernel::Matrix<double> mat(3, 3, true);

  bool powderMode = TargWSDescription.isPowder();

  bool has_lattice(true);
  if (!TargWSDescription.hasLattice())
    has_lattice = false;

  if (!(powderMode || has_lattice)) {
    std::string inWsName = TargWSDescription.getWSName();
    // notice about 3D case without lattice
    g_Log.notice()
        << "Can not obtain transformation matrix from the input workspace: "
        << inWsName << " as no oriented lattice has been defined. \n"
                       "Will use unit transformation matrix.\n";
  }
  // set the frame ID to the values, requested by properties
  CnvrtToMD::TargetFrame CoordFrameID(FrameID);
  if (FrameID == AutoSelect || powderMode) // if this value is auto-select, find
                                           // appropriate frame from workspace
                                           // properties
    CoordFrameID = findTargetFrame(TargWSDescription);
  else // if not, and specific target frame requested, verify if everything is
       // available on the workspace for this frame
    checkTargetFrame(
        TargWSDescription,
        CoordFrameID); // throw, if the information is not available

  switch (CoordFrameID) {
  case (CnvrtToMD::LabFrame): {
    ScaleID = NoScaling;
    TargWSDescription.m_Wtransf =
        buildQTrahsf(TargWSDescription, ScaleID, true);
    // ignore goniometer
    mat = TargWSDescription.m_Wtransf;
    break;
  }
  case (CnvrtToMD::SampleFrame): {
    ScaleID = NoScaling;
    TargWSDescription.m_Wtransf =
        buildQTrahsf(TargWSDescription, ScaleID, true);
    // Obtain the transformation matrix to Cartesian related to Crystal
    mat = TargWSDescription.getGoniometerMatr() * TargWSDescription.m_Wtransf;
    break;
  }
  case (CnvrtToMD::HKLFrame): {
    TargWSDescription.m_Wtransf =
        buildQTrahsf(TargWSDescription, ScaleID, false);
    // Obtain the transformation matrix to Cartesian related to Crystal
    if (TargWSDescription.hasGoniometer())
      mat = TargWSDescription.getGoniometerMatr() * TargWSDescription.m_Wtransf;
    else
      mat = TargWSDescription.m_Wtransf;

    break;
  }
  default:
    throw(std::invalid_argument(" Unknown or undefined Target Frame ID"));
  }
  //
  // and this is the transformation matrix to notional
  mat.Invert();

  std::vector<double> rotMat = mat.getVector();
  g_Log.debug()
      << " *********** Q-transformation matrix ***********************\n";
  g_Log.debug()
      << "***     *qx         !     *qy         !     *qz           !\n";
  g_Log.debug() << "q1= " << rotMat[0] << " ! " << rotMat[1] << " ! "
                << rotMat[2] << " !\n";
  g_Log.debug() << "q2= " << rotMat[3] << " ! " << rotMat[4] << " ! "
                << rotMat[5] << " !\n";
  g_Log.debug() << "q3= " << rotMat[6] << " ! " << rotMat[7] << " ! "
                << rotMat[8] << " !\n";
  g_Log.debug()
      << " *********** *********************** ***********************\n";
  return rotMat;
}
/**
 Method builds transformation Q=R*U*B*W*h where W-transf is W or WB or
 W*Unit*Lattice_param depending on inputs
*/
Kernel::DblMatrix
MDWSTransform::buildQTrahsf(MDEvents::MDWSDescription &TargWSDescription,
                            CnvrtToMD::CoordScaling ScaleID,
                            bool UnitUB) const {
  // implements strategy
  if (!(TargWSDescription.hasLattice() || UnitUB)) {
    throw(std::invalid_argument("this function should be called only on "
                                "workspace with defined oriented lattice"));
  }

  // if u,v us default, Wmat is unit transformation
  Kernel::DblMatrix Wmat(3, 3, true);
  // derive rotation from u0,v0 u0||ki to u,v
  if (!m_isUVdefault) {
    Wmat[0][0] = m_UProj[0];
    Wmat[1][0] = m_UProj[1];
    Wmat[2][0] = m_UProj[2];
    Wmat[0][1] = m_VProj[0];
    Wmat[1][1] = m_VProj[1];
    Wmat[2][1] = m_VProj[2];
    Wmat[0][2] = m_WProj[0];
    Wmat[1][2] = m_WProj[1];
    Wmat[2][2] = m_WProj[2];
  }
  if (ScaleID == OrthogonalHKLScale) {
    std::vector<Kernel::V3D> dim_directions;
    std::vector<Kernel::V3D> uv(2);
    uv[0] = m_UProj;
    uv[1] = m_VProj;
    dim_directions = Kernel::V3D::makeVectorsOrthogonal(uv);
    for (size_t i = 0; i < 3; ++i)
      for (size_t j = 0; j < 3; ++j)
        Wmat[i][j] = dim_directions[j][i];
  }
  // Now define lab frame to target frame transformation
  Kernel::DblMatrix Scale(3, 3, true);
  Kernel::DblMatrix Transf(3, 3, true);
  boost::shared_ptr<Geometry::OrientedLattice> spLatt;
  if (UnitUB)
    spLatt = boost::shared_ptr<Geometry::OrientedLattice>(
        new Geometry::OrientedLattice(1, 1, 1));
  else
    spLatt = TargWSDescription.getLattice();

  switch (ScaleID) {
  case NoScaling: //< momentums in A^-1
  {
    Transf = spLatt->getU();
    break;
  }
  case SingleScale: //< momentums divided by  2*Pi/Lattice -- equivalent to
    // d-spacing in some sense
    {
      double dMax(-1.e+32);
      for (int i = 0; i < 3; i++)
        dMax = (dMax > spLatt->a(i)) ? (dMax) : (spLatt->a(i));
      for (int i = 0; i < 3; i++)
        Scale[i][i] = (2 * M_PI) / dMax;
      Transf = spLatt->getU();
      break;
    }
  case OrthogonalHKLScale: //< each momentum component divided by appropriate
    // lattice parameter; equivalent to hkl for orthogonal
    // axis
    {
      if (spLatt) {
        for (int i = 0; i < 3; i++) {
          Scale[i][i] = (2 * M_PI) / spLatt->a(i);
        }
        Transf = spLatt->getU();
      }
      break;
    }
  case HKLScale: //< non-orthogonal system for non-orthogonal lattice
  {
    if (spLatt)
      Scale = spLatt->getUB() * (2 * M_PI);
    break;
  }

  default:
    throw(std::invalid_argument("unrecognized conversion mode"));
  }
  TargWSDescription.addProperty("W_MATRIX", Wmat.getVector(), true);
  return Transf * Scale * Wmat;
}

/** Build meaningful dimension names for different conversion modes
 * @param TargWSDescription the class-container to keep the dimension names and
 dimension unints
 * @param FrameID -- the ID describing the target transformation frame (lab,
 sample, hkl)
 * @param ScaleID -- the scale ID which define how the dimensions are scaled

*/
void MDWSTransform::setQ3DDimensionsNames(
    MDEvents::MDWSDescription &TargWSDescription,
    CnvrtToMD::TargetFrame FrameID, CnvrtToMD::CoordScaling ScaleID) const {

  std::vector<Kernel::V3D> dimDirections;
  // set default dimension names:
  std::vector<std::string> dimNames = TargWSDescription.getDimNames();

  // define B-matrix and Lattice parameters to one in case if no OrientedLattice
  // is there
  Kernel::DblMatrix Bm(3, 3, true);
  std::vector<double> LatPar(3, 1);
  if (TargWSDescription.hasLattice()) { // redefine B-matrix and Lattice
                                        // parameters from real oriented lattice
                                        // if there is one
    auto spLatt = TargWSDescription.getLattice();
    Bm = spLatt->getB();
    for (int i = 0; i < 3; i++)
      LatPar[i] = spLatt->a(i);
  }
  if (FrameID == CnvrtToMD::AutoSelect)
    FrameID = findTargetFrame(TargWSDescription);

  switch (FrameID) {
  case (CnvrtToMD::LabFrame): {
    dimNames[0] = "Q_lab_x";
    dimNames[1] = "Q_lab_y";
    dimNames[2] = "Q_lab_z";
    TargWSDescription.setCoordinateSystem(Mantid::Kernel::QLab);
    break;
  }
  case (CnvrtToMD::SampleFrame): {
    dimNames[0] = "Q_sample_x";
    dimNames[1] = "Q_sample_y";
    dimNames[2] = "Q_sample_z";
    TargWSDescription.setCoordinateSystem(Mantid::Kernel::QSample);
    break;
  }
  case (CnvrtToMD::HKLFrame): {
    dimNames[0] = "H";
    dimNames[1] = "K";
    dimNames[2] = "L";
    TargWSDescription.setCoordinateSystem(Mantid::Kernel::HKL);
    break;
  }
  default:
    throw(std::invalid_argument(" Unknown or undefined Target Frame ID"));
  }

  dimDirections.resize(3);
  dimDirections[0] = m_UProj;
  dimDirections[1] = m_VProj;
  dimDirections[2] = m_WProj;
  if (ScaleID == OrthogonalHKLScale) {
    std::vector<Kernel::V3D> uv(2);
    uv[0] = m_UProj;
    uv[1] = m_VProj;
    dimDirections = Kernel::V3D::makeVectorsOrthogonal(uv);
  }
  // axis names:
  if ((FrameID == CnvrtToMD::LabFrame) || (FrameID == CnvrtToMD::SampleFrame))
    for (int i = 0; i < 3; i++)
      TargWSDescription.setDimName(i, dimNames[i]);
  else
    for (int i = 0; i < 3; i++)
      TargWSDescription.setDimName(
          i, MDEvents::makeAxisName(dimDirections[i], dimNames));

  if (ScaleID == NoScaling) {
    for (int i = 0; i < 3; i++)
      TargWSDescription.setDimUnit(i, "A^-1");
  }
  if (ScaleID == SingleScale) {
    double dMax(-1.e+32);
    for (int i = 0; i < 3; i++)
      dMax = (dMax > LatPar[i]) ? (dMax) : (LatPar[i]);
    for (int i = 0; i < 3; i++)
      TargWSDescription.setDimUnit(
          i, "in " + MDEvents::sprintfd(2 * M_PI / dMax, 1.e-3) + " A^-1");
  }
  if ((ScaleID == OrthogonalHKLScale) || (ScaleID == HKLScale)) {
    // get the length along each of the axes
    std::vector<double> len;
    Kernel::V3D x;
    x = Bm * dimDirections[0];
    len.push_back(2 * M_PI * x.norm());
    x = Bm * dimDirections[1];
    len.push_back(2 * M_PI * x.norm());
    x = Bm * dimDirections[2];
    len.push_back(2 * M_PI * x.norm());
    for (int i = 0; i < 3; i++)
      TargWSDescription.setDimUnit(
          i, "in " + MDEvents::sprintfd(len[i], 1.e-3) + " A^-1");
  }
}

void MDWSTransform::setModQDimensionsNames(
    MDEvents::MDWSDescription &TargWSDescription,
    const std::string &QScaleRequested) const { // TODO: nothing meaningful has
                                                // been done at the moment,
                                                // should enable scaling if
                                                // different coord transf modes?

  UNUSED_ARG(TargWSDescription);
  UNUSED_ARG(QScaleRequested);
}
/** check if input vector is defined */
bool MDWSTransform::v3DIsDefault(const std::vector<double> &vect,
                                 const std::string &message) const {
  bool def = true;
  if (!vect.empty()) {

    if (vect.size() == 3) {
      def = false;
    } else {
      g_Log.warning() << message;
    }
  }
  return def;
}
//
void MDWSTransform::setUVvectors(const std::vector<double> &ut,
                                 const std::vector<double> &vt,
                                 const std::vector<double> &wt) {
  // identify if u,v are present among input parameters and use defaults if not
  bool u_default(true), v_default(true), w_default(true);

  u_default =
      v3DIsDefault(ut, " u projection vector specified but its dimensions are "
                       "not equal to 3, using default values [1,0,0]\n");
  v_default =
      v3DIsDefault(vt, " v projection vector specified but its dimensions are "
                       "not equal to 3, using default values [0,1,0]\n");
  w_default =
      v3DIsDefault(wt, " w projection vector specified but its dimensions are "
                       "not equal to 3, using default values [0,0,1]\n");

  if (u_default) {
    m_UProj = Kernel::V3D(1., 0., 0.);
  } else {
    m_UProj = Kernel::V3D(ut[0], ut[1], ut[2]);
  }

  if (v_default) {
    m_VProj = Kernel::V3D(0., 1., 0.);
  } else {
    m_VProj = Kernel::V3D(vt[0], vt[1], vt[2]);
  }

  if (w_default) {
    m_WProj = Kernel::V3D(0., 0., 1.);
  } else {
    m_WProj = Kernel::V3D(wt[0], wt[1], wt[2]);
  }

  m_isUVdefault = u_default && v_default && w_default;

  // check if u, v, w are coplanar
  if (fabs((m_UProj.cross_prod(m_VProj)).scalar_prod(m_WProj)) <
      Kernel::Tolerance) {
    m_UProj = Kernel::V3D(1., 0., 0.);
    m_VProj = Kernel::V3D(0., 1., 0.);
    m_WProj = Kernel::V3D(0., 0., 1.);
    m_isUVdefault = true;
    throw std::invalid_argument("Projections are coplanar");
  }
}
/** function which convert input string representing coordinate scaling to
 * correspondent enum */
CoordScaling MDWSTransform::getQScaling(const std::string &ScID) const {
  int nScaling = Kernel::Strings::isMember(m_QScalingID, ScID);

  if (nScaling < 0)
    throw(std::invalid_argument(" The Q scale with ID: " + ScID +
                                " is unavailable"));

  return CoordScaling(nScaling);
}
/** Method to convert enum describing target scaling to its string
 * representation */
std::string
MDWSTransform::getQScaling(const CnvrtToMD::CoordScaling ScaleID) const {
  return m_QScalingID[ScaleID];
}

/** function which convert input string representing Target coordinate frame to
 * correspondent enum */
TargetFrame MDWSTransform::getTargetFrame(const std::string &FrameID) const {
  int nFrame = Kernel::Strings::isMember(m_TargFramesID, FrameID);

  if (nFrame < 0)
    throw(std::invalid_argument(" The Target Frame with ID: " + FrameID +
                                " is unavailable"));

  return TargetFrame(nFrame);
}
/** Method to convert enum describing target coordinate frame to its string
 * representation */
std::string
MDWSTransform::getTargetFrame(const CnvrtToMD::TargetFrame FrameID) const {
  return m_TargFramesID[FrameID];
}

//
MDWSTransform::MDWSTransform()
    : m_isUVdefault(true), m_QScalingID(NCoordScalings),
      m_TargFramesID(NTargetFrames) {
  m_UProj[0] = 1;
  m_UProj[1] = 0;
  m_UProj[2] = 0;
  m_VProj[0] = 0;
  m_VProj[1] = 1;
  m_VProj[2] = 0;
  m_WProj[0] = 0;
  m_WProj[1] = 0;
  m_WProj[2] = 1;

  m_QScalingID[NoScaling] = "Q in A^-1";
  m_QScalingID[SingleScale] = "Q in lattice units";
  m_QScalingID[OrthogonalHKLScale] = "Orthogonal HKL";
  m_QScalingID[HKLScale] = "HKL";

  m_TargFramesID[AutoSelect] = "AutoSelect";
  m_TargFramesID[LabFrame] = "Q_lab";
  m_TargFramesID[SampleFrame] = "Q_sample";
  m_TargFramesID[HKLFrame] = "HKL";
}
}
}
