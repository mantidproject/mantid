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
#include "MantidCrystal/SCDPanelErrors.h"
#include <fstream>
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Crystal/ReducedCell.h"
#include <boost/math/special_functions/round.hpp>
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
constexpr double MAX_DET_HW_SCALE = 1.15;
constexpr double MIN_DET_HW_SCALE = 0.85;
constexpr double RAD_TO_DEG = 180. / M_PI;
}

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
  Mantid::MantidVec errB;
  bounds.clear();
  bounds.push_back(0);

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
            weight = 1.0 / peak.getSigmaIntensity();
          else if (peak.getIntensity() > 0.) // next favorite weight by I
            weight = 1.0 / peak.getIntensity();
          else if (peak.getBinCount() > 0.) // then by counts in peak centre
            weight = 1.0 / peak.getBinCount();

          const double PEAK_INDEX = static_cast<double>(j);
          for (size_t i = 0; i < 3; ++i) {
            xRef.push_back(PEAK_INDEX);
            errB.push_back(weight);
          }
        }
    } // for @ peak

    bounds.push_back(N);
  } // for @ bank name

  if (N < 4) // If not well indexed
    return boost::make_shared<DataObjects::Workspace2D>();

  auto mwkspc = API::createWorkspace<Workspace2D>(1, N, N);

  mwkspc->setPoints(0, xRef);
  mwkspc->setCounts(0, xRef.size(), 0.0);
  mwkspc->setCountStandardDeviations(0, std::move(errB));

  return mwkspc;
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
    g_log.notice() << "Improper PreCalibInstrument for " << bankName << '\n';
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

  // determine the lattice constants
  Kernel::Matrix<double> UB(3, 3);
  IndexingUtils::Optimize_UB(UB, hkl, qVecs);
  std::vector<double> lat(7);
  IndexingUtils::GetLatticeParameters(UB, lat);
  OrientedLattice o_lattice;
  o_lattice.setUB(UB);
  peaksWs->mutableSample().setOrientedLattice(&o_lattice);

  Kernel::Logger g_log("Calibration");
  g_log.notice() << "Lattice before optimization: " << lat[0] << " " << lat[1]
                 << " " << lat[2] << " " << lat[3] << " " << lat[4] << " "
                 << lat[5] << "\n";

  // see if the lattice constants are no worse than 25% out
  if (fabs(lat[0] - a) / a > .25)
    return false;
  if (fabs(lat[1] - b) / b > .25)
    return false;
  if (fabs(lat[2] - c) / c > .25)
    return false;
  if (fabs(lat[3] - alpha) / alpha > .25)
    return false;
  if (fabs(lat[4] - beta) / beta > .25)
    return false;
  if (fabs(lat[5] - gamma) / gamma > .25)
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
  //-----------------------------------------------------------------------------------------
  /**
    @param  ws           Name of workspace containing peaks
    @param  bankName     Name of bank containing peak
    @param  col          Column number containing peak
    @param  row          Row number containing peak
    @param  Edge         Number of edge points for each bank
    @return True if peak is on edge
  */
bool SCDCalibratePanels::edgePixel(PeaksWorkspace_sptr ws, std::string bankName,
                                   int col, int row, int Edge) {
  if (bankName.compare("None") == 0)
    return false;
  Geometry::Instrument_const_sptr Iptr = ws->getInstrument();
  boost::shared_ptr<const IComponent> parent =
      Iptr->getComponentByName(bankName);
  if (parent->type().compare("RectangularDetector") == 0) {
    boost::shared_ptr<const RectangularDetector> RDet =
        boost::dynamic_pointer_cast<const RectangularDetector>(parent);

    return col < Edge || col >= (RDet->xpixels() - Edge) || row < Edge ||
           row >= (RDet->ypixels() - Edge);
  } else {
    std::vector<Geometry::IComponent_const_sptr> children;
    boost::shared_ptr<const Geometry::ICompAssembly> asmb =
        boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(parent);
    asmb->getChildren(children, false);
    int startI = 1;
    if (children[0]->getName() == "sixteenpack") {
      startI = 0;
      parent = children[0];
      children.clear();
      boost::shared_ptr<const Geometry::ICompAssembly> asmb =
          boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(parent);
      asmb->getChildren(children, false);
    }
    boost::shared_ptr<const Geometry::ICompAssembly> asmb2 =
        boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(children[0]);
    std::vector<Geometry::IComponent_const_sptr> grandchildren;
    asmb2->getChildren(grandchildren, false);
    int NROWS = static_cast<int>(grandchildren.size());
    int NCOLS = static_cast<int>(children.size());
    // Wish pixels and tubes start at 1 not 0
    return col - startI < Edge || col - startI >= (NCOLS - Edge) ||
           row - startI < Edge || row - startI >= (NROWS - Edge);
  }
  return false;
}

void SCDCalibratePanels::exec() {
  PeaksWorkspace_sptr peaksWs = getProperty("PeakWorkspace");
  // We must sort the peaks
  std::vector<std::pair<std::string, bool>> criteria;
  criteria.push_back(std::pair<std::string, bool>("BankName", true));
  peaksWs->sort(criteria);
  // Remove peaks on edge
  int edge = this->getProperty("EdgePixels");
  if (edge > 0) {
    for (int i = int(peaksWs->getNumberPeaks()) - 1; i >= 0; --i) {
      const std::vector<Peak> &peaks = peaksWs->getPeaks();
      if (edgePixel(peaksWs, peaks[i].getBankName(), peaks[i].getCol(),
                    peaks[i].getRow(), edge)) {
        peaksWs->removePeak(i);
      }
    }
  }
  IAlgorithm_sptr ub_alg;
  try {
    ub_alg = createChildAlgorithm("CalculateUMatrix", -1, -1, false);
  } catch (Exception::NotFoundError &) {
    g_log.error("Can't locate CalculateUMatrix algorithm");
    throw;
  }
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
  }
  ub_alg->setProperty("PeaksWorkspace", peaksWs);
  ub_alg->setProperty("a", a);
  ub_alg->setProperty("b", b);
  ub_alg->setProperty("c", c);
  ub_alg->setProperty("alpha", alpha);
  ub_alg->setProperty("beta", beta);
  ub_alg->setProperty("gamma", gamma);
  ub_alg->executeAsChildAlg();

  for (int i = int(peaksWs->getNumberPeaks()) - 1; i >= 0; --i) {
    Peak peak = peaksWs->getPeak(i);
    if (peak.getHKL() == V3D(0,0,0) || peak.getBankName() != "bank13") {
      peaksWs->removePeak(i);
    }
  }
  int nPeaks = peaksWs->getNumberPeaks();

  MatrixWorkspace_sptr q3DWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
        API::WorkspaceFactory::Instance().create("Workspace2D", 1,
                                                 nPeaks*3, nPeaks*3));

    auto &outSpec = q3DWS->getSpectrum(0);
    MantidVec &yVec = outSpec.dataY();
    MantidVec &xVec = outSpec.dataX();

  OrientedLattice lattice = peaksWs->mutableSample().getOrientedLattice();
  for (int i = 0; i < nPeaks; i++) {
    Peak peak = peaksWs->getPeak(i);
    V3D hkl =  V3D(boost::math::iround(peak.getH()), boost::math::iround(peak.getK()),
         boost::math::iround(peak.getL()));
    V3D Q2 = lattice.qFromHKL(hkl);
    xVec[i*3] = i*3;
    xVec[i*3+1] = i*3+1;
    xVec[i*3+2] = i*3+2;
    yVec[i*3] = Q2.X();
    yVec[i*3+1] = Q2.Y();
    yVec[i*3+2] = Q2.Z();
  }

  IAlgorithm_sptr fit_alg;
  try {
    fit_alg = createChildAlgorithm("Fit", -1, -1, false);
  } catch (Exception::NotFoundError &) {
    g_log.error("Can't locate Fit algorithm");
    throw;
  }
  std::ostringstream fun_str;
  fun_str << "name=SCDPanelErrors,Workspace="<<peaksWs->name()<<",Bank=bank13";
  fit_alg->setPropertyValue("Function", fun_str.str());
  fit_alg->setProperty("InputWorkspace", q3DWS);
  fit_alg->setProperty("CreateOutput", true);
  fit_alg->setProperty("Output", "fit");
  fit_alg->setProperty("MaxIterations", 0);
  fit_alg->executeAsChildAlg();
  MatrixWorkspace_sptr fitWS = fit_alg->getProperty("OutputWorkspace");
  AnalysisDataService::Instance().addOrReplace("fit", fitWS);
  ITableWorkspace_sptr paramsWS = fit_alg->getProperty("OutputParameters");
  AnalysisDataService::Instance().addOrReplace("params", paramsWS);
  double xShift = paramsWS->getRef<double>("Value",0);
  double yShift = paramsWS->getRef<double>("Value",1);
  double zShift = paramsWS->getRef<double>("Value",2);
  double xRotate = paramsWS->getRef<double>("Value",3);
  double yRotate = paramsWS->getRef<double>("Value",4);
  double zRotate = paramsWS->getRef<double>("Value",5);
  SCDPanelErrors det;
  det.moveDetector(xShift, yShift, zShift, xRotate, yRotate, zRotate, "bank13", peaksWs);

  string DetCalFileName = getProperty("DetCalFilename");
  set<string> MyBankNames;
  MyBankNames.insert("bank13");
  Instrument_sptr inst = boost::const_pointer_cast<Instrument>(peaksWs->getInstrument());
  saveIsawDetCal(inst, MyBankNames, 0.0,
                 DetCalFileName);
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

    string bankName = bankPrefixName + std::to_string(id);

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
      string GroupName = string("Group") + std::to_string(g);
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

  //setProperty("ResultWorkspace", Result);
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
    boost::shared_ptr<Instrument> &instrument, set<string> &AllBankName,
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
  std::vector<std::string> cellTypes;
  cellTypes.push_back(ReducedCell::CUBIC());
  cellTypes.push_back(ReducedCell::TETRAGONAL());
  cellTypes.push_back(ReducedCell::ORTHORHOMBIC());
  cellTypes.push_back(ReducedCell::HEXAGONAL());
  cellTypes.push_back(ReducedCell::RHOMBOHEDRAL());
  cellTypes.push_back(ReducedCell::MONOCLINIC());
  cellTypes.push_back(ReducedCell::TRICLINIC());
  declareProperty("CellType", cellTypes[6],
                  boost::make_shared<StringListValidator>(cellTypes),
                  "Select the cell type.");
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
  declareProperty(
      Kernel::make_unique<FileProperty>("DetCalFilename", "SCDCalibrate.DetCal",
                                        FileProperty::Save, detcalExts),
      "Path to an ISAW-style .detcal file to save.");

  declareProperty(
      Kernel::make_unique<FileProperty>("XmlFilename", "",
                                        FileProperty::OptionalSave, ".xml"),
      "Path to an Mantid .xml description(for LoadParameterFile) file to "
      "save.");

  /*declareProperty(
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
      "Workspace comparing calculated and theoretical TOF of each peak.");*/

  const string OUTPUTS("Outputs");
  setPropertyGroup("DetCalFilename", OUTPUTS);
  setPropertyGroup("XmlFilename", OUTPUTS);
  /*setPropertyGroup("ResultWorkspace", OUTPUTS);
  setPropertyGroup("ColWorkspace", OUTPUTS);
  setPropertyGroup("RowWorkspace", OUTPUTS);
  setPropertyGroup("TofWorkspace", OUTPUTS);*/

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

  declareProperty("EdgePixels", 0,
                  "Remove peaks that are at pixels this close to edge. ");
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
    cout << "Could not create fit function\n";
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
    // cout<<"Thru param fix for "<<bankName<<". pos="<<bank->getPos()<<'\n';
  } // For @ bank
}

void writeXmlParameter(ofstream &ostream, const string &name,
                       const double value) {
  ostream << "  <parameter name =\"" << name << "\"><value val=\"" << value
          << "\" /> </parameter>\n";
}

void SCDCalibratePanels::saveXmlFile(
    string const FileName, vector<vector<string>> const Groups,
    Instrument_const_sptr const instrument) const {
  if (FileName.empty())
    return;

  g_log.notice() << "Saving parameter file as " << FileName << "\n";

  // create the file and add the header
  ofstream oss3(FileName.c_str());
  oss3 << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
  oss3 << " <parameter-file instrument=\"" << instrument->getName()
       << "\" valid-from=\"" << instrument->getValidFromDate().toISO8601String()
       << "\">\n";
  ParameterMap_sptr pmap = instrument->getParameterMap();

  // write out the detector banks
  for (const auto &Group : Groups) {
    for (const auto &bankName : Group) {
      oss3 << "<component-link name=\"" << bankName << "\">\n";

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
           << "\" /> </parameter>\n";
      oss3 << "  <parameter name =\"scaley\"><value val=\"" << scaley
           << "\" /> </parameter>\n";
      oss3 << "</component-link>\n";
    } // for each bank in the group
  }   // for each group

  // write out the source
  IComponent_const_sptr source = instrument->getSource();

  oss3 << "<component-link name=\"" << source->getName() << "\">\n";
  IComponent_const_sptr sample = instrument->getSample();
  V3D sourceRelPos = source->getRelativePos();

  writeXmlParameter(oss3, "x", sourceRelPos.X());
  writeXmlParameter(oss3, "y", sourceRelPos.Y());
  writeXmlParameter(oss3, "z", sourceRelPos.Z());
  oss3 << "</component-link>\n";
  oss3 << "</parameter-file>\n";

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
