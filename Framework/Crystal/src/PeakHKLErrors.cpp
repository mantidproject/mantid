// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/*
 * PeakHKLErrors.cpp
 *
 *  Created on: Jan 26, 2013
 *      Author: ruth
 */
#include "MantidCrystal/PeakHKLErrors.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/Sample.h"
#include "MantidGeometry/Crystal/IPeak.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/Unit.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/math/special_functions/round.hpp>

using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Kernel::Units;
using Mantid::Geometry::IPeak;

namespace Mantid::Crystal {
namespace {
/// static logger
Kernel::Logger g_log("PeakHKLErrors");
} // namespace

DECLARE_FUNCTION(PeakHKLErrors)

PeakHKLErrors::PeakHKLErrors() : ParamFunction(), IFunction1D(), OptRuns(""), PeakWorkspaceName("") { initMode = 0; }

void PeakHKLErrors::init() {
  declareParameter("SampleXOffset", 0.0, "Sample x offset");
  declareParameter("SampleYOffset", 0.0, "Sample y offset");
  declareParameter("SampleZOffset", 0.0, "Sample z offset");
  declareParameter("GonRotx", 0.0, "3rd Rotation of Goniometer about the x axis");
  declareParameter("GonRoty", 0.0, "2nd Rotation of Goniometer about the y axis");
  declareParameter("GonRotz", 0.0, "1st Rotation of Goniometer about the z axis");
  initMode = 1;
  if (OptRuns.empty())
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
  if (!OptRuns.empty() && OptRuns.at(0) == '/')
    OptRunstemp = OptRunstemp.substr(1, OptRunstemp.size() - 1);

  if (!OptRunstemp.empty() && OptRunstemp.at(OptRunstemp.size() - 1) == '/')
    OptRunstemp.pop_back();

  boost::split(OptRunNums, OptRunstemp, boost::is_any_of("/"));

  for (const auto &OptRunNum : OptRunNums) {
    declareParameter("phi" + OptRunNum, 0.0, "Phi sample orientation value");
    declareParameter("chi" + OptRunNum, 0.0, "Chi sample orientation value");
    declareParameter("omega" + OptRunNum, 0.0, "Omega sample orientation value");
  }
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
void PeakHKLErrors::getRun2MatMap(PeaksWorkspace_sptr &Peaks, const std::string &OptRuns,
                                  std::map<int, Mantid::Kernel::Matrix<double>> &Res) const {

  for (int i = 0; i < Peaks->getNumberPeaks(); ++i) {
    const Geometry::IPeak &peak_old = Peaks->getPeak(i);

    int runNum = peak_old.getRunNumber();
    std::string runNumStr = std::to_string(runNum);
    size_t N = OptRuns.find("/" + runNumStr + "/");
    if (N < OptRuns.size()) {
      double chi = getParameter("chi" + boost::lexical_cast<std::string>(runNumStr));
      double phi = getParameter("phi" + boost::lexical_cast<std::string>(runNumStr));
      double omega = getParameter("omega" + boost::lexical_cast<std::string>(runNumStr));
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
Matrix<double> PeakHKLErrors::RotationMatrixAboutRegAxis(double theta, char axis) {
  int cint = toupper(axis);
  auto c = static_cast<char>(cint);
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
Matrix<double> PeakHKLErrors::DerivRotationMatrixAboutRegAxis(double theta, char axis) {
  int cint = toupper(axis);
  auto c = static_cast<char>(cint);
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
void PeakHKLErrors::function1D(double *out, const double *xValues, const size_t nData) const {
  PeaksWorkspace_sptr peaksWs = AnalysisDataService::Instance().retrieveWS<PeaksWorkspace>(PeakWorkspaceName);

  if (!peaksWs)
    throw std::invalid_argument("Peaks not stored under the name " + PeakWorkspaceName);

  V3D const samplePosition = peaksWs->componentInfo().samplePosition();

  std::map<int, Mantid::Kernel::Matrix<double>> RunNum2GonMatrixMap;
  getRun2MatMap(peaksWs, OptRuns, RunNum2GonMatrixMap);
  const DblMatrix &UBx = peaksWs->sample().getOrientedLattice().getUB();

  DblMatrix UBinv(UBx);
  UBinv.Invert();
  UBinv /= (2 * M_PI);

  double GonRotx = getParameter("GonRotx");
  double GonRoty = getParameter("GonRoty");
  double GonRotz = getParameter("GonRotz");
  Matrix<double> GonRot = RotationMatrixAboutRegAxis(GonRotx, 'x') * RotationMatrixAboutRegAxis(GonRoty, 'y') *
                          RotationMatrixAboutRegAxis(GonRotz, 'z');

  double ChiSqTot = 0.0;
  for (size_t i = 0; i < nData; i += 3) {
    int peakNum = boost::math::iround(xValues[i]);
    const Peak &peak_old = peaksWs->getPeak(peakNum);

    int runNum = peak_old.getRunNumber();
    std::string runNumStr = std::to_string(runNum);
    Peak peak = createNewPeak(peak_old, samplePosition, 0, peak_old.getL1());

    size_t N = OptRuns.find("/" + runNumStr + "/");
    if (N < OptRuns.size()) {
      peak.setGoniometerMatrix(GonRot * RunNum2GonMatrixMap[runNum]);

    } else {
      peak.setGoniometerMatrix(GonRot * peak.getGoniometerMatrix());
    }
    V3D sampOffsets(getParameter("SampleXOffset"), getParameter("SampleYOffset"), getParameter("SampleZOffset"));
    peak.setSamplePos(peak.getSamplePos() + sampOffsets);

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
                   "--------------------\n";
  for (size_t p = 0; p < nParams(); p++) {
    g_log.debug() << parameterName(p) << "(" << getParameter(p) << "),";
  }
  g_log.debug() << '\n';
  g_log.debug() << "Off constraints=";
  for (size_t p = 0; p < nParams(); p++) {
    IConstraint *constr = getConstraint(p);
    if (constr)
      if ((constr->check() > 0))
        g_log.debug() << "(" << parameterName(p) << "=" << constr->check() << ");";
  }
  g_log.debug() << '\n';

  g_log.debug() << "    Chi**2 = " << ChiSqTot << "     nData = " << nData << '\n';
}

void PeakHKLErrors::functionDeriv1D(Jacobian *out, const double *xValues, const size_t nData) {
  PeaksWorkspace_sptr peaksWs = AnalysisDataService::Instance().retrieveWS<PeaksWorkspace>(PeakWorkspaceName);

  if (!peaksWs)
    throw std::invalid_argument("Peaks not stored under the name " + PeakWorkspaceName);

  const DblMatrix &UB = peaksWs->sample().getOrientedLattice().getUB();
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
  getRun2MatMap(peaksWs, OptRuns, RunNums2GonMatrix);

  g_log.debug() << "----------------------------Derivative------------------------\n";

  auto const &componentInfo = peaksWs->componentInfo();
  V3D const samplePosition = componentInfo.samplePosition();
  const IPeak &ppeak = peaksWs->getPeak(0);
  double L0 = ppeak.getL1();
  double velocity = (L0 + ppeak.getL2()) / ppeak.getTOF();

  double K = 2 * M_PI / ppeak.getWavelength() / velocity; // 2pi/lambda = K* velocity
  V3D const beamDir = normalize(samplePosition - componentInfo.sourcePosition());

  const size_t paramNums[] = {parameterIndex(std::string("SampleXOffset")),
                              parameterIndex(std::string("SampleYOffset")),
                              parameterIndex(std::string("SampleZOffset"))};

  for (size_t i = 0; i < nData; i += 3) {
    int peakNum = boost::math::iround(xValues[i]);
    const Peak &peak_old = peaksWs->getPeak(peakNum);
    Peak peak = createNewPeak(peak_old, samplePosition, 0, peak_old.getL1());

    int runNum = peak_old.getRunNumber();
    std::string runNumStr = std::to_string(runNum);

    for (int kk = 0; kk < static_cast<int>(nParams()); kk++) {
      out->set(i, kk, 0.0);
      out->set(i + 1, kk, 0.0);
      out->set(i + 2, kk, 0.0);
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
    V3D sampOffsets(getParameter("SampleXOffset"), getParameter("SampleYOffset"), getParameter("SampleZOffset"));
    peak.setSamplePos(peak.getSamplePos() + sampOffsets);
    // NOTE:Use getQLabFrame except for below.
    // For parameters the getGoniometerMatrix should remove GonRot, for derivs
    // wrt GonRot*, wrt chi*,phi*,etc.

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
      V3D Dhkl2 = UBinv * InvR * peak.getQLabFrame(); // R.transpose should be R inverse

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
                 DerivRotationMatrixAboutRegAxis(-GonRotx, 'x') * // - gives inverse of GonRot
                 peak.getQLabFrame()) *
                -1;

    V3D DGony = (UBinv * InvGon * InvGonRotzMat * DerivRotationMatrixAboutRegAxis(-GonRoty, 'y') * InvGonRotxMat *
                 peak.getQLabFrame()) *
                -1;
    V3D DGonz = (UBinv * InvGon * DerivRotationMatrixAboutRegAxis(-GonRotz, 'z') * InvGonRotyMat * InvGonRotxMat *
                 peak.getQLabFrame()) *
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

Peak PeakHKLErrors::createNewPeak(DataObjects::Peak const &peak_old, V3D const &samplePos, double T0, double L0) {
  double T = peak_old.getTOF() + T0;

  int ID = peak_old.getDetectorID();

  Kernel::V3D hkl = peak_old.getHKL();
  // peak_old.setDetectorID(ID); //set det positions
  Peak peak(peak_old.getInstrument(), ID, peak_old.getWavelength(), hkl, peak_old.getGoniometerMatrix());
  peak.setSamplePos(samplePos);

  Wavelength wl;

  wl.initialize(L0, 0,
                {{UnitParams::l2, peak.getL2()},
                 {UnitParams::twoTheta, peak.getScattering()},
                 {UnitParams::efixed, peak_old.getInitialEnergy()}});

  peak.setWavelength(wl.singleFromTOF(T));
  peak.setIntensity(peak_old.getIntensity());
  peak.setSigmaIntensity(peak_old.getSigmaIntensity());
  peak.setRunNumber(peak_old.getRunNumber());
  peak.setBinCount(peak_old.getBinCount());

  //!!!peak.setDetectorID(ID);
  return peak;
}
} // namespace Mantid::Crystal
