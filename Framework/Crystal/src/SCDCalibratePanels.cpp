// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/SCDCalibratePanels.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ConstraintFactory.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidCrystal/CalibrationHelpers.h"
#include "MantidCrystal/SCDPanelErrors.h"
#include "MantidCrystal/SelectCellWithForm.h"
#include "MantidDataObjects/Peak.h"
#include "MantidGeometry/Crystal/EdgePixel.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Crystal/ReducedCell.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include <Poco/File.h>
#include <boost/container/flat_set.hpp>
#include <boost/math/special_functions/round.hpp>
#include <fstream>
#include <sstream>

using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace std;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace Mantid::Crystal {

DECLARE_ALGORITHM(SCDCalibratePanels)

const std::string SCDCalibratePanels::name() const { return "SCDCalibratePanels"; }

int SCDCalibratePanels::version() const { return 1; }

const std::string SCDCalibratePanels::category() const { return "Crystal\\Corrections"; }

void SCDCalibratePanels::exec() {
  PeaksWorkspace_sptr peaksWs = getProperty("PeakWorkspace");
  // We must sort the peaks
  std::vector<std::pair<std::string, bool>> criteria{{"BankName", true}};
  peaksWs->sort(criteria);
  // Remove peaks on edge
  int edge = this->getProperty("EdgePixels");
  Geometry::Instrument_const_sptr inst = peaksWs->getInstrument();
  if (edge > 0) {
    std::vector<Peak> &peaks = peaksWs->getPeaks();
    auto it = std::remove_if(peaks.begin(), peaks.end(), [edge, inst](const Peak &pk) {
      return edgePixel(inst, pk.getBankName(), pk.getCol(), pk.getRow(), edge);
    });
    peaks.erase(it, peaks.end());
  }
  findU(peaksWs);

  auto nPeaks = static_cast<int>(peaksWs->getNumberPeaks());
  bool changeL1 = getProperty("ChangeL1");
  bool changeT0 = getProperty("ChangeT0");
  bool bankPanels = getProperty("CalibrateBanks");
  bool snapPanels = getProperty("CalibrateSNAPPanels");

  if (changeT0)
    findT0(nPeaks, peaksWs);
  if (changeL1)
    findL1(nPeaks, peaksWs);

  boost::container::flat_set<string> MyBankNames;
  boost::container::flat_set<string> MyPanels;
  if (snapPanels) {
    MyPanels.insert("East");
    MyPanels.insert("West");
    int maxRecurseDepth = 4;

    PRAGMA_OMP(parallel for schedule(dynamic, 1) )
    for (int num = 1; num < 64; ++num) {
      PARALLEL_START_INTERRUPT_REGION
      std::ostringstream mess;
      mess << "bank" << num;
      IComponent_const_sptr comp = inst->getComponentByName(mess.str(), maxRecurseDepth);
      PARALLEL_CRITICAL(MyBankNames)
      if (comp)
        MyBankNames.insert(mess.str());
      PARALLEL_END_INTERRUPT_REGION
    }
    PARALLEL_CHECK_INTERRUPT_REGION
  } else {
    for (int i = 0; i < nPeaks; ++i) {
      std::string bankName = peaksWs->getPeak(i).getBankName();
      if (bankName != "None")
        MyBankNames.insert(bankName);
    }
  }

  std::vector<std::string> fit_workspaces(MyBankNames.size() + MyPanels.size(), "fit_");
  std::vector<std::string> parameter_workspaces(MyBankNames.size() + MyPanels.size(), "params_");
  int bankAndPanelCount = 0;
  for (auto &MyPanel : MyPanels) {
    fit_workspaces[bankAndPanelCount] += MyPanel;
    parameter_workspaces[bankAndPanelCount] += MyPanel;
    bankAndPanelCount++;
  }
  if (snapPanels) {
    findL2(MyPanels, peaksWs);
    ITableWorkspace_sptr results = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("params_West");
    double delta = results->cell<double>(4, 1);
    g_log.notice() << "For west rotation change det_arc1 " << delta << " degrees\n";
    results = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("params_East");
    delta = results->cell<double>(4, 1);
    g_log.notice() << "For east rotation change det_arc2 " << delta << " degrees\n";
  }

  for (auto &MyBankName : MyBankNames) {
    fit_workspaces[bankAndPanelCount] += MyBankName;
    parameter_workspaces[bankAndPanelCount] += MyBankName;
    bankAndPanelCount++;
  }
  if (bankPanels) {
    findL2(MyBankNames, peaksWs);
  }

  // remove skipped banks
  for (int j = bankAndPanelCount - 1; j >= 0; j--) {
    if (!AnalysisDataService::Instance().doesExist(fit_workspaces[j]))
      fit_workspaces.erase(fit_workspaces.begin() + j);
    if (!AnalysisDataService::Instance().doesExist(parameter_workspaces[j]))
      parameter_workspaces.erase(parameter_workspaces.begin() + j);
  }

  // Try again to optimize L1
  if (changeL1) {
    findL1(nPeaks, peaksWs);
    parameter_workspaces.emplace_back("params_L1");
    fit_workspaces.emplace_back("fit_L1");
  }
  // Add T0 files to groups
  if (changeT0) {
    parameter_workspaces.emplace_back("params_T0");
    fit_workspaces.emplace_back("fit_T0");
  }
  std::sort(parameter_workspaces.begin(), parameter_workspaces.end());
  std::sort(fit_workspaces.begin(), fit_workspaces.end());

  // collect output of fit for each spectrum into workspace groups
  auto groupAlg = AlgorithmManager::Instance().createUnmanaged("GroupWorkspaces");
  groupAlg->initialize();
  groupAlg->setProperty("InputWorkspaces", parameter_workspaces);
  groupAlg->setProperty("OutputWorkspace", "Fit_Parameters");
  groupAlg->execute();

  groupAlg = AlgorithmManager::Instance().createUnmanaged("GroupWorkspaces");
  groupAlg->initialize();
  groupAlg->setProperty("InputWorkspaces", fit_workspaces);
  groupAlg->setProperty("OutputWorkspace", "Fit_Residuals");
  groupAlg->execute();

  // Use new instrument for PeaksWorkspace
  Geometry::Instrument_sptr inst2 = std::const_pointer_cast<Geometry::Instrument>(peaksWs->getInstrument());
  Geometry::OrientedLattice lattice0 = peaksWs->mutableSample().getOrientedLattice();
  PARALLEL_FOR_IF(Kernel::threadSafe(*peaksWs))
  for (int i = 0; i < nPeaks; i++) {
    PARALLEL_START_INTERRUPT_REGION
    DataObjects::Peak &peak = peaksWs->getPeak(i);
    try {
      peak.setInstrument(inst2);
    } catch (const std::exception &exc) {
      g_log.notice() << "Problem in applying calibration to peak " << i << " : " << exc.what() << "\n";
    }
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  // Find U again for optimized geometry and index peaks
  findU(peaksWs);
  // Save as DetCal and XML if requested
  string DetCalFileName = getProperty("DetCalFilename");
  API::Run &run = peaksWs->mutableRun();
  double mT0 = 0.0;
  if (run.hasProperty("T0")) {
    mT0 = run.getPropertyValueAsType<double>("T0");
  }
  saveIsawDetCal(inst2, MyBankNames, mT0, DetCalFileName);
  string XmlFileName = getProperty("XmlFilename");
  saveXmlFile(XmlFileName, MyBankNames, *inst2);
  // create table of theoretical vs calculated
  //----------------- Calculate & Create Calculated vs Theoretical
  // workspaces------------------,);
  MatrixWorkspace_sptr ColWksp =
      Mantid::API::WorkspaceFactory::Instance().create("Workspace2D", MyBankNames.size(), nPeaks, nPeaks);
  ColWksp->setInstrument(inst2);
  MatrixWorkspace_sptr RowWksp =
      Mantid::API::WorkspaceFactory::Instance().create("Workspace2D", MyBankNames.size(), nPeaks, nPeaks);
  RowWksp->setInstrument(inst2);
  MatrixWorkspace_sptr TofWksp =
      Mantid::API::WorkspaceFactory::Instance().create("Workspace2D", MyBankNames.size(), nPeaks, nPeaks);
  TofWksp->setInstrument(inst2);
  OrientedLattice lattice = peaksWs->mutableSample().getOrientedLattice();
  const DblMatrix &UB = lattice.getUB();
  // sort again since edge peaks can trace to other banks
  peaksWs->sort(criteria);
  PARALLEL_FOR_IF(Kernel::threadSafe(*ColWksp, *RowWksp, *TofWksp))
  for (int i = 0; i < static_cast<int>(MyBankNames.size()); ++i) {
    PARALLEL_START_INTERRUPT_REGION
    const std::string &bankName = *std::next(MyBankNames.begin(), i);
    size_t k = bankName.find_last_not_of("0123456789");
    int bank = 0;
    if (k < bankName.length())
      bank = boost::lexical_cast<int>(bankName.substr(k + 1));
    ColWksp->getSpectrum(i).setSpectrumNo(specnum_t(bank));
    RowWksp->getSpectrum(i).setSpectrumNo(specnum_t(bank));
    TofWksp->getSpectrum(i).setSpectrumNo(specnum_t(bank));
    auto &ColX = ColWksp->mutableX(i);
    auto &ColY = ColWksp->mutableY(i);
    auto &RowX = RowWksp->mutableX(i);
    auto &RowY = RowWksp->mutableY(i);
    auto &TofX = TofWksp->mutableX(i);
    auto &TofY = TofWksp->mutableY(i);
    int icount = 0;
    for (int j = 0; j < nPeaks; j++) {
      Peak peak = peaksWs->getPeak(j);
      if (peak.getBankName() == bankName) {
        try {
          V3D q_lab = (peak.getGoniometerMatrix() * UB) * peak.getHKL() * M_2_PI;
          Peak theoretical(peak.getInstrument(), q_lab);
          ColX[icount] = peak.getCol();
          ColY[icount] = theoretical.getCol();
          RowX[icount] = peak.getRow();
          RowY[icount] = theoretical.getRow();
          TofX[icount] = peak.getTOF();
          TofY[icount] = theoretical.getTOF();
        } catch (...) {
          // g_log.debug() << "Problem only in printing peaks\n";
        }
        icount++;
      }
    }
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  string colFilename = getProperty("ColFilename");
  string rowFilename = getProperty("RowFilename");
  string tofFilename = getProperty("TofFilename");
  saveNexus(colFilename, ColWksp);
  saveNexus(rowFilename, RowWksp);
  saveNexus(tofFilename, TofWksp);
}

void SCDCalibratePanels::saveNexus(const std::string &outputFile, const MatrixWorkspace_sptr &outputWS) {
  auto save = createChildAlgorithm("SaveNexus");
  save->setProperty("InputWorkspace", outputWS);
  save->setProperty("FileName", outputFile);
  save->execute();
}

void SCDCalibratePanels::findL1(int nPeaks, const DataObjects::PeaksWorkspace_sptr &peaksWs) {
  MatrixWorkspace_sptr L1WS = std::dynamic_pointer_cast<MatrixWorkspace>(
      API::WorkspaceFactory::Instance().create("Workspace2D", 1, 3 * nPeaks, 3 * nPeaks));

  IAlgorithm_sptr fitL1_alg;
  try {
    fitL1_alg = createChildAlgorithm("Fit", -1, -1, false);
  } catch (Exception::NotFoundError &) {
    g_log.error("Can't locate Fit algorithm");
    throw;
  }
  std::ostringstream fun_str;
  fun_str << "name=SCDPanelErrors,Workspace=" << peaksWs->getName() << ",Bank=moderator";
  std::ostringstream tie_str;
  tie_str << "XShift=0.0,YShift=0.0,XRotate=0.0,YRotate=0.0,ZRotate=0.0,"
             "ScaleWidth=1.0,ScaleHeight=1.0,T0Shift ="
          << mT0;
  fitL1_alg->setPropertyValue("Function", fun_str.str());
  fitL1_alg->setProperty("Ties", tie_str.str());
  fitL1_alg->setProperty("InputWorkspace", L1WS);
  fitL1_alg->setProperty("CreateOutput", true);
  fitL1_alg->setProperty("Output", "fit");
  fitL1_alg->executeAsChildAlg();
  std::string fitL1Status = fitL1_alg->getProperty("OutputStatus");
  double chisqL1 = fitL1_alg->getProperty("OutputChi2overDoF");
  MatrixWorkspace_sptr fitL1 = fitL1_alg->getProperty("OutputWorkspace");
  AnalysisDataService::Instance().addOrReplace("fit_L1", fitL1);
  ITableWorkspace_sptr paramsL1 = fitL1_alg->getProperty("OutputParameters");
  AnalysisDataService::Instance().addOrReplace("params_L1", paramsL1);
  double deltaL1 = paramsL1->getRef<double>("Value", 2);
  SCDPanelErrors com;
  com.moveDetector(0.0, 0.0, deltaL1, 0.0, 0.0, 0.0, 1.0, 1.0, "moderator", peaksWs);
  g_log.notice() << "L1 = " << -peaksWs->getInstrument()->getSource()->getPos().Z() << "  " << fitL1Status
                 << " Chi2overDoF " << chisqL1 << "\n";
}

void SCDCalibratePanels::findT0(int nPeaks, const DataObjects::PeaksWorkspace_sptr &peaksWs) {
  MatrixWorkspace_sptr T0WS = std::dynamic_pointer_cast<MatrixWorkspace>(
      API::WorkspaceFactory::Instance().create("Workspace2D", 1, 3 * nPeaks, 3 * nPeaks));

  IAlgorithm_sptr fitT0_alg;
  try {
    fitT0_alg = createChildAlgorithm("Fit", -1, -1, false);
  } catch (Exception::NotFoundError &) {
    g_log.error("Can't locate Fit algorithm");
    throw;
  }
  std::ostringstream fun_str;
  fun_str << "name=SCDPanelErrors,Workspace=" << peaksWs->getName() << ",Bank=none";
  std::ostringstream tie_str;
  tie_str << "XShift=0.0,YShift=0.0,ZShift=0.0,XRotate=0.0,YRotate=0.0,ZRotate=0.0,"
             "ScaleWidth=1.0,ScaleHeight=1.0";
  fitT0_alg->setPropertyValue("Function", fun_str.str());
  fitT0_alg->setProperty("Ties", tie_str.str());
  fitT0_alg->setProperty("InputWorkspace", T0WS);
  fitT0_alg->setProperty("CreateOutput", true);
  fitT0_alg->setProperty("Output", "fit");
  // Does not converge with derviative minimizers
  fitT0_alg->setProperty("Minimizer", "Simplex");
  fitT0_alg->setProperty("MaxIterations", 1000);
  fitT0_alg->executeAsChildAlg();
  std::string fitT0Status = fitT0_alg->getProperty("OutputStatus");
  double chisqT0 = fitT0_alg->getProperty("OutputChi2overDoF");
  MatrixWorkspace_sptr fitT0 = fitT0_alg->getProperty("OutputWorkspace");
  AnalysisDataService::Instance().addOrReplace("fit_T0", fitT0);
  ITableWorkspace_sptr paramsT0 = fitT0_alg->getProperty("OutputParameters");
  AnalysisDataService::Instance().addOrReplace("params_T0", paramsT0);
  mT0 = paramsT0->getRef<double>("Value", 8);
  API::Run &run = peaksWs->mutableRun();
  // set T0 in the run parameters adding to value in peaks file
  double oldT0 = 0.0;
  if (run.hasProperty("T0")) {
    oldT0 = run.getPropertyValueAsType<double>("T0");
  }
  run.addProperty<double>("T0", mT0 + oldT0, true);
  g_log.notice() << "T0 = " << mT0 << "  " << fitT0Status << " Chi2overDoF " << chisqT0 << "\n";
  for (int i = 0; i < peaksWs->getNumberPeaks(); i++) {
    DataObjects::Peak &peak = peaksWs->getPeak(i);

    Units::Wavelength wl;

    wl.initialize(peak.getL1(), 0, {{UnitParams::l2, peak.getL2()}, {UnitParams::twoTheta, peak.getScattering()}});
    peak.setWavelength(wl.singleFromTOF(peak.getTOF() + mT0));
  }
}

void SCDCalibratePanels::findU(const DataObjects::PeaksWorkspace_sptr &peaksWs) {
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
  if ((a == EMPTY_DBL() || b == EMPTY_DBL() || c == EMPTY_DBL() || alpha == EMPTY_DBL() || beta == EMPTY_DBL() ||
       gamma == EMPTY_DBL()) &&
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

  // Reindex peaks with new UB
  auto alg = createChildAlgorithm("IndexPeaks");
  alg->setPropertyValue("PeaksWorkspace", peaksWs->getName());
  alg->setProperty("Tolerance", 0.15);
  alg->executeAsChildAlg();
  int numIndexed = alg->getProperty("NumIndexed");
  g_log.notice() << "Number Indexed = " << numIndexed << "\n";
  g_log.notice() << peaksWs->sample().getOrientedLattice().getUB() << "\n";
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
void SCDCalibratePanels::saveIsawDetCal(std::shared_ptr<Instrument> &instrument,
                                        boost::container::flat_set<string> &AllBankName, double T0,
                                        const string &filename) {
  // having a filename triggers doing the work
  if (filename.empty())
    return;

  g_log.notice() << "Saving DetCal file in " << filename << "\n";

  // create a workspace to pass to SaveIsawDetCal
  const size_t number_spectra = instrument->getNumberDetectors();
  DataObjects::Workspace2D_sptr wksp = std::dynamic_pointer_cast<DataObjects::Workspace2D>(
      WorkspaceFactory::Instance().create("Workspace2D", number_spectra, 2, 1));
  wksp->setInstrument(instrument);
  wksp->rebuildSpectraMapping(true /* include monitors */);

  // convert the bank names into a vector
  std::vector<string> banknames(AllBankName.begin(), AllBankName.end());

  // call SaveIsawDetCal
  auto alg = createChildAlgorithm("SaveIsawDetCal");
  alg->setProperty("InputWorkspace", wksp);
  alg->setProperty("Filename", filename);
  alg->setProperty("TimeOffset", T0);
  alg->setProperty("BankNames", banknames);
  alg->executeAsChildAlg();
}

void SCDCalibratePanels::init() {
  declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>("PeakWorkspace", "", Kernel::Direction::InOut),
                  "Workspace of Indexed Peaks");

  auto mustBePositive = std::make_shared<BoundedValidator<double>>();
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
  declareProperty("ChangeL1", true, "Change the L1(source to sample) distance");
  declareProperty("ChangeT0", false, "Change the T0 (initial TOF)");
  declareProperty("ChangePanelSize", true,
                  "Change the height and width of the "
                  "detectors.  Implemented only for "
                  "RectangularDetectors.");

  declareProperty("EdgePixels", 0, "Remove peaks that are at pixels this close to edge. ");
  declareProperty("CalibrateBanks", true, "Calibrate the panels of the banks.");
  declareProperty("CalibrateSNAPPanels", false,
                  "Calibrate the 3 X 3 panels of the "
                  "sides of SNAP.");

  // ---------- outputs
  const std::vector<std::string> detcalExts{".DetCal", ".Det_Cal"};
  declareProperty(
      std::make_unique<FileProperty>("DetCalFilename", "SCDCalibrate.DetCal", FileProperty::Save, detcalExts),
      "Path to an ISAW-style .detcal file to save.");

  declareProperty(std::make_unique<FileProperty>("XmlFilename", "", FileProperty::OptionalSave, ".xml"),
                  "Path to an Mantid .xml description(for LoadParameterFile) file to "
                  "save.");

  declareProperty(std::make_unique<FileProperty>("ColFilename", "ColCalcvsTheor.nxs", FileProperty::Save, ".nxs"),
                  "Path to a NeXus file comparing calculated and theoretical "
                  "column of each peak.");

  declareProperty(std::make_unique<FileProperty>("RowFilename", "RowCalcvsTheor.nxs", FileProperty::Save, ".nxs"),
                  "Path to a NeXus file comparing calculated and theoretical "
                  "row of each peak.");

  declareProperty(std::make_unique<FileProperty>("TofFilename", "TofCalcvsTheor.nxs", FileProperty::Save, ".nxs"),
                  "Path to a NeXus file comparing calculated and theoretical "
                  "TOF of each peak.");

  const string OUTPUTS("Outputs");
  setPropertyGroup("DetCalFilename", OUTPUTS);
  setPropertyGroup("XmlFilename", OUTPUTS);
  setPropertyGroup("ColFilename", OUTPUTS);
  setPropertyGroup("RowFilename", OUTPUTS);
  setPropertyGroup("TofFilename", OUTPUTS);
}

void writeXmlParameter(ofstream &ostream, const string &name, const double value) {
  ostream << "  <parameter name =\"" << name << "\"><value val=\"" << value << "\" /> </parameter>\n";
}

void SCDCalibratePanels::saveXmlFile(const string &FileName, const boost::container::flat_set<string> &AllBankNames,
                                     const Instrument &instrument) const {
  if (FileName.empty())
    return;

  g_log.notice() << "Saving parameter file as " << FileName << "\n";

  // create the file and add the header
  ofstream oss3(FileName.c_str());
  oss3 << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
  oss3 << " <parameter-file instrument=\"" << instrument.getName() << "\" valid-from=\""
       << instrument.getValidFromDate().toISO8601String() << "\">\n";
  ParameterMap_sptr pmap = instrument.getParameterMap();

  // write out the detector banks
  for (auto bankName : AllBankNames) {
    if (instrument.getName().compare("CORELLI") == 0.0)
      bankName.append("/sixteenpack");
    oss3 << "<component-link name=\"" << bankName << "\">\n";
    std::shared_ptr<const IComponent> bank = instrument.getComponentByName(bankName);

    Quat relRot = bank->getRelativeRot();

    std::vector<double> relRotAngles = relRot.getEulerAngles("XYZ");

    writeXmlParameter(oss3, "rotx", relRotAngles[0]);
    writeXmlParameter(oss3, "roty", relRotAngles[1]);
    writeXmlParameter(oss3, "rotz", relRotAngles[2]);

    V3D pos1 = bank->getRelativePos();
    writeXmlParameter(oss3, "x", pos1.X());
    writeXmlParameter(oss3, "y", pos1.Y());
    writeXmlParameter(oss3, "z", pos1.Z());

    vector<double> oldScalex = pmap->getDouble(bank->getName(), string("scalex"));
    vector<double> oldScaley = pmap->getDouble(bank->getName(), string("scaley"));

    double scalex, scaley;
    if (!oldScalex.empty())
      scalex = oldScalex[0];
    else
      scalex = 1.;

    if (!oldScaley.empty())
      scaley = oldScaley[0];
    else
      scaley = 1.;

    oss3 << R"(  <parameter name ="scalex"><value val=")" << scalex << "\" /> </parameter>\n";
    oss3 << R"(  <parameter name ="scaley"><value val=")" << scaley << "\" /> </parameter>\n";
    oss3 << "</component-link>\n";
  } // for each bank in the group

  // write out the source
  IComponent_const_sptr source = instrument.getSource();

  oss3 << "<component-link name=\"" << source->getName() << "\">\n";
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
void SCDCalibratePanels::findL2(boost::container::flat_set<string> MyBankNames,
                                const DataObjects::PeaksWorkspace_sptr &peaksWs) {
  bool changeSize = getProperty("ChangePanelSize");
  Geometry::Instrument_const_sptr inst = peaksWs->getInstrument();

  PARALLEL_FOR_IF(Kernel::threadSafe(*peaksWs))
  for (int bankIndex = 0; bankIndex < static_cast<int>(MyBankNames.size()); ++bankIndex) {
    PARALLEL_START_INTERRUPT_REGION
    const std::string &iBank = *std::next(MyBankNames.begin(), bankIndex);
    const std::string bankName = "__PWS_" + iBank;
    PeaksWorkspace_sptr local = peaksWs->clone();
    AnalysisDataService::Instance().addOrReplace(bankName, local);
    std::vector<Peak> &localPeaks = local->getPeaks();
    auto lit = std::remove_if(localPeaks.begin(), localPeaks.end(), [&iBank](const Peak &pk) {
      std::string name = pk.getBankName();
      IComponent_const_sptr det = pk.getInstrument()->getComponentByName(name);
      if (det && iBank.substr(0, 4) != "bank") {
        IComponent_const_sptr parent = det->getParent();
        if (parent) {
          IComponent_const_sptr grandparent = parent->getParent();
          if (grandparent) {
            name = grandparent->getName();
          }
        }
      }

      return name != iBank;
    });
    localPeaks.erase(lit, localPeaks.end());

    int nBankPeaks = local->getNumberPeaks();
    if (nBankPeaks < 6) {
      g_log.notice() << "Too few peaks for " << iBank << "\n";
      continue;
    }

    MatrixWorkspace_sptr q3DWS = std::dynamic_pointer_cast<MatrixWorkspace>(
        API::WorkspaceFactory::Instance().create("Workspace2D", 1, 3 * nBankPeaks, 3 * nBankPeaks));

    auto &outSpec = q3DWS->getSpectrum(0);
    auto &yVec = outSpec.mutableY();
    auto &eVec = outSpec.mutableE();
    auto &xVec = outSpec.mutableX();
    yVec = 0.0;

    for (int i = 0; i < nBankPeaks; i++) {
      const DataObjects::Peak &peak = local->getPeak(i);
      // 1/sigma is considered the weight for the fit
      double weight = 1.;                // default is even weighting
      if (peak.getSigmaIntensity() > 0.) // prefer weight by sigmaI
        weight = 1.0 / peak.getSigmaIntensity();
      else if (peak.getIntensity() > 0.) // next favorite weight by I
        weight = 1.0 / peak.getIntensity();
      else if (peak.getBinCount() > 0.) // then by counts in peak centre
        weight = 1.0 / peak.getBinCount();
      for (int j = 0; j < 3; j++) {
        int k = i * 3 + j;
        xVec[k] = k;
        eVec[k] = weight;
      }
    }

    IAlgorithm_sptr fit_alg;
    try {
      fit_alg = createChildAlgorithm("Fit", -1, -1, false);
    } catch (Exception::NotFoundError &) {
      g_log.error("Can't locate Fit algorithm");
      throw;
    }
    std::ostringstream fun_str;
    fun_str << "name=SCDPanelErrors,Workspace=" + bankName << ",Bank=" << iBank;
    fit_alg->setPropertyValue("Function", fun_str.str());
    std::ostringstream tie_str;
    tie_str << "ScaleWidth=1.0,ScaleHeight=1.0,T0Shift =" << mT0;
    fit_alg->setProperty("Ties", tie_str.str());
    fit_alg->setProperty("InputWorkspace", q3DWS);
    fit_alg->setProperty("CreateOutput", true);
    fit_alg->setProperty("Output", "fit");
    fit_alg->executeAsChildAlg();
    std::string fitStatus = fit_alg->getProperty("OutputStatus");
    double fitChisq = fit_alg->getProperty("OutputChi2overDoF");
    g_log.notice() << iBank << "  " << fitStatus << " Chi2overDoF " << fitChisq << "\n";
    MatrixWorkspace_sptr fitWS = fit_alg->getProperty("OutputWorkspace");
    AnalysisDataService::Instance().addOrReplace("fit_" + iBank, fitWS);
    ITableWorkspace_sptr paramsWS = fit_alg->getProperty("OutputParameters");
    AnalysisDataService::Instance().addOrReplace("params_" + iBank, paramsWS);
    double xShift = paramsWS->getRef<double>("Value", 0);
    double yShift = paramsWS->getRef<double>("Value", 1);
    double zShift = paramsWS->getRef<double>("Value", 2);
    double xRotate = paramsWS->getRef<double>("Value", 3);
    double yRotate = paramsWS->getRef<double>("Value", 4);
    double zRotate = paramsWS->getRef<double>("Value", 5);
    double scaleWidth = 1.0;
    double scaleHeight = 1.0;
    // Scaling only implemented for Rectangular Detectors
    Geometry::IComponent_const_sptr comp = peaksWs->getInstrument()->getComponentByName(iBank);
    std::shared_ptr<const Geometry::RectangularDetector> rectDet =
        std::dynamic_pointer_cast<const Geometry::RectangularDetector>(comp);
    if (rectDet && changeSize) {
      IAlgorithm_sptr fit2_alg;
      try {
        fit2_alg = createChildAlgorithm("Fit", -1, -1, false);
      } catch (Exception::NotFoundError &) {
        g_log.error("Can't locate Fit algorithm");
        throw;
      }
      fit2_alg->setPropertyValue("Function", fun_str.str());
      std::ostringstream tie_str2;
      tie_str2 << "XShift=" << xShift << ",YShift=" << yShift << ",ZShift=" << zShift << ",XRotate=" << xRotate
               << ",YRotate=" << yRotate << ",ZRotate=" << zRotate << ",T0Shift =" << mT0;
      fit2_alg->setProperty("Ties", tie_str2.str());
      fit2_alg->setProperty("InputWorkspace", q3DWS);
      fit2_alg->setProperty("CreateOutput", true);
      fit2_alg->setProperty("Output", "fit");
      fit2_alg->executeAsChildAlg();
      std::string fit2Status = fit2_alg->getProperty("OutputStatus");
      double fit2Chisq = fit2_alg->getProperty("OutputChi2overDoF");
      g_log.notice() << iBank << "  " << fit2Status << " Chi2overDoF " << fit2Chisq << "\n";
      fitWS = fit2_alg->getProperty("OutputWorkspace");
      AnalysisDataService::Instance().addOrReplace("fit_" + iBank, fitWS);
      paramsWS = fit2_alg->getProperty("OutputParameters");
      AnalysisDataService::Instance().addOrReplace("params_" + iBank, paramsWS);
      scaleWidth = paramsWS->getRef<double>("Value", 6);
      scaleHeight = paramsWS->getRef<double>("Value", 7);
    }
    AnalysisDataService::Instance().remove(bankName);
    SCDPanelErrors det;
    det.moveDetector(xShift, yShift, zShift, xRotate, yRotate, zRotate, scaleWidth, scaleHeight, iBank, peaksWs);
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION
}
} // namespace Mantid::Crystal
