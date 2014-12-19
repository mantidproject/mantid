/*
 * OptimizeCrystalPlacement.cpp
 *
 *  Created on: Jan 26, 2013
 *      Author: ruth
 */
#include "MantidAPI/IPeak.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidCrystal/OptimizeCrystalPlacement.h"
#include "MantidCrystal/PeakHKLErrors.h"
#include "MantidCrystal/SCDCalibratePanels.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/IPropertySettings.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/V3D.h"
#include <boost/lexical_cast.hpp>
#include <cstdarg>
#include <iostream>
#include <map>

#include <math.h>
#include <sstream>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using Mantid::Geometry::IndexingUtils;
using Mantid::Geometry::Instrument;
using Mantid::Geometry::Instrument_const_sptr;
using Mantid::Geometry::ParameterMap;

namespace Mantid {

namespace Crystal {

DECLARE_ALGORITHM(OptimizeCrystalPlacement)

class OrEnabledWhenProperties : public Kernel::IPropertySettings {
public:
  OrEnabledWhenProperties(std::string prop1Name, ePropertyCriterion prop1Crit,
                          std::string prop1Value, std::string prop2Name,
                          ePropertyCriterion prop2Crit, std::string prop2Value)
      : IPropertySettings(), propName1(prop1Name), propName2(prop2Name),
        Criteria1(prop1Crit), Criteria2(prop2Crit), value1(prop1Value),
        value2(prop2Value)

  {
    Prop1 = new Kernel::EnabledWhenProperty(propName1, Criteria1, value1);
    Prop2 = new Kernel::EnabledWhenProperty(propName2, Criteria2, value2);
  }
  ~OrEnabledWhenProperties() // responsible for deleting all supplied
                             // EnabledWhenProperites
  {
    delete Prop1;
    delete Prop2;
  }

  IPropertySettings *clone() {
    return new OrEnabledWhenProperties(propName1, Criteria1, value1, propName2,
                                       Criteria2, value2);
  }

  bool isEnabled(const IPropertyManager *algo) const {
    return Prop1->isEnabled(algo) && Prop2->isEnabled(algo);
  }

private:
  std::string propName1, propName2;
  ePropertyCriterion Criteria1, Criteria2;
  std::string value1, value2;
  Kernel::EnabledWhenProperty *Prop1, *Prop2;
};

OptimizeCrystalPlacement::OptimizeCrystalPlacement() : Algorithm() {}
OptimizeCrystalPlacement::~OptimizeCrystalPlacement() {}

void OptimizeCrystalPlacement::init() {
  declareProperty(new WorkspaceProperty<PeaksWorkspace>("PeaksWorkspace", "",
                                                        Direction::Input),
                  "Workspace of Peaks with UB loaded");
  declareProperty(new ArrayProperty<int>(std::string("KeepGoniometerFixedfor"),
                                         Direction::Input),
                  "List of run Numbers for which the goniometer settings will "
                  "NOT be changed");

  declareProperty(
      new WorkspaceProperty<PeaksWorkspace>("ModifiedPeaksWorkspace", "",
                                            Direction::Output),
      "Output Workspace of Peaks with optimized sample Orientations");

  declareProperty(new WorkspaceProperty<ITableWorkspace>(
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

  declareProperty("MaxIndexingError", .25, "Use only peaks whose fractional "
                                           "hkl values are below this "
                                           "tolerance(def=.25)");

  declareProperty("MaxHKLPeaks2Use", -1.0, "If less than 0 all peaks are used, "
                                           "otherwise only peaks whose h,k, "
                                           "and l values are below the level "
                                           "are used(def=-1)");
  declareProperty("MaxSamplePositionChangeMeters", .0005,
                  "Maximum Change in Sample position in meters(def=.0005)");

  setPropertyGroup("MaxAngularChange", "Tolerance settings");

  setPropertyGroup("MaxSamplePositionChangeMeters", "Tolerance settings");
  setPropertyGroup("MaxHKLPeaks2Use", "Tolerance settings");
  setPropertyGroup("MaxIndexingError", "Tolerance settings");

  setPropertySettings(
      "MaxSamplePositionChangeMeters",
      new EnabledWhenProperty("AdjustSampleOffsets", Kernel::IS_EQUAL_TO, "1"));

  setPropertySettings("KeepGoniometerFixedfor",
                      new OrEnabledWhenProperties(
                          "AdjustSampleOffsets", Kernel::IS_EQUAL_TO, "0",
                          "OptimizeGoniometerTilt", Kernel::IS_EQUAL_TO, "0"));
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
  PeaksWorkspace_sptr Peaks = getProperty("PeaksWorkspace");
  PeaksWorkspace_sptr OutPeaks = getProperty("ModifiedPeaksWorkspace");

  if (Peaks != OutPeaks) {
    boost::shared_ptr<PeaksWorkspace> X(Peaks->clone());
    OutPeaks = X;
  }

  std::vector<int> NOoptimizeRuns = getProperty("KeepGoniometerFixedfor");

  const DblMatrix X = Peaks->sample().getOrientedLattice().getUB();
  Matrix<double> UBinv(X);
  UBinv.Invert();

  //--------------------------------- Set up data for call to PeakHKLErrors
  // fitting function  ----------
  //              ---- Setting up workspace supplied to PeakHKLErrors
  //              ---------------
  std::vector<int> RunNumList;
  std::vector<V3D> ChiPhiOmega;
  Mantid::MantidVecPtr pX;
  Mantid::MantidVec &xRef = pX.access();
  Mantid::MantidVecPtr yvals;
  Mantid::MantidVecPtr errs;
  Mantid::MantidVec &yvalB = yvals.access();
  Mantid::MantidVec &errB = errs.access();

  int nPeaksUsed = 0;
  double HKLintOffsetMax = getProperty("MaxIndexingError");
  double HKLMax = getProperty("MaxHKLPeaks2Use");
  for (int i = 0; i < Peaks->getNumberPeaks(); i++) {
    IPeak &peak = Peaks->getPeak(i);
    int runNum = peak.getRunNumber();
    std::vector<int>::iterator it = RunNumList.begin();
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
      ChiPhiOmega.push_back(
          V3D(phichiOmega[1], phichiOmega[2], phichiOmega[0]));
    }

    if (use) // add to lists for workspace
    {
      nPeaksUsed++;
      xRef.push_back((double)i);
      yvalB.push_back(0.0);
      errB.push_back(1.0);
      xRef.push_back((double)i);
      yvalB.push_back(0.0);
      errB.push_back(1.0);
      xRef.push_back((double)i);
      yvalB.push_back(0.0);
      errB.push_back(1.0);
    }
  }

  g_log.notice() << "Number initially indexed = " << nPeaksUsed
                 << " at tolerance = " << HKLintOffsetMax << std::endl;
  MatrixWorkspace_sptr mwkspc;

  if (nPeaksUsed < 1) {
    g_log.error() << "Error in UB too large. 0 peaks indexed at "
                  << HKLintOffsetMax << std::endl;
    throw std::invalid_argument("Error in UB too large. 0 peaks indexed ");
  }

  int N = 3 * nPeaksUsed; // Peaks->getNumberPeaks();
  mwkspc = WorkspaceFactory::Instance().create("Workspace2D", (size_t)1, N, N);
  mwkspc->setX(0, pX);
  mwkspc->setData(0, yvals, errs);

  std::string FuncArg = "name=PeakHKLErrors,PeakWorkspaceName=" +
                        getPropertyValue("PeaksWorkspace") + "";

  std::string OptRunNums;

  //--------- Setting Function and Constraint argumens to PeakHKLErrors
  //---------------
  std::vector<std::string> ChRunNumList;
  std::string predChar = "";
  for (std::vector<int>::iterator it = RunNumList.begin();
       it != RunNumList.end(); ++it) {
    int runNum = *it;

    std::vector<int>::iterator it1 = NOoptimizeRuns.begin();
    for (; it1 != NOoptimizeRuns.end() && *it1 != runNum; ++it1) {
    }

    if (it1 == NOoptimizeRuns.end()) {
      std::string runNumStr = boost::lexical_cast<std::string>(runNum);
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
  if (OptRunNums.size() > 0 && !omitRuns)
    FuncArg += ",OptRuns=" + OptRunNums;

  //------------- Add initial parameter values to FuncArg -----------

  std::ostringstream oss(std::ostringstream::out);
  oss.precision(3);
  std::ostringstream oss1(std::ostringstream::out); // constraints
  oss1.precision(3);

  int nParams = 3;
  double DegreeTol = getProperty("MaxAngularChange");
  std::string startConstraint = "";

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

  Instrument_const_sptr instr = Peaks->getPeak(0).getInstrument();
  V3D sampPos = instr->getSample()->getPos();

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

  g_log.debug() << "Function argument=" << FuncArg << std::endl;
  g_log.debug() << "Constraint argument=" << Constr << std::endl;

  //--------------------- set up Fit algorithm call-----------------

  boost::shared_ptr<Algorithm> fit_alg =
      createChildAlgorithm("Fit", .1, .93, true);

  fit_alg->setProperty("Function", FuncArg);

  fit_alg->setProperty("MaxIterations", 60);

  fit_alg->setProperty("Constraints", Constr);

  fit_alg->setProperty("InputWorkspace", mwkspc);

  fit_alg->setProperty("CreateOutput", true);

  std::string Ties = "";

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
  std::cout << "Fit finished. Status="
            << (std::string)fit_alg->getProperty("OutputStatus") << std::endl;

  setProperty("Chi2overDoF", chisq);

  setProperty("nPeaks", nPeaksUsed);
  setProperty("nParams", nParams);

  g_log.debug() << "Chi2overDof=" << chisq << "    # Peaks used=" << nPeaksUsed
                << "# fitting parameters =" << nParams
                << "   dof=" << (nPeaksUsed - nParams) << std::endl;

  ITableWorkspace_sptr RRes = fit_alg->getProperty("OutputParameters");

  double sigma = sqrt(chisq);

  std::string OutputStatus = fit_alg->getProperty("OutputStatus");
  g_log.notice() << "Output Status=" << OutputStatus << std::endl;

  //------------------ Fix up Covariance output --------------------
  declareProperty(new WorkspaceProperty<ITableWorkspace>(
                      "OutputNormalisedCovarianceMatrixOptX", "CovarianceInfo",
                      Direction::Output),
                  "The name of the TableWorkspace in which to store the final "
                  "covariance matrix");

  ITableWorkspace_sptr NormCov =
      fit_alg->getProperty("OutputNormalisedCovarianceMatrix");
  setProperty("OutputNormalisedCovarianceMatrixOptX",
              NormCov); // What if 2 instances are run

  if (chisq < 0 || chisq != chisq)
    sigma = -1;

  //------------- Fix up Result workspace values ----------------------------
  std::map<std::string, double> Results;
  for (int prm = 0; prm < (int)RRes->rowCount(); ++prm) {
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

  IPeak &peak = Peaks->getPeak(0);
  boost::shared_ptr<const Instrument> OldInstrument = peak.getInstrument();
  boost::shared_ptr<const ParameterMap> pmap_old =
      OldInstrument->getParameterMap();
  boost::shared_ptr<ParameterMap> pmap_new(new ParameterMap());

  PeakHKLErrors::cLone(pmap_new, OldInstrument, pmap_old);

  double L0 = peak.getL1();
  V3D oldSampPos = OldInstrument->getSample()->getPos();
  V3D newSampPos(Results["SampleXOffset"], Results["SampleYOffset"],
                 Results["SampleZOffset"]);

  boost::shared_ptr<const Instrument> Inst = OldInstrument;

  if (OldInstrument->isParametrized())
    Inst = OldInstrument->baseInstrument();

  boost::shared_ptr<const Instrument> NewInstrument(
      new Geometry::Instrument(Inst, pmap_new));

  SCDCalibratePanels::FixUpSourceParameterMap(NewInstrument, L0, newSampPos,
                                              pmap_old);

  for (int i = 0; i < OutPeaks->getNumberPeaks(); i++)
    OutPeaks->getPeak(i).setInstrument(NewInstrument);

  OutPeaks->setInstrument(NewInstrument);

  Matrix<double> GonTilt =
      PeakHKLErrors::RotationMatrixAboutRegAxis(Results["GonRotx"], 'x') *
      PeakHKLErrors::RotationMatrixAboutRegAxis(Results["GonRoty"], 'y') *
      PeakHKLErrors::RotationMatrixAboutRegAxis(Results["GonRotz"], 'z');

  int prevRunNum = -1;
  std::map<int, Matrix<double>> MapRunNum2GonMat;
  std::string OptRun2 = "/" + OptRunNums + "/";

  int nIndexed = 0;
  UBinv = OutPeaks->sample().getOrientedLattice().getUB();
  UBinv.Invert();
  UBinv /= (2 * M_PI);
  for (int i = 0; i < OutPeaks->getNumberPeaks(); ++i) {

    int RunNum = OutPeaks->getPeak(i).getRunNumber();
    std::string RunNumStr = boost::lexical_cast<std::string>(RunNum);
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
      GonMatrix = GonTilt * OutPeaks->getPeak(i).getGoniometerMatrix();
      MapRunNum2GonMat[RunNum] = GonMatrix;
    }

    OutPeaks->getPeak(i).setGoniometerMatrix(GonMatrix);
    V3D hkl = UBinv * OutPeaks->getPeak(i).getQSampleFrame();
    if (Geometry::IndexingUtils::ValidIndex(hkl, HKLintOffsetMax))
      nIndexed++;

    prevRunNum = RunNum;
  }

  if (MapRunNum2GonMat.size() == 1) // Only one RunNumber in this PeaksWorkspace
  {
    Matrix<double> GonMatrix =
        MapRunNum2GonMat[OutPeaks->getPeak(0).getRunNumber()];
    Geometry::Goniometer Gon(GonMatrix);
    OutPeaks->mutableRun().setGoniometer(Gon, false);
  }

  setProperty("ModifiedPeaksWorkspace", OutPeaks);
  setProperty("nIndexed", nIndexed);
  g_log.notice() << "Number indexed after optimization= " << nIndexed
                 << " at tolerance = " << HKLintOffsetMax << std::endl;

} // exec

} // namespace Crystal

} // namespace Mantid
