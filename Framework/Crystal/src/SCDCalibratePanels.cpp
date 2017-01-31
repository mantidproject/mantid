#include "MantidCrystal/SCDCalibratePanels.h"
#include "MantidAPI/AnalysisDataService.h"
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
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Sample.h"
#include <fstream>
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Crystal/ReducedCell.h"
#include <boost/math/special_functions/round.hpp>
#include <boost/container/flat_set.hpp>
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

//-----------------------------------------------------------------------------------------
/**
  @param  ws           Name of workspace containing peaks
  @param  bankName     Name of bank containing peak
  @param  col          Column number containing peak
  @param  row          Row number containing peak
  @param  Edge         Number of edge points for each bank
  @return True if peak is on edge
*/
bool SCDCalibratePanels::edgePixel(const PeaksWorkspace &ws,
                                   const std::string &bankName, int col,
                                   int row, int Edge) {
  if (bankName.compare("None") == 0)
    return false;
  auto Iptr = ws.getInstrument();
  auto parent = Iptr->getComponentByName(bankName);
  if (parent->type().compare("RectangularDetector") == 0) {
    auto RDet = boost::dynamic_pointer_cast<const RectangularDetector>(parent);
    return col < Edge || col >= (RDet->xpixels() - Edge) || row < Edge ||
           row >= (RDet->ypixels() - Edge);
  } else {
    std::vector<Geometry::IComponent_const_sptr> children;
    auto asmb =
        boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(parent);
    asmb->getChildren(children, false);
    int startI = 1;
    if (children[0]->getName() == "sixteenpack") {
      startI = 0;
      parent = children[0];
      children.clear();
      auto asmb =
          boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(parent);
      asmb->getChildren(children, false);
    }
    auto asmb2 =
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
  std::vector<std::pair<std::string, bool>> criteria{{"BankName", true}};
  peaksWs->sort(criteria);
  // Remove peaks on edge
  int edge = this->getProperty("EdgePixels");
  if (edge > 0) {
    std::vector<Peak> &peaks = peaksWs->getPeaks();
    auto it = std::remove_if(
        peaks.begin(), peaks.end(), [&peaksWs, edge, this](const Peak &pk) {
          return this->edgePixel(*peaksWs, pk.getBankName(), pk.getCol(),
                                 pk.getRow(), edge);
        });
    peaks.erase(it, peaks.end());
  }
  findU(peaksWs);

  std::vector<Peak> &peaks = peaksWs->getPeaks();
  auto it = std::remove_if(peaks.begin(), peaks.end(), [](const Peak &pk) {
    return pk.getHKL() == V3D(0, 0, 0);
  });
  peaks.erase(it, peaks.end());

  int nPeaks = static_cast<int>(peaksWs->getNumberPeaks());
  bool changeL1 = getProperty("ChangeL1");
  bool changeSize = getProperty("ChangePanelSize");

  if (changeL1)
    findL1(nPeaks, peaksWs);
  boost::container::flat_set<string> MyBankNames;
  for (int i = 0; i < nPeaks; ++i) {
    MyBankNames.insert(peaksWs->getPeak(i).getBankName());
  }

  std::vector<std::string> fit_workspaces(MyBankNames.size(), "fit_");
  std::vector<std::string> parameter_workspaces(MyBankNames.size(), "params_");

  PARALLEL_FOR_IF(Kernel::threadSafe(*peaksWs))
  for (int i = 0; i < static_cast<int>(MyBankNames.size()); ++i) {
    PARALLEL_START_INTERUPT_REGION
    const std::string &iBank = *std::next(MyBankNames.begin(), i);
    const std::string bankName = "__PWS_" + iBank;
    PeaksWorkspace_sptr local = peaksWs->clone();
    AnalysisDataService::Instance().addOrReplace(bankName, local);
    std::vector<Peak> &localPeaks = local->getPeaks();
    auto lit = std::remove_if(
        localPeaks.begin(), localPeaks.end(),
        [&iBank](const Peak &pk) { return pk.getBankName() != iBank; });
    localPeaks.erase(lit, localPeaks.end());

    int nBankPeaks = local->getNumberPeaks();
    if (nBankPeaks < 6) {
      g_log.notice() << "Too few peaks for " << iBank << "\n";
      continue;
    }

    MatrixWorkspace_sptr q3DWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
        API::WorkspaceFactory::Instance().create(
            "Workspace2D", 1, 3 * nBankPeaks, 3 * nBankPeaks));

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
    tie_str << "ScaleWidth=1.0,ScaleHeight=1.0";
    fit_alg->setProperty("Ties", tie_str.str());
    fit_alg->setProperty("InputWorkspace", q3DWS);
    fit_alg->setProperty("CreateOutput", true);
    fit_alg->setProperty("Output", "fit");
    fit_alg->executeAsChildAlg();
    std::string fitStatus = fit_alg->getProperty("OutputStatus");
    double chisq = fit_alg->getProperty("OutputChi2overDoF");
    g_log.notice() << iBank << "  " << fitStatus << " Chi2overDoF " << chisq
                   << "\n";
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
    Geometry::IComponent_const_sptr comp =
        peaksWs->getInstrument()->getComponentByName(iBank);
    boost::shared_ptr<const Geometry::RectangularDetector> rectDet =
        boost::dynamic_pointer_cast<const Geometry::RectangularDetector>(comp);
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
      tie_str2 << "XShift=" << xShift << ",YShift=" << yShift
               << ",ZShift=" << zShift << ",XRotate=" << xRotate
               << ",YRotate=" << yRotate << ",ZRotate=" << zRotate;
      fit2_alg->setProperty("Ties", tie_str2.str());
      fit2_alg->setProperty("InputWorkspace", q3DWS);
      fit2_alg->setProperty("CreateOutput", true);
      fit2_alg->setProperty("Output", "fit");
      fit2_alg->executeAsChildAlg();
      std::string fitStatus = fit2_alg->getProperty("OutputStatus");
      double chisq = fit2_alg->getProperty("OutputChi2overDoF");
      g_log.notice() << iBank << "  " << fitStatus << " Chi2overDoF " << chisq
                     << "\n";
      fitWS = fit2_alg->getProperty("OutputWorkspace");
      AnalysisDataService::Instance().addOrReplace("fit_" + iBank, fitWS);
      paramsWS = fit2_alg->getProperty("OutputParameters");
      AnalysisDataService::Instance().addOrReplace("params_" + iBank, paramsWS);
      scaleWidth = paramsWS->getRef<double>("Value", 6);
      scaleHeight = paramsWS->getRef<double>("Value", 7);
    }
    AnalysisDataService::Instance().remove(bankName);
    SCDPanelErrors det;
    det.moveDetector(xShift, yShift, zShift, xRotate, yRotate, zRotate,
                     scaleWidth, scaleHeight, iBank, peaksWs);
    parameter_workspaces[i] += iBank;
    fit_workspaces[i] += iBank;
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // remove skipped banks
  fit_workspaces.erase(
      std::remove(fit_workspaces.begin(), fit_workspaces.end(), "fit_"),
      fit_workspaces.end());
  parameter_workspaces.erase(std::remove(parameter_workspaces.begin(),
                                         parameter_workspaces.end(), "params_"),
                             parameter_workspaces.end());

  // Try again to optimize L1
  if (changeL1)
    findL1(nPeaks, peaksWs);
  parameter_workspaces.push_back("params_L1");
  fit_workspaces.push_back("fit_L1");
  std::sort(parameter_workspaces.begin(), parameter_workspaces.end());
  std::sort(fit_workspaces.begin(), fit_workspaces.end());

  // collect output of fit for each spectrum into workspace groups
  API::IAlgorithm_sptr groupAlg =
      AlgorithmManager::Instance().createUnmanaged("GroupWorkspaces");
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
  Geometry::Instrument_sptr inst =
      boost::const_pointer_cast<Geometry::Instrument>(peaksWs->getInstrument());
  Geometry::OrientedLattice lattice0 =
      peaksWs->mutableSample().getOrientedLattice();
  PARALLEL_FOR_IF(Kernel::threadSafe(*peaksWs))
  for (int i = 0; i < nPeaks; i++) {
    PARALLEL_START_INTERUPT_REGION
    DataObjects::Peak &peak = peaksWs->getPeak(i);
    V3D hkl =
        V3D(boost::math::iround(peak.getH()), boost::math::iround(peak.getK()),
            boost::math::iround(peak.getL()));
    V3D Q2 = lattice0.qFromHKL(hkl);
    peak.setInstrument(inst);
    peak.setQSampleFrame(Q2);
    peak.setHKL(hkl);
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  // Find U again for optimized geometry and index peaks
  findU(peaksWs);
  // Save as DetCal and XML if requested
  string DetCalFileName = getProperty("DetCalFilename");
  saveIsawDetCal(inst, MyBankNames, 0.0, DetCalFileName);
  string XmlFileName = getProperty("XmlFilename");
  saveXmlFile(XmlFileName, MyBankNames, *inst);
  // create table of theoretical vs calculated
  //----------------- Calculate & Create Calculated vs Theoretical
  // workspaces------------------,);
  MatrixWorkspace_sptr ColWksp =
      Mantid::API::WorkspaceFactory::Instance().create(
          "Workspace2D", MyBankNames.size(), nPeaks, nPeaks);
  ColWksp->setInstrument(inst);
  MatrixWorkspace_sptr RowWksp =
      Mantid::API::WorkspaceFactory::Instance().create(
          "Workspace2D", MyBankNames.size(), nPeaks, nPeaks);
  RowWksp->setInstrument(inst);
  MatrixWorkspace_sptr TofWksp =
      Mantid::API::WorkspaceFactory::Instance().create(
          "Workspace2D", MyBankNames.size(), nPeaks, nPeaks);
  TofWksp->setInstrument(inst);
  OrientedLattice lattice = peaksWs->mutableSample().getOrientedLattice();
  const DblMatrix &UB = lattice.getUB();
  // sort again since edge peaks can trace to other banks
  peaksWs->sort(criteria);
  PARALLEL_FOR_IF(Kernel::threadSafe(*ColWksp, *RowWksp, *TofWksp))
  for (int i = 0; i < static_cast<int>(MyBankNames.size()); ++i) {
    PARALLEL_START_INTERUPT_REGION
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
          V3D q_lab =
              (peak.getGoniometerMatrix() * UB) * peak.getHKL() * M_2_PI;
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
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  string colFilename = getProperty("ColFilename");
  string rowFilename = getProperty("RowFilename");
  string tofFilename = getProperty("TofFilename");
  saveNexus(colFilename, ColWksp);
  saveNexus(rowFilename, RowWksp);
  saveNexus(tofFilename, TofWksp);
}

void SCDCalibratePanels::saveNexus(std::string outputFile,
                                   MatrixWorkspace_sptr outputWS) {
  IAlgorithm_sptr save = this->createChildAlgorithm("SaveNexus");
  save->setProperty("InputWorkspace", outputWS);
  save->setProperty("FileName", outputFile);
  save->execute();
}

void SCDCalibratePanels::findL1(int nPeaks,
                                DataObjects::PeaksWorkspace_sptr peaksWs) {
  MatrixWorkspace_sptr L1WS = boost::dynamic_pointer_cast<MatrixWorkspace>(
      API::WorkspaceFactory::Instance().create("Workspace2D", 1, 3 * nPeaks,
                                               3 * nPeaks));

  auto &outSp = L1WS->getSpectrum(0);
  auto &yVec = outSp.mutableY();
  auto &eVec = outSp.mutableE();
  auto &xVec = outSp.mutableX();
  yVec = 0.0;

  for (int i = 0; i < nPeaks; i++) {
    const DataObjects::Peak &peak = peaksWs->getPeak(i);

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
  IAlgorithm_sptr fitL1_alg;
  try {
    fitL1_alg = createChildAlgorithm("Fit", -1, -1, false);
  } catch (Exception::NotFoundError &) {
    g_log.error("Can't locate Fit algorithm");
    throw;
  }
  std::ostringstream fun_str;
  fun_str << "name=SCDPanelErrors,Workspace=" << peaksWs->getName()
          << ",Bank=moderator";
  std::ostringstream tie_str;
  tie_str << "XShift=0.0,YShift=0.0,XRotate=0.0,YRotate=0.0,ZRotate=0.0,"
             "ScaleWidth=1.0,ScaleHeight=1.0";
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
  com.moveDetector(0.0, 0.0, deltaL1, 0.0, 0.0, 0.0, 1.0, 1.0, "moderator",
                   peaksWs);
  g_log.notice() << "L1 = "
                 << -peaksWs->getInstrument()->getSource()->getPos().Z() << "  "
                 << fitL1Status << " Chi2overDoF " << chisqL1 << "\n";
}

void SCDCalibratePanels::findU(DataObjects::PeaksWorkspace_sptr peaksWs) {
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

  // Reindex peaks with new UB
  Mantid::API::IAlgorithm_sptr alg = createChildAlgorithm("IndexPeaks");
  alg->setPropertyValue("PeaksWorkspace", peaksWs->getName());
  alg->setProperty("Tolerance", 0.15);
  alg->executeAsChildAlg();
  g_log.notice() << peaksWs->sample().getOrientedLattice().getUB() << "\n";
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
    boost::shared_ptr<const Instrument> &instrument,
    boost::container::flat_set<string> &AllBankName, double &T0, double &L0,
    string filename, string bankPrefixName) {

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

  // setProperty("ResultWorkspace", Result);
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
    boost::shared_ptr<Instrument> &instrument,
    boost::container::flat_set<string> &AllBankName, double T0,
    string filename) {
  // having a filename triggers doing the work
  if (filename.empty())
    return;

  g_log.notice() << "Saving DetCal file in " << filename << "\n";

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
  alg->executeAsChildAlg();
}

void SCDCalibratePanels::init() {
  declareProperty(Kernel::make_unique<WorkspaceProperty<PeaksWorkspace>>(
                      "PeakWorkspace", "", Kernel::Direction::InOut),
                  "Workspace of Indexed Peaks");

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
  declareProperty("ChangeL1", true, "Change the L1(source to sample) distance");
  declareProperty("ChangePanelSize", true, "Change the height and width of the "
                                           "detectors.  Implemented only for "
                                           "RectangularDetectors.");

  declareProperty("EdgePixels", 0,
                  "Remove peaks that are at pixels this close to edge. ");

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

  declareProperty(Kernel::make_unique<FileProperty>("ColFilename",
                                                    "ColCalcvsTheor.nxs",
                                                    FileProperty::Save, ".nxs"),
                  "Path to a NeXus file comparing calculated and theoretical "
                  "column of each peak.");

  declareProperty(Kernel::make_unique<FileProperty>("RowFilename",
                                                    "RowCalcvsTheor.nxs",
                                                    FileProperty::Save, ".nxs"),
                  "Path to a NeXus file comparing calculated and theoretical "
                  "row of each peak.");

  declareProperty(Kernel::make_unique<FileProperty>("TofFilename",
                                                    "TofCalcvsTheor.nxs",
                                                    FileProperty::Save, ".nxs"),
                  "Path to a NeXus file comparing calculated and theoretical "
                  "TOF of each peak.");

  const string OUTPUTS("Outputs");
  setPropertyGroup("DetCalFilename", OUTPUTS);
  setPropertyGroup("XmlFilename", OUTPUTS);
  setPropertyGroup("ColFilename", OUTPUTS);
  setPropertyGroup("RowFilename", OUTPUTS);
  setPropertyGroup("TofFilename", OUTPUTS);
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
    const string &FileName,
    const boost::container::flat_set<string> &AllBankNames,
    const Instrument &instrument) const {
  if (FileName.empty())
    return;

  g_log.notice() << "Saving parameter file as " << FileName << "\n";

  // create the file and add the header
  ofstream oss3(FileName.c_str());
  oss3 << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
  oss3 << " <parameter-file instrument=\"" << instrument.getName()
       << "\" valid-from=\"" << instrument.getValidFromDate().toISO8601String()
       << "\">\n";
  ParameterMap_sptr pmap = instrument.getParameterMap();

  // write out the detector banks
  for (auto bankName : AllBankNames) {
    if (instrument.getName().compare("CORELLI") == 0.0)
      bankName.append("/sixteenpack");
    oss3 << "<component-link name=\"" << bankName << "\">\n";
    boost::shared_ptr<const IComponent> bank =
        instrument.getComponentByName(bankName);

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

  // write out the source
  IComponent_const_sptr source = instrument.getSource();

  oss3 << "<component-link name=\"" << source->getName() << "\">\n";
  IComponent_const_sptr sample = instrument.getSample();
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

} // namespace Crystal
} // namespace Mantid
