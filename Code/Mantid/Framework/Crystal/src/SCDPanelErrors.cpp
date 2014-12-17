#include "MantidCrystal/SCDPanelErrors.h"
#include "MantidCrystal/SCDCalibratePanels.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Instrument/Parameter.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include <boost/lexical_cast.hpp>
#include <stdio.h>
#include <math.h>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::Kernel::Units;
using std::invalid_argument;
using std::logic_error;
using std::map;
using std::runtime_error;
using std::setw;
using std::string;
using std::vector;

// TODO: add const's and & to arguments.  PeaksWorkspace_ptr should be const_ptr

namespace Mantid {
namespace Crystal {
namespace {
Kernel::Logger g_log("SCDPanelErrors");
}

DECLARE_FUNCTION(SCDPanelErrors)

// Assumes UB from optimize UB maps hkl to qxyz/2PI. So conversion factors to an
// from
// UB ified q's are below.

namespace { // anonymous namespace
static const double ONE_OVER_TWO_PI = 1. / M_2_PI;
static int doMethod = 1;
const string LATTICE_A("a");
const string LATTICE_B("b");
const string LATTICE_C("c");
const string LATTICE_ALPHA("alpha");
const string LATTICE_BETA("beta");
const string LATTICE_GAMMA("gamma");
const string BANK_NAMES("BankNames");
const string NUM_GROUPS("NGroups");
const string X_START("startX");
const string X_END("endX");
const string PEAKS_WKSP("PeakWorkspaceName");
const string ROTATE_CEN("RotateCenters");
const string SAMPLE_OFF("SampleOffsets");
const string SAMPLE_X("SampleX");
const string SAMPLE_Y("SampleY");
const string SAMPLE_Z("SampleZ");
}

void initializeAttributeList(vector<string> &attrs) {
  attrs.clear();
  attrs.push_back(LATTICE_A);
  attrs.push_back(LATTICE_B);
  attrs.push_back(LATTICE_C);
  attrs.push_back(LATTICE_ALPHA);
  attrs.push_back(LATTICE_BETA);
  attrs.push_back(LATTICE_GAMMA);
  attrs.push_back(PEAKS_WKSP);
  attrs.push_back(BANK_NAMES);
  attrs.push_back(X_START);
  attrs.push_back(X_END);
  attrs.push_back(NUM_GROUPS);
  attrs.push_back(ROTATE_CEN);
  attrs.push_back(SAMPLE_OFF);
  attrs.push_back(SAMPLE_X);
  attrs.push_back(SAMPLE_Y);
  attrs.push_back(SAMPLE_Z);
}

SCDPanelErrors::SCDPanelErrors()
    : API::ParamFunction(), IFunction1D(), tolerance(.6), m_startX(-1),
      m_endX(-1) {
  initializeAttributeList(m_attrNames);

  a_set = b_set = c_set = alpha_set = beta_set = gamma_set = PeakName_set =
      BankNames_set = endX_set = startX_set = NGroups_set = sampleX_set =
          sampleY_set = sampleZ_set = false;

  // g_log.setLevel(7);

  BankNames = "";

  PeakName = "";

  a = b = c = alpha = beta = gamma = 0;

  NGroups = 1;
  RotateCenters = false;
  SampleOffsets = false;
}

SCDPanelErrors::~SCDPanelErrors() {}

size_t SCDPanelErrors::nAttributes() const { return m_attrNames.size(); }

std::vector<std::string> SCDPanelErrors::getAttributeNames() const {
  return m_attrNames;
}

IFunction::Attribute
SCDPanelErrors::getAttribute(const std::string &attName) const {
  if (!hasAttribute(attName))
    throw std::invalid_argument("Not a valid attribute name \"" + attName +
                                "\"");

  if (attName == LATTICE_A)
    return Attribute(a);
  if (attName == LATTICE_B)
    return Attribute(b);
  if (attName == LATTICE_C)
    return Attribute(c);
  if (attName == LATTICE_ALPHA)
    return Attribute(alpha);
  if (attName == LATTICE_BETA)
    return Attribute(beta);
  if (attName == LATTICE_GAMMA)
    return Attribute(gamma);
  if (attName == (BANK_NAMES))
    return Attribute(BankNames);
  else if (attName == PEAKS_WKSP)
    return Attribute(PeakName);
  else if (attName == NUM_GROUPS)
    return Attribute(NGroups);
  else if (attName == ROTATE_CEN) {
    if (RotateCenters)
      return Attribute(1);
    else
      return Attribute(0);
  } else if (attName == SAMPLE_OFF) {
    if (SampleOffsets)

      return Attribute(1);
    else
      return Attribute(0);
  } else if (attName == X_START)
    return Attribute(static_cast<int>(m_startX));
  else if (attName == X_END)
    return Attribute(static_cast<int>(m_endX));
  else if (attName == SAMPLE_X)
    return Attribute(SampleX);
  else if (attName == SAMPLE_Y)
    return Attribute(SampleY);
  else if (attName == SAMPLE_Z)
    return Attribute(SampleZ);

  throw std::invalid_argument("Not a valid attribute name \"" + attName + "\"");
}

bool SCDPanelErrors::hasAttribute(const std::string &attName) const {
  return (std::find(m_attrNames.begin(), m_attrNames.end(), attName) !=
          m_attrNames.end());
}

SCDPanelErrors::SCDPanelErrors(DataObjects::PeaksWorkspace_sptr &pwk,
                               std::string &Component_name, double ax,
                               double bx, double cx, double alphax,
                               double betax, double gammax, double tolerance1)
    : API::ParamFunction(), IFunction1D() {
  initializeAttributeList(m_attrNames);

  m_peaks = pwk;
  BankNames = Component_name;

  tolerance = tolerance1;
  NGroups = 1;
  a_set = b_set = c_set = alpha_set = beta_set = gamma_set = PeakName_set =
      BankNames_set = endX_set = startX_set = NGroups_set = false;

  setAttribute(LATTICE_A, Attribute(ax));
  setAttribute(LATTICE_B, Attribute(bx));
  setAttribute(LATTICE_C, Attribute(cx));
  setAttribute(LATTICE_ALPHA, Attribute(alphax));
  setAttribute(LATTICE_BETA, Attribute(betax));
  setAttribute(LATTICE_GAMMA, Attribute(gammax));

  setAttribute(PEAKS_WKSP, Attribute("xxx"));
  setAttribute(BANK_NAMES, Attribute(Component_name));
  setAttribute(X_START, Attribute(-1));
  setAttribute(X_END, Attribute(-1));
  setAttribute(ROTATE_CEN, Attribute(0));
  setAttribute(SAMPLE_OFF, Attribute(0));
  setAttribute(SAMPLE_X, Attribute(0.0));
  setAttribute(SAMPLE_Y, Attribute(0.0));
  setAttribute(SAMPLE_Z, Attribute(0.0));
  init();
}

void SCDPanelErrors::init() {

  declareParameter("f0_detWidthScale", 1.0, "panel Width");
  declareParameter("f0_detHeightScale", 1.0, "panel Height");

  declareParameter("f0_Xoffset", 0.0, "Panel Center x offset");
  declareParameter("f0_Yoffset", 0.0, "Panel Center y offset");
  declareParameter("f0_Zoffset", 0.0, "Panel Center z offset");

  declareParameter("f0_Xrot", 0.0,
                   "Rotation(degrees) Panel Center in x axis direction");
  declareParameter("f0_Yrot", 0.0,
                   "Rotation(degrees) Panel Center in y axis direction");
  declareParameter("f0_Zrot", 0.0,
                   "Rotation(degrees) Panel Center in z axis direction");

  declareParameter("l0", 0.0, "Initial Flight Path");
  declareParameter("t0", 0.0, "Time offset");
  declareParameter("SampleX", 0.0, "Sample x offset");
  declareParameter("SampleY", 0.0, "Sample y offset");
  declareParameter("SampleZ", 0.0, "Sample z offset");
}

void SCDPanelErrors::getPeaks() const {
  // if the pointer is set just return
  if (m_peaks && m_peaks->rowCount() > 0)
    return;

  if (PeakName.empty())
    throw std::invalid_argument(
        "Cannot retrieve peaks workspace from empty string");

  // get the workspace from the framework
  m_peaks =
      AnalysisDataService::Instance().retrieveWS<PeaksWorkspace>(PeakName);

  // error check it
  if (!m_peaks || m_peaks->rowCount() < 1)
    throw std::invalid_argument("There are no peaks in the peaks workspace or "
                                "no PeakWorkspace named \"" +
                                PeakName + "\"");
}

void SCDPanelErrors::Check(DataObjects::PeaksWorkspace_sptr &pkwsp,
                           const double *xValues, const size_t nData,
                           size_t &StartX, size_t &EndX) const {
  // TODO need to error check this
  //  if (NLatticeParametersSet < (int) nAttributes()-2)
  //  {
  //    g_log.error("Not all lattice parameters have been set");
  //    throw std::invalid_argument("Not all lattice parameters have been set");
  //  }

  if (!pkwsp) {
    throw std::invalid_argument("Cannot find a PeaksWorkspace ");
  }

  if (pkwsp->getNumberPeaks() < 4) {
    throw std::invalid_argument("Not enough peaks to fit ");
  }

  if ((m_startX > (int)nData - 1) || (m_endX > (int)nData - 1)) {
    throw std::invalid_argument(X_START + " and " + X_END +
                                " attributes are out of range");
  }

  StartX = 0;
  if (m_startX > 0)
    StartX = static_cast<size_t>(m_startX);
  EndX = nData - 1;
  if (m_endX > static_cast<int>(StartX))
    EndX = static_cast<size_t>(m_endX);

  if (xValues[StartX] != floor(xValues[StartX])) {
    throw std::invalid_argument("Improper workspace. xVals must be integer");
  }

  if (xValues[StartX] < 0 || xValues[StartX] >= pkwsp->rowCount()) {

    throw std::invalid_argument("Improper workspace. xVals correspond to an "
                                "index in the PeaksWorkspace");
  }

  if ((EndX - StartX + 1) / 3 < 4) {
    throw std::invalid_argument("Not enough peaks to process banks " +
                                BankNames);
  }
}

Instrument_sptr SCDPanelErrors::getNewInstrument(const API::IPeak &peak) const {

  Geometry::Instrument_const_sptr instSave = peak.getInstrument();
  boost::shared_ptr<Geometry::ParameterMap> pmap(new ParameterMap());
  boost::shared_ptr<const Geometry::ParameterMap> pmapSv =
      instSave->getParameterMap();

  if (!instSave) {
    g_log.error(" Not all peaks have an instrument");
    throw std::invalid_argument(" Not all peaks have an instrument");
  }
  boost::shared_ptr<Geometry::Instrument> instChange(
      new Geometry::Instrument());

  if (!instSave->isParametrized()) {
    boost::shared_ptr<Geometry::Instrument> instClone(instSave->clone());
    boost::shared_ptr<Geometry::Instrument> Pinsta(
        new Geometry::Instrument(instSave, pmap));

    instChange = Pinsta;
  } else // catch(...)
  {
    // TODO eliminate next 2 lines. not used
    boost::shared_ptr<const IComponent> inst3 =
        boost::dynamic_pointer_cast<const IComponent>(instSave);
    // updateParams(pmapSv, pmap, inst3);

    boost::shared_ptr<Geometry::Instrument> P1(
        new Geometry::Instrument(instSave->baseInstrument(), pmap));
    instChange = P1;
  }

  if (!instChange) {
    g_log.error("Cannot 'clone' instrument");
    throw logic_error("Cannot clone instrument");
  }
  std::vector<std::string> GroupBanks;

  boost::split(GroupBanks, BankNames, boost::is_any_of("!"));

  for (size_t group = 0; group < (size_t)GroupBanks.size(); ++group) {
    string prefix = "f" + boost::lexical_cast<std::string>(group) + "_";

    std::vector<std::string> bankNames;
    Quat rot = Quat(getParameter(prefix + "Xrot"), Kernel::V3D(1.0, 0.0, 0.0)) *
               Quat(getParameter(prefix + "Yrot"), Kernel::V3D(0.0, 1.0, 0.0)) *
               Quat(getParameter(prefix + "Zrot"), Kernel::V3D(0.0, 0.0, 1.0));

    boost::split(bankNames, GroupBanks[group], boost::is_any_of("/"));

    SCDCalibratePanels::FixUpBankParameterMap(
        bankNames, instChange,
        V3D(getParameter(prefix + "Xoffset"), getParameter(prefix + "Yoffset"),
            getParameter(prefix + "Zoffset")),
        rot, getParameter(prefix + "detWidthScale"),
        getParameter(prefix + "detHeightScale"), pmapSv, RotateCenters);

  } // for each group

  V3D SampPos = instChange->getSample()->getPos();
  SampPos[0] += getParameter("SampleX") + SampleX;
  SampPos[1] += getParameter("SampleY") + SampleY;
  SampPos[2] += getParameter("SampleZ") + SampleZ;

  SCDCalibratePanels::FixUpSourceParameterMap(instChange, getParameter("l0"),
                                              SampPos, pmapSv);

  return instChange;
}

Peak SCDPanelErrors::createNewPeak(const API::IPeak &peak_old,
                                   Geometry::Instrument_sptr instrNew,
                                   double T0, double L0) {
  Geometry::Instrument_const_sptr inst = peak_old.getInstrument();
  if (inst->getComponentID() != instrNew->getComponentID()) {
    g_log.error("All peaks must have the same instrument");
    throw invalid_argument("All peaks must have the same instrument");
  }

  double T = peak_old.getTOF() + T0;

  int ID = peak_old.getDetectorID();

  Kernel::V3D hkl = peak_old.getHKL();
  // peak_old.setDetectorID(ID); //set det positions
  Peak peak(instrNew, ID, peak_old.getWavelength(), hkl,
            peak_old.getGoniometerMatrix());

  Wavelength wl;

  wl.initialize(L0, peak.getL2(), peak.getScattering(), 0,
                peak_old.getInitialEnergy(), 0.0);

  peak.setWavelength(wl.singleFromTOF(T));
  peak.setIntensity(peak_old.getIntensity());
  peak.setSigmaIntensity(peak_old.getSigmaIntensity());
  peak.setRunNumber(peak_old.getRunNumber());
  peak.setBinCount(peak_old.getBinCount());

  //!!!peak.setDetectorID(ID);
  return peak;
}

void SCDPanelErrors::function1D(double *out, const double *xValues,
                                const size_t nData) const {
  g_log.debug() << "Start function 1D\n";

  // return early if there is nothing to do
  if (nData == 0)
    return;

  if (!m_unitCell)
    throw runtime_error(
        "Cannot evaluate function without setting the lattice constants");

  // error check the parameters
  double r = checkForNonsenseParameters();
  if (r != 0) {
    for (size_t i = 0; i < nData; ++i)
      out[i] = 100 + r;

    g_log.debug() << "Parametersxx  for " << BankNames << ">=";
    for (size_t i = 0; i < nParams(); ++i)
      g_log.debug() << getParameter(i) << ",";
    g_log.debug() << "\n";

    return;
  }

  // determine the range of data to fit by index
  size_t StartX;
  size_t EndX;
  this->getPeaks();
  Check(m_peaks, xValues, nData, StartX, EndX);

  g_log.debug() << "BankNames " << BankNames << "   Number of peaks"
                << (EndX - StartX + 1) / 3 << std::endl;

  // some pointers for the updated instrument
  boost::shared_ptr<Geometry::Instrument> instChange =
      getNewInstrument(m_peaks->getPeak(0));
  V3D samplePosition = instChange->getSample()->getPos();

  //---------------------------- Calculate q and hkl vectors-----------------

  vector<Kernel::V3D> hkl_vectors;
  vector<Kernel::V3D> q_vectors;
  double t0 = getParameter("t0");
  double l0 = getParameter("l0");
  for (size_t i = StartX; i <= EndX; i += 3) {
    // the x-values are the peak indices as triplets, convert them to size_t
    if (xValues[i] < 0.)
      throw invalid_argument(
          "Improper workspace. xVals must be positive integers");
    size_t pkIndex =
        static_cast<size_t>(xValues[i] + .5); // just round to nearest int
    if (pkIndex >= m_peaks->rowCount()) {

      g_log.error() << "Improper workspace set " << pkIndex << "\n";
      throw invalid_argument("Improper workspace. xVals correspond to an index "
                             "in the PeaksWorkspace");
    }

    IPeak &peak_old = m_peaks->getPeak((int)pkIndex);
    Kernel::V3D hkl = peak_old.getHKL();

    // eliminate tolerance cause only those peaks that are OK should be here
    if (IndexingUtils::ValidIndex(hkl, tolerance)) {
      hkl_vectors.push_back(hkl);
      Peak peak = createNewPeak(peak_old, instChange, t0, l0);
      q_vectors.push_back(peak.getQSampleFrame());
    }
  }

  //----------------------------------Calculate out
  //----------------------------------

  // determine the OrientedLattice for converting to Q-sample
  Geometry::OrientedLattice lattice(m_unitCell.get());
  try {
    Kernel::Matrix<double> UB(3, 3, false);
    Geometry::IndexingUtils::Optimize_UB(UB, hkl_vectors, q_vectors);
    Geometry::OrientedLattice lat;
    lat.setUB(UB);
    lattice.setU(lat.getU());
  } catch (...) {
    for (size_t i = StartX; i <= EndX; ++i)
      out[i] = 10000;
    g_log.debug() << "Could Not find a UB matix" << std::endl;
    return;
  }

  // cumulative error
  double chiSq = 0; // for debug log message

  for (size_t i = 0; i < StartX; ++i)
    out[i] = 0.;
  for (size_t i = 0; i < q_vectors.size(); ++i) {
    Kernel::V3D err = q_vectors[i] - lattice.qFromHKL(hkl_vectors[i]);

    size_t outIndex = 3 * i + StartX;
    out[outIndex + 0] = err[0];
    out[outIndex + 1] = err[1];
    out[outIndex + 2] = err[2];
    chiSq += err[0] * err[0] + err[1] * err[1] + err[2] * err[2];
  }

  for (size_t i = EndX; i < nData; ++i)
    out[i] = 0.;

  g_log.debug() << "Parameters" << std::endl;

  for (size_t i = 0; i < this->nParams(); ++i)
    g_log.debug() << setw(20) << parameterName(i) << setw(20) << getParameter(i)
                  << std::endl;

  g_log.debug() << "      chi Squared=" << std::setprecision(12) << chiSq
                << std::endl;

  // Get values for test program. TODO eliminate
  g_log.debug() << "  out[evenxx]=";
  for (size_t i = 0; i < std::min<size_t>(nData, 30); ++i)
    g_log.debug() << out[i] << "  ";

  g_log.debug() << std::endl;
}

Matrix<double> SCDPanelErrors::CalcDiffDerivFromdQ(
    Matrix<double> const &DerivQ, Matrix<double> const &Mhkl,
    Matrix<double> const &MhklT, Matrix<double> const &InvhklThkl,
    Matrix<double> const &UB) const {
  try {
    Matrix<double> dUB = DerivQ * Mhkl * InvhklThkl * ONE_OVER_TWO_PI;

    Geometry::OrientedLattice lat;
    lat.setUB(Matrix<double>(UB) + dUB * .001);
    const Kernel::DblMatrix U2 = lat.getU();

    Kernel::DblMatrix U2A(U2);
    lat.setUB(Matrix<double>(UB) - dUB * .001);
    const Kernel::DblMatrix U1 = lat.getU();

    Kernel::DblMatrix dU = (U2A - U1) * (1 / .002);
    if (dU == Kernel::DblMatrix())
      std::cout << "zero dU in CalcDiffDerivFromdQ" << std::endl;
    Kernel::DblMatrix dUB0 = dU * m_unitCell->getB();

    Kernel::DblMatrix dQtheor = dUB0 * MhklT;
    Kernel::DblMatrix Deriv = Matrix<double>(DerivQ) - dQtheor * M_2_PI;

    return Deriv;

  } catch (...) {

    for (size_t i = 0; i < nParams(); ++i)
      g_log.debug() << getParameter(i) << ",";

    g_log.debug() << "\n";

    throw std::invalid_argument(" Invalid initial data ");
  }
}

double SCDPanelErrors::checkForNonsenseParameters() const {

  double Dwdth = getParameter(0);
  double Dhght = getParameter(1);
  double x = getParameter(2);
  double y = getParameter(3);
  double z = getParameter(4);
  double rx = getParameter(5);
  double ry = getParameter(6);
  double rz = getParameter(7);
  double L0 = getParameter(8);
  double T0 = getParameter(9);

  double r = 0.;
  if (L0 < 1.)
    r = 1. - L0;

  if (fabs(T0) > 20.)
    r += (T0 - 20.) * 2.;

  if (Dwdth < .5 || Dwdth > 2.)
    r += 3. * fabs(1 - Dwdth);

  if (Dhght < .5 || Dhght > 2.)
    r += 3. * fabs(1. - Dhght);

  if (fabs(x) > .35)
    r += fabs(x) * .2;

  if (fabs(y) > .35)
    r += fabs(y) * .2;

  if (fabs(z) > .35)
    r += fabs(z) * .2;

  if (fabs(rx) > 15.)
    r += fabs(rx) * .02;

  if (fabs(ry) > 15.)
    r += fabs(ry) * .02;

  if (fabs(rz) > 15.)
    r += fabs(ry) * .02;

  return 5. * r;
}

void updateDerivResult(PeaksWorkspace_sptr peaks, V3D &unRotDeriv,
                       Matrix<double> &Result, size_t peak,
                       vector<int> &peakIndx) {
  Matrix<double> GonMatrix =
      peaks->getPeak(peakIndx[peak]).getGoniometerMatrix();
  GonMatrix.Invert();

  V3D RotDeriv = GonMatrix * unRotDeriv;

  for (int kk = 0; kk < 3; ++kk)
    Result[kk][peak] = RotDeriv[kk];
}

void SCDPanelErrors::functionDeriv1D(Jacobian *out, const double *xValues,
                                     const size_t nData) {
  if (nData <= 0)
    return;

  if (!m_unitCell)
    throw runtime_error(
        "Cannot evaluate function without setting the lattice constants");

  size_t L0param = parameterIndex("l0");
  size_t T0param = parameterIndex("t0");

  double rr;
  vector<int> row, col, peakIndx, NPanelrows, NPanelcols;
  vector<V3D> pos, xvec, yvec, hkl, qlab, qXtal;
  vector<V3D> PanelCenter;
  vector<double> time;
  double K, L0;

  string lastBankName = "";
  V3D last_xvec, last_yvec, last_Center;
  int last_Nrows, last_Ncols;
  double velocity;
  Matrix<double> InvhklThkl(3, 3);
  Matrix<double> UB(3, 3);
  map<string, size_t> bankName2Group;
  vector<string> Groups;

  rr = checkForNonsenseParameters();

  if (rr > 0) {
    for (size_t i = 0; i < nParams(); ++i)
      for (size_t k = 0; k < nData; ++k)
        out->set(k, i, 10 + rr);

    return;
  }

  size_t StartX;
  size_t EndX;
  this->getPeaks();
  Check(m_peaks, xValues, nData, StartX, EndX);

  Instrument_sptr instrNew = getNewInstrument(m_peaks->getPeak(0));

  boost::shared_ptr<ParameterMap> pmap = instrNew->getParameterMap();

  V3D SamplePos = instrNew->getSample()->getPos();
  V3D SourcePos = instrNew->getSource()->getPos();
  const IPeak &ppeak = m_peaks->getPeak(0);
  L0 = ppeak.getL1();

  velocity = (L0 + ppeak.getL2()) / ppeak.getTOF();
  K = 2. * M_PI / ppeak.getWavelength() / velocity; // 2pi/lambda = K*velocity

  for (size_t xval = StartX; xval <= EndX; xval += 3) {

    double x = floor(xValues[xval]);
    Peak peak;
    V3D HKL;
    string thisBankName;
    Quat Rot;
    IPeak &peak_old = m_peaks->getPeak((int)x);

    peak = createNewPeak(peak_old, instrNew, getParameter("t0"),
                         getParameter("l0"));

    peakIndx.push_back((int)x);
    qlab.push_back(peak.getQLabFrame());
    qXtal.push_back(peak.getQSampleFrame());
    row.push_back(peak.getRow());
    col.push_back(peak.getCol());
    time.push_back(peak.getTOF());

    HKL = peak.getHKL();
    hkl.push_back(
        V3D(floor(.5 + HKL.X()), floor(.5 + HKL.Y()), floor(.5 + HKL.Z())));

    pos.push_back(peak.getDetPos());

    thisBankName = peak.getBankName();

    if (thisBankName == lastBankName) {
      xvec.push_back(last_xvec);
      yvec.push_back(last_yvec);
      PanelCenter.push_back(last_Center);
      NPanelrows.push_back(last_Nrows);
      NPanelcols.push_back(last_Ncols);

    } else {
      V3D x_vec(1., 0., 0.);
      V3D y_vec(0., 1., 0.);
      boost::shared_ptr<const IComponent> panel =
          instrNew->getComponentByName(thisBankName);
      Rot = panel->getRotation();
      Rot.rotate(x_vec);
      Rot.rotate(y_vec);

      boost::shared_ptr<const RectangularDetector> rPanel =
          boost::dynamic_pointer_cast<const RectangularDetector>(panel);
      x_vec *= rPanel->xstep();
      y_vec *= rPanel->ystep();
      int Nrows = rPanel->ypixels();
      int Ncols = rPanel->xpixels();

      NPanelrows.push_back(Nrows);
      NPanelcols.push_back(Ncols);

      last_Nrows = Nrows;
      last_Ncols = Ncols;

      PanelCenter.push_back(rPanel->getPos());
      last_Center = rPanel->getPos();
      xvec.push_back(x_vec);
      yvec.push_back(y_vec);
      last_xvec = x_vec;
      last_yvec = y_vec;
      lastBankName = thisBankName;
    }
  }
  Matrix<double> Mhkl(hkl.size(), 3);

  for (size_t rw = 0; rw < hkl.size(); ++rw)
    for (size_t cl = 0; cl < 3; ++cl)
      Mhkl[rw][cl] = hkl[rw][cl];

  Matrix<double> MhklT(Mhkl);
  MhklT.Transpose();

  InvhklThkl = MhklT * Mhkl;

  InvhklThkl.Invert();

  try {
    Geometry::IndexingUtils::Optimize_UB(UB, hkl, qXtal);

  } catch (std::exception &s) {

    g_log.error("Not enough points to find Optimized UB1 =" +
                std::string(s.what()));
    throw runtime_error("Not enough good points to find Optimized UB");
  } catch (char *s1) {
    g_log.error("Not enough points to find Optimized UB2=" + std::string(s1));
    throw runtime_error("Not enough good points to find Optimized UB");
  } catch (...) {
    g_log.error("Not enough points to find Optimized UB3");
    throw runtime_error("Not enough good points to find Optimized UB");
  }

  boost::split(Groups, BankNames, boost::is_any_of("!"));

  for (size_t gr = 0; gr < Groups.size(); ++gr) {
    vector<string> banknames;
    boost::split(banknames, Groups[gr], boost::is_any_of("/"));
    for (vector<string>::iterator it = banknames.begin(); it != banknames.end();
         ++it)
      bankName2Group[(*it)] = gr;
  }
  // derivative formulas documentation
  // Qvec=-K*Vvec +K*v_mag*beamDir
  // Dvec= pos-samplePos
  // v_mag=(|L0'|+|Dvec|)/tof
  // t1=tof- |L0'|/v_mag <--time from source to sample
  // L0' =L0 +samplePos[z]
  // Vvec= Dvec/t1

  vector<double> vMag;
  vector<double> t1;
  // vector<double> V;
  vector<V3D> vMagdxyz;
  vector<V3D> t1dxyz;
  V3D samplePos = instrNew->getSample()->getPos();
  V3D beamDir = instrNew->getBeamDirection();
  for (size_t peak = 0; peak < qlab.size(); ++peak) {
    Peak Peak1(m_peaks->getPeak(peakIndx[peak]));
    Matrix<double> Gon = Peak1.getGoniometerMatrix();
    Gon.Invert();

    V3D Samp1(samplePos);
    double L0a = L0 + Samp1[2];
    V3D D = pos[peak] - Samp1;
    double magV = (L0a + D.norm()) / time[peak];
    vMag.push_back(magV);
    double T1 = time[peak] - L0a / magV;
    t1.push_back(T1);
    // V.push_back(D * (1 / T1));
    vMagdxyz.push_back(D * (1 / time[peak] / D.norm()));
    t1dxyz.push_back(D * (L0a / magV / magV / D.norm() / time[peak]));
  }
  vector<V3D> Unrot_dQ[3];
  Matrix<double> Result(3, qlab.size());

  for (size_t gr = 0; gr < (size_t)NGroups; ++gr) {
    Unrot_dQ[0].clear();
    Unrot_dQ[1].clear();
    Unrot_dQ[2].clear();

    //-------- xyz offset parameters ----------------------
    size_t StartPos =
        parameterIndex("f" + boost::lexical_cast<string>(gr) + "_Xoffset");

    for (size_t param = StartPos; param <= StartPos + (size_t)2; ++param)

    {

      V3D parxyz(0, 0, 0);
      parxyz[param - StartPos] = 1.;

      Matrix<double> Result(3, qlab.size());
      for (size_t peak = 0; peak < qlab.size(); ++peak)
        if (bankName2Group[m_peaks->getPeak(peakIndx[peak]).getBankName()] !=
            gr) {
          Unrot_dQ[param - StartPos].push_back(
              V3D(0.0, 0.0, 0.0)); // Save for later calculations

          Result[0][peak] = 0;
          Result[1][peak] = 0;
          Result[2][peak] = 0;

        } else {
          size_t xyz = param - StartPos;
          V3D dQlab = beamDir * (vMagdxyz[peak][xyz] * K);
          // V = D/t1 where D =pos-samplepos
          V3D D = pos[peak] - samplePos;
          V3D dV = parxyz * (1. / t1[peak]);
          double x = t1dxyz[peak][xyz] / t1[peak] / t1[peak];
          V3D dV1 = D * x;
          dV = dV - dV1;
          dQlab += dV * (-1. * K);

          Matrix<double> GonMatrix =
              m_peaks->getPeak(peakIndx[peak]).getGoniometerMatrix();
          GonMatrix.Invert();
          V3D dQsamp = GonMatrix * dQlab;

          Unrot_dQ[param - StartPos].push_back(
              dQlab); // Save for later calculations

          Result[0][peak] = dQsamp.X();
          Result[1][peak] = dQsamp.Y();
          Result[2][peak] = dQsamp.Z();

        } // for each peak

      Kernel::DblMatrix Deriv =
          CalcDiffDerivFromdQ(Result, Mhkl, MhklT, InvhklThkl, UB);
      // Better not set everything to zero in case of composites. Have no idea
      // what is done with them.
      // for (size_t w = 0; w < nData; w++)
      //    out->set(w, param, 0.0);

      size_t x = StartX;
      for (size_t coll = 0; coll < Deriv.numCols(); ++coll)
        for (size_t roww = 0; roww < 3; ++roww) {
          out->set(x, param, Deriv[roww][coll]);
          x++;
        }

    } // for params

    /* Derivative Formulas
            d qxyz/d rot*= d qxyz/dx*dx/d rot* + d qxyz/dy*dy/d rot+ d
       qxyz/dz*dz/d rot*
            dqxyz/dxyz done above and saved in Unrot_Dq

            (x,y,z)= center +(col-centCol)*xvec+(row-centRow)*yvec
            Rotations are around x axis, y axis or z axis.
            Get transforms, take their deriv in degr(eval at 0) and apply to
            above formula for (x,y,z) to get d xyz/d rot*.

         */
    //-------------------- Derivatives with respect to rotx,roty, and rotz
    //---------

    size_t StartRot =
        parameterIndex("f" + boost::lexical_cast<string>(gr) + "_Xrot");

    const double DEG_TO_RAD = M_PI / 180.;
    for (size_t param = StartRot; param <= StartRot + 2; ++param) {
      Matrix<double> Result(3, qlab.size());
      Matrix<double> Rot2dRot(3, 3); // deriv of rot matrix at angle=0
      Rot2dRot.zeroMatrix();
      int r1 = (int)param - (int)StartRot;
      int r = (r1 + 1) % 3;

      Rot2dRot[r][(r + 1) % 3] = -1;
      r = (r + 1) % 3;
      Rot2dRot[r][(r + 2) % 3] = +1;
      Rot2dRot *= DEG_TO_RAD;

      for (size_t peak = 0; peak < qlab.size(); ++peak)
        if (bankName2Group[m_peaks->getPeak(peakIndx[peak]).getBankName()] !=
            gr) {
          Result[0][peak] = 0;
          Result[1][peak] = 0;
          Result[2][peak] = 0;

        } else {
          int Nwrt = 3;
          int NderOf = 3;
          Matrix<double> Bas(NderOf, Nwrt); // partial Qxyz wrt xyx
          Bas.zeroMatrix();

          for (int rr = 0; rr < NderOf; ++rr)
            for (int cc = 0; cc < Nwrt; ++cc) {
              Bas[rr][cc] = Unrot_dQ[cc][peak][rr];
            }

          V3D dXvec = Rot2dRot * xvec[peak];
          V3D dYvec = Rot2dRot * yvec[peak];
          // TODO check new stuff dCenter def next 2 lines , used after
          V3D Center = PanelCenter[peak];
          V3D dCenter = Rot2dRot * Center;
          if (!RotateCenters)
            dCenter = V3D(0, 0, 0);
          V3D dxyz2theta = dXvec * (col[peak] - NPanelcols[peak] / 2.0 + .5) +
                           dYvec * (row[peak] - NPanelrows[peak] / 2.0 + .5) +
                           dCenter;

          // dxyz2theta is partials xyz wrt rot x
          V3D unRotDeriv = Bas * dxyz2theta;

          if (doMethod == 0) {
            Matrix<double> GonMatrix =
                m_peaks->getPeak(peakIndx[peak]).getGoniometerMatrix();
            GonMatrix.Invert();

            V3D RotDeriv = GonMatrix * unRotDeriv;

            for (int kk = 0; kk < 3; ++kk)
              Result[kk][peak] = RotDeriv[kk];
          } else
            updateDerivResult(m_peaks, unRotDeriv, Result, peak, peakIndx);
        }

      Kernel::DblMatrix Deriv =
          CalcDiffDerivFromdQ(Result, Mhkl, MhklT, InvhklThkl, UB);

      size_t x = StartX;
      for (size_t coll = 0; coll < Deriv.numCols(); ++coll)
        for (int roww = 0; roww < 3; ++roww) {
          out->set(x, param, Deriv[roww][coll]);
          x++;
        }
    }

    /* Derivative Formulas
                    d qxyz/d scale*= d qxyz/dx*dx/d scale* + d qxyz/dy*dy/d
       scale*+ d qxyz/dz*dz/d scale*
                    dqxyz/dxyz done above and saved in Unrot_Dq

                    (x,y,z)= center
       +(col-centCol)*xvec*scaleWidth+(row-centRow)*yvec*scaleHeight


                 */
    size_t param = parameterIndex("f" + boost::lexical_cast<string>(gr) +
                                  "_detWidthScale");

    for (size_t peak = 0; peak < qlab.size(); ++peak)
      if (bankName2Group[m_peaks->getPeak(peakIndx[peak]).getBankName()] !=
          gr) {
        Result[0][peak] = 0;
        Result[1][peak] = 0;
        Result[2][peak] = 0;

      } else {
        int Nwrt = 3;
        int NderOf = 3;
        Matrix<double> Bas(NderOf, Nwrt);
        Bas.zeroMatrix();
        for (int rr = 0; rr < NderOf; ++rr)
          for (int cc = 0; cc < Nwrt; ++cc)
            Bas[rr][cc] = Unrot_dQ[cc][peak][rr];

        V3D Xvec =
            xvec[peak] *
            (col[peak] - NPanelcols[peak] / 2); // partial xyz wrt widthScale

        V3D unRotDeriv = Bas * Xvec;
        if (doMethod == 0) {
          Matrix<double> GonMatrix =
              m_peaks->getPeak(peakIndx[peak]).getGoniometerMatrix();
          GonMatrix.Invert();

          V3D RotDeriv = GonMatrix * unRotDeriv;

          for (int kk = 0; kk < 3; ++kk)
            Result[kk][peak] = RotDeriv[kk];
        } else
          updateDerivResult(m_peaks, unRotDeriv, Result, peak, peakIndx);
      }

    Kernel::DblMatrix Deriv =
        CalcDiffDerivFromdQ(Result, Mhkl, MhklT, InvhklThkl, UB);

    size_t x = StartX;
    for (size_t coll = 0; coll < Deriv.numCols(); ++coll)
      for (int roww = 0; roww < 3; ++roww) {
        out->set(x, param, Deriv[roww][coll]);
        x++;
      }

    param = parameterIndex("f" + boost::lexical_cast<string>(gr) +
                           "_detHeightScale");

    Result.zeroMatrix();
    for (size_t peak = 0; peak < qlab.size(); ++peak)
      if (bankName2Group[m_peaks->getPeak(peakIndx[peak]).getBankName()] !=
          gr) {
        Result[0][peak] = 0;
        Result[1][peak] = 0;
        Result[2][peak] = 0;

      } else {
        int Nwrt = 3;
        int NderOf = 3;
        Matrix<double> Bas(NderOf, Nwrt);
        Bas.zeroMatrix();

        for (int rr = 0; rr < NderOf; ++rr)
          for (int cc = 0; cc < Nwrt; ++cc)
            Bas[rr][cc] = Unrot_dQ[cc][peak][rr];

        V3D Yvec =
            yvec[peak] *
            (row[peak] - NPanelrows[peak] / 2); // partial xyz wrt heightScale

        V3D unRotDeriv = Bas * Yvec;

        if (doMethod == 0) {
          Matrix<double> GonMatrix =
              m_peaks->getPeak(peakIndx[peak]).getGoniometerMatrix();
          GonMatrix.Invert();

          V3D RotDeriv = GonMatrix * unRotDeriv;

          for (int kk = 0; kk < 3; ++kk)
            Result[kk][peak] = RotDeriv[kk];
        } else
          updateDerivResult(m_peaks, unRotDeriv, Result, peak, peakIndx);
      }

    Deriv = CalcDiffDerivFromdQ(Result, Mhkl, MhklT, InvhklThkl, UB);
    for (size_t w = 0; w < nData; ++w)
      out->set(w, param, 0.0);

    x = StartX;
    for (size_t coll = 0; coll < Deriv.numCols(); ++coll)
      for (int roww = 0; roww < 3; ++roww) {
        out->set(x, param, Deriv[roww][coll]);
        x++;
      }

    /*Derivative Formulas
        See formulas for translations
          d v_mag/dL0 =d v_mag/dL0'* dL0'/dL0 = 1/tof*1
          d v_mag/dt0 =d v_mag/d tof* dtof/dt0 = -(L0a+|D|)/tof^2 =-v_mag/tof

          Qvec =K*v_mag*(scat dir - beam dir)<-- dir does not change as t0 or L0
       change

        */
    param = L0param; // L0.  partial unRotQxyz wrt L0 = unRotQxyz/|v|/tof

    Result.zeroMatrix();
    for (size_t peak = 0; peak < qlab.size(); ++peak) {

      // double L1 = (pos[peak]-samplePos).norm();

      double velMag = vMag[peak];
      double KK = 1 / velMag / time[peak];

      V3D unRotDeriv = qlab[peak] * KK;

      updateDerivResult(m_peaks, unRotDeriv, Result, peak, peakIndx);
    }

    Deriv = CalcDiffDerivFromdQ(Result, Mhkl, MhklT, InvhklThkl, UB);
    // for (size_t w = 0; w < nData; w++)
    //   out->set(w, param, 0.0);

    x = StartX;
    for (size_t coll = 0; coll < Deriv.numCols(); ++coll)
      for (int roww = 0; roww < 3; ++roww) {
        out->set(x, param, Deriv[roww][coll]);
        x++;
      }

    param = T0param; // t0 partial unRotQxyz wrt t0 = -unRotQxyz/tof
    Result.zeroMatrix();
    for (size_t peak = 0; peak < qlab.size(); ++peak) {
      double KK = -1 / time[peak];
      V3D unRotDeriv = qlab[peak] * KK;

      Matrix<double> GonMatrix =
          m_peaks->getPeak(peakIndx[peak]).getGoniometerMatrix();
      GonMatrix.Invert();
      V3D RotDeriv = GonMatrix * unRotDeriv;

      for (int kk = 0; kk < 3; ++kk)
        Result[kk][peak] = RotDeriv[kk];
    }

    Deriv = CalcDiffDerivFromdQ(Result, Mhkl, MhklT, InvhklThkl, UB);
    // for (size_t w = 0; w < nData; w++)
    //   out->set(w, param, 0.0);
    x = StartX;
    for (size_t coll = 0; coll < Deriv.numCols(); ++coll)
      for (int roww = 0; roww < 3; ++roww) {
        out->set(x, param, Deriv[roww][coll]);
        x++;
      }
    if (SampleOffsets) {
      /*
            See formulas from translations
            d v_mag/dsxsysx = (dL0a/dsxsysz +d|D|/dxyz)*1/tof
            d t1/dsxsysz= -1/v_mag*d L0a/dsxsysx +L0a/v_mag^^2*d v_mag/dsxsysz
            dD/dsxsysz=(-1,-1,-1)
            d|D|/dsxsysz =1/2|D|*2(x-sx.y-sy,z-sz)
          */
      vector<V3D> v_magdsxsysz;
      vector<V3D> t1dsxsysz;

      for (size_t peak = 0; peak < qlab.size(); peak++) {
        V3D Ddsx, Ddsy, Ddsz;
        Matrix<double> GonMatrix =
            m_peaks->getPeak(peakIndx[peak]).getGoniometerMatrix();
        GonMatrix.Invert();
        Matrix<double> Ssxsyszdsx1sy1sz1(
            GonMatrix.Transpose()); // row 1 wrt sx,row2 wrt sy
        // std::cout<<"Dsxyz'/dsxy"<<Ssxsyszdsx1sy1sz1<<std::endl;

        V3D D = pos[peak] - samplePos;

        // D dot Dtransp = |D|^2. deriv  2 Dds dot Dtranspose= 2|D| d|D|ds*
        V3D Dmagdsxsysz(-D[0], -D[1], -D[2]);
        Dmagdsxsysz /= D.norm();

        V3D vmagd = Dmagdsxsysz;
        vmagd /= time[peak];
        v_magdsxsysz.push_back(vmagd);

        V3D samp1(samplePos);
        V3D t1ds = vmagd * (L0 / vMag[peak] / vMag[peak]);
        t1dsxsysz.push_back(t1ds);
      }

      Result.zeroMatrix();
      size_t startParam = parameterIndex("SampleX");
      for (size_t param = startParam; param < startParam + 3; param++) {
        size_t k = param - startParam;
        for (size_t peak = 0; peak < qlab.size(); peak++) {

          V3D D = pos[peak] - samplePos;
          V3D Vds(0, 0, 0);
          Vds[k] = -1;
          V3D dKV1 = Vds * (K / t1[peak]);
          dKV1 = dKV1 - D * (K * t1dsxsysz[peak][k] / t1[peak] / t1[peak]);

          V3D dKV2 = beamDir * (K * v_magdsxsysz[peak][k]);
          V3D dKV = dKV2 - dKV1;

          V3D dQlab = (dKV);
          Matrix<double> GonMatrix =
              m_peaks->getPeak(peakIndx[peak]).getGoniometerMatrix();
          GonMatrix.Invert();
          V3D dQsamp = GonMatrix * dQlab;

          for (int kk = 0; kk < 3; ++kk)
            Result[kk][peak] = dQsamp[kk];
        }

        Kernel::DblMatrix Deriv =
            CalcDiffDerivFromdQ(Result, Mhkl, MhklT, InvhklThkl, UB);

        size_t x = StartX;
        for (size_t coll = 0; coll < Deriv.numCols(); ++coll)
          for (size_t roww = 0; roww < 3; ++roww) {
            out->set(x, param, Deriv[roww][coll]);
            x++;
          }
      }
    }

  } // for each group
}

DataObjects::Workspace2D_sptr
SCDPanelErrors::calcWorkspace(DataObjects::PeaksWorkspace_sptr &pwks,
                              std::vector<std::string> &bankNames,
                              double tolerance) {
  int N = 0;
  Mantid::MantidVecPtr pX;
  if (tolerance < 0)
    tolerance = .5;
  tolerance = std::min<double>(.5, tolerance);

  Mantid::MantidVec &xRef = pX.access();
  Mantid::MantidVecPtr yvals;
  Mantid::MantidVec &yvalB = yvals.access();

  for (size_t k = 0; k < bankNames.size(); ++k)
    for (size_t j = 0; j < pwks->rowCount(); ++j) {
      API::IPeak &peak = pwks->getPeak((int)j);
      if (peak.getBankName().compare(bankNames[k]) == 0)
        if (peak.getH() != 0 || peak.getK() != 0 || peak.getL() != 0)
          if (peak.getH() - floor(peak.getH()) < tolerance ||
              floor(peak.getH() + 1) - peak.getH() < tolerance)
            if (peak.getK() - floor(peak.getK()) < tolerance ||
                floor(peak.getK() + 1) - peak.getK() < tolerance)
              if (peak.getL() - floor(peak.getL()) < tolerance ||
                  floor(peak.getL() + 1) - peak.getL() < tolerance) {
                N++;
                xRef.push_back((double)j);
                xRef.push_back((double)j);
                xRef.push_back((double)j);
                yvalB.push_back(0.0);
                yvalB.push_back(0.0);
                yvalB.push_back(0.0);
              }
    }

  MatrixWorkspace_sptr mwkspc = API::WorkspaceFactory::Instance().create(
      "Workspace2D", (size_t)3, 3 * N, 3 * N);

  mwkspc->setX(0, pX);
  mwkspc->setX(1, pX);
  mwkspc->setX(2, pX);
  mwkspc->setData(0, yvals);
  mwkspc->setData(0, yvals);
  mwkspc->setData(0, yvals);

  return boost::dynamic_pointer_cast<DataObjects::Workspace2D>(mwkspc);
}

void SCDPanelErrors::setAttribute(const std::string &attName,
                                  const Attribute &value) {
  if (!hasAttribute(attName))
    throw std::invalid_argument("Not a valid attribute name \"" + attName +
                                "\"");

  bool recalcB = false;
  if (attName == LATTICE_A) {
    a = value.asDouble();
    a_set = true;
    recalcB = true;
  } else if (attName == LATTICE_B) {
    b = value.asDouble();
    b_set = true;
    recalcB = true;
  } else if (attName == LATTICE_C) {
    c = value.asDouble();
    c_set = true;
    recalcB = true;
  } else if (attName == LATTICE_ALPHA) {
    alpha = value.asDouble();
    alpha_set = true;
    recalcB = true;
  } else if (attName == LATTICE_BETA) {
    beta = value.asDouble();
    beta_set = true;
    recalcB = true;
  } else if (attName == LATTICE_GAMMA) {
    gamma = value.asDouble();
    gamma_set = true;
    recalcB = true;
  } else if (attName == (BANK_NAMES)) {
    BankNames = value.asString();
    BankNames_set = true;
  } else if (attName == PEAKS_WKSP) {
    PeakName = value.asString();
    PeakName_set = true;
  } else if (attName == NUM_GROUPS) {
    if (NGroups_set) {
      g_log.error("Cannot set NGroups more than once");
      throw new std::invalid_argument("Cannot set NGroups more than once");
    }
    NGroups = value.asInt();
    for (int k = 1; k < NGroups; ++k) {
      std::string prefix = "f" + boost::lexical_cast<std::string>(k) + "_";
      declareParameter(prefix + "detWidthScale", 1.0, "panel Width");
      declareParameter(prefix + "detHeightScale", 1.0, "panelHeight");

      declareParameter(prefix + "Xoffset", 0.0, "Panel Center x offset");
      declareParameter(prefix + "Yoffset", 0.0, "Panel Center y offset");
      declareParameter(prefix + "Zoffset", 0.0, "Panel Center z offset");

      declareParameter(prefix + "Xrot", 0.0,
                       "Rotation(degrees) Panel Center in x axis direction");
      declareParameter(prefix + "Yrot", 0.0,
                       "Rotation(degrees) Panel Center in y axis direction");
      declareParameter(prefix + "Zrot", 0.0,
                       "Rotation(degrees) Panel Center in z axis direction");
    }
    NGroups_set = true;
  } else if (attName == ROTATE_CEN) {
    int v = value.asInt();
    if (v == 0)
      RotateCenters = false;
    else
      RotateCenters = true;

  } else if (attName == SAMPLE_OFF) {
    int v = value.asInt();
    if (v == 0)
      SampleOffsets = false;
    else
      SampleOffsets = true;

  } else if (attName == SAMPLE_X) {
    SampleX = value.asDouble();
    sampleX_set = true;
  } else if (attName == SAMPLE_Y) {
    SampleY = value.asDouble();
    sampleY_set = true;
  } else if (attName == SAMPLE_Z) {
    SampleZ = value.asDouble();
    sampleZ_set = true;
  } else if (attName == X_START) {
    m_startX = value.asInt();
    startX_set = true;
  } else if (attName == "endX") {
    m_endX = value.asInt();
    endX_set = true;
  } else
    throw std::invalid_argument("Not a valid attribute name \"" + attName +
                                "\"");

  if (recalcB) {
    if (a_set && b_set && c_set && alpha_set && beta_set && gamma_set) {
      m_unitCell = boost::shared_ptr<Geometry::UnitCell>(
          new Geometry::UnitCell(a, b, c, alpha, beta, gamma));
    }
  }
}

} // namespace Crystal
} // namespace Mantid
