#include "MantidCrystal/SCDPanelErrors.h"
#include "MantidCrystal/SCDCalibratePanels.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidKernel/ConfigService.h"

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
// static const double ONE_OVER_TWO_PI = 1. / M_2_PI;
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
  convention = Kernel::ConfigService::Instance().getString("Q.convention");
  initializeAttributeList(m_attrNames);

  SampleX = SampleY = SampleZ = 0.;

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

  if ((m_startX > static_cast<int>(nData) - 1) ||
      (m_endX > static_cast<int>(nData) - 1)) {
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

Instrument_sptr
SCDPanelErrors::getNewInstrument(const Geometry::IPeak &peak) const {

  Geometry::Instrument_const_sptr instSave = peak.getInstrument();
  auto pmap = boost::make_shared<ParameterMap>();
  boost::shared_ptr<const Geometry::ParameterMap> pmapSv =
      instSave->getParameterMap();

  if (!instSave) {
    g_log.error(" Not all peaks have an instrument");
    throw std::invalid_argument(" Not all peaks have an instrument");
  }
  auto instChange = boost::make_shared<Geometry::Instrument>();

  if (!instSave->isParametrized()) {
    boost::shared_ptr<Geometry::Instrument> instClone(instSave->clone());
    auto Pinsta = boost::make_shared<Geometry::Instrument>(instSave, pmap);

    instChange = Pinsta;
  } else // catch(...)
  {
    // TODO eliminate next 2 lines. not used
    boost::shared_ptr<const IComponent> inst3 =
        boost::dynamic_pointer_cast<const IComponent>(instSave);
    // updateParams(pmapSv, pmap, inst3);

    auto P1 = boost::make_shared<Geometry::Instrument>(
        instSave->baseInstrument(), pmap);
    instChange = P1;
  }

  if (!instChange) {
    g_log.error("Cannot 'clone' instrument");
    throw logic_error("Cannot clone instrument");
  }
  std::vector<std::string> GroupBanks;

  boost::split(GroupBanks, BankNames, boost::is_any_of("!"));

  for (size_t group = 0; group < (size_t)GroupBanks.size(); ++group) {
    string prefix = "f" + std::to_string(group) + "_";

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

Peak SCDPanelErrors::createNewPeak(const Geometry::IPeak &peak_old,
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
                << (EndX - StartX + 1) / 3 << '\n';

  // some pointers for the updated instrument
  boost::shared_ptr<Geometry::Instrument> instChange =
      getNewInstrument(m_peaks->getPeak(0));

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
    size_t pkIndex = std::lround(xValues[i]); // just round to nearest int
    if (pkIndex >= m_peaks->rowCount()) {

      g_log.error() << "Improper workspace set " << pkIndex << "\n";
      throw invalid_argument("Improper workspace. xVals correspond to an index "
                             "in the PeaksWorkspace");
    }

    IPeak &peak_old = m_peaks->getPeak(static_cast<int>(pkIndex));
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
  Geometry::OrientedLattice lattice(*m_unitCell);
  lattice.setUB(m_peaks->sample().getOrientedLattice().getUB());

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

  g_log.debug() << "Parameters\n";

  for (size_t i = 0; i < this->nParams(); ++i)
    g_log.debug() << setw(20) << parameterName(i) << setw(20) << getParameter(i)
                  << '\n';

  g_log.debug() << "      chi Squared=" << std::setprecision(12) << chiSq
                << '\n';

  // Get values for test program. TODO eliminate
  g_log.debug() << "  out[evenxx]=";
  for (size_t i = 0; i < std::min<size_t>(nData, 30); ++i)
    g_log.debug() << out[i] << "  ";

  g_log.debug() << '\n';
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
    r += fabs(rz) * .02;

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
  FunctionDomain1DView domain(xValues, nData);
  calNumericalDeriv(domain, *out);
}

DataObjects::Workspace2D_sptr
SCDPanelErrors::calcWorkspace(DataObjects::PeaksWorkspace_sptr &pwks,
                              std::vector<std::string> &bankNames,
                              double tolerance) {
  int N = 0;
  if (tolerance < 0)
    tolerance = .5;
  tolerance = std::min<double>(.5, tolerance);

  Mantid::MantidVec xRef;

  for (auto &bankName : bankNames)
    for (size_t j = 0; j < pwks->rowCount(); ++j) {
      Geometry::IPeak &peak = pwks->getPeak(static_cast<int>(j));
      if (peak.getBankName().compare(bankName) == 0)
        if (peak.getH() != 0 || peak.getK() != 0 || peak.getL() != 0)
          if (peak.getH() - floor(peak.getH()) < tolerance ||
              floor(peak.getH() + 1) - peak.getH() < tolerance)
            if (peak.getK() - floor(peak.getK()) < tolerance ||
                floor(peak.getK() + 1) - peak.getK() < tolerance)
              if (peak.getL() - floor(peak.getL()) < tolerance ||
                  floor(peak.getL() + 1) - peak.getL() < tolerance) {
                N++;
                xRef.push_back(static_cast<double>(j));
                xRef.push_back(static_cast<double>(j));
                xRef.push_back(static_cast<double>(j));
              }
    }

  MatrixWorkspace_sptr mwkspc = API::WorkspaceFactory::Instance().create(
      "Workspace2D", static_cast<size_t>(3), 3 * N, 3 * N);

  auto pX = Kernel::make_cow<HistogramData::HistogramX>(std::move(xRef));
  mwkspc->setX(0, pX);
  mwkspc->setX(1, pX);
  mwkspc->setX(2, pX);

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
      std::string prefix = "f" + std::to_string(k) + "_";
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
      m_unitCell =
          boost::make_shared<Geometry::UnitCell>(a, b, c, alpha, beta, gamma);
    }
  }
}

boost::shared_ptr<const Geometry::IComponent>
SCDPanelErrors::findBank(std::string bankName) {
  return bankDetMap.find(bankName)->second;
}

} // namespace Crystal
} // namespace Mantid
