#include "MantidCrystal/SCDCalibratePanels.h"
#include "MantidAPI/ConstraintFactory.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidCrystal/SelectCellWithForm.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction1D.h"
#include <fstream>
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include <Poco/File.h>
#include <sstream>

using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace std;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace Mantid {
namespace Crystal {

DECLARE_ALGORITHM(SCDCalibratePanels)

namespace {
const double MAX_DET_HW_SCALE = 1.15;
const double MIN_DET_HW_SCALE = 0.85;
const double RAD_TO_DEG = 180. / M_PI;
}

SCDCalibratePanels::SCDCalibratePanels() : API::Algorithm() {
  // g_log.setLevel(7);
}

SCDCalibratePanels::~SCDCalibratePanels() {}

const std::string SCDCalibratePanels::name() const {
  return "SCDCalibratePanels";
}

int SCDCalibratePanels::version() const { return 1; }

const std::string SCDCalibratePanels::category() const {
  return "Crystal\\Corrections";
}

/**
 * Converts a Quaternion to a corresponding matrix produce Rotx*Roty*Rotz,
 * corresponding to the order
 * Mantid uses in calculating rotations
 * @param Q      The Quaternion. It will be normalized to represent a rotation
 * @param Rotx   The angle in degrees for rotating around the x-axis
 * @param Roty   The angle in degrees for rotating around the y-axis
 * @param Rotz   The angle in degrees for rotating around the z-axis
 */
void SCDCalibratePanels::Quat2RotxRotyRotz(const Quat Q, double &Rotx,
                                           double &Roty, double &Rotz) {
  Quat R(Q);
  R.normalize();
  V3D X(1, 0, 0);
  V3D Y(0, 1, 0);
  V3D Z(0, 0, 1);
  R.rotate(X);
  R.rotate(Y);
  R.rotate(Z);
  if (Z[1] != 0 || Z[2] != 0) {
    double tx = atan2(-Z[1], Z[2]);
    double tz = atan2(-Y[0], X[0]);
    double cosy = Z[2] / cos(tx);
    double ty = atan2(Z[0], cosy);
    Rotx = (tx * RAD_TO_DEG);
    Roty = (ty * RAD_TO_DEG);
    Rotz = (tz * RAD_TO_DEG);
  } else // roty = 90 0r 270 def
  {
    double k = 1;
    if (Z[0] < 0)
      k = -1;
    double roty = k * 90;
    double rotx = 0;
    double rotz = atan2(X[2], Y[2]);

    Rotx = (rotx * RAD_TO_DEG);
    Roty = (roty * RAD_TO_DEG);
    Rotz = (rotz * RAD_TO_DEG);
  }
}

/**
 * Creates the Workspace that will be supplied to the SCDPanelErrors Fit
 * function
 * @param pwks      The peaks workspace of indexed peaks.
 * @param bankNames  The bank names where all banks from Group 0 are first,
 * Group 1 are second, etc.
 * @param tolerance  If h,k, and l values are not rounded, this is the indexing
 * tolerance for a peak to be included.
 *                   NOTE: if rounded, only indexed peaks( h,k,l values not all
 * 0) are included.
 * @param bounds    The positions in bankNames vector of  the start of Group *
 * peaks
 */
DataObjects::Workspace2D_sptr
SCDCalibratePanels::calcWorkspace(DataObjects::PeaksWorkspace_sptr &pwks,
                                  vector<string> &bankNames, double tolerance,
                                  vector<int> &bounds) {
  int N = 0;
  if (tolerance <= 0)
    tolerance = .5;
  tolerance = min<double>(.5, tolerance);

  // For the fake data the values are
  //   X = peak index (repeated 3 times
  //   Y = 0. as the function evals to (Q-vec) - (UB * hkl * 2pi)
  //   E = the weighting as used in the cost function
  Mantid::MantidVec xRef;
  Mantid::MantidVecPtr yvals;
  Mantid::MantidVec &yvalB = yvals.access();
  Mantid::MantidVecPtr errs;
  Mantid::MantidVec &errB = errs.access();
  bounds.clear();
  bounds.push_back(0);

  double sumSigInt = 0.0;
  double sumInt = 0.0;
  double sumBinCnt = 0.0;
  for (int it = 0; it != pwks->getNumberPeaks(); ++it) {
    const Peak &peak = pwks->getPeak(it);
    sumSigInt += peak.getSigmaIntensity();
    sumInt += peak.getIntensity();
    sumBinCnt += peak.getBinCount();
  }

  for (size_t k = 0; k < bankNames.size(); ++k) {
    for (int j = 0; j < pwks->getNumberPeaks(); ++j) {
      const Geometry::IPeak &peak = pwks->getPeak(j);
      if (std::find(bankNames.begin(), bankNames.end(), peak.getBankName()) !=
          bankNames.end())
        if (IndexingUtils::ValidIndex(peak.getHKL(), tolerance)) {
          N += 3;

          // 1/sigma is considered the weight for the fit
          // weight = 1/error in FunctionDomain1DSpectrumCreator
          double weight = 1.;                // default is even weighting
          if (peak.getSigmaIntensity() > 0.) // prefer weight by sigmaI
            weight = sumSigInt / peak.getSigmaIntensity();
          else if (peak.getIntensity() > 0.) // next favorite weight by I
            weight = sumInt / peak.getIntensity();
          else if (peak.getBinCount() > 0.) // then by counts in peak centre
            weight = sumBinCnt / peak.getBinCount();

          const double PEAK_INDEX = static_cast<double>(j);
          for (size_t i = 0; i < 3; ++i) {
            xRef.push_back(PEAK_INDEX);
            errB.push_back(weight);
          }
        }
    } // for @ peak
    bounds.push_back(N);
  } // for @ bank name

  yvalB.assign(xRef.size(), 0.0);

  if (N < 4) // If not well indexed
    return boost::make_shared<DataObjects::Workspace2D>();

  MatrixWorkspace_sptr mwkspc =
      API::WorkspaceFactory::Instance().create("Workspace2D", 1, N, N);

  mwkspc->setPoints(0, xRef);
  mwkspc->setData(0, yvals, errs);

  return boost::dynamic_pointer_cast<DataObjects::Workspace2D>(mwkspc);
}

/**
 * Converts the Grouping indicated by the user to an internal more usable form
 * @param AllBankNames  All the bang names
 * @param Grouping      The Grouping choice(one per bank, all together or
 * specify)
 * @param bankPrefix    The prefix for the bank names
 * @param bankingCode   If Grouping Choice is specify, this is what the user
 * specifies along with bankPrefix
 * @param &Groups       The internal form for grouping.
 */
void SCDCalibratePanels::CalculateGroups(
    set<string, compareBanks> &AllBankNames, string Grouping, string bankPrefix,
    string bankingCode, vector<vector<string>> &Groups) {
  Groups.clear();

  if (Grouping == "OnePanelPerGroup") {
    for (const auto &bankName : AllBankNames) {
      vector<string> vbankName;
      vbankName.push_back(bankName);
      Groups.push_back(vbankName);
    }

  } else if (Grouping == "AllPanelsInOneGroup") {
    vector<string> vbankName;

    for (const auto &bankName : AllBankNames) {
      vbankName.push_back(bankName);
    }

    Groups.push_back(vbankName);

  } else if (Grouping == "SpecifyGroups") {
    boost::trim(bankingCode);

    vector<string> GroupA;
    boost::split(GroupA, bankingCode, boost::is_any_of("]"));
    set<string> usedInts;

    for (auto S : GroupA) {
      boost::trim(S);

      if (S.empty())
        break;
      if (S[0] == ',')
        S.erase(0, 1);
      boost::trim(S);
      if (S[0] == '[')
        S.erase(0, 1);
      boost::trim(S);

      vector<string> GroupB;
      boost::split(GroupB, S, boost::is_any_of(","));

      vector<string> Group0;
      for (auto rangeOfBanks : GroupB) {
        boost::trim(rangeOfBanks);

        vector<string> StrtStopStep;
        boost::split(StrtStopStep, rangeOfBanks, boost::is_any_of(":"));

        if (StrtStopStep.size() > 3) {
          g_log.error("Improper use of : in " + rangeOfBanks);
          throw invalid_argument("Improper use of : in " + rangeOfBanks);
        }
        int start, stop, step;
        step = 1;

        if (StrtStopStep.size() == 3) {
          boost::trim(StrtStopStep[2]);
          step = boost::lexical_cast<int>(StrtStopStep[2]);

          if (step <= 0)
            step = 0;
        }
        start = -1;
        if (!StrtStopStep.empty()) {
          boost::trim(StrtStopStep[0]);
          start = boost::lexical_cast<int>(StrtStopStep[0].c_str());
        }
        if (start <= 0) {
          g_log.error("Improper use of : in " + rangeOfBanks);
          throw invalid_argument("Improper use of : in " + rangeOfBanks);
        }
        stop = start;

        if (StrtStopStep.size() >= 2) {
          boost::trim(StrtStopStep[1]);
          stop = boost::lexical_cast<int>(StrtStopStep[1].c_str());

          if (stop <= 0)
            stop = start;
        }

        for (long ind = start; ind <= stop; ind += step) {
          ostringstream oss(ostringstream::out);
          oss << bankPrefix << ind;

          string bankName = oss.str();

          string postName = bankName.substr(bankPrefix.length());

          if (AllBankNames.find(string(bankName)) != AllBankNames.end())
            if (usedInts.find(postName) == usedInts.end()) {
              Group0.push_back(bankName);
              usedInts.insert(postName);
            }
        }
      }
      if (!Group0.empty())
        Groups.push_back(Group0);
    }
  } else {
    g_log.error("No mode " + Grouping + " defined yet");
    throw invalid_argument("No mode " + Grouping + " defined yet");
  }
}

/**
 * Modifies the instrument to correspond to an already modified instrument
 *
 * @param instrument         The base instrument to be modified with a
 *parameterMap
 * @param preprocessCommand  Type of preprocessing file with modification
 *information
 * @param preprocessFilename The name of the file with preprocessing information
 * @param timeOffset         The time offset in preprocessing if used
 * @param L0                 The initial path length from the preprocessing file
 *if used
 * @param  AllBankNames      The names of all the banks of interest in this
 *instrument
 */
boost::shared_ptr<const Instrument> SCDCalibratePanels::GetNewCalibInstrument(
    boost::shared_ptr<const Instrument> instrument, string preprocessCommand,
    string preprocessFilename, double &timeOffset, double &L0,
    vector<string> &AllBankNames) {
  if (preprocessCommand == "A)No PreProcessing")
    return instrument;

  bool xml = (preprocessCommand == "C)Apply a LoadParameter.xml type file");

  boost::shared_ptr<const ParameterMap> pmap0 = instrument->getParameterMap();
  auto pmap1 = boost::make_shared<ParameterMap>();

  for (const auto &bankName : AllBankNames) {
    updateBankParams(instrument->getComponentByName(bankName), pmap1, pmap0);
  }

  //---------------------update params for
  // moderator.------------------------------

  boost::shared_ptr<const Instrument> newInstr(
      new Instrument(instrument->baseInstrument(), pmap1));

  double L1, norm = 1.0;
  V3D beamline, sampPos;
  instrument->getInstrumentParameters(L1, beamline, norm, sampPos);
  FixUpSourceParameterMap(newInstr, L0, sampPos, pmap0);

  if (xml) {
    vector<int> detIDs = instrument->getDetectorIDs();
    MatrixWorkspace_sptr wsM = WorkspaceFactory::Instance().create(
        "Workspace2D", detIDs.size(), static_cast<size_t>(100),
        static_cast<size_t>(100));

    Workspace2D_sptr ws =
        boost::dynamic_pointer_cast<DataObjects::Workspace2D>(wsM);
    ws->setInstrument(newInstr);
    ws->populateInstrumentParameters();

    boost::shared_ptr<Algorithm> loadParFile =
        createChildAlgorithm("LoadParameterFile");
    loadParFile->initialize();
    loadParFile->setProperty("Workspace", ws);
    loadParFile->setProperty("Filename", preprocessFilename);
    loadParFile->executeAsChildAlg();

    boost::shared_ptr<const Instrument> newInstrument = ws->getInstrument();
    newInstrument->getInstrumentParameters(L0, beamline, norm, sampPos);
    return newInstrument;

  } else {
    set<string> bankNames;
    LoadISawDetCal(newInstr, bankNames, timeOffset, L0, preprocessFilename,
                   "bank");
    return newInstr;
  }
}

/**
 *  Calculates initial parameters for the fitting parameters
 *  @param bank_rect        The bank(panel)
 *  @param  instrument       The instrument
 *  @param PreCalibinstrument The instrument with precalibrated values
 * incorporated
 *  @param detWidthScale0     The ratio of base instrument to PreCalib
 * Instrument for this panel's width
 *  @param detHeightScale0    The ratio of base instrument to PreCalib
 * Instrument for this panel's height
 *  @param Xoffset0         The difference between base instrument and PreCalib
 * Instrument for this panel's center X
 *  @param Yoffset0         The difference between base instrument and PreCalib
 * Instrument for this panel's center Y
 *  @param Zoffset0       The difference between base instrument and PreCalib
 * Instrument for this panel's center Z
 *  @param Xrot0       The difference between base instrument and PreCalib
 * Instrument for this panel's Rot in X direction
 *  @param Yrot0       The difference between base instrument and PreCalib
 * Instrument for this panel's Rot in Y direction
 *  @param Zrot0       The difference between base instrument and PreCalib
 * Instrument for this panel's Rot in Z direction
 */
void SCDCalibratePanels::CalcInitParams(
    RectangularDetector_const_sptr bank_rect, Instrument_const_sptr instrument,
    Instrument_const_sptr PreCalibinstrument, double &detWidthScale0,
    double &detHeightScale0, double &Xoffset0, double &Yoffset0,
    double &Zoffset0, double &Xrot0, double &Yrot0, double &Zrot0) {
  string bankName = bank_rect->getName();
  RectangularDetector_const_sptr newBank =
      boost::dynamic_pointer_cast<const RectangularDetector>(
          PreCalibinstrument->getComponentByName(bankName));

  if (!newBank) {
    detWidthScale0 = 1;
    detHeightScale0 = 1;
    Xoffset0 = 0;
    Yoffset0 = 0;
    Zoffset0 = 0;
    Xrot0 = 0;
    Yrot0 = 0;
    Zrot0 = 0;
    g_log.notice() << "Improper PreCalibInstrument for " << bankName << endl;
    return;
  }

  boost::shared_ptr<Geometry::ParameterMap> pmap =
      instrument->getParameterMap();
  boost::shared_ptr<Geometry::ParameterMap> pmapPre =
      PreCalibinstrument->getParameterMap();

  vector<V3D> RelPosI = pmap->getV3D(bankName, "pos");
  vector<V3D> RelPosPre = pmapPre->getV3D(bankName, "pos");

  V3D posI, posPre;

  if (!RelPosI.empty())
    posI = RelPosI[0];
  else
    posI = bank_rect->getRelativePos();

  if (!RelPosPre.empty())
    posPre = RelPosPre[0];
  else
    posPre = newBank->getRelativePos();

  V3D change = posPre - posI;

  Xoffset0 = change.X();
  Yoffset0 = change.Y();
  Zoffset0 = change.Z();

  double scalexI = 1.;
  double scalexPre = 1.;
  double scaleyI = 1.;
  double scaleyPre = 1.;

  vector<double> ScalexI = pmap->getDouble(bankName, "scalex");
  vector<double> ScalexPre = pmapPre->getDouble(bankName, "scalex");
  vector<double> ScaleyI = pmap->getDouble(bankName, "scaley");
  vector<double> ScaleyPre = pmapPre->getDouble(bankName, "scaley");

  if (!ScalexI.empty())
    scalexI = ScalexI[0];

  if (!ScaleyI.empty())
    scaleyI = ScaleyI[0];

  if (!ScalexPre.empty())
    scalexPre = ScalexPre[0];

  if (!ScaleyPre.empty())
    scaleyPre = ScaleyPre[0];

  // scaling

  detWidthScale0 = scalexPre / scalexI;
  detHeightScale0 = scaleyPre / scaleyI;

  Quat rotI = bank_rect->getRelativeRot();
  Quat rotPre = newBank->getRelativeRot();

  rotI.inverse();
  Quat ChgRot = rotPre * rotI;

  Quat2RotxRotyRotz(ChgRot, Xrot0, Yrot0, Zrot0);
}

/**
 * Tests inputs. Does the indexing correspond to the entered lattice parameters
 * @param peaksWs  The peaks workspace with indexed peaks
 * @param a        The lattice parameter a
 * @param  b       The lattice parameter b
 * @param  c       The lattice parameter c
 * @param  alpha       The lattice parameter alpha
 * @param  beta       The lattice parameter beta
 * @param  gamma       The lattice parameter gamma
 * @param  tolerance   The indexing tolerance
 */
bool GoodStart(const PeaksWorkspace_sptr &peaksWs, double a, double b, double c,
               double alpha, double beta, double gamma, double tolerance) {
  // put together a list of indexed peaks
  std::vector<V3D> hkl;
  int nPeaks = peaksWs->getNumberPeaks();
  hkl.reserve(nPeaks);
  std::vector<V3D> qVecs;
  qVecs.reserve(nPeaks);
  for (int i = 0; i < nPeaks; i++) {
    const Peak &peak = peaksWs->getPeak(i);
    if (IndexingUtils::ValidIndex(peak.getHKL(), tolerance)) {
      hkl.push_back(peak.getHKL());
      qVecs.push_back(peak.getQSampleFrame());
    }
  }

  Kernel::Logger g_log("Calibration");
  // determine the lattice constants
  OrientedLattice lat = peaksWs->sample().getOrientedLattice();
  g_log.notice() << "Lattice before optimization: " << lat << "\n";

  // see if the lattice constants are no worse than 25% out
  if (fabs(lat.a() - a) / a > .25)
    return false;
  if (fabs(lat.b() - b) / b > .25)
    return false;
  if (fabs(lat.c() - c) / c > .25)
    return false;
  if (fabs(lat.alpha() - alpha) / alpha > .25)
    return false;
  if (fabs(lat.beta() - beta) / beta > .25)
    return false;
  if (fabs(lat.gamma() - gamma) / gamma > .25)
    return false;

  return true;
}

namespace { // anonymous namespace
            /**
             * Adds a tie to the IFunction.
             * @param iFunc The function to add the tie to.
             * @param tie Whether or not to actually do it.
             * @param parName The name of the parameter to tie.
             * @param value The value to tie it to.
             */
static inline void tie(IFunction_sptr &iFunc, const bool tie,
                       const string &parName, const double value) {
  if (!tie)
    return;
  std::ostringstream ss;
  ss << std::fixed << value;
  iFunc->tie(parName, ss.str());
}

static inline void constrain(IFunction_sptr &iFunc, const string &parName,
                             const double min, const double max) {
  std::ostringstream ss;
  ss << std::fixed << min << "<" << parName << "<" << std::fixed << max;
  IConstraint *constraint =
      API::ConstraintFactory::Instance().createInitialized(iFunc.get(),
                                                           ss.str());
  iFunc->addConstraint(constraint);
}

} // end anonymous namespace

void SCDCalibratePanels::exec() {
  PeaksWorkspace_sptr peaksWs = getProperty("PeakWorkspace");
  // We must sort the peaks
  std::vector<std::pair<std::string, bool>> criteria;
  criteria.push_back(std::pair<std::string, bool>("BankName", true));
  peaksWs->sort(criteria);
  int nPeaks = peaksWs->getNumberPeaks();

  double a = getProperty("a");
  double b = getProperty("b");
  double c = getProperty("c");
  double alpha = getProperty("alpha");
  double beta = getProperty("beta");
  double gamma = getProperty("gamma");
  if ((a == EMPTY_DBL() || b == EMPTY_DBL() || c == EMPTY_DBL() ||
       alpha == EMPTY_DBL() || beta == EMPTY_DBL() || gamma == EMPTY_DBL()) &&
      peaksWs->sample().hasOrientedLattice()) {
    OrientedLattice latt = peaksWs->mutableSample().getOrientedLattice();
    a = latt.a();
    b = latt.b();
    c = latt.c();
    alpha = latt.alpha();
    beta = latt.beta();
    gamma = latt.gamma();
  } else {
    boost::shared_ptr<Algorithm> alg =
        createChildAlgorithm("FindUBUsingLatticeParameters", .2, .9, true);
    alg->setProperty("PeaksWorkspace", peaksWs);
    alg->setProperty("a", a);
    alg->setProperty("b", b);
    alg->setProperty("c", c);
    alg->setProperty("alpha", alpha);
    alg->setProperty("beta", beta);
    alg->setProperty("gamma", gamma);
    alg->setProperty("NumInitial", 15);
    alg->setProperty("Tolerance", 0.12);
    alg->executeAsChildAlg();
  }
  double tolerance = getProperty("tolerance");

  string DetCalFileName = getProperty("DetCalFilename");
  if (Poco::File(DetCalFileName).exists())
    Poco::File(DetCalFileName).remove();

  bool useL0 = getProperty("useL0");
  bool useTimeOffset = getProperty("useTimeOffset");
  int nIter = 1;
  if (useL0 || useTimeOffset)
    nIter = 2;
  bool use_PanelWidth = getProperty("usePanelWidth");
  bool use_PanelHeight = getProperty("usePanelHeight");
  bool use_PanelPosition = getProperty("usePanelPosition");
  bool use_PanelOrientation = getProperty("usePanelOrientation");
  double SampleXoffset = getProperty("SampleXoffset");
  double SampleYoffset = getProperty("SampleYoffset");
  double SampleZoffset = getProperty("SampleZoffset");

  string Grouping = getProperty("PanelGroups");
  string bankPrefix = getProperty("PanelNamePrefix");
  string bankingCode = getProperty("Grouping");

  //----------------- Set Up Bank Name Vectors -------------------------
  set<string, compareBanks> AllBankNames;
  for (int i = 0; i < nPeaks; ++i)
    AllBankNames.insert(peaksWs->getPeak(i).getBankName());

  vector<vector<string>> Groups;
  CalculateGroups(AllBankNames, Grouping, bankPrefix, bankingCode, Groups);

  //----------------- Calculate & Create Calculated vs Theoretical
  // workspaces------------------,);
  int nGroups = static_cast<int>(AllBankNames.size());
  MatrixWorkspace_sptr ColWksp =
      Mantid::API::WorkspaceFactory::Instance().create("Workspace2D", nGroups,
                                                       nPeaks, nPeaks);
  MatrixWorkspace_sptr RowWksp =
      Mantid::API::WorkspaceFactory::Instance().create("Workspace2D", nGroups,
                                                       nPeaks, nPeaks);
  MatrixWorkspace_sptr TofWksp =
      Mantid::API::WorkspaceFactory::Instance().create("Workspace2D", nGroups,
                                                       nPeaks, nPeaks);
  nGroups = static_cast<int>(Groups.size());

  double chisqSum = 0;
  int NDofSum = 0;
  if (!GoodStart(peaksWs, a, b, c, alpha, beta, gamma, tolerance)) {
    g_log.warning() << "**** Indexing is NOT compatible with given lattice "
                       "parameters******" << std::endl;
    g_log.warning() << "        Index with conventional orientation matrix???"
                    << std::endl;
  }

  //----------- Initialize peaksWorkspace, initial parameter values
  // etc.---------
  boost::shared_ptr<const Instrument> instrument =
      peaksWs->getPeak(0).getInstrument();

  // Time offset from property
  const API::Run &run = peaksWs->run();
  double T0 = 0.0;
  if (run.hasProperty("T0")) {
    Kernel::Property *prop = run.getProperty("T0");
    T0 = boost::lexical_cast<double, std::string>(prop->value());
    if (T0 != 0) {
      g_log.notice() << "T0 = " << T0 << std::endl;
    }
  }
  if ((string)getProperty("PreProcessInstrument") ==
      "C)Apply a LoadParameter.xml type file")
    T0 = getProperty("InitialTimeOffset"); //!*****

  double L0 = peaksWs->getPeak(0).getL1();
  std::ofstream outfile(DetCalFileName);
  g_log.debug() << "Initial L0,T0=" << L0 << "," << T0 << endl;
  std::vector<std::string> detcal;

  for (auto iIter = 0; iIter != nIter; ++iIter) {
    detcal.clear();
    PARALLEL_FOR1(peaksWs)
    for (int iGr = 0; iGr < nGroups; iGr++) {
      PARALLEL_START_INTERUPT_REGION
      auto group = Groups.begin() + iGr;
      vector<string> banksVec;
      for (auto &bankName : *group) {
        banksVec.push_back(bankName);
      }
      //------------------ Set Up Workspace for IFitFunction Fit---------------
      vector<int> bounds;
      Workspace2D_sptr ws = calcWorkspace(peaksWs, banksVec, tolerance, bounds);
      double T0_bank = T0;
      double L0_bank = L0;
      boost::shared_ptr<const Instrument> PreCalibinstrument =
          GetNewCalibInstrument(instrument,
                                (string)getProperty("PreProcessInstrument"),
                                (string)getProperty("PreProcFilename"), T0_bank,
                                L0_bank, banksVec);
      V3D samplePos =
          peaksWs->getPeak(0).getInstrument()->getSample()->getPos();

      string PeakWSName = getPropertyValue("PeakWorkspace");
      if (PeakWSName.length() < 1) {
        PeakWSName = "xxx";
        AnalysisDataService::Instance().addOrReplace("xxx", peaksWs);
      }

      int NGroups = 1; //(int)Groups.size();
      double detWidthScale0, detHeightScale0, Xoffset0, Yoffset0, Zoffset0,
          Xrot0, Yrot0, Zrot0;

      //------------------- For each Group set up Function,
      //--------------------------
      //---------------Ties, and Constraint Properties for Fit
      // algorithm--------------------

      // set up the string for specifying groups
      string BankNameString = "";
      // for (auto group = Groups.begin(); group != Groups.end(); ++group) {
      // if (group != Groups.begin())
      // BankNameString += "!";
      for (auto bank = group->begin(); bank != group->end(); ++bank) {
        if (bank != group->begin())
          BankNameString += "/";

        BankNameString += (*bank);
      }
      //}

      int RotGroups = 0;
      if (getProperty("RotateCenters"))
        RotGroups = 1;
      int SampOffsets = 0;
      if (getProperty("AllowSampleShift"))
        SampOffsets = 1;

      // first round of function setup
      IFunction_sptr iFunc =
          FunctionFactory::Instance().createFunction("SCDPanelErrors");
      iFunc->setAttributeValue("PeakWorkspaceName", PeakWSName);
      iFunc->setAttributeValue("a", a);
      iFunc->setAttributeValue("b", b);
      iFunc->setAttributeValue("c", c);
      iFunc->setAttributeValue("alpha", alpha);
      iFunc->setAttributeValue("beta", beta);
      iFunc->setAttributeValue("gamma", gamma);
      iFunc->setAttributeValue("NGroups", NGroups);
      iFunc->setAttributeValue("BankNames", BankNameString);
      iFunc->setAttributeValue("startX", -1);
      iFunc->setAttributeValue("endX", -1);
      iFunc->setAttributeValue("RotateCenters", RotGroups);
      iFunc->setAttributeValue("SampleOffsets", SampOffsets);
      iFunc->setParameter("l0", L0_bank);
      iFunc->setParameter("t0", T0_bank);

      double maxXYOffset = getProperty("MaxPositionChange_meters");

      boost::shared_ptr<const RectangularDetector> bank_rect;

      string name = group->front();
      boost::shared_ptr<const IComponent> bank_cmp =
          instrument->getComponentByName(name);
      bank_rect =
          boost::dynamic_pointer_cast<const RectangularDetector>(bank_cmp);

      if (!bank_rect) {
        g_log.error("No Rectangular detector bank " + banksVec[0] +
                    " in instrument");
        throw invalid_argument("No Rectangular detector bank " + banksVec[0] +
                               " in instrument");
      }

      // if( it1 == (*itv).begin())
      CalcInitParams(bank_rect, instrument, PreCalibinstrument, detWidthScale0,
                     detHeightScale0, Xoffset0, Yoffset0, Zoffset0, Xrot0,
                     Yrot0, Zrot0);

      // --- set Function property ----------------------
      iFunc->setParameter("f0_detWidthScale", detWidthScale0);
      iFunc->setParameter("f0_detHeightScale", detHeightScale0);
      iFunc->setParameter("f0_Xoffset", Xoffset0);
      iFunc->setParameter("f0_Yoffset", Yoffset0);
      iFunc->setParameter("f0_Zoffset", Zoffset0);
      iFunc->setParameter("f0_Xrot", Xrot0);
      iFunc->setParameter("f0_Yrot", Yrot0);
      iFunc->setParameter("f0_Zrot", Zrot0);

      set<string> MyBankNames;
      for (int i = 0; i < nPeaks; ++i)
        MyBankNames.insert(banksVec[0]);
      int startX = bounds[0];
      int endXp1 = bounds[group->size()];
      if (endXp1 - startX < 13) {
        g_log.error() << "Bank Group " << BankNameString
                      << " does not have enough peaks for fitting" << endl;
        saveIsawDetCal(instrument, MyBankNames, T0_bank,
                       DetCalFileName + boost::lexical_cast<std::string>(iGr));
        continue;
      }

      //---------- setup ties ----------------------------------
      tie(iFunc, !use_PanelWidth, "f0_detWidthScale", detWidthScale0);
      tie(iFunc, !use_PanelHeight, "f0_detHeightScale", detHeightScale0);
      tie(iFunc, !use_PanelPosition, "f0_Xoffset", Xoffset0);
      tie(iFunc, !use_PanelPosition, "f0_Yoffset", Yoffset0);
      tie(iFunc, !use_PanelPosition, "f0_Zoffset", Zoffset0);
      tie(iFunc, !use_PanelOrientation, "f0_Xrot", Xrot0);
      tie(iFunc, !use_PanelOrientation, "f0_Yrot", Yrot0);
      tie(iFunc, !use_PanelOrientation, "f0_Zrot", Zrot0);

      //--------------- setup constraints ------------------------------
      constrain(iFunc, "l0", (MIN_DET_HW_SCALE * L0_bank),
                (MAX_DET_HW_SCALE * L0_bank));
      constrain(iFunc, "t0", -5., 5.);

      constrain(iFunc, "f0_detWidthScale", MIN_DET_HW_SCALE * detWidthScale0,
                MAX_DET_HW_SCALE * detWidthScale0);
      constrain(iFunc, "f0_detHeightScale", MIN_DET_HW_SCALE * detHeightScale0,
                MAX_DET_HW_SCALE * detHeightScale0);
      constrain(iFunc, "f0_Xoffset", -1. * maxXYOffset + Xoffset0,
                maxXYOffset + Xoffset0);
      constrain(iFunc, "f0_Yoffset", -1. * maxXYOffset + Yoffset0,
                maxXYOffset + Yoffset0);
      constrain(iFunc, "f0_Zoffset", -1. * maxXYOffset + Zoffset0,
                maxXYOffset + Zoffset0);

      double MaxRotOffset = getProperty("MaxRotationChangeDegrees");
      constrain(iFunc, "f0_Xrot", -1. * MaxRotOffset, MaxRotOffset);
      constrain(iFunc, "f0_Yrot", -1. * MaxRotOffset, MaxRotOffset);
      constrain(iFunc, "f0_Zrot", -1. * MaxRotOffset, MaxRotOffset);
      //} // for vector< string > in Groups

      // Function supports setting the sample position even when it isn't be
      // refined
      iFunc->setAttributeValue("SampleX", samplePos.X() + SampleXoffset);
      iFunc->setAttributeValue("SampleY", samplePos.Y() + SampleYoffset);
      iFunc->setAttributeValue("SampleZ", samplePos.Z() + SampleZoffset);

      // Constraints for sample offsets
      if (getProperty("AllowSampleShift")) {
        maxXYOffset = getProperty("MaxSamplePositionChangeMeters");
        constrain(iFunc, "SampleX", samplePos.X() + SampleXoffset - maxXYOffset,
                  samplePos.X() + SampleXoffset + maxXYOffset);
        constrain(iFunc, "SampleY", samplePos.Y() + SampleYoffset - maxXYOffset,
                  samplePos.Y() + SampleYoffset + maxXYOffset);
        constrain(iFunc, "SampleZ", samplePos.Z() + SampleZoffset - maxXYOffset,
                  samplePos.Z() + SampleZoffset + maxXYOffset);
      } else {
        tie(iFunc, true, "SampleX", samplePos.X() + SampleXoffset);
        tie(iFunc, true, "SampleY", samplePos.Y() + SampleYoffset);
        tie(iFunc, true, "SampleZ", samplePos.Z() + SampleZoffset);
      }

      tie(iFunc, !useL0, "l0", L0_bank);
      tie(iFunc, !useTimeOffset, "t0", T0_bank);

      //--------------------- Set up Fit Algorithm and
      // Execute-------------------
      boost::shared_ptr<Algorithm> fit_alg =
          createChildAlgorithm("Fit", .2, .9, true);

      if (!fit_alg)
        throw invalid_argument("Cannot find Fit algorithm");
      fit_alg->initialize();

      int Niterations = getProperty("NumIterations");
      std::string minimizerError = getProperty("MinimizerError");
      std::string minimizer = getProperty("Minimizer");
      fit_alg->setProperty("Function", iFunc);
      fit_alg->setProperty("MaxIterations", Niterations);
      fit_alg->setProperty("InputWorkspace", ws);
      fit_alg->setProperty("Output", "out");
      // fit_alg->setProperty("CreateOutput", true);
      fit_alg->setProperty("CalcErrors", false);
      fit_alg->setPropertyValue("Minimizer", minimizer + ",AbsError=" +
                                                 minimizerError + ",RelError=" +
                                                 minimizerError);
      fit_alg->executeAsChildAlg();

      PARALLEL_CRITICAL(afterFit) {
        // MatrixWorkspace_sptr fitWS = fit_alg->getProperty("OutputWorkspace");
        // AnalysisDataService::Instance().addOrReplace("out"+boost::lexical_cast<std::string>(iGr),
        // fitWS);
        g_log.debug() << "Finished executing Fit algorithm\n";
        string OutputStatus = fit_alg->getProperty("OutputStatus");
        g_log.notice() << BankNameString << " Output Status=" << OutputStatus
                       << "\n";

        //--------------------- Get and Process Results -----------------------
        double chisq = fit_alg->getProperty("OutputChi2overDoF");
        chisqSum += chisq;

        if (chisq > 1) {
          g_log.warning() << "************* This is a large chi squared value "
                             "************\n";
          g_log.warning()
              << "    the indexing may have been using an incorrect\n";
          g_log.warning() << "    orientation matrix, instrument geometry or "
                             "goniometer info\n";
        }
        ITableWorkspace_sptr RRes = fit_alg->getProperty("OutputParameters");
        vector<double> params;
        vector<double> errs;
        vector<string> names;
        double sigma = sqrt(chisq);

        if (chisq < 0 || chisq != chisq)
          sigma = -1;
        string fieldBaseNames =
            ";l0;t0;detWidthScale;detHeightScale;Xoffset;Yoffset;"
            "Zoffset;Xrot;Yrot;Zrot;";
        if (getProperty("AllowSampleShift"))
          fieldBaseNames += "SampleX;SampleY;SampleZ;";
        for (size_t prm = 0; prm < RRes->rowCount(); ++prm) {
          string namee = RRes->getRef<string>("Name", prm);
          size_t dotPos = namee.find('_');
          if (dotPos >= namee.size())
            dotPos = 0;
          else
            dotPos++;
          string Field = namee.substr(dotPos);
          size_t FieldNum = fieldBaseNames.find(";" + Field + ";");
          if (FieldNum > fieldBaseNames.size())
            continue;
          if (dotPos != 0) {
            int col = atoi(namee.substr(1, dotPos).c_str());
            if (col < 0 || col >= NGroups)
              continue;
          }
          names.push_back(namee);
          params.push_back(RRes->getRef<double>("Value", prm));
          double err = RRes->getRef<double>("Error", prm);
          errs.push_back(sigma * err);
        }

        //------------------- Report chi^2 value --------------------
        int nVars = 8; // NGroups;

        if (!use_PanelWidth)
          nVars--;
        if (!use_PanelHeight)
          nVars--;
        if (!use_PanelPosition)
          nVars -= 3;
        if (!use_PanelOrientation)
          nVars -= 3;
        nVars *= NGroups;
        nVars += 2;

        if (!useL0)
          nVars--;
        if (!useTimeOffset)
          nVars--;

        // g_log.notice() << "      nVars=" <<nVars<< endl;
        int NDof = (static_cast<int>(ws->dataX(0).size()) - nVars);
        NDofSum = +NDof;

        map<string, double> result;

        for (size_t i = 0; i < min<size_t>(params.size(), names.size()); ++i) {
          result[names[i]] = params[i];
        }

        g_log.notice() << BankNameString << " ChiSqoverDoF =" << chisq
                       << " NDof =" << NDof << " l0 = " << result["l0"]
                       << " T0 = " << result["t0"]
                       << " peaks = " << endXp1 - startX << "\n";

        //--------------------- Create Result Table Workspace-------------------
        this->progress(.92, "Creating Results table");
        createResultWorkspace(nGroups, iGr + 1, names, params, errs);

        //---------------- Create new instrument with ------------------------
        //--------------new parameters to SAVE to files---------------------

        auto pmap = boost::make_shared<ParameterMap>();
        boost::shared_ptr<const ParameterMap> pmapOld =
            instrument->getParameterMap();
        boost::shared_ptr<const Instrument> NewInstrument =
            boost::make_shared<Instrument>(instrument->baseInstrument(), pmap);

        boost::shared_ptr<const RectangularDetector> bank_rect;
        double rotx, roty, rotz;

        rotx = result["f0_Xrot"];
        roty = result["f0_Yrot"];
        rotz = result["f0_Zrot"];

        Quat newRelRot = Quat(rotx, V3D(1, 0, 0)) * Quat(roty, V3D(0, 1, 0)) *
                         Quat(rotz, V3D(0, 0, 1)); //*RelRot;

        FixUpBankParameterMap((banksVec), NewInstrument,
                              V3D(result["f0_Xoffset"], result["f0_Yoffset"],
                                  result["f0_Zoffset"]),
                              newRelRot, result["f0_detWidthScale"],
                              result["f0_detHeightScale"], pmapOld,
                              getProperty("RotateCenters"));

        //} // For @ group

        V3D sampPos(
            NewInstrument->getSample()->getPos()); // should be (0,0,0)???
        if (getProperty("AllowSampleShift"))
          sampPos =
              V3D(result["SampleX"], result["SampleY"], result["SampleZ"]);

        FixUpSourceParameterMap(NewInstrument, result["l0"], sampPos, pmapOld);

        //---------------------- Save new instrument to DetCal-------------
        //-----------------------or xml(for LoadParameterFile) files-----------
        this->progress(.94, "Saving detcal file");
        saveIsawDetCal(NewInstrument, MyBankNames, result["t0"],
                       DetCalFileName + boost::lexical_cast<std::string>(iGr));

        this->progress(.96, "Saving xml param file");
        string XmlFileName = getProperty("XmlFilename");
        saveXmlFile(XmlFileName, Groups, NewInstrument);

        //--------------- Create Function argument for the
        // FunctionHandler------------

        size_t nData = ws->dataX(0).size();
        vector<double> out(nData);
        vector<double> xVals = ws->dataX(0);

        CreateFxnGetValues(ws, NGroups, names, params, BankNameString,
                           out.data(), xVals.data(), nData);
      }
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION
    std::vector<double> l0vec, t0vec;
    std::string line, seven;
    for (int iGr = 0; iGr < nGroups; iGr++) {
      std::ifstream infile(DetCalFileName +
                           boost::lexical_cast<std::string>(iGr));
      while (std::getline(infile, line)) {
        if (iGr == 0 && iIter == nIter - 1) {
          if (line[0] == '#' || line[0] == '6')
            outfile << line << "\n";
        }
        if (line[0] == '7') {
          double L0bank, T0bank;
          std::stringstream(line) >> seven >> L0bank >> T0bank;
          l0vec.push_back(L0bank);
          t0vec.push_back(T0bank);
        } else if (line[0] == '5')
          detcal.push_back(line);
      }
      infile.close();
      if (Poco::File(DetCalFileName + boost::lexical_cast<std::string>(iGr))
              .exists())
        Poco::File(DetCalFileName + boost::lexical_cast<std::string>(iGr))
            .remove();
    }
    if (useL0) {
      removeOutliers(l0vec);
      Statistics stats = getStatistics(l0vec);
      L0 = stats.mean * 0.01; // cm when read from file
      useL0 = false;
    }
    if (useTimeOffset) {
      removeOutliers(t0vec);
      Statistics stats = getStatistics(t0vec);
      T0 = stats.mean;
      // set T0 in the run parameters
      API::Run &m_run = peaksWs->mutableRun();
      m_run.addProperty<double>("T0", T0, true);
      useTimeOffset = false;
    }
  }
  L0 *= 100; // ISAW uses cm
  outfile << "7  " << std::setprecision(4) << std::fixed << L0;
  outfile << std::setw(13) << std::setprecision(3) << T0 << std::endl;
  outfile << "4 DETNUM  NROWS  NCOLS   WIDTH   HEIGHT   DEPTH   DETD   CenterX "
             "  CenterY   CenterZ    BaseX    BaseY    BaseZ      UpX      UpY "
             "     UpZ" << std::endl;
  for (vector<std::string>::const_iterator itdet = detcal.begin();
       itdet != detcal.end(); ++itdet)
    outfile << *itdet << "\n";
  outfile.close();

  setProperty("ColWorkspace", ColWksp);
  setProperty("RowWorkspace", RowWksp);
  setProperty("TofWorkspace", TofWksp);
  setProperty("ChiSqOverDOF", chisqSum);
  setProperty("DOF", NDofSum);

  set<string> bankNames;
  for (int i = 0; i < nPeaks; ++i) {
    IPeak &peak = peaksWs->getPeak(i);
    instrument = peak.getInstrument();
    LoadISawDetCal(instrument, bankNames, T0, L0, DetCalFileName, "bank");
    peak.setInstrument(instrument);
    peak.setDetectorID(peak.getDetectorID());
  }

  IAlgorithm_sptr ub_alg;
  try {
    ub_alg = createChildAlgorithm("CalculateUMatrix", -1, -1, false);
  } catch (Exception::NotFoundError &) {
    g_log.error("Can't locate CalculateUMatrix algorithm");
    throw;
  }
  ub_alg->setProperty("PeaksWorkspace", peaksWs);
  ub_alg->setProperty("a", a);
  ub_alg->setProperty("b", b);
  ub_alg->setProperty("c", c);
  ub_alg->setProperty("alpha", alpha);
  ub_alg->setProperty("beta", beta);
  ub_alg->setProperty("gamma", gamma);
  ub_alg->executeAsChildAlg();
  OrientedLattice o_lattice = peaksWs->mutableSample().getOrientedLattice();
  Kernel::Matrix<double> UB(3, 3);
  UB = o_lattice.getUB();
  std::vector<double> sigabc(6);
  SelectCellWithForm::DetermineErrors(sigabc, UB, peaksWs, tolerance);
  o_lattice.setError(sigabc[0], sigabc[1], sigabc[2], sigabc[3], sigabc[4],
                     sigabc[5]);
  peaksWs->mutableSample().setOrientedLattice(&o_lattice);
  g_log.notice() << "Lattice after optimization: " << o_lattice << "\n";

  // We must sort the peaks
  // std::vector<std::pair<std::string, bool>> criteria;
  // criteria.push_back(std::pair<std::string, bool>("BankName", true));
  criteria.push_back(std::pair<std::string, bool>("RunNumber", true));
  criteria.push_back(std::pair<std::string, bool>("h", true));
  criteria.push_back(std::pair<std::string, bool>("k", true));
  criteria.push_back(std::pair<std::string, bool>("l", true));
  peaksWs->sort(criteria);

  // create table of theoretical vs calculated
  int bankLast = -1;
  int iSpectrum = -1;
  int icount = 0;

  for (int j = 0; j < nPeaks; ++j) {
    const Geometry::IPeak &peak = peaksWs->getPeak(j);
    string bankName = peak.getBankName();
    size_t k = bankName.find_last_not_of("0123456789");
    int bank = 0;
    if (k < bankName.length())
      bank = boost::lexical_cast<int>(bankName.substr(k + 1));
    if (bank != bankLast) {
      iSpectrum++;
      ColWksp->getSpectrum(iSpectrum)->setSpectrumNo(specnum_t(bank));
      RowWksp->getSpectrum(iSpectrum)->setSpectrumNo(specnum_t(bank));
      TofWksp->getSpectrum(iSpectrum)->setSpectrumNo(specnum_t(bank));
      bankLast = bank;
      icount = 0;
    }

    try {
      V3D q_lab = (peak.getGoniometerMatrix() * UB) * peak.getHKL() * M_2_PI;
      Peak theoretical(peak.getInstrument(), q_lab);
      ColWksp->dataX(iSpectrum)[icount] = peak.getCol();
      ColWksp->dataY(iSpectrum)[icount] = theoretical.getCol();
      RowWksp->dataX(iSpectrum)[icount] = peak.getRow();
      RowWksp->dataY(iSpectrum)[icount] = theoretical.getRow();
      TofWksp->dataX(iSpectrum)[icount] = peak.getTOF();
      TofWksp->dataY(iSpectrum)[icount] = theoretical.getTOF();
      icount++;
    } catch (...) {
      // g_log.debug() << "Problem only in printing peaks" << std::endl;
    }
  }
}

/**
 *  This is part of the algorithm, LoadIsawDetCal, starting with an existing
 *instrument
 *  to be modified.  Only banks in AllBankName are affected.
 *
 *  @param instrument   The instrument to be modified
 *  @param AllBankName  The bank names in this instrument that will be modified
 *  @param T0           The time offset from the DetCal file
 *  @param L0           The length offset from the DetCal file
 *  @param filename       The DetCal file name
 *  @param bankPrefixName   The prefix to the bank names.
 */
void SCDCalibratePanels::LoadISawDetCal(
    boost::shared_ptr<const Instrument> &instrument, set<string> &AllBankName,
    double &T0, double &L0, string filename, string bankPrefixName) {

  V3D beamline, samplePos;
  double beamlineLen;
  instrument->getInstrumentParameters(L0, beamline, beamlineLen, samplePos);
  int count, id, nrows, ncols;
  double width, height, depth, detd, x, y, z, base_x, base_y, base_z, up_x,
      up_y, up_z;

  ifstream input(filename.c_str(), ios_base::in);
  string line;

  boost::shared_ptr<Mantid::Geometry::ParameterMap> pmap =
      instrument->getParameterMap();
  while (getline(input, line)) {
    if (line[0] == '7') {
      double mL1;
      stringstream(line) >> count >> mL1 >> T0;
      double scaleL0 = .01 * mL1 / beamlineLen;
      const IComponent_const_sptr source = instrument->getSource();
      V3D NewSourcePos =
          samplePos - beamline * scaleL0 * 2.0; // beamLine is 2*length.
      L0 = beamline.norm() * scaleL0 * 2.0;
      V3D RelSourcePos =
          source->getRelativePos() + NewSourcePos - source->getPos();
      pmap->addPositionCoordinate(source.get(), "x", RelSourcePos.X());
      pmap->addPositionCoordinate(source.get(), "y", RelSourcePos.Y());
      pmap->addPositionCoordinate(source.get(), "z", RelSourcePos.Z());
    }

    if (line[0] != '5')
      continue;
    stringstream(line) >> count >> id >> nrows >> ncols >> width >> height >>
        depth >> detd >> x >> y >> z >> base_x >> base_y >> base_z >> up_x >>
        up_y >> up_z;

    string bankName = bankPrefixName + boost::lexical_cast<string>(id);

    if (!AllBankName.empty() && AllBankName.find(bankName) == AllBankName.end())
      continue;
    boost::shared_ptr<const RectangularDetector> det =
        boost::dynamic_pointer_cast<const RectangularDetector>(
            instrument->getComponentByName(bankName, 3));
    if (!det)
      continue;

    // Adjust pmap to the new scaling
    double scalex = 1.0; // previous scale factor on this detector
    double scaley = 1.0;
    if (pmap->contains(det.get(), "scalex"))
      scalex = pmap->getDouble(det->getName(), "scalex")[0];
    if (pmap->contains(det.get(), "scaley"))
      scaley = pmap->getDouble(det->getName(), "scaley")[0];
    double ScaleX = scalex * 0.01 * width / det->xsize();
    double ScaleY = scaley * 0.01 * height / det->ysize();
    pmap->addDouble(det.get(), "scalex", ScaleX);
    pmap->addDouble(det.get(), "scaley", ScaleY);

    // Adjust pmap to the new center position. Note:in pmap the pos values
    //                                          are rel positions to parent
    x *= 0.01;
    y *= 0.01;
    z *= 0.01;
    V3D pos = det->getPos();
    V3D RelPos = V3D(x, y, z) - pos;
    if (pmap->contains(det.get(), "pos"))
      RelPos += pmap->getV3D(det->getName(), "pos")[0];
    pmap->addPositionCoordinate(det.get(), "x", RelPos.X());
    pmap->addPositionCoordinate(det.get(), "y", RelPos.Y());
    pmap->addPositionCoordinate(det.get(), "z", RelPos.Z());

    // Adjust pmap to the orientation of the panel
    V3D rX = V3D(base_x, base_y, base_z);
    rX.normalize();
    V3D rY = V3D(up_x, up_y, up_z);
    rY.normalize();
    // V3D rZ=rX.cross_prod(rY);

    // These are the original axes
    V3D oX = V3D(1., 0., 0.);
    V3D oY = V3D(0., 1., 0.);

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
    // boost::shared_ptr<const IComponent> comp =
    // instrument->getComponentByName(detname);
    boost::shared_ptr<const IComponent> parent = det->getParent();
    if (parent) {
      Quat rot0 = parent->getRelativeRot();
      rot0.inverse();
      Rot = Rot * rot0;
    }
    boost::shared_ptr<const IComponent> grandparent = parent->getParent();
    if (grandparent) // Why this is not correct but most Rectangular detectors
                     // have no grandparent.
    {
      Quat rot0 = grandparent->getRelativeRot();
      rot0.inverse();
      Rot = Rot * rot0;
    }

    // Set or overwrite "rot" instrument parameter.
    pmap->addQuat(det.get(), "rot", Rot);

  } // While reading thru file
}

void SCDCalibratePanels::createResultWorkspace(const int numGroups,
                                               const int colNum,
                                               const vector<string> &names,
                                               const vector<double> &params,
                                               const vector<double> &errs) {
  // make the table the correct size
  int nn(0);
  if (getProperty("AllowSampleShift"))
    nn = 3;
  if (!Result) {
    // create the results table
    Result =
        Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");

    // column for the field names
    Result->addColumn("str", "Field");
    // and one for each group
    for (int g = 0; g < numGroups; ++g) {
      string GroupName = string("Group") + boost::lexical_cast<string>(g);
      Result->addColumn("double", GroupName);
    }
    Result->setRowCount(2 * (10 + nn));
    Result->setComment(
        string("t0(microseconds),l0 & offsets(meters),rot(degrees"));
  }

  // determine the field names, the leading '_' is the break point
  vector<string> TableFieldNames;
  for (auto fieldName : names) {
    size_t dotPos = fieldName.find('_');
    if (dotPos < fieldName.size())
      fieldName = fieldName.substr(dotPos + 1);

    if (std::find(TableFieldNames.begin(), TableFieldNames.end(), fieldName) ==
        TableFieldNames.end())
      TableFieldNames.push_back(fieldName);
  }

  // create the row labels
  for (size_t p = 0; p < TableFieldNames.size(); p++) {
    Result->cell<string>(p, 0) = TableFieldNames[p];
    Result->cell<string>(TableFieldNames.size() + p, 0) =
        "Err_" + TableFieldNames[p];
  }

  // put in the data
  for (size_t p = 0; p < names.size(); ++p) {
    // get the column to update and the name of the field
    string fieldName = names[p];
    size_t dotPos = fieldName.find('_');
    // int colNum = 1;
    if (dotPos < fieldName.size()) {
      // the 1 is to skip the leading 'f'
      // colNum = atoi(fieldName.substr(1, dotPos).c_str()) + 1;
      // everything after is the field name
      fieldName = fieldName.substr(dotPos + 1);
    }

    // find the row
    int rowNum = 0;
    auto fieldIter =
        std::find(TableFieldNames.begin(), TableFieldNames.end(), fieldName);
    if (fieldIter != TableFieldNames.end()) {
      rowNum = static_cast<int>(fieldIter - TableFieldNames.begin());
    }

    // fill in the values
    Result->cell<double>(rowNum, colNum) = params[p];
    Result->cell<double>(rowNum + 10 + nn, colNum) = errs[p];
  }

  setProperty("ResultWorkspace", Result);
}

/**
 * Really this is the operator SaveIsawDetCal but only the results of the given
 * banks are saved.  L0 and T0 are also saved.
 *
 * @param instrument   -The instrument with the correct panel geometries
 *                         and initial path length
 * @param AllBankName  -the set of the NewInstrument names of the banks(panels)
 * @param T0           -The time offset from the DetCal file
 * @param filename     -The name of the DetCal file to save the results to
 */
void SCDCalibratePanels::saveIsawDetCal(
    boost::shared_ptr<const Instrument> &instrument, set<string> &AllBankName,
    double T0, string filename) {
  // having a filename triggers doing the work
  if (filename.empty())
    return;

  // g_log.notice() << "Saving DetCal file in " << filename << "\n";

  // create a workspace to pass to SaveIsawDetCal
  const size_t number_spectra = instrument->getNumberDetectors();
  DataObjects::Workspace2D_sptr wksp =
      boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
          WorkspaceFactory::Instance().create("Workspace2D", number_spectra, 2,
                                              1));
  wksp->setInstrument(instrument);
  wksp->rebuildSpectraMapping(true /* include monitors */);

  // convert the bank names into a vector
  std::vector<string> banknames(AllBankName.begin(), AllBankName.end());

  // call SaveIsawDetCal
  API::IAlgorithm_sptr alg = createChildAlgorithm("SaveIsawDetCal");
  alg->setProperty("InputWorkspace", wksp);
  alg->setProperty("Filename", filename);
  alg->setProperty("TimeOffset", T0);
  alg->setProperty("BankNames", banknames);
  // alg->setProperty("AppendFile", true);
  alg->executeAsChildAlg();
}

void SCDCalibratePanels::init() {
  declareProperty(Kernel::make_unique<WorkspaceProperty<PeaksWorkspace>>(
                      "PeakWorkspace", "", Kernel::Direction::Input),
                  "Workspace of Indexed Peaks");

  vector<string> choices{"OnePanelPerGroup", "AllPanelsInOneGroup",
                         "SpecifyGroups"};
  declareProperty(string("PanelGroups"), string("OnePanelPerGroup"),
                  boost::make_shared<Kernel::StringListValidator>(choices),
                  "Select grouping of Panels");

  declareProperty("PanelNamePrefix", "bank",
                  "Prefix for the names of panels(followed by a number)");
  declareProperty("Grouping", "[ 1:20,22],[3,5,7]",
                  "A bracketed([]) list of groupings( comma or :(for range) "
                  "separated list of bank numbers");
  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);

  declareProperty("a", EMPTY_DBL(), mustBePositive,
                  "Lattice Parameter a (Leave empty to use lattice constants "
                  "in peaks workspace)");
  declareProperty("b", EMPTY_DBL(), mustBePositive,
                  "Lattice Parameter b (Leave empty to use lattice constants "
                  "in peaks workspace)");
  declareProperty("c", EMPTY_DBL(), mustBePositive,
                  "Lattice Parameter c (Leave empty to use lattice constants "
                  "in peaks workspace)");
  declareProperty("alpha", EMPTY_DBL(), mustBePositive,
                  "Lattice Parameter alpha in degrees (Leave empty to use "
                  "lattice constants in peaks workspace)");
  declareProperty("beta", EMPTY_DBL(), mustBePositive,
                  "Lattice Parameter beta in degrees (Leave empty to use "
                  "lattice constants in peaks workspace)");
  declareProperty("gamma", EMPTY_DBL(), mustBePositive,
                  "Lattice Parameter gamma in degrees (Leave empty to use "
                  "lattice constants in peaks workspace)");

  declareProperty("useL0", false, "Fit the L0(source to sample) distance");
  declareProperty("usetimeOffset", false, "Fit the time offset value");
  declareProperty("usePanelWidth", false, "Fit the Panel Width value");
  declareProperty("usePanelHeight", false, "Fit the Panel Height");
  declareProperty("usePanelPosition", true, "Fit the PanelPosition");
  declareProperty("usePanelOrientation", false, "Fit the PanelOrientation");
  declareProperty("RotateCenters", true,
                  "Rotate bank Centers with panel orientations");
  declareProperty("AllowSampleShift", false,
                  "Allow and fit for a sample that is off center");
  declareProperty("SampleXoffset", 0.0, "Specify Sample x offset");
  declareProperty("SampleYoffset", 0.0, "Specify Sample y offset");
  declareProperty("SampleZoffset", 0.0, "Specify Sample z offset");

  // ---------- preprocessing
  vector<string> preProcessOptions{"A)No PreProcessing",
                                   "B)Apply a ISAW.DetCal File",
                                   "C)Apply a LoadParameter.xml type file"};

  declareProperty(
      string("PreProcessInstrument"), string("A)No PreProcessing"),
      boost::make_shared<Kernel::StringListValidator>(preProcessOptions),
      "Select PreProcessing info");

  const vector<string> exts2{".DetCal", ".xml"};
  declareProperty(Kernel::make_unique<FileProperty>(
                      "PreProcFilename", "", FileProperty::OptionalLoad, exts2),
                  "Path to file with preprocessing information");

  declareProperty("InitialTimeOffset", 0.0,
                  "Initial time offset when using xml files");

  const string PREPROC("Preprocessing");
  setPropertyGroup("PreProcessInstrument", PREPROC);
  setPropertyGroup("PreProcFilename", PREPROC);
  setPropertyGroup("InitialTimeOffset", PREPROC);

  // ---------- outputs
  const std::vector<std::string> detcalExts{".DetCal", ".Det_Cal"};
  declareProperty(Kernel::make_unique<FileProperty>("DetCalFilename", "",
                                                    FileProperty::OptionalSave,
                                                    detcalExts),
                  "Path to an ISAW-style .detcal file to save.");

  declareProperty(
      Kernel::make_unique<FileProperty>("XmlFilename", "",
                                        FileProperty::OptionalSave, ".xml"),
      "Path to an Mantid .xml description(for LoadParameterFile) file to "
      "save.");

  declareProperty(
      Kernel::make_unique<WorkspaceProperty<ITableWorkspace>>(
          "ResultWorkspace", "ResultWorkspace", Kernel::Direction::Output),
      "Workspace of Results");

  declareProperty(
      Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "ColWorkspace", "ColWorkspace", Kernel::Direction::Output),
      "Workspace comparing calculated and theoretical column of each peak.");

  declareProperty(
      Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "RowWorkspace", "RowWorkspace", Kernel::Direction::Output),
      "Workspace comparing calculated and theoretical row of each peak.");

  declareProperty(
      Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "TofWorkspace", "TofWorkspace", Kernel::Direction::Output),
      "Workspace comparing calculated and theoretical TOF of each peak.");

  const string OUTPUTS("Outputs");
  setPropertyGroup("DetCalFilename", OUTPUTS);
  setPropertyGroup("XmlFilename", OUTPUTS);
  setPropertyGroup("ResultWorkspace", OUTPUTS);
  setPropertyGroup("ColWorkspace", OUTPUTS);
  setPropertyGroup("RowWorkspace", OUTPUTS);
  setPropertyGroup("TofWorkspace", OUTPUTS);

  //------------------------------------ Tolerance
  // settings-------------------------

  declareProperty("tolerance", .12, mustBePositive,
                  "offset of hkl values from integer for GOOD Peaks");
  declareProperty("MinimizerError", 1.e-12, mustBePositive,
                  "error for minimizer");
  std::vector<std::string> minimizerOptions;
  minimizerOptions.push_back("Levenberg-Marquardt");
  minimizerOptions.push_back("Simplex");
  declareProperty(
      "Minimizer", "Levenberg-Marquardt",
      Kernel::IValidator_sptr(
          new Kernel::ListValidator<std::string>(minimizerOptions)),
      "If Levenberg-Marquardt does not find minimum, "
      "try Simplex which works better with a poor inital starting point.",
      Kernel::Direction::Input);
  declareProperty("NumIterations", 60, "Number of iterations");
  declareProperty(
      "MaxRotationChangeDegrees", 5.0,
      "Maximum Change in Rotations about x,y,or z in degrees(def=5)");
  declareProperty("MaxPositionChange_meters", .010,
                  "Maximum Change in Panel positions in meters(def=.01)");
  declareProperty("MaxSamplePositionChangeMeters", .005,
                  "Maximum Change in Sample position in meters(def=.005)");

  const string TOLERANCES("Tolerance settings");
  setPropertyGroup("tolerance", TOLERANCES);
  setPropertyGroup("MinimizerError", TOLERANCES);
  setPropertyGroup("Minimizer", TOLERANCES);
  setPropertyGroup("NumIterations", TOLERANCES);
  setPropertyGroup("MaxRotationChangeDegrees", TOLERANCES);
  setPropertyGroup("MaxPositionChange_meters", TOLERANCES);
  setPropertyGroup("MaxSamplePositionChangeMeters", TOLERANCES);

  declareProperty("ChiSqOverDOF", -1.0, "ChiSqOverDOF",
                  Kernel::Direction::Output);
  declareProperty("DOF", -1, "Degrees of Freedom", Kernel::Direction::Output);
  setPropertySettings("PanelNamePrefix",
                      Kernel::make_unique<EnabledWhenProperty>(
                          "PanelGroups", Kernel::IS_EQUAL_TO, "SpecifyGroups"));

  setPropertySettings("Grouping",
                      Kernel::make_unique<EnabledWhenProperty>(
                          "PanelGroups", Kernel::IS_EQUAL_TO, "SpecifyGroups"));

  setPropertySettings("PreProcFilename",
                      Kernel::make_unique<EnabledWhenProperty>(
                          "PreProcessInstrument", Kernel::IS_NOT_EQUAL_TO,
                          "A)No PreProcessing"));

  setPropertySettings("InitialTimeOffset",
                      Kernel::make_unique<EnabledWhenProperty>(
                          "PreProcessInstrument", Kernel::IS_EQUAL_TO,
                          "C)Apply a LoadParameter.xml type file"));

  setPropertySettings("MaxSamplePositionChangeMeters",
                      Kernel::make_unique<EnabledWhenProperty>(
                          "AllowSampleShift", Kernel::IS_EQUAL_TO, "1"));

  setPropertySettings("MaxRotationChangeDegrees",
                      Kernel::make_unique<EnabledWhenProperty>(
                          "usePanelOrientation", Kernel::IS_EQUAL_TO, "1"));
}

/**
 * Creates The SCDPanelErrors function with the optimum parameters to get the
 * resultant out,xvals to report results.
 * @param ws      The workspace sent to SCDPanelErrors
 * @param NGroups  The number if Groups
 * @param names     The parameter names
 * @param params    The parameter values
 * @param BankNameString   The /separated list of bank names. Groups separated
 * by !
 * @param out           The result of function1D. These are the differences in
 * the qx, qy,and qz values from the
 *                      theoretical qx,qy, and qz values
 * @param xVals        The xVals or indices of the peak in the PeakWorkspace
 * @param nData        The size of xVals and out
 */
void SCDCalibratePanels::CreateFxnGetValues(
    Workspace2D_sptr const ws, int const NGroups, vector<string> const names,
    vector<double> const params, string const BankNameString, double *out,
    const double *xVals, const size_t nData) const {
  boost::shared_ptr<IFunction1D> fit = boost::dynamic_pointer_cast<IFunction1D>(
      FunctionFactory::Instance().createFunction("SCDPanelErrors"));
  if (!fit)
    cout << "Could not create fit function" << endl;
  PeaksWorkspace_sptr peaksWs = getProperty("PeakWorkspace");
  OrientedLattice latt = peaksWs->mutableSample().getOrientedLattice();
  fit->setAttribute("a", IFunction::Attribute(latt.a()));
  fit->setAttribute("b", IFunction::Attribute(latt.b()));
  fit->setAttribute("c", IFunction::Attribute(latt.c()));
  fit->setAttribute("alpha", IFunction::Attribute(latt.alpha()));
  fit->setAttribute("beta", IFunction::Attribute(latt.beta()));
  fit->setAttribute("gamma", IFunction::Attribute(latt.gamma()));
  string PeakWSName = getPropertyValue("PeakWorkspace");
  if (PeakWSName.length() < 1)
    PeakWSName = "xxx";
  fit->setAttribute("PeakWorkspaceName", IFunction::Attribute(PeakWSName));
  fit->setAttribute("startX", IFunction::Attribute(-1));
  fit->setAttribute("endX", IFunction::Attribute(-1));
  fit->setAttribute("NGroups", IFunction::Attribute(NGroups));
  fit->setAttribute("BankNames", IFunction::Attribute(BankNameString));

  string fieldBase[8] = {"detWidthScale", "detHeightScale", "Xoffset",
                         "Yoffset",       "Zoffset",        "Xrot",
                         "Yrot",          "Zrot"};
  set<string> FieldB(fieldBase, fieldBase + 8);

  for (int g = 0; g < NGroups; ++g) {
    // Now add parameter values
    ostringstream prefixStrm(ostringstream::out);
    prefixStrm << "f" << g << "_";
    string prefix = prefixStrm.str();

    for (int nm = 0; nm < static_cast<int>(names.size()); ++nm) {
      if (names[nm].compare(0, prefix.length(), prefix) == 0) {
        string prm = names[nm].substr(prefix.length());
        if (FieldB.find(prm) != FieldB.end()) {
          fit->setParameter(names[nm], params[nm]);
        }
      } else if (names[nm] == "l0" || names[nm] == "t0")
        fit->setParameter(names[nm], params[nm]);
    }
  }

  fit->setWorkspace(ws);

  //------Call SCDPanelErrors to get the q errors ------------------
  fit->function1D(out, xVals, nData);
}

void SCDCalibratePanels::updateBankParams(
    boost::shared_ptr<const Geometry::IComponent> bank_const,
    boost::shared_ptr<Geometry::ParameterMap> pmap,
    boost::shared_ptr<const Geometry::ParameterMap> pmapSv) {
  vector<V3D> posv = pmapSv->getV3D(bank_const->getName(), "pos");

  if (!posv.empty()) {
    V3D pos = posv[0];
    pmap->addDouble(bank_const.get(), "x", pos.X());
    pmap->addDouble(bank_const.get(), "y", pos.Y());
    pmap->addDouble(bank_const.get(), "z", pos.Z());
    pmap->addV3D(bank_const.get(), "pos", pos);
  }

  boost::shared_ptr<Parameter> rot = pmapSv->get(bank_const.get(), ("rot"));
  if (rot) {
    pmap->addQuat(bank_const.get(), "rot", rot->value<Quat>());
  }

  vector<double> scalex = pmapSv->getDouble(bank_const->getName(), "scalex");
  vector<double> scaley = pmapSv->getDouble(bank_const->getName(), "scaley");
  if (!scalex.empty()) {
    pmap->addDouble(bank_const.get(), "scalex", scalex[0]);
  }
  if (!scaley.empty()) {
    pmap->addDouble(bank_const.get(), "scaley", scaley[0]);
  }

  boost::shared_ptr<const Geometry::IComponent> parent =
      bank_const->getParent();
  if (parent) {
    updateBankParams(parent, pmap, pmapSv);
  }
}

void SCDCalibratePanels::updateSourceParams(
    boost::shared_ptr<const Geometry::IComponent> bank_const,
    boost::shared_ptr<Geometry::ParameterMap> pmap,
    boost::shared_ptr<const Geometry::ParameterMap> pmapSv) {
  vector<V3D> posv = pmapSv->getV3D(bank_const->getName(), "pos");

  if (!posv.empty()) {
    V3D pos = posv[0];
    pmap->addDouble(bank_const.get(), "x", pos.X());
    pmap->addDouble(bank_const.get(), "y", pos.Y());
    pmap->addDouble(bank_const.get(), "z", pos.Z());
    pmap->addV3D(bank_const.get(), "pos", pos);
  }

  boost::shared_ptr<Parameter> rot = pmapSv->get(bank_const.get(), "rot");
  if (rot)
    pmap->addQuat(bank_const.get(), "rot", rot->value<Quat>());
}

void SCDCalibratePanels::FixUpSourceParameterMap(
    boost::shared_ptr<const Instrument> NewInstrument, double const L0,
    V3D const newSampPos, boost::shared_ptr<const ParameterMap> const pmapOld) {
  boost::shared_ptr<ParameterMap> pmap = NewInstrument->getParameterMap();
  IComponent_const_sptr source = NewInstrument->getSource();
  updateSourceParams(source, pmap, pmapOld);

  IComponent_const_sptr sample = NewInstrument->getSample();
  V3D SamplePos = sample->getPos();
  if (SamplePos != newSampPos) {
    V3D newSampRelPos = newSampPos - SamplePos;
    pmap->addPositionCoordinate(sample.get(), string("x"), newSampRelPos.X());
    pmap->addPositionCoordinate(sample.get(), string("y"), newSampRelPos.Y());
    pmap->addPositionCoordinate(sample.get(), string("z"), newSampRelPos.Z());
  }
  V3D sourceRelPos = source->getRelativePos();
  V3D sourcePos = source->getPos();
  V3D parentSourcePos = sourcePos - sourceRelPos;
  V3D source2sampleDir = SamplePos - source->getPos();

  double scalee = L0 / source2sampleDir.norm();
  V3D newsourcePos = sample->getPos() - source2sampleDir * scalee;
  V3D newsourceRelPos = newsourcePos - parentSourcePos;

  pmap->addPositionCoordinate(source.get(), string("x"), newsourceRelPos.X());
  pmap->addPositionCoordinate(source.get(), string("y"), newsourceRelPos.Y());
  pmap->addPositionCoordinate(source.get(), string("z"), newsourceRelPos.Z());
}

void SCDCalibratePanels::FixUpBankParameterMap(
    vector<string> const bankNames,
    boost::shared_ptr<const Instrument> NewInstrument, V3D const pos,
    Quat const rot, double const DetWScale, double const DetHtScale,
    boost::shared_ptr<const ParameterMap> const pmapOld, bool RotCenters) {
  boost::shared_ptr<ParameterMap> pmap = NewInstrument->getParameterMap();

  for (const auto &bankName : bankNames) {

    boost::shared_ptr<const IComponent> bank1 =
        NewInstrument->getComponentByName(bankName);
    boost::shared_ptr<const Geometry::RectangularDetector> bank =
        boost::dynamic_pointer_cast<const RectangularDetector>(
            bank1); // Component
    updateBankParams(bank, pmap, pmapOld);

    Quat RelRot = bank->getRelativeRot();
    Quat newRelRot = rot * RelRot;
    double rotx, roty, rotz;
    Quat2RotxRotyRotz(newRelRot, rotx, roty, rotz);

    pmap->addRotationParam(bank.get(), string("rotx"), rotx);
    pmap->addRotationParam(bank.get(), string("roty"), roty);
    pmap->addRotationParam(bank.get(), string("rotz"), rotz);
    pmap->addQuat(bank.get(), "rot",
                  newRelRot); // Should not have had to do this???
    //---------Rotate center of bank ----------------------
    V3D Center = bank->getPos();
    V3D Center_orig(Center);
    if (RotCenters)
      rot.rotate(Center);

    V3D pos1 = bank->getRelativePos();

    pmap->addPositionCoordinate(bank.get(), string("x"), pos.X() + pos1.X() +
                                                             Center.X() -
                                                             Center_orig.X());
    pmap->addPositionCoordinate(bank.get(), string("y"), pos.Y() + pos1.Y() +
                                                             Center.Y() -
                                                             Center_orig.Y());
    pmap->addPositionCoordinate(bank.get(), string("z"), pos.Z() + pos1.Z() +
                                                             Center.Z() -
                                                             Center_orig.Z());

    Quat2RotxRotyRotz(rot, rotx, roty, rotz);

    vector<double> oldScalex =
        pmap->getDouble(bank->getName(), string("scalex"));
    vector<double> oldScaley =
        pmap->getDouble(bank->getName(), string("scaley"));

    double scalex, scaley;
    if (!oldScalex.empty())
      scalex = oldScalex[0] * DetWScale;
    else
      scalex = DetWScale;

    if (!oldScaley.empty())
      scaley = oldScaley[0] * DetHtScale;
    else
      scaley = DetHtScale;

    pmap->addDouble(bank.get(), string("scalex"), scalex);
    pmap->addDouble(bank.get(), string("scaley"), scaley);
    // cout<<"Thru param fix for "<<bankName<<". pos="<<bank->getPos()<<endl;
  } // For @ bank
}

void writeXmlParameter(ofstream &ostream, const string &name,
                       const double value) {
  ostream << "  <parameter name =\"" << name << "\"><value val=\"" << value
          << "\" /> </parameter>" << endl;
}

void SCDCalibratePanels::saveXmlFile(
    string const FileName, vector<vector<string>> const Groups,
    Instrument_const_sptr const instrument) const {
  if (FileName.empty())
    return;

  g_log.notice() << "Saving parameter file as " << FileName << "\n";

  // create the file and add the header
  ofstream oss3(FileName.c_str());
  oss3 << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" << endl;
  oss3 << " <parameter-file instrument=\"" << instrument->getName()
       << "\" valid-from=\"" << instrument->getValidFromDate().toISO8601String()
       << "\">" << endl;
  ParameterMap_sptr pmap = instrument->getParameterMap();

  // write out the detector banks
  for (const auto &Group : Groups) {
    for (const auto &bankName : Group) {
      oss3 << "<component-link name=\"" << bankName << "\">" << endl;

      boost::shared_ptr<const IComponent> bank =
          instrument->getComponentByName(bankName);

      Quat RelRot = bank->getRelativeRot();

      double rotx, roty, rotz;

      SCDCalibratePanels::Quat2RotxRotyRotz(RelRot, rotx, roty, rotz);
      writeXmlParameter(oss3, "rotx", rotx);
      writeXmlParameter(oss3, "roty", roty);
      writeXmlParameter(oss3, "rotz", rotz);

      V3D pos1 = bank->getRelativePos();
      writeXmlParameter(oss3, "x", pos1.X());
      writeXmlParameter(oss3, "y", pos1.Y());
      writeXmlParameter(oss3, "z", pos1.Z());

      vector<double> oldScalex =
          pmap->getDouble(bank->getName(), string("scalex"));
      vector<double> oldScaley =
          pmap->getDouble(bank->getName(), string("scaley"));

      double scalex, scaley;
      if (!oldScalex.empty())
        scalex = oldScalex[0];
      else
        scalex = 1.;

      if (!oldScaley.empty())
        scaley = oldScaley[0];
      else
        scaley = 1.;

      oss3 << "  <parameter name =\"scalex\"><value val=\"" << scalex
           << "\" /> </parameter>" << endl;
      oss3 << "  <parameter name =\"scaley\"><value val=\"" << scaley
           << "\" /> </parameter>" << endl;
      oss3 << "</component-link>" << endl;
    } // for each bank in the group
  }   // for each group

  // write out the source
  IComponent_const_sptr source = instrument->getSource();

  oss3 << "<component-link name=\"" << source->getName() << "\">" << endl;
  IComponent_const_sptr sample = instrument->getSample();
  V3D sourceRelPos = source->getRelativePos();

  writeXmlParameter(oss3, "x", sourceRelPos.X());
  writeXmlParameter(oss3, "y", sourceRelPos.Y());
  writeXmlParameter(oss3, "z", sourceRelPos.Z());
  oss3 << "</component-link>" << endl;
  oss3 << "</parameter-file>" << endl;

  // flush and close the file
  oss3.flush();
  oss3.close();
}
/// Removes values that deviates more than sigmaCritical from the
/// intensities' mean.
void SCDCalibratePanels::removeOutliers(std::vector<double> &intensities) {

  if (intensities.size() > 2) {
    const std::vector<double> &zScores = Kernel::getZscore(intensities);

    std::vector<size_t> outlierIndices;
    for (size_t i = 0; i < zScores.size(); ++i) {
      if (zScores[i] > 1.0) {
        outlierIndices.push_back(i);
      }
    }

    if (outlierIndices.size() == intensities.size())
      return;

    if (!outlierIndices.empty()) {
      for (auto it = outlierIndices.rbegin(); it != outlierIndices.rend();
           ++it) {
        intensities.erase(intensities.begin() + (*it));
      }
    }
  }
}

} // namespace Crystal
} // namespace Mantid
