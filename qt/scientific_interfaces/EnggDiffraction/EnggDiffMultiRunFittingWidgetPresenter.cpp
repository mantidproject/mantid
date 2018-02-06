#include "EnggDiffMultiRunFittingWidgetPresenter.h"
#include "EnggDiffMultiRunFittingWidgetAdder.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/make_unique.h"
#include "MantidQtWidgets/LegacyQwt/QwtHelper.h"

#include <boost/algorithm/string.hpp>
#include <cctype>

namespace {

bool isDigit(const std::string &text) {
  return std::all_of(text.cbegin(), text.cend(), isdigit);
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
  bool isNum = isDigit(chunks.back());
  if (!chunks.empty() && isNum) {
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
  const auto runNumber = ws->getRunNumber();
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

RunLabel EnggDiffMultiRunFittingWidgetPresenter::getSelectedRunLabel() const {
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
  updatePlot(getSelectedRunLabel());
}

void EnggDiffMultiRunFittingWidgetPresenter::processPlotToSeparateWindow() {
  if (!m_view->hasSelectedRunLabel()) {
    m_view->userError("Please select a run to plot",
                      "Cannot plot to separate window without selecting a "
                      "run from the list");
    return;
  }
  const auto runLabel = m_view->getSelectedRunLabel();
  const auto focusedRun = m_model->getFocusedRun(runLabel);

  if (!focusedRun) {
    m_view->userError(
        "Invalid focused run identifier",
        "Tried to access invalid run, run number " +
            std::to_string(runLabel.runNumber) + " and bank ID " +
            std::to_string(runLabel.bank) +
            ". Please contact the development team with this message");
    return;
  }

  const auto focusedRunName = (*focusedRun)->getName();

  auto &ADS = Mantid::API::AnalysisDataService::Instance();

  if (!ADS.doesExist(focusedRunName)) {
    ADS.add(focusedRunName, *focusedRun);
  }

  std::string fittedPeaksName = "";
  if (showFitResultsSelected() && m_model->hasFittedPeaksForRun(runLabel)) {
    const auto fittedPeaks = m_model->getFittedPeaks(runLabel);
    fittedPeaksName = (*fittedPeaks)->getName();

    if (!ADS.doesExist(fittedPeaksName)) {
      ADS.add(fittedPeaksName, *fittedPeaks);
    }
  }

  m_view->plotToSeparateWindow(focusedRunName, fittedPeaksName);
}

void EnggDiffMultiRunFittingWidgetPresenter::processRemoveRun() {
  if (m_view->hasSelectedRunLabel()) {
    const auto selectedRunLabel = m_view->getSelectedRunLabel();
    m_model->removeRun(selectedRunLabel);
    m_view->updateRunList(m_model->getAllWorkspaceLabels());
  }
}

void EnggDiffMultiRunFittingWidgetPresenter::processSelectRun() {
  if (m_view->hasSelectedRunLabel()) {
    const auto selectedRunLabel = m_view->getSelectedRunLabel();
    updatePlot(selectedRunLabel);
  }
}

bool EnggDiffMultiRunFittingWidgetPresenter::showFitResultsSelected() const {
  return m_view->showFitResultsSelected();
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
