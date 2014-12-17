#include "MantidAPI/Algorithm.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidCrystal/GoniometerAnglesFromPhiRotation.h"
#include "MantidCrystal/FindUBUsingFFT.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/Property.h"
#include <cstdio>
#include <math.h>

using Mantid::Kernel::Direction;
using Mantid::Kernel::Logger;
using Mantid::API::IFunction;

namespace Mantid {
namespace Crystal {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(GoniometerAnglesFromPhiRotation)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

//--------------------------------------------------------------------------
/** Constructor
 */
GoniometerAnglesFromPhiRotation::GoniometerAnglesFromPhiRotation()
    : Algorithm() {}

GoniometerAnglesFromPhiRotation::~GoniometerAnglesFromPhiRotation() {}

void GoniometerAnglesFromPhiRotation::init() {
  declareProperty(new WorkspaceProperty<PeaksWorkspace>(
                      "PeaksWorkspace1", "", Kernel::Direction::Input),
                  "Input Peaks Workspace for Run 1");

  declareProperty(new WorkspaceProperty<PeaksWorkspace>(
                      "PeaksWorkspace2", "", Kernel::Direction::InOut),
                  "Input Peaks Workspace for Run 2");

  declareProperty(
      "Tolerance", .12,
      "Integer offset for h,k,and l values to be considered valid.(def=.12)");

  declareProperty("MIND", -1.0, "Minimium d-spacing to consider,(def=-1)");
  declareProperty("MAXD", -1.0, "Maximum d-spacing to consider,(def=-1)");

  declareProperty("Run1Phi", 0.0, "Phi for Run 1(def=0.0)");

  declareProperty(std::string("Phi2"), 0.0,
                  std::string("Phi angle for Run2(def=0.0)"),
                  Kernel::Direction::InOut);

  declareProperty("Chi2", 0.0, "Chi angle for Run2", Kernel::Direction::Output);
  declareProperty("Omega2", 0.0, "Omega angle for Run2",
                  Kernel::Direction::Output);

  declareProperty("NIndexed", 0, "Number peaks indexed",
                  Kernel::Direction::Output);

  declareProperty("AvErrIndex", 0.0,
                  "Average abs offset from integer values for indexed peaks",
                  Kernel::Direction::Output);

  declareProperty("AvErrAll", 0.0,
                  "Average abs offset from integer values for all peaks",
                  Kernel::Direction::Output);
}

/**
 * Calculate indexing stats if the peaks had been indexed with given UBraw by
 *NOT applying the goniometer settings,i.e.
 * UBraw is applied directly to Qlab.  NOTE:The h,k,l values of the peaks are
 *NOT changed.
 *
 * @param Peaks  The list of peaks
 * @param UBraw  The UB matrix that will be applied to Qlab. No goniometer
 *adjustments are made.
 * @param Nindexed    The number of peaks that would be indexed at the given
 *tolerance
 * @param AvErrIndexed The average error in the hkl values of the peaks that
 *would have been indexed at the given tolerance
 * @param AvErrorAll  The average error in the hkl values of all the peaks
 * @param tolerance  The indexing tolerance
 */
void GoniometerAnglesFromPhiRotation::IndexRaw(
    const PeaksWorkspace_sptr &Peaks, const Kernel::Matrix<double> &UBraw,
    int &Nindexed, double &AvErrIndexed, double &AvErrorAll,
    double tolerance) const {
  Kernel::Matrix<double> InvUB2(UBraw);

  InvUB2.Invert();
  InvUB2 /= (2 * M_PI);

  Nindexed = 0;
  double TotOffsetIndx = 0;
  double TotOffsetAll = 0;
  int Npeaks = Peaks->getNumberPeaks();

  for (int i = 0; i < Npeaks; i++) {
    V3D hkl = InvUB2 * Peaks->getPeak(i).getQLabFrame();
    double maxOffset = 0;
    for (int k = 0; k < 3; k++) {
      double offset = hkl[k] - floor(hkl[k]);
      if (offset > .5)
        offset -= 1;
      offset = fabs(offset);
      if (offset > maxOffset)
        maxOffset = offset;
    }

    if (maxOffset < tolerance) {
      Nindexed++;
      TotOffsetIndx += maxOffset;
    }

    TotOffsetAll += maxOffset;
  }

  if (Nindexed > 0)

    AvErrIndexed = TotOffsetIndx / Nindexed;

  else

    AvErrIndexed = -1.0;

  if (Npeaks > 0)

    AvErrorAll = TotOffsetAll / Npeaks;

  else

    AvErrorAll = -1.0;
}
void GoniometerAnglesFromPhiRotation::exec() {

  PeaksWorkspace_sptr PeaksRun1 = getProperty("PeaksWorkspace1");
  PeaksWorkspace_sptr PeaksRun2 = getProperty("PeaksWorkspace2");

  double Tolerance = getProperty("Tolerance");

  Kernel::Matrix<double> Gon1(3, 3);
  Kernel::Matrix<double> Gon2(3, 3);
  if (!CheckForOneRun(PeaksRun1, Gon1) || !CheckForOneRun(PeaksRun2, Gon2)) {
    g_log.error("Each peaks workspace MUST have only one run");
    throw std::invalid_argument("Each peaks workspace MUST have only one run");
  }

  Kernel::Matrix<double> UB1, UB2;

  bool Run1HasOrientedLattice = true;
  if (!PeaksRun1->sample().hasOrientedLattice()) {

    Run1HasOrientedLattice = false;

    const std::string fft("FindUBUsingFFT");
    API::IAlgorithm_sptr findUB = this->createChildAlgorithm(fft);
    findUB->initialize();
    findUB->setProperty<PeaksWorkspace_sptr>("PeaksWorkspace",
                                             getProperty("PeaksWorkspace1"));
    findUB->setProperty("MIND", (double)getProperty("MIND"));
    findUB->setProperty("MAXD", (double)getProperty("MAXD"));
    findUB->setProperty("Tolerance", Tolerance);

    findUB->executeAsChildAlg();

    if (!PeaksRun1->sample().hasOrientedLattice()) {
      g_log.notice(std::string("Could not find UB for ") +
                   std::string(PeaksRun1->name()));
      throw std::invalid_argument(std::string("Could not find UB for ") +
                                  std::string(PeaksRun1->name()));
    }
  }
  //-------------get UB raw :No goniometer----------------

  UB1 = PeaksRun1->sample().getOrientedLattice().getUB();

  UB1 = getUBRaw(UB1, Gon1);

  int N1;
  double avErrIndx, avErrAll;
  IndexRaw(PeaksRun1, UB1, N1, avErrIndx, avErrAll, Tolerance);

  if (N1 < .6 * PeaksRun1->getNumberPeaks()) {
    g_log.notice(std::string("UB did not index well for ") +
                 std::string(PeaksRun1->name()));
    throw std::invalid_argument(std::string("UB did not index well for ") +
                                std::string(PeaksRun1->name()));
  }

  //----------------------------------------------

  Geometry::OrientedLattice lat2 = PeaksRun1->sample().getOrientedLattice();

  lat2.setUB(UB1);
  PeaksRun2->mutableSample().setOrientedLattice(&lat2);

  PeaksWorkspace_sptr Peakss = getProperty("PeaksWorkspace2");

  if (!Run1HasOrientedLattice)
    PeaksRun1->mutableSample().setOrientedLattice(NULL);

  double dphi = (double)getProperty("Phi2") - (double)getProperty("Run1Phi");
  Kernel::Matrix<double> Gon22(3, 3, true);

  for (int i = 0; i < PeaksRun2->getNumberPeaks(); i++) {
    PeaksRun2->getPeak(i).setGoniometerMatrix(Gon22);
  }

  int RunNum = PeaksRun2->getPeak(0).getRunNumber();
  std::string RunNumStr = boost::lexical_cast<std::string>(RunNum);
  int Npeaks = PeaksRun2->getNumberPeaks();

  std::vector<double> MinData(5); // n indexed, av err, phi, chi,omega
  MinData[0] = 0.0;
  std::vector<V3D> directionList = IndexingUtils::MakeHemisphereDirections(50);

  API::FrameworkManager::Instance();

  for (size_t d = 0; d < directionList.size(); d++)
    for (int sgn = 1; sgn > -2; sgn -= 2) {
      V3D dir = directionList[d];
      dir.normalize();
      Quat Q(sgn * dphi, dir);
      Q.normalize();
      Kernel::Matrix<double> Rot(Q.getRotation());

      Kernel::Matrix<double> UB2(Rot * UB1);

      int Nindexed;
      double avErrIndx, avErrAll;
      IndexRaw(PeaksRun2, Rot * UB1, Nindexed, avErrIndx, avErrAll, Tolerance);

      if (Nindexed > MinData[0]) {
        MinData[0] = Nindexed;
        MinData[1] = sgn;
        MinData[2] = dir[0];

        MinData[3] = dir[1];
        MinData[4] = dir[2];
      }
    }

  g_log.debug() << "Best direction unOptimized is ("
                << (MinData[1] * MinData[2]) << "," << (MinData[1] * MinData[3])
                << "," << (MinData[1] * MinData[4]) << ")" << std::endl;

  //----------------------- Optimize around best
  //-------------------------------------------

  //               --------Create Workspace -------------------
  boost::shared_ptr<DataObjects::Workspace2D> ws =
      boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
          WorkspaceFactory::Instance().create("Workspace2D", 1, 3 * Npeaks,
                                              3 * Npeaks));
  MantidVecPtr Xvals, Yvals;

  for (int i = 0; i < Npeaks; ++i) {
    Xvals.access().push_back(i);
    Yvals.access().push_back(0.0);
    Xvals.access().push_back(i);
    Yvals.access().push_back(0.0);
    Xvals.access().push_back(i);
    Yvals.access().push_back(0.0);
  }
  ws->setX(0, Xvals);
  ws->setData(0, Yvals);

  //       -------------Set up other Fit function arguments------------------
  V3D dir(MinData[2], MinData[3], MinData[4]);
  dir.normalize();
  Quat Q(MinData[1] * dphi, dir);
  Q.normalize();
  Kernel::Matrix<double> Rot(Q.getRotation());

  Goniometer Gon(Rot);
  std::vector<double> omchiphi = Gon.getEulerAngles("yzy");
  MinData[2] = omchiphi[2];
  MinData[3] = omchiphi[1];
  MinData[4] = omchiphi[0];

  std::string FunctionArgs =
      "name=PeakHKLErrors, PeakWorkspaceName=" + PeaksRun2->name() +
      ",OptRuns=" + RunNumStr + ",phi" + RunNumStr + "=" +
      boost::lexical_cast<std::string>(MinData[2]) + ",chi" + RunNumStr + "=" +
      boost::lexical_cast<std::string>(MinData[3]) + ",omega" + RunNumStr +
      "=" + boost::lexical_cast<std::string>(MinData[4]);

  std::string Constr = boost::lexical_cast<std::string>(MinData[2] - 5) +
                       "<phi" + RunNumStr + "<" +
                       boost::lexical_cast<std::string>(MinData[2] + 5);
  Constr += "," + boost::lexical_cast<std::string>(MinData[3] - 5) + "<chi" +
            RunNumStr + "<" + boost::lexical_cast<std::string>(MinData[3] + 5) +
            ",";

  Constr += boost::lexical_cast<std::string>(MinData[4] - 5) + "<omega" +
            RunNumStr + "<" + boost::lexical_cast<std::string>(MinData[4] + 5);

  std::string Ties = "SampleXOffset=0.0,SampleYOffset=0.0,SampleZOffset=0.0,"
                     "GonRotx=0.0,GonRoty=0.0,GonRotz=0.0";

  boost::shared_ptr<Algorithm> Fit = createChildAlgorithm("Fit");

  Fit->initialize();
  Fit->setProperty("Function", FunctionArgs);
  Fit->setProperty("Ties", Ties);
  Fit->setProperty("Constraints", Constr);
  Fit->setProperty("InputWorkspace", ws);
  Fit->setProperty("CreateOutput", true);

  std::string outputName = "out";

  Fit->setProperty("Output", outputName);

  Fit->executeAsChildAlg();

  // std::string status = Fit->getProperty("OutputStatus");

  boost::shared_ptr<API::ITableWorkspace> results =
      Fit->getProperty("OutputParameters");
  double chisq = Fit->getProperty("OutputChi2overDoF");

  MinData[0] = chisq;
  MinData[2] = results->Double(6, 1);
  MinData[3] = results->Double(7, 1);
  MinData[4] = results->Double(8, 1);

  g_log.debug() << "Best direction Optimized is (" << (MinData[2]) << ","
                << (MinData[3]) << "," << (MinData[4]) << ")\n";

  //          ---------------------Find number indexed -----------------------
  Quat Q1 = Quat(MinData[4], V3D(0, 1, 0)) * Quat(MinData[3], V3D(0, 0, 1)) *
            Quat(MinData[2], V3D(0, 1, 0));

  int Nindexed;
  Kernel::Matrix<double> Mk(Q1.getRotation());
  IndexRaw(PeaksRun2, Mk * UB1, Nindexed, avErrIndx, avErrAll, Tolerance);

  //------------------------------------ Convert/Save Results
  //-----------------------------

  double deg, ax1, ax2, ax3;
  Q1.getAngleAxis(deg, ax1, ax2, ax3);
  if (dphi * deg < 0) {
    deg = -deg;
    ax1 = -ax1;
    ax2 = -ax2;
    ax3 = -ax3;
  }

  double phi2 = (double)getProperty("Run1Phi") + dphi;
  double chi2 = acos(ax2) / M_PI * 180;
  double omega2 = atan2(ax3, -ax1) / M_PI * 180;

  g_log.notice()
      << "============================ Results ============================"
      << std::endl;
  g_log.notice() << "     phi,chi, and omega= (" << phi2 << "," << chi2 << ","
                 << omega2 << ")" << std::endl;
  g_log.notice() << "     #indexed =" << Nindexed << std::endl;
  g_log.notice()
      << "              =============================================="
      << std::endl;

  // std::cout << "============================ Results
  // ============================" << std::endl;
  // std::cout << "     phi,chi, and omega= (" << phi2 << "," << chi2 << "," <<
  // omega2 << ")"
  //     << std::endl;
  // std::cout << "     #indexed =" << Nindexed << std::endl;
  // std::cout << "              =============================================="
  // << std::endl;

  setProperty("Phi2", phi2);
  setProperty("Chi2", chi2);
  setProperty("Omega2", omega2);

  setProperty("NIndexed", Nindexed);

  setProperty("AvErrIndex", avErrIndx);

  setProperty("AvErrAll", avErrAll);

  Q1 = Quat(omega2, V3D(0, 1, 0)) * Quat(chi2, V3D(0, 0, 1)) *
       Quat(phi2, V3D(0, 1, 0));
  Kernel::Matrix<double> Gon2a(Q1.getRotation());
  for (int i = 0; i < PeaksRun2->getNumberPeaks(); i++) {
    PeaksRun2->getPeak(i).setGoniometerMatrix(Gon2a);
  }

  OrientedLattice latt2(PeaksRun2->mutableSample().getOrientedLattice());
  // Kernel::Matrix<double> UB = latt2.getUB();
  Rot.Invert();
  Gon2a.Invert();
  latt2.setUB(Gon2a * Mk * UB1);

  PeaksRun2->mutableSample().setOrientedLattice(&latt2);
}

/**
 * Checks that a PeaksWorkspace has only one run.
 *
 * @param Peaks   The PeaksWorkspace
 * @param GoniometerMatrix  the goniometer matrix for the run
 */
bool GoniometerAnglesFromPhiRotation::CheckForOneRun(
    const PeaksWorkspace_sptr &Peaks,
    Kernel::Matrix<double> &GoniometerMatrix) const {

  int RunNumber = -1;
  for (int peak = 0; peak < Peaks->getNumberPeaks(); peak++) {
    int thisRunNum = Peaks->getPeak(peak).getRunNumber();
    GoniometerMatrix = Peaks->getPeak(peak).getGoniometerMatrix();

    if (RunNumber < 0)

      RunNumber = thisRunNum;

    else if (thisRunNum != RunNumber)

      return false;
  }

  return true;
}

/**
 * Returns the raw UB, its inverse indexes using Q lab.
 * @param UB  The UB matrix whose inverse is applied to QSample
 * @param GoniometerMatrix  the goniometer matrix
 *
 * @return The raw UB
 */
Kernel::Matrix<double> GoniometerAnglesFromPhiRotation::getUBRaw(
    const Kernel::Matrix<double> &UB,
    const Kernel::Matrix<double> &GoniometerMatrix) const {
  return GoniometerMatrix * UB;
}

} // Crystal
} // Mantid
