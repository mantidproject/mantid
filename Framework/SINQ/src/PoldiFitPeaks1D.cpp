// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceProperty.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidSINQ/PoldiFitPeaks1D.h"

#include "MantidSINQ/PoldiUtilities/UncertainValue.h"
#include "MantidSINQ/PoldiUtilities/UncertainValueIO.h"

#include "MantidAPI/CompositeFunction.h"

namespace Mantid {
namespace Poldi {

using namespace Kernel;
using namespace API;
using namespace DataObjects;
using namespace CurveFitting;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PoldiFitPeaks1D)

/// Algorithm's name for identification. @see Algorithm::name
const std::string PoldiFitPeaks1D::name() const { return "PoldiFitPeaks1D"; }

/// Algorithm's version for identification. @see Algorithm::version
int PoldiFitPeaks1D::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string PoldiFitPeaks1D::category() const { return "SINQ\\Poldi"; }

void PoldiFitPeaks1D::init() {
  declareProperty(
      std::make_unique<WorkspaceProperty<Workspace2D>>("InputWorkspace", "",
                                                       Direction::Input),
      "An input workspace containing a POLDI auto-correlation spectrum.");
  boost::shared_ptr<BoundedValidator<double>> minFwhmPerDirection =
      boost::make_shared<BoundedValidator<double>>();
  minFwhmPerDirection->setLower(2.0);
  declareProperty(
      "FwhmMultiples", 2.0, minFwhmPerDirection,
      "Each peak will be fitted using x times FWHM data in each direction.",
      Direction::Input);

  std::vector<std::string> peakFunctions =
      FunctionFactory::Instance().getFunctionNames<IPeakFunction>();
  auto peakFunctionNames =
      boost::make_shared<ListValidator<std::string>>(peakFunctions);
  declareProperty("PeakFunction", "Gaussian", peakFunctionNames,
                  "Peak function that will be fitted to all peaks.",
                  Direction::Input);

  declareProperty(std::make_unique<WorkspaceProperty<TableWorkspace>>(
                      "PoldiPeakTable", "", Direction::Input),
                  "A table workspace containing POLDI peak data.");

  declareProperty(std::make_unique<WorkspaceProperty<TableWorkspace>>(
                      "OutputWorkspace", "RefinedPeakTable", Direction::Output),
                  "Output workspace with refined peak data.");
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(
                      "FitPlotsWorkspace", "FitPlots", Direction::Output),
                  "Plots of all peak fits.");

  m_backgroundTemplate = FunctionFactory::Instance().createInitialized(
      "name=UserFunction, Formula=A0 + A1*(x - x0)^2");
  m_profileTies = "f1.x0 = f0.PeakCentre";
}

void PoldiFitPeaks1D::setPeakFunction(const std::string &peakFunction) {
  m_profileTemplate = peakFunction;
}

PoldiPeakCollection_sptr PoldiFitPeaks1D::getInitializedPeakCollection(
    const DataObjects::TableWorkspace_sptr &peakTable) const {
  auto peakCollection = boost::make_shared<PoldiPeakCollection>(peakTable);
  peakCollection->setProfileFunctionName(m_profileTemplate);

  return peakCollection;
}

IFunction_sptr
PoldiFitPeaks1D::getPeakProfile(const PoldiPeak_sptr &poldiPeak) const {
  IPeakFunction_sptr clonedProfile = boost::dynamic_pointer_cast<IPeakFunction>(
      FunctionFactory::Instance().createFunction(m_profileTemplate));
  clonedProfile->setCentre(poldiPeak->q());
  clonedProfile->setFwhm(poldiPeak->fwhm(PoldiPeak::AbsoluteQ));
  clonedProfile->setHeight(poldiPeak->intensity());

  IFunction_sptr clonedBackground = m_backgroundTemplate->clone();

  auto totalProfile = boost::make_shared<CompositeFunction>();
  totalProfile->initialize();
  totalProfile->addFunction(clonedProfile);
  totalProfile->addFunction(clonedBackground);

  if (!m_profileTies.empty()) {
    totalProfile->addTies(m_profileTies);
  }

  return totalProfile;
}

void PoldiFitPeaks1D::setValuesFromProfileFunction(
    PoldiPeak_sptr poldiPeak, const IFunction_sptr &fittedFunction) const {
  CompositeFunction_sptr totalFunction =
      boost::dynamic_pointer_cast<CompositeFunction>(fittedFunction);

  if (totalFunction) {
    IPeakFunction_sptr peakFunction =
        boost::dynamic_pointer_cast<IPeakFunction>(
            totalFunction->getFunction(0));

    if (peakFunction) {
      poldiPeak->setIntensity(
          UncertainValue(peakFunction->height(), peakFunction->getError(0)));
      poldiPeak->setQ(
          UncertainValue(peakFunction->centre(), peakFunction->getError(1)));
      poldiPeak->setFwhm(UncertainValue(peakFunction->fwhm(),
                                        getFwhmWidthRelation(peakFunction) *
                                            peakFunction->getError(2)));
    }
  }
}

double
PoldiFitPeaks1D::getFwhmWidthRelation(IPeakFunction_sptr peakFunction) const {
  return peakFunction->fwhm() / peakFunction->getParameter(2);
}

void PoldiFitPeaks1D::exec() {
  setPeakFunction(getProperty("PeakFunction"));

  // Number of points around the peak center to use for the fit
  m_fwhmMultiples = getProperty("FwhmMultiples");

  // try to construct PoldiPeakCollection from provided TableWorkspace
  TableWorkspace_sptr poldiPeakTable = getProperty("PoldiPeakTable");
  m_peaks = getInitializedPeakCollection(poldiPeakTable);

  g_log.information() << "Peaks to fit: " << m_peaks->peakCount() << '\n';

  Workspace2D_sptr dataWorkspace = getProperty("InputWorkspace");

  auto fitPlotGroup = boost::make_shared<WorkspaceGroup>();

  for (size_t i = 0; i < m_peaks->peakCount(); ++i) {
    PoldiPeak_sptr currentPeak = m_peaks->peak(i);
    IFunction_sptr currentProfile = getPeakProfile(currentPeak);

    IAlgorithm_sptr fit =
        getFitAlgorithm(dataWorkspace, currentPeak, currentProfile);

    bool fitSuccess = fit->execute();

    if (fitSuccess) {
      setValuesFromProfileFunction(currentPeak, fit->getProperty("Function"));

      MatrixWorkspace_sptr fpg = fit->getProperty("OutputWorkspace");
      fitPlotGroup->addWorkspace(fpg);
    }
  }

  setProperty("OutputWorkspace", m_peaks->asTableWorkspace());
  setProperty("FitPlotsWorkspace", fitPlotGroup);
}

IAlgorithm_sptr
PoldiFitPeaks1D::getFitAlgorithm(const Workspace2D_sptr &dataWorkspace,
                                 const PoldiPeak_sptr &peak,
                                 const IFunction_sptr &profile) {
  double width = peak->fwhm();
  double extent = std::min(0.05, std::max(0.002, width)) * m_fwhmMultiples;

  std::pair<double, double> xBorders(peak->q() - extent, peak->q() + extent);

  IAlgorithm_sptr fitAlgorithm = createChildAlgorithm("Fit", -1, -1, false);
  fitAlgorithm->setProperty("CreateOutput", true);
  fitAlgorithm->setProperty("Output", "FitPeaks1D");
  fitAlgorithm->setProperty("CalcErrors", true);
  fitAlgorithm->setProperty("Function", profile);
  fitAlgorithm->setProperty("InputWorkspace", dataWorkspace);
  fitAlgorithm->setProperty("WorkspaceIndex", 0);
  fitAlgorithm->setProperty("StartX", xBorders.first);
  fitAlgorithm->setProperty("EndX", xBorders.second);

  return fitAlgorithm;
}

} // namespace Poldi
} // namespace Mantid
