// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "EnggDiffMultiRunFittingWidgetPresenter.h"
#include "EnggDiffMultiRunFittingWidgetAdder.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/make_unique.h"
#include "MantidQtWidgets/Plotting/Qwt/QwtHelper.h"

#include <boost/algorithm/string.hpp>
#include <cctype>

namespace {

bool isDigit(const std::string &text) {
  return std::all_of(text.cbegin(), text.cend(), isdigit);
}

std::string
generateFittedPeaksName(const MantidQt::CustomInterfaces::RunLabel &runLabel) {
  return runLabel.runNumber + "_" +
         std::to_string(runLabel.bank) + "_fitted_peaks_external_plot";
}

std::string
generateFocusedRunName(const MantidQt::CustomInterfaces::RunLabel &runLabel) {
  return runLabel.runNumber + "_" +
         std::to_string(runLabel.bank) + "_external_plot";
}

size_t guessBankID(Mantid::API::MatrixWorkspace_const_sptr ws) {
  const static std::string bankIDName = "bankid";
  if (ws->run().hasProperty(bankIDName)) {
    const auto log = dynamic_cast<Mantid::Kernel::PropertyWithValue<int> *>(
        ws->run().getLogData(bankIDName));
    return boost::lexical_cast<size_t>(log->value());
  }

  // couldn't get it from sample logs - try using the old naming convention
  const std::string name = ws->getName();
  std::vector<std::string> chunks;
  boost::split(chunks, name, boost::is_any_of("_"));
  if (!chunks.empty()) {
    const bool isNum = isDigit(chunks.back());
    if (isNum) {
      try {
        return boost::lexical_cast<size_t>(chunks.back());
      } catch (boost::exception &) {
        // If we get a bad cast or something goes wrong then
        // the file is probably not what we were expecting
        // so throw a runtime error
        throw std::runtime_error(
            "Failed to fit file: The data was not what is expected. "
            "Does the file contain a focused workspace?");
      }
    }
  }

  throw std::runtime_error("Could not guess run number from input workspace. "
                           "Are you sure it has been focused correctly?");
}
} // anonymous namespace

namespace MantidQt {
namespace CustomInterfaces {

EnggDiffMultiRunFittingWidgetPresenter::EnggDiffMultiRunFittingWidgetPresenter(
    std::unique_ptr<IEnggDiffMultiRunFittingWidgetModel> model,
    IEnggDiffMultiRunFittingWidgetView *view)
    : m_model(std::move(model)), m_view(view) {}

void EnggDiffMultiRunFittingWidgetPresenter::addFittedPeaks(
    const RunLabel &runLabel, const Mantid::API::MatrixWorkspace_sptr ws) {
  m_model->addFittedPeaks(runLabel, ws);
  updatePlot(runLabel);
}

void EnggDiffMultiRunFittingWidgetPresenter::addFocusedRun(
    const Mantid::API::MatrixWorkspace_sptr ws) {
  const auto runNumber = std::to_string(ws->getRunNumber());
  const auto bankID = guessBankID(ws);

  m_model->addFocusedRun(RunLabel(runNumber, bankID), ws);
  m_view->updateRunList(m_model->getAllWorkspaceLabels());
}

void EnggDiffMultiRunFittingWidgetPresenter::displayFitResults(
    const RunLabel &runLabel) {
  const auto fittedPeaks = m_model->getFittedPeaks(runLabel);
  if (!fittedPeaks) {
    m_view->reportPlotInvalidFittedPeaks(runLabel);
  } else {
    const auto plottablePeaks = API::QwtHelper::curveDataFromWs(*fittedPeaks);
    m_view->plotFittedPeaks(plottablePeaks);
  }
}

std::vector<RunLabel>
EnggDiffMultiRunFittingWidgetPresenter::getAllRunLabels() const {
  return m_view->getAllRunLabels();
}

std::unique_ptr<IEnggDiffMultiRunFittingWidgetAdder>
EnggDiffMultiRunFittingWidgetPresenter::getWidgetAdder() const {
  return Mantid::Kernel::make_unique<EnggDiffMultiRunFittingWidgetAdder>(
      m_view);
}

boost::optional<Mantid::API::MatrixWorkspace_sptr>
EnggDiffMultiRunFittingWidgetPresenter::getFittedPeaks(
    const RunLabel &runLabel) const {
  return m_model->getFittedPeaks(runLabel);
}

boost::optional<Mantid::API::MatrixWorkspace_sptr>
EnggDiffMultiRunFittingWidgetPresenter::getFocusedRun(
    const RunLabel &runLabel) const {
  return m_model->getFocusedRun(runLabel);
}

boost::optional<RunLabel>
EnggDiffMultiRunFittingWidgetPresenter::getSelectedRunLabel() const {
  return m_view->getSelectedRunLabel();
}

void EnggDiffMultiRunFittingWidgetPresenter::notify(
    IEnggDiffMultiRunFittingWidgetPresenter::Notification notif) {
  switch (notif) {
  case Notification::PlotPeaksStateChanged:
    processPlotPeaksStateChanged();
    break;

  case Notification::PlotToSeparateWindow:
    processPlotToSeparateWindow();
    break;

  case Notification::RemoveRun:
    processRemoveRun();
    break;

  case Notification::SelectRun:
    processSelectRun();
    break;
  }
}

void EnggDiffMultiRunFittingWidgetPresenter::processPlotPeaksStateChanged() {
  const auto runLabel = getSelectedRunLabel();
  if (runLabel) {
    updatePlot(*runLabel);
  }
}

void EnggDiffMultiRunFittingWidgetPresenter::processPlotToSeparateWindow() {
  const auto runLabel = m_view->getSelectedRunLabel();
  if (!runLabel) {
    m_view->reportNoRunSelectedForPlot();
    return;
  }

  const auto focusedRun = m_model->getFocusedRun(*runLabel);
  if (!focusedRun) {
    m_view->reportPlotInvalidFocusedRun(*runLabel);
    return;
  }

  auto &ADS = Mantid::API::AnalysisDataService::Instance();
  const auto focusedRunName = generateFocusedRunName(*runLabel);
  ADS.add(focusedRunName, *focusedRun);

  boost::optional<std::string> fittedPeaksName = boost::none;
  if (m_view->showFitResultsSelected() &&
      m_model->hasFittedPeaksForRun(*runLabel)) {
    fittedPeaksName = generateFittedPeaksName(*runLabel);
    const auto fittedPeaks = m_model->getFittedPeaks(*runLabel);
    ADS.add(*fittedPeaksName, *fittedPeaks);
  }

  m_view->plotToSeparateWindow(focusedRunName, fittedPeaksName);

  ADS.remove(focusedRunName);
  if (fittedPeaksName) {
    ADS.remove(*fittedPeaksName);
  }
}

void EnggDiffMultiRunFittingWidgetPresenter::processRemoveRun() {
  const auto runLabel = getSelectedRunLabel();
  if (runLabel) {
    m_model->removeRun(*runLabel);
    m_view->updateRunList(m_model->getAllWorkspaceLabels());
    m_view->resetCanvas();
  }
}

void EnggDiffMultiRunFittingWidgetPresenter::processSelectRun() {
  const auto runLabel = getSelectedRunLabel();
  if (runLabel) {
    updatePlot(*runLabel);
  }
}

void EnggDiffMultiRunFittingWidgetPresenter::updatePlot(
    const RunLabel &runLabel) {
  const auto focusedRun = m_model->getFocusedRun(runLabel);

  if (!focusedRun) {
    m_view->reportPlotInvalidFocusedRun(runLabel);
  } else {
    const auto plottableCurve = API::QwtHelper::curveDataFromWs(*focusedRun);

    m_view->resetCanvas();
    m_view->plotFocusedRun(plottableCurve);

    if (m_model->hasFittedPeaksForRun(runLabel) &&
        m_view->showFitResultsSelected()) {
      displayFitResults(runLabel);
    }
  }
}

} // namespace CustomInterfaces
} // namespace MantidQt
