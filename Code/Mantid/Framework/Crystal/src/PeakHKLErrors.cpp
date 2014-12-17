/*
 * PeakHKLErrors.cpp
 *
 *  Created on: Jan 26, 2013
 *      Author: ruth
 */
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/IPeak.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidCrystal/PeakHKLErrors.h"
#include "MantidCrystal/SCDPanelErrors.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"
#include <cctype>
#include <string>

using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using Mantid::Geometry::CompAssembly;
using Mantid::Geometry::IObjComponent_const_sptr;
using Mantid::Geometry::IComponent_const_sptr;

namespace Mantid {

namespace Crystal {
namespace {
/// static logger
Kernel::Logger g_log("PeakHKLErrors");
}

DECLARE_FUNCTION(PeakHKLErrors)

PeakHKLErrors::PeakHKLErrors() : ParamFunction(), IFunction1D() {
  OptRuns = "";
  PeakWorkspaceName = "";
  initMode = 0;
}

PeakHKLErrors::~PeakHKLErrors() {}

void PeakHKLErrors::init() {
  declareParameter("SampleXOffset", 0.0, "Sample x offset");
  declareParameter("SampleYOffset", 0.0, "Sample y offset");
  declareParameter("SampleZOffset", 0.0, "Sample z offset");
  declareParameter("GonRotx", 0.0,
                   "3rd Rotation of Goniometer about the x axis");
  declareParameter("GonRoty", 0.0,
                   "2nd Rotation of Goniometer about the y axis");
  declareParameter("GonRotz", 0.0,
                   "1st Rotation of Goniometer about the z axis");
  initMode = 1;
  if (OptRuns == "")
    return;

  initMode = 2;
  setUpOptRuns();
}
/**
 * Declares parameters for the chi,phi and omega angles for the run numbers
 * where these will be optimized.
 */
void PeakHKLErrors::setUpOptRuns() {

  std::vector<std::string> OptRunNums;
  std::string OptRunstemp(OptRuns);
  if (OptRuns.size() > 0 && OptRuns.at(0) == '/')
    OptRunstemp = OptRunstemp.substr(1, OptRunstemp.size() - 1);

  if (OptRunstemp.size() > 0 && OptRunstemp.at(OptRunstemp.size() - 1) == '/')
    OptRunstemp = OptRunstemp.substr(0, OptRunstemp.size() - 1);

  boost::split(OptRunNums, OptRunstemp, boost::is_any_of("/"));

  for (size_t i = 0; i < OptRunNums.size(); i++) {
    declareParameter("phi" + OptRunNums[i], 0.0,
                     "Phi sample orientation value");
    declareParameter("chi" + OptRunNums[i], 0.0,
                     "Chi sample orientation value");
    declareParameter("omega" + OptRunNums[i], 0.0,
                     "Omega sample orientation value");
  }
}

/**
 * "Clones" a parameter map duplicating all Parameters with double,V3D,int and
 *string parameter
 * values that apply to the given component and all(most) of the components
 *children.
 *
 * If the component is an instrument, this parameter map can be used to create
 * a separate parameterized instrument close to the original instrument.
 *
 * NOTE: For speed purposes, if a component( or subcomponent) has too many
 *children(180
 * or more),the parameters corresponding to these children( and subchildren)
 *will not
 * be added to the parameter map
 *
 *
 * @param pmap       The new parameter map to which the new Parameters are to be
 *added
 *
 * @param component  The component along with most of its children and
 *subchildren for
 *                   which Parameters that correspond to these will be
 *considered.
 *
 * @param pmapSv     The old parameter map from which copies of the parameters
 *corresponding
 *                   to the given component or subchild are added to pmap
 */
void
PeakHKLErrors::cLone(boost::shared_ptr<Geometry::ParameterMap> &pmap,
                     boost::shared_ptr<const Geometry::IComponent> component,
                     boost::shared_ptr<const Geometry::ParameterMap> &pmapSv) {
  if (!component)
    return;
  if (component->isParametrized()) {

    std::set<std::string> nms = pmapSv->names(component.get());
    for (std::set<std::string>::iterator it = nms.begin(); it != nms.end();
         ++it) {

      if (pmapSv->contains(component.get(), *it, "double")) {
        std::vector<double> dparams =
            pmapSv->getDouble(component->getName(), *it);
        pmap->addDouble(component.get(), *it, dparams[0]);
        continue;
      }

      if (pmapSv->contains(component.get(), *it, "V3D")) {
        std::vector<V3D> V3Dparams = pmapSv->getV3D(component->getName(), *it);
        pmap->addV3D(component.get(), *it, V3Dparams[0]);
        continue;
      }

      if (pmapSv->contains(component.get(), *it, "int")) {
        std::vector<int> iparams =
            pmapSv->getType<int>(component->getName(), *it);
        pmap->addInt(component.get(), *it, iparams[0]);
        continue;
      }

      if (pmapSv->contains(component.get(), *it, "string")) {
        std::vector<std::string> sparams =
            pmapSv->getString(component->getName(), *it);
        pmap->addString(component.get(), *it, sparams[0]);
        continue;
      }

      if (pmapSv->contains(component.get(), *it, "Quat")) {
        std::vector<Kernel::Quat> sparams =
            pmapSv->getType<Kernel::Quat>(component->getName(), *it);
        pmap->addQuat(component.get(), *it, sparams[0]);
        continue;
      }
    }

    boost::shared_ptr<const CompAssembly> parent =
        boost::dynamic_pointer_cast<const CompAssembly>(component);
    if (parent && parent->nelements() < 180) //# need speed up. Assume pixel
      // elements of a Panel have no
      // attributes
      for (int child = 0; child < parent->nelements(); child++) {
        boost::shared_ptr<const Geometry::IComponent> kid =
            boost::const_pointer_cast<const Geometry::IComponent>(
                parent->getChild(child));
        if (kid)
          cLone(pmap, kid, pmapSv);
      }
  }
}

/**
 * Creates a new parameterized instrument for which the parameter values can be
 *changed
 *
 * @param Peaks - a PeaksWorkspace used to get the original instrument.  The
 *instrument from the 0th peak is
 *                the one that is used.
 *
 * NOTE: All the peaks in the PeaksWorkspace must use the same instrument.
 */
boost::shared_ptr<Geometry::Instrument>
PeakHKLErrors::getNewInstrument(PeaksWorkspace_sptr Peaks) const {
  Geometry::Instrument_const_sptr instSave = Peaks->getPeak(0).getInstrument();
  boost::shared_ptr<Geometry::ParameterMap> pmap(new Geometry::ParameterMap());
  boost::shared_ptr<const Geometry::ParameterMap> pmapSv =
      instSave->getParameterMap();

  if (!instSave) {
    g_log.error(" Peaks workspace does not have an instrument");
    throw std::invalid_argument(" Not all peaks have an instrument");
  }
  boost::shared_ptr<Geometry::Instrument> instChange(
      new Geometry::Instrument());

  if (!instSave->isParametrized()) {

    boost::shared_ptr<Geometry::Instrument> instClone(instSave->clone());
    boost::shared_ptr<Geometry::Instrument> Pinsta(
        new Geometry::Instrument(instSave, pmap));

    instChange = Pinsta;
  } else // catch(... )
  {
    boost::shared_ptr<Geometry::Instrument> P1(
        new Geometry::Instrument(instSave->baseInstrument(), pmap));
    instChange = P1;
  }

  if (!instChange) {
    g_log.error("Cannot 'clone' instrument");
    throw std::logic_error("Cannot clone instrument");
  }
  //------------------"clone" orig instruments pmap -------------------

  cLone(pmap, instSave, pmapSv);
  IComponent_const_sptr sample = instChange->getSample();
  V3D sampPos = sample->getRelativePos();
  V3D sampOffsets(getParameter("SampleXOffset"), getParameter("SampleYOffset"),
                  getParameter("SampleZOffset"));

  pmap->addPositionCoordinate(sample.get(), std::string("x"),
                              sampPos.X() + sampOffsets.X());
  pmap->addPositionCoordinate(sample.get(), std::string("y"),
                              sampPos.Y() + sampOffsets.Y());
  pmap->addPositionCoordinate(sample.get(), std::string("z"),
                              sampPos.Z() + sampOffsets.Z());

  return instChange;
}

/**
 * Updates the map from run number to GoniometerMatrix
 *
 * @param Peaks    The PeaksWorkspace whose peaks contain the run numbers
 *                   along with the corresponding GoniometerMatrix
 *
 * @param OptRuns  A '/' separated "list" of run numbers to include in the
 *                  map. This string must also start and end with a '/'
 *
 * @param Res      The resultant map.
 */
void PeakHKLErrors::getRun2MatMap(
    PeaksWorkspace_sptr &Peaks, const std::string &OptRuns,
    std::map<int, Mantid::Kernel::Matrix<double>> &Res) const {

  for (int i = 0; i < Peaks->getNumberPeaks(); ++i) {
    IPeak &peak_old = Peaks->getPeak((int)i);

    int runNum = peak_old.getRunNumber();
    std::string runNumStr = boost::lexical_cast<std::string>(runNum);
    size_t N = OptRuns.find("/" + runNumStr + "/");
    if (N < OptRuns.size()) {
      double chi =
          getParameter("chi" + boost::lexical_cast<std::string>(runNumStr));
      double phi =
          getParameter("phi" + boost::lexical_cast<std::string>(runNumStr));
      double omega =
          getParameter("omega" + boost::lexical_cast<std::string>(runNumStr));
      Mantid::Geometry::Goniometer uniGonio;
      uniGonio.makeUniversalGoniometer();
      uniGonio.setRotationAngle("phi", phi);
      uniGonio.setRotationAngle("chi", chi);
      uniGonio.setRotationAngle("omega", omega);
      Res[runNum] = uniGonio.getR();
    }
  }
}

/**
 *  Returns the matrix corresponding to a rotation of theta(degrees) around axis
 *
 *  @param theta   the angle of rotation in degrees
 *  @param  axis   either x,y,z, or X,Y, or Z.
 *
 *  @return The matrix that corresponds to this action.
 *
 *  Replace by Quats?
 */
Matrix<double> PeakHKLErrors::RotationMatrixAboutRegAxis(double theta,
                                                         char axis) {
  int cint = toupper(axis);
  char c = (char)cint;
  std::string S(std::string("") + c);
  size_t axisPos = std::string("XYZ").find(S);

  if (axisPos > 2) {
    Matrix<double> Res(3, 3, true);

    return Res;
  }
  double rTheta = theta / 180 * M_PI;
  Matrix<double> Res(3, 3);
  Res.zeroMatrix();
  Res[axisPos][axisPos] = 1.0;
  Res[(axisPos + 1) % 3][(axisPos + 1) % 3] = cos(rTheta);
  Res[(axisPos + 1) % 3][(axisPos + 2) % 3] = -sin(rTheta);
  Res[(axisPos + 2) % 3][(axisPos + 2) % 3] = cos(rTheta);
  Res[(axisPos + 2) % 3][(axisPos + 1) % 3] = sin(rTheta);
  return Res;
}

/**
  *  Returns the derivative of the matrix corresponding to a rotation of
  *theta(degrees) around axis
  *  with respect to the angle or rotation in degrees.
  *
  *  @param theta   the angle of rotation in degrees
  *  @param  axis   either x,y,z, or X,Y, or Z.
  *
  *  @return The derivative of the matrix that corresponds to this action with
  *respect to degree rotation.
  */
Matrix<double> PeakHKLErrors::DerivRotationMatrixAboutRegAxis(double theta,
                                                              char axis) {
  int cint = toupper(axis);
  char c = (char)cint;
  std::string S(std::string("") + c);
  size_t axisPos = std::string("XYZ").find(S);

  if (axisPos > 2) {
    Matrix<double> Res(3, 3, true);

    return Res;
  }
  double rTheta = theta / 180 * M_PI;
  Matrix<double> Res(3, 3);
  Res.zeroMatrix();
  Res[axisPos][axisPos] = 0.0;
  Res[(axisPos + 1) % 3][(axisPos + 1) % 3] = -sin(rTheta);
  Res[(axisPos + 1) % 3][(axisPos + 2) % 3] = -cos(rTheta);
  Res[(axisPos + 2) % 3][(axisPos + 2) % 3] = -sin(rTheta);
  Res[(axisPos + 2) % 3][(axisPos + 1) % 3] = cos(rTheta);
  return Res * (M_PI / 180.);
}

/**
 * Calculates the h,k, and l offsets from an integer for (some of )the peaks,
 *given the parameter values.
 *
 * @param out  For each peak there are 3 consecutive elements in this array. The
 *first is for the h offset from an
 *             integer, the second is the k offset and the 3rd is the l offset
 * @param xValues  xValues give the index in the PeaksWorkspace for the peak.
 *For each peak considered there are
 *              three consecutive entries all with the same index
 * @param nData The size of the xValues and out arrays
 */
void PeakHKLErrors::function1D(double *out, const double *xValues,
                               const size_t nData) const {
  PeaksWorkspace_sptr Peaks =
      AnalysisDataService::Instance().retrieveWS<PeaksWorkspace>(
          PeakWorkspaceName);

  boost::shared_ptr<Geometry::Instrument> instNew = getNewInstrument(Peaks);

  if (!Peaks)
    throw std::invalid_argument("Peaks not stored under the name " +
                                PeakWorkspaceName);

  std::map<int, Mantid::Kernel::Matrix<double>> RunNum2GonMatrixMap;
  getRun2MatMap(Peaks, OptRuns, RunNum2GonMatrixMap);
  const DblMatrix &UBx = Peaks->sample().getOrientedLattice().getUB();

  DblMatrix UBinv(UBx);
  UBinv.Invert();
  UBinv /= (2 * M_PI);

  double GonRotx = getParameter("GonRotx");
  double GonRoty = getParameter("GonRoty");
  double GonRotz = getParameter("GonRotz");
  Matrix<double> GonRot = RotationMatrixAboutRegAxis(GonRotx, 'x') *
                          RotationMatrixAboutRegAxis(GonRoty, 'y') *
                          RotationMatrixAboutRegAxis(GonRotz, 'z');

  double ChiSqTot = 0.0;
  for (size_t i = 0; i < nData; i += 3) {
    int peakNum = (int)(.5 + xValues[i]);
    IPeak &peak_old = Peaks->getPeak(peakNum);

    int runNum = peak_old.getRunNumber();
    std::string runNumStr = boost::lexical_cast<std::string>(runNum);
    Peak peak =
        SCDPanelErrors::createNewPeak(peak_old, instNew, 0, peak_old.getL1());

    size_t N = OptRuns.find("/" + runNumStr + "/");
    if (N < OptRuns.size()) {
      peak.setGoniometerMatrix(GonRot * RunNum2GonMatrixMap[runNum]);

    } else {
      peak.setGoniometerMatrix(GonRot * peak.getGoniometerMatrix());
    }

    V3D hkl = UBinv * peak.getQSampleFrame();

    for (int k = 0; k < 3; k++) {
      double d1 = hkl[k] - floor(hkl[k]);
      if (d1 > .5)
        d1 = d1 - 1;
      if (d1 < -.5)
        d1 = d1 + 1;

      out[i + k] = d1;
      ChiSqTot += d1 * d1;
    }
  }

  g_log.debug() << "------------------------Function---------------------------"
                   "--------------------" << std::endl;
  for (size_t p = 0; p < nParams(); p++) {
    g_log.debug() << parameterName(p) << "(" << getParameter(p) << "),";
    if ((p + 1) % 6 == 0)
      g_log.debug() << std::endl;
  }
  g_log.debug() << std::endl;
  g_log.debug() << "Off constraints=";
  for (size_t p = 0; p < nParams(); p++) {
    IConstraint *constr = getConstraint(p);
    if (constr)
      if ((constr->check() > 0))
        g_log.debug() << "(" << parameterName(p) << "=" << constr->check()
                      << ");";
  }
  g_log.debug() << std::endl;

  g_log.debug() << "    Chi**2 = " << ChiSqTot << "     nData = " << nData
                << std::endl;
}

void PeakHKLErrors::functionDeriv1D(Jacobian *out, const double *xValues,
                                    const size_t nData) {
  PeaksWorkspace_sptr Peaks =
      AnalysisDataService::Instance().retrieveWS<PeaksWorkspace>(
          PeakWorkspaceName);
  boost::shared_ptr<Geometry::Instrument> instNew = getNewInstrument(Peaks);

  const DblMatrix &UB = Peaks->sample().getOrientedLattice().getUB();
  DblMatrix UBinv(UB);
  UBinv.Invert();
  UBinv /= 2 * M_PI;

  double GonRotx = getParameter("GonRotx");
  double GonRoty = getParameter("GonRoty");
  double GonRotz = getParameter("GonRotz");
  Matrix<double> InvGonRotxMat = RotationMatrixAboutRegAxis(GonRotx, 'x');
  Matrix<double> InvGonRotyMat = RotationMatrixAboutRegAxis(GonRoty, 'y');
  Matrix<double> InvGonRotzMat = RotationMatrixAboutRegAxis(GonRotz, 'z');
  Matrix<double> GonRot = InvGonRotxMat * InvGonRotyMat * InvGonRotzMat;

  InvGonRotxMat.Invert();
  InvGonRotyMat.Invert();
  InvGonRotzMat.Invert();

  std::map<int, Kernel::Matrix<double>> RunNums2GonMatrix;
  getRun2MatMap(Peaks, OptRuns, RunNums2GonMatrix);

  g_log.debug()
      << "----------------------------Derivative------------------------"
      << std::endl;

  V3D samplePosition = instNew->getSample()->getPos();
  IPeak &ppeak = Peaks->getPeak(0);
  double L0 = ppeak.getL1();
  double velocity = (L0 + ppeak.getL2()) / ppeak.getTOF();

  double K =
      2 * M_PI / ppeak.getWavelength() / velocity; // 2pi/lambda = K* velocity
  V3D beamDir = instNew->getBeamDirection();

  size_t paramNums[] = {parameterIndex(std::string("SampleXOffset")),
                        parameterIndex(std::string("SampleYOffset")),
                        parameterIndex(std::string("SampleZOffset"))};

  for (size_t i = 0; i < nData; i += 3) {
    int peakNum = (int)(.5 + xValues[i]);
    IPeak &peak_old = Peaks->getPeak(peakNum);
    Peak peak =
        SCDPanelErrors::createNewPeak(peak_old, instNew, 0, peak_old.getL1());

    int runNum = peak_old.getRunNumber();
    std::string runNumStr = boost::lexical_cast<std::string>(runNum);

    for (int kk = 0; kk < (int)nParams(); kk++) {
      out->set(i, kk, 0.0);
      out->set(i + 1, kk, 0.0);
      out->set(i + 1, kk, 0.0);
    }

    double chi, phi, omega;
    size_t chiParamNum, phiParamNum, omegaParamNum;

    size_t N = OptRuns.find("/" + runNumStr);
    if (N < OptRuns.size()) {
      chi = getParameter("chi" + (runNumStr));
      phi = getParameter("phi" + (runNumStr));
      omega = getParameter("omega" + (runNumStr));

      peak.setGoniometerMatrix(GonRot * RunNums2GonMatrix[runNum]);

      chiParamNum = parameterIndex("chi" + (runNumStr));
      phiParamNum = parameterIndex("phi" + (runNumStr));
      omegaParamNum = parameterIndex("omega" + (runNumStr));
    } else {

      Geometry::Goniometer Gon(peak.getGoniometerMatrix());
      std::vector<double> phichiOmega = Gon.getEulerAngles("YZY");
      chi = phichiOmega[1];
      phi = phichiOmega[2];
      omega = phichiOmega[0];
      // peak.setGoniometerMatrix( GonRot*Gon.getR());
      chiParamNum = phiParamNum = omegaParamNum = nParams() + 10;
      peak.setGoniometerMatrix(GonRot * peak.getGoniometerMatrix());
    }
    // NOTE:Use getQLabFrame except for below.
    // For parameters the getGoniometerMatrix should remove GonRot, for derivs
    // wrt GonRot*, wrt chi*,phi*,etc.

    V3D hkl = UBinv * peak.getQSampleFrame();

    // Deriv wrt chi phi and omega
    if (phiParamNum < nParams()) {
      Matrix<double> chiMatrix = RotationMatrixAboutRegAxis(chi, 'z');
      Matrix<double> phiMatrix = RotationMatrixAboutRegAxis(phi, 'y');
      Matrix<double> omegaMatrix = RotationMatrixAboutRegAxis(omega, 'y');

      Matrix<double> dchiMatrix = DerivRotationMatrixAboutRegAxis(chi, 'z');
      Matrix<double> dphiMatrix = DerivRotationMatrixAboutRegAxis(phi, 'y');
      Matrix<double> domegaMatrix = DerivRotationMatrixAboutRegAxis(omega, 'y');

      Matrix<double> InvG = omegaMatrix * chiMatrix * phiMatrix;
      InvG.Invert();
      // Calculate Derivatives wrt chi(phi,omega) in degrees
      Matrix<double> R = omegaMatrix * chiMatrix * dphiMatrix;
      Matrix<double> InvR = InvG * R * InvG * -1;
      V3D lab = peak.getQLabFrame();
      V3D Dhkl0 = UBinv * InvR * lab;

      R = omegaMatrix * dchiMatrix * phiMatrix;
      InvR = InvG * R * InvG * -1;
      V3D Dhkl1 = UBinv * InvR * peak.getQLabFrame();

      R = domegaMatrix * chiMatrix * phiMatrix;
      InvR = InvG * R * InvG * -1;
      V3D Dhkl2 =
          UBinv * InvR * peak.getQLabFrame(); // R.transpose should be R inverse

      out->set(i, chiParamNum, Dhkl1[0]);
      out->set(i + 1, chiParamNum, Dhkl1[1]);
      out->set(i + 2, chiParamNum, Dhkl1[2]);
      out->set(i, phiParamNum, Dhkl0[0]);
      out->set(i + 1, phiParamNum, Dhkl0[1]);
      out->set(i + 2, phiParamNum, Dhkl0[2]);
      out->set(i, omegaParamNum, Dhkl2[0]);
      out->set(i + 1, omegaParamNum, Dhkl2[1]);
      out->set(i + 2, omegaParamNum, Dhkl2[2]);

    } // if optimize for chi phi and omega on this peak

    //------------------------Goniometer Rotation Derivatives
    //-----------------------
    Matrix<double> InvGonRot(GonRot);
    InvGonRot.Invert();
    Matrix<double> InvGon = InvGonRot * peak.getGoniometerMatrix();
    InvGon.Invert();
    V3D DGonx = (UBinv * InvGon * InvGonRotzMat * InvGonRotyMat *
                 DerivRotationMatrixAboutRegAxis(
                     -GonRotx, 'x') * // - gives inverse of GonRot
                 peak.getQLabFrame()) *
                -1;

    V3D DGony = (UBinv * InvGon * InvGonRotzMat *
                 DerivRotationMatrixAboutRegAxis(-GonRoty, 'y') *
                 InvGonRotxMat * peak.getQLabFrame()) *
                -1;
    V3D DGonz =
        (UBinv * InvGon * DerivRotationMatrixAboutRegAxis(-GonRotz, 'z') *
         InvGonRotyMat * InvGonRotxMat * peak.getQLabFrame()) *
        -1;

    size_t paramnum = parameterIndex("GonRotx");
    out->set(i, paramnum, DGonx[0]);
    out->set(i + 1, paramnum, DGonx[1]);
    out->set(i + 2, paramnum, DGonx[2]);
    out->set(i, parameterIndex("GonRoty"), DGony[0]);
    out->set(i + 1, parameterIndex("GonRoty"), DGony[1]);
    out->set(i + 2, parameterIndex("GonRoty"), DGony[2]);
    out->set(i, parameterIndex("GonRotz"), DGonz[0]);
    out->set(i + 1, parameterIndex("GonRotz"), DGonz[1]);
    out->set(i + 2, parameterIndex("GonRotz"), DGonz[2]);
    //-------------------- Sample Orientation derivatives
    //----------------------------------
    // Qlab = -KV + k|V|*beamdir
    // D = pos-sampPos
    //|V|= vmag=(L0 + D )/tof
    // t1= tof - L0/|V|   {time from sample to pixel}
    // V = D/t1
    V3D D = peak.getDetPos() - samplePosition;
    double vmag = (L0 + D.norm()) / peak.getTOF();
    double t1 = peak.getTOF() - L0 / vmag;
    V3D V = D / t1;

    // Derivs wrt sample x, y, z
    // Ddsx =( - 1, 0, 0),  d|D|^2/dsx -> 2|D|d|D|/dsx =d(tranp(D)* D)/dsx =2
    // Ddsx* tranp(D)
    //|D| also called Dmag
    V3D Dmagdsxsysz(D);
    Dmagdsxsysz *= (-1 / D.norm());

    V3D vmagdsxsysz = Dmagdsxsysz / peak.getTOF();

    V3D t1dsxsysz = vmagdsxsysz * (L0 / vmag / vmag);
    Matrix<double> Gon = peak.getGoniometerMatrix();
    Gon.Invert();

    // x=0 is deriv wrt SampleXoffset, x=1 is deriv wrt SampleYoffset, etc.
    for (int x = 0; x < 3; x++) {
      V3D pp;
      pp[x] = 1;
      V3D dQlab1 = pp / -t1 - D * (t1dsxsysz[x] / t1 / t1);
      V3D dQlab2 = beamDir * vmagdsxsysz[x];
      V3D dQlab = dQlab2 - dQlab1;
      dQlab *= K;

      V3D dQSamp = Gon * dQlab;
      V3D dhkl = UBinv * dQSamp;

      out->set(i, paramNums[x], dhkl[0]);
      out->set(i + 1, paramNums[x], dhkl[1]);
      out->set(i + 2, paramNums[x], dhkl[2]);
    }
  }
}
}
}
