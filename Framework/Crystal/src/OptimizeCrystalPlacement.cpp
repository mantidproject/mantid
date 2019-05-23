// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/*
 *
 * OptimizeCrystalPlacement.cpp
 *
 *  Created on: Jan 26, 2013
 *      Author: ruth
 */
#include "MantidCrystal/OptimizeCrystalPlacement.h"

#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidCrystal/CalibrationHelpers.h"
#include "MantidCrystal/PeakHKLErrors.h"
#include "MantidCrystal/SCDCalibratePanels.h"
#include "MantidGeometry/Crystal/IPeak.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/EnabledWhenProperty.h"

#include <cstdarg>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using Mantid::Geometry::IndexingUtils;
using Mantid::Geometry::Instrument_const_sptr;
using namespace Mantid::Geometry;

namespace Mantid {

namespace Crystal {

DECLARE_ALGORITHM(OptimizeCrystalPlacement)

class OrEnabledWhenProperties : public Kernel::IPropertySettings {
public:
  OrEnabledWhenProperties(const std::string &prop1Name,
                          ePropertyCriterion prop1Crit,
                          const std::string &prop1Value,
                          const std::string &prop2Name,
                          ePropertyCriterion prop2Crit,
                          const std::string &prop2Value)
      : IPropertySettings(), propName1(prop1Name), propName2(prop2Name),
        Criteria1(prop1Crit), Criteria2(prop2Crit), value1(prop1Value),
        value2(prop2Value)

  {
    Prop1 = std::make_unique<Kernel::EnabledWhenProperty>(propName1, Criteria1, value1);
    Prop2 = std::make_unique<Kernel::EnabledWhenProperty>(propName2, Criteria2, value2);
  }
  ~OrEnabledWhenProperties() override // responsible for deleting all supplied
                                      // EnabledWhenProperites
  {
  }

  IPropertySettings *clone() const override {
    return new OrEnabledWhenProperties(propName1, Criteria1, value1, propName2,
                                       Criteria2, value2);
  }

  bool isEnabled(const IPropertyManager *algo) const override {
    return Prop1->isEnabled(algo) && Prop2->isEnabled(algo);
  }

private:
  std::string propName1, propName2;
  ePropertyCriterion Criteria1, Criteria2;
  std::string value1, value2;
  std::unique_ptr<Kernel::EnabledWhenProperty> Prop1, Prop2;
};

void OptimizeCrystalPlacement::init() {
  declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>(
                      "PeaksWorkspace", "", Direction::Input),
                  "Workspace of Peaks with UB loaded");
  declareProperty(std::make_unique<ArrayProperty<int>>(
                      std::string("KeepGoniometerFixedfor"), Direction::Input),
                  "List of run Numbers for which the goniometer settings will "
                  "NOT be changed");

  declareProperty(
      std::make_unique<WorkspaceProperty<PeaksWorkspace>>(
          "ModifiedPeaksWorkspace", "", Direction::Output),
      "Output Workspace of Peaks with optimized sample Orientations");

  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>(
                      "FitInfoTable", "FitInfoTable", Direction::Output),
                  "Workspace of Results");

  declareProperty("AdjustSampleOffsets", false,
                  "If true sample offsets will be adjusted to give better "
                  "fits, otherwise they will be fixed as zero(def=true)");

  declareProperty(
      "OptimizeGoniometerTilt", false,
      "Set true if main error is due to a tilted Goniometer(def=false)");

  declareProperty("Chi2overDoF", -1.0, "chi squared over dof",
                  Direction::Output);
  declareProperty("nPeaks", -1, "Number of Peaks Used", Direction::Output);
  declareProperty("nParams", -1, "Number of Parameters fit", Direction::Output);
  declareProperty(
      "nIndexed", -1,
      "Number of new Peaks that WOULD be indexed at 'MaxIndexingError'",
      Direction::Output);

  declareProperty("MaxAngularChange", 5.0,
                  "Max offset in degrees from current settings(def=5)");

  declareProperty("MaxIndexingError", 0.15,
                  "Use only peaks whose fractional "
                  "hkl values are below this "
                  "tolerance(def=0.15)");

  declareProperty("MaxHKLPeaks2Use", -1.0,
                  "If less than 0 all peaks are used, "
                  "otherwise only peaks whose h,k, "
                  "and l values are below the level "
                  "are used(def=-1)");
  declareProperty("MaxSamplePositionChangeMeters", .0005,
                  "Maximum Change in Sample position in meters(def=.0005)");

  setPropertyGroup("MaxAngularChange", "Tolerance settings");

  setPropertyGroup("MaxSamplePositionChangeMeters", "Tolerance settings");
  setPropertyGroup("MaxHKLPeaks2Use", "Tolerance settings");
  setPropertyGroup("MaxIndexingError", "Tolerance settings");

  setPropertySettings("MaxSamplePositionChangeMeters",
                      std::make_unique<EnabledWhenProperty>(
                          "AdjustSampleOffsets", Kernel::IS_EQUAL_TO, "1"));

  setPropertySettings("KeepGoniometerFixedfor",
                      std::make_unique<OrEnabledWhenProperties>(
                          "AdjustSampleOffsets", Kernel::IS_EQUAL_TO, "0",
                          "OptimizeGoniometerTilt", Kernel::IS_EQUAL_TO, "0"));

  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>(
                      "OutputNormalisedCovarianceMatrixOptX", "CovarianceInfo",
                      Direction::Output),
                  "The name of the TableWorkspace in which to store the final "
                  "covariance matrix");
}

/**
 * Execute algorithm. Steps:
 * a) Get property values
 * b) Set up data for call to PeakHKLErrors fitting function
 * c) execute and get results
 * d) Convert results to output information
 *
 */
void OptimizeCrystalPlacement::exec() {
  PeaksWorkspace_sptr peaks = getProperty("PeaksWorkspace");
  PeaksWorkspace_sptr outPeaks = getProperty("ModifiedPeaksWorkspace");

  if (peaks != outPeaks) {
    outPeaks = peaks->clone();
  }

  std::vector<int> NOoptimizeRuns = getProperty("KeepGoniometerFixedfor");

  const DblMatrix X = peaks->sample().getOrientedLattice().getUB();
  Matrix<double> UBinv(X);
  UBinv.Invert();

  //--------------------------------- Set up data for call to PeakHKLErrors
  // fitting function  ----------
  //              ---- Setting up workspace supplied to PeakHKLErrors
  //              ---------------
  std::vector<int> RunNumList;
  std::vector<V3D> ChiPhiOmega;
  Mantid::MantidVec xRef;

  int nPeaksUsed = 0;
  double HKLintOffsetMax = getProperty("MaxIndexingError");
  double HKLMax = getProperty("MaxHKLPeaks2Use");
  for (int i = 0; i < peaks->getNumberPeaks(); i++) {
    IPeak &peak = peaks->getPeak(i);
    int runNum = peak.getRunNumber();
    auto it = RunNumList.begin();
    for (; it != RunNumList.end() && *it != runNum; ++it) {
    }

    V3D hkl = UBinv * (peak.getQSampleFrame()) / (2.0 * M_PI);
    bool use =
        IndexingUtils::ValidIndex(hkl, HKLintOffsetMax); // use this peak???

    if (use && HKLMax > 0)
      for (int k = 0; k < 3; k++) {
        if (fabs(hkl[k]) > HKLMax)
          use = false;
      }

    if (it == RunNumList.end() &&
        use) // add to list of unique run numbers in workspace
    {
      RunNumList.push_back(runNum);

      Geometry::Goniometer Gon(peak.getGoniometerMatrix());
      std::vector<double> phichiOmega = Gon.getEulerAngles("YZY");
      ChiPhiOmega.emplace_back(phichiOmega[1], phichiOmega[2], phichiOmega[0]);
    }

    if (use) // add to lists for workspace
    {
      nPeaksUsed++;
      xRef.push_back(static_cast<double>(i));
      xRef.push_back(static_cast<double>(i));
      xRef.push_back(static_cast<double>(i));
    }
  }

  g_log.notice() << "Number initially indexed = " << nPeaksUsed
                 << " at tolerance = " << HKLintOffsetMax << '\n';

  if (nPeaksUsed < 1) {
    g_log.error() << "Error in UB too large. 0 peaks indexed at "
                  << HKLintOffsetMax << '\n';
    throw std::invalid_argument("Error in UB too large. 0 peaks indexed ");
  }

  int N = 3 * nPeaksUsed; // peaks->getNumberPeaks();
  auto mwkspc = createWorkspace<Workspace2D>(1, N, N);
  mwkspc->setPoints(0, xRef);
  mwkspc->setCounts(0, N, 0.0);
  mwkspc->setCountStandardDeviations(0, N, 1.0);

  std::string FuncArg = "name=PeakHKLErrors,PeakWorkspaceName=" +
                        getPropertyValue("PeaksWorkspace") + "";

  std::string OptRunNums;

  //--------- Setting Function and Constraint argumens to PeakHKLErrors
  //---------------
  std::vector<std::string> ChRunNumList;
  std::string predChar;
  for (auto runNum : RunNumList) {
    auto it1 = NOoptimizeRuns.begin();
    for (; it1 != NOoptimizeRuns.end() && *it1 != runNum; ++it1) {
    }

    if (it1 == NOoptimizeRuns.end()) {
      std::string runNumStr = std::to_string(runNum);
      OptRunNums += predChar + runNumStr;
      predChar = "/";
      ChRunNumList.push_back(runNumStr);
    }
  }

  bool omitRuns = (bool)getProperty("AdjustSampleOffsets") ||
                  (bool)getProperty("OptimizeGoniometerTilt");
  if (omitRuns) {
    NOoptimizeRuns = RunNumList;
    OptRunNums = "";
    std::string message = "No Goniometer Angles ";
    if ((bool)getProperty("OptimizeGoniometerTilt"))
      message += "relative to the tilted Goniometer ";
    message += "will be 'changed'";
    g_log.notice(message);
  }
  if (!OptRunNums.empty() && !omitRuns)
    FuncArg += ",OptRuns=" + OptRunNums;

  //------------- Add initial parameter values to FuncArg -----------

  std::ostringstream oss(std::ostringstream::out);
  oss.precision(3);
  std::ostringstream oss1(std::ostringstream::out); // constraints
  oss1.precision(3);

  int nParams = 3;
  double DegreeTol = getProperty("MaxAngularChange");
  std::string startConstraint;

  for (size_t i = 0; i < RunNumList.size(); i++) {
    int runNum = RunNumList[i];

    size_t k = 0;
    for (; k < NOoptimizeRuns.size(); k++) {
      if (NOoptimizeRuns[k] == runNum)
        break;
    }
    if (k >= NOoptimizeRuns.size()) {
      V3D chiphiomega = ChiPhiOmega[i];
      oss << ",chi" << runNum << "=" << chiphiomega[0] << ",phi" << runNum
          << "=" << chiphiomega[1] << ",omega" << runNum << "="
          << chiphiomega[2];

      oss1 << startConstraint << chiphiomega[0] - DegreeTol << "<chi" << runNum
           << "<" << chiphiomega[0] + DegreeTol;
      oss1 << "," << chiphiomega[1] - DegreeTol << "<phi" << runNum << "<"
           << chiphiomega[1] + DegreeTol;

      oss1 << "," << chiphiomega[2] - DegreeTol << "<omega" << runNum << "<"
           << chiphiomega[2] + DegreeTol;
      startConstraint = ",";
      nParams += 3;
    }
  }

  // offset of previous sample position so should start at 0
  V3D sampPos = V3D(0., 0., 0.);

  oss << ",SampleXOffset=" << sampPos.X() << ",SampleYOffset=" << sampPos.Y()
      << ",SampleZOffset=" << sampPos.Z();
  oss << ",GonRotx=0.0,GonRoty=0.0,GonRotz=0.0";

  double maxSampshift = getProperty("MaxSamplePositionChangeMeters");
  oss1 << startConstraint << sampPos.X() - maxSampshift << "<SampleXOffset<"
       << sampPos.X() + maxSampshift << "," << sampPos.Y() - maxSampshift
       << "<SampleYOffset<" << sampPos.Y() + maxSampshift << ","
       << sampPos.Z() - maxSampshift << "<SampleZOffset<"
       << sampPos.Z() + maxSampshift;

  oss1 << "," << -DegreeTol << "<GonRotx<" << DegreeTol << "," << -DegreeTol
       << "<GonRoty<" << DegreeTol << "," << -DegreeTol << "<GonRotz<"
       << DegreeTol;

  FuncArg += oss.str();
  std::string Constr = oss1.str();

  g_log.debug() << "Function argument=" << FuncArg << '\n';
  g_log.debug() << "Constraint argument=" << Constr << '\n';

  //--------------------- set up Fit algorithm call-----------------

  boost::shared_ptr<Algorithm> fit_alg =
      createChildAlgorithm("Fit", .1, .93, true);

  fit_alg->setProperty("Function", FuncArg);

  fit_alg->setProperty("MaxIterations", 60);

  fit_alg->setProperty("Constraints", Constr);

  fit_alg->setProperty("InputWorkspace", mwkspc);

  fit_alg->setProperty("CreateOutput", true);

  std::string Ties;

  if (!(bool)getProperty("AdjustSampleOffsets")) {
    std::ostringstream oss3(std::ostringstream::out);
    oss3.precision(3);

    oss3 << "SampleXOffset=" << sampPos.X() << ",SampleYOffset=" << sampPos.Y()
         << ",SampleZOffset=" << sampPos.Z();
    Ties = oss3.str();
  }

  if (!(bool)getProperty("OptimizeGoniometerTilt")) {
    if (!Ties.empty())
      Ties += ",";

    Ties += "GonRotx=0.0,GonRoty=0.0,GonRotz=0.0";
  }

  if (!Ties.empty())
    fit_alg->setProperty("Ties", Ties);

  fit_alg->setProperty("Output", "out");

  fit_alg->executeAsChildAlg();

  //------------------------- Get/Report  Results ------------------

  double chisq = fit_alg->getProperty("OutputChi2overDoF");
  g_log.notice() << "Fit finished. Status="
                 << (std::string)fit_alg->getProperty("OutputStatus") << '\n';

  setProperty("Chi2overDoF", chisq);

  setProperty("nPeaks", nPeaksUsed);
  setProperty("nParams", nParams);

  g_log.debug() << "Chi2overDof=" << chisq << "    # Peaks used=" << nPeaksUsed
                << "# fitting parameters =" << nParams
                << "   dof=" << (nPeaksUsed - nParams) << '\n';

  ITableWorkspace_sptr RRes = fit_alg->getProperty("OutputParameters");

  double sigma = sqrt(chisq);

  std::string OutputStatus = fit_alg->getProperty("OutputStatus");
  g_log.notice() << "Output Status=" << OutputStatus << '\n';

  //------------------ Fix up Covariance output --------------------
  ITableWorkspace_sptr NormCov =
      fit_alg->getProperty("OutputNormalisedCovarianceMatrix");
  setProperty("OutputNormalisedCovarianceMatrixOptX",
              NormCov); // What if 2 instances are run

  if (chisq < 0 || chisq != chisq)
    sigma = -1;

  //------------- Fix up Result workspace values ----------------------------
  std::map<std::string, double> Results;
  for (int prm = 0; prm < static_cast<int>(RRes->rowCount()); ++prm) {
    std::string namee = RRes->getRef<std::string>("Name", prm);

    std::string start = namee.substr(0, 3);
    if (start != "chi" && start != "phi" && start != "ome" && start != "Sam" &&
        start != "Gon")
      continue;

    double value = RRes->getRef<double>("Value", prm);
    Results[namee] = value;

    // Set sigma==1 in optimization. A better estimate is sqrt(Chi2overDoF)
    double v = sigma * RRes->getRef<double>("Error", prm);
    RRes->getRef<double>("Error", prm) = v;
  }

  //-----------Fix up Resultant workspace return info -------------------

  setProperty("FitInfoTable", RRes);

  //----------- update instrument -------------------------
  V3D newSampPos(Results["SampleXOffset"], Results["SampleYOffset"],
                 Results["SampleZOffset"]);

  auto &componentInfo = outPeaks->mutableComponentInfo();
  CalibrationHelpers::adjustUpSampleAndSourcePositions(
      componentInfo.l1(), newSampPos, componentInfo);

  Matrix<double> GonTilt =
      PeakHKLErrors::RotationMatrixAboutRegAxis(Results["GonRotx"], 'x') *
      PeakHKLErrors::RotationMatrixAboutRegAxis(Results["GonRoty"], 'y') *
      PeakHKLErrors::RotationMatrixAboutRegAxis(Results["GonRotz"], 'z');

  int prevRunNum = -1;
  std::map<int, Matrix<double>> MapRunNum2GonMat;
  std::string OptRun2 = "/" + OptRunNums + "/";

  int nIndexed = 0;
  UBinv = outPeaks->sample().getOrientedLattice().getUB();
  UBinv.Invert();
  UBinv /= (2 * M_PI);
  for (int i = 0; i < outPeaks->getNumberPeaks(); ++i) {
    auto &peak = outPeaks->getPeak(i);
    peak.setSamplePos(peak.getSamplePos() + newSampPos);
    int RunNum = peak.getRunNumber();
    std::string RunNumStr = std::to_string(RunNum);
    Matrix<double> GonMatrix;
    if (RunNum == prevRunNum ||
        MapRunNum2GonMat.find(RunNum) != MapRunNum2GonMat.end())
      GonMatrix = MapRunNum2GonMat[RunNum];
    else if (OptRun2.find("/" + RunNumStr + "/") < OptRun2.size() - 2) {

      double chi = Results["chi" + RunNumStr];
      double phi = Results["phi" + RunNumStr];
      double omega = Results["omega" + RunNumStr];

      Mantid::Geometry::Goniometer uniGonio;
      uniGonio.makeUniversalGoniometer();

      uniGonio.setRotationAngle("phi", phi);
      uniGonio.setRotationAngle("chi", chi);
      uniGonio.setRotationAngle("omega", omega);

      GonMatrix = GonTilt * uniGonio.getR();
      MapRunNum2GonMat[RunNum] = GonMatrix;
    } else {
      GonMatrix = GonTilt * peak.getGoniometerMatrix();
      MapRunNum2GonMat[RunNum] = GonMatrix;
    }

    peak.setGoniometerMatrix(GonMatrix);
    V3D hkl = UBinv * peak.getQSampleFrame();
    if (Geometry::IndexingUtils::ValidIndex(hkl, HKLintOffsetMax))
      nIndexed++;

    prevRunNum = RunNum;
  }

  if (MapRunNum2GonMat.size() == 1) // Only one RunNumber in this PeaksWorkspace
  {
    Matrix<double> GonMatrix =
        MapRunNum2GonMat[outPeaks->getPeak(0).getRunNumber()];
    Geometry::Goniometer Gon(GonMatrix);
    outPeaks->mutableRun().setGoniometer(Gon, false);
  }

  setProperty("ModifiedPeaksWorkspace", outPeaks);
  setProperty("nIndexed", nIndexed);
  g_log.notice() << "Number indexed after optimization= " << nIndexed
                 << " at tolerance = " << HKLintOffsetMax << '\n';

} // exec

} // namespace Crystal

} // namespace Mantid
