// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "EnggDiffFittingViewQtWidget.h"
#include "EnggDiffFittingModel.h"
#include "EnggDiffFittingPresenter.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidKernel/make_unique.h"
#include "MantidQtWidgets/Common/AlgorithmInputHistory.h"
#include "MantidQtWidgets/Plotting/Qwt/PeakPicker.h"

#include <array>
#include <iomanip>
#include <random>
#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/make_shared.hpp>

#include <Poco/Path.h>

#include <QEvent>
#include <QFileDialog>
#include <QHelpEvent>
#include <QSettings>

#include <qwt_plot_curve.h>
#include <qwt_plot_zoomer.h>
#include <qwt_symbol.h>

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;

namespace MantidQt {
namespace CustomInterfaces {

const std::string EnggDiffFittingViewQtWidget::g_settingsGroup =
    "CustomInterfaces/EnggDiffraction/FittingView";

const std::string EnggDiffFittingViewQtWidget::g_peaksListExt =
    "Peaks list File: CSV "
    "(*.csv *.txt);;"
    "Other extensions/all files (*)";

std::vector<std::string> EnggDiffFittingViewQtWidget::m_fitting_runno_dir_vec;

EnggDiffFittingViewQtWidget::EnggDiffFittingViewQtWidget(
    QWidget * /*parent*/, boost::shared_ptr<IEnggDiffractionUserMsg> mainMsg,
    boost::shared_ptr<IEnggDiffractionSettings> mainSettings,
    boost::shared_ptr<IEnggDiffractionCalibration> mainCalib,
    boost::shared_ptr<IEnggDiffractionParam> mainParam,
    boost::shared_ptr<IEnggDiffractionPythonRunner> mainPythonRunner,
    boost::shared_ptr<IEnggDiffractionParam> fileSettings)
    : IEnggDiffFittingView(), m_fittedDataVector(),
      m_fileSettings(fileSettings), m_mainMsgProvider(mainMsg),
      m_mainSettings(mainSettings), m_mainPythonRunner(mainPythonRunner),
      m_presenter(boost::make_shared<EnggDiffFittingPresenter>(
          this, Mantid::Kernel::make_unique<EnggDiffFittingModel>(), mainCalib,
          mainParam)) {

  initLayout();
  m_presenter->notify(IEnggDiffFittingPresenter::Start);
}

EnggDiffFittingViewQtWidget::~EnggDiffFittingViewQtWidget() {
  m_presenter->notify(IEnggDiffFittingPresenter::ShutDown);

  for (auto curves : m_focusedDataVector) {
    curves->detach();
    delete curves;
  }

  for (auto curves : m_fittedDataVector) {
    curves->detach();
    delete curves;
  }
}

void EnggDiffFittingViewQtWidget::initLayout() {
  m_ui.setupUi(this);

  readSettings();
  doSetup();
}

void EnggDiffFittingViewQtWidget::doSetup() {
  connect(m_ui.pushButton_fitting_browse_run_num, SIGNAL(released()), this,
          SLOT(browseFitFocusedRun()));

  connect(m_ui.lineEdit_pushButton_run_num, SIGNAL(returnPressed()), this,
          SLOT(loadClicked()));

  connect(m_ui.pushButton_fitting_browse_peaks, SIGNAL(released()), this,
          SLOT(browseClicked()));

  connect(m_ui.pushButton_load, SIGNAL(released()), this, SLOT(loadClicked()));

  connect(m_ui.pushButton_fit, SIGNAL(released()), this, SLOT(fitClicked()));

  connect(m_ui.pushButton_fit_all, SIGNAL(released()), this,
          SLOT(fitAllClicked()));

  // add peak by clicking the button
  connect(m_ui.pushButton_select_peak, SIGNAL(released()), SLOT(setPeakPick()));

  connect(m_ui.pushButton_add_peak, SIGNAL(released()), SLOT(addClicked()));

  connect(m_ui.pushButton_save_peak_list, SIGNAL(released()),
          SLOT(saveClicked()));

  connect(m_ui.pushButton_clear_peak_list, SIGNAL(released()),
          SLOT(clearPeakList()));

  connect(m_ui.pushButton_plot_separate_window, SIGNAL(released()),
          SLOT(plotSeparateWindow()));

  connect(m_ui.listWidget_fitting_run_num,
          SIGNAL(itemClicked(QListWidgetItem *)), this,
          SLOT(listWidget_fitting_run_num_clicked(QListWidgetItem *)));

  connect(m_ui.checkBox_plotFittedPeaks, SIGNAL(stateChanged(int)), this,
          SLOT(plotFittedPeaksStateChanged()));

  // Tool-tip button
  connect(m_ui.pushButton_tooltip, SIGNAL(released()), SLOT(showToolTipHelp()));

  // Remove run button
  connect(m_ui.pushButton_remove_run, SIGNAL(released()), this,
          SLOT(removeRunClicked()));

  m_ui.dataPlot->setCanvasBackground(Qt::white);
  m_ui.dataPlot->setAxisTitle(QwtPlot::xBottom, "d-Spacing (A)");
  m_ui.dataPlot->setAxisTitle(QwtPlot::yLeft, "Counts (us)^-1");
  QFont font("MS Shell Dlg 2", 8);
  m_ui.dataPlot->setAxisFont(QwtPlot::xBottom, font);
  m_ui.dataPlot->setAxisFont(QwtPlot::yLeft, font);

  // constructor of the peakPicker
  // XXX: Being a QwtPlotItem, should get deleted when m_ui.plot gets deleted
  // (auto-delete option)
  m_peakPicker = new MantidWidgets::PeakPicker(m_ui.dataPlot, Qt::red);
  setPeakPickerEnabled(false);

  m_zoomTool =
      new QwtPlotZoomer(QwtPlot::xBottom, QwtPlot::yLeft,
                        QwtPicker::DragSelection | QwtPicker::CornerToCorner,
                        QwtPicker::AlwaysOff, m_ui.dataPlot->canvas());
  m_zoomTool->setRubberBandPen(QPen(Qt::black));
  setZoomTool(false);
}

void EnggDiffFittingViewQtWidget::readSettings() {
  QSettings qs;
  qs.beginGroup(QString::fromStdString(g_settingsGroup));

  // user params
  m_ui.lineEdit_pushButton_run_num->setText(
      qs.value("user-params-fitting-focused-file", "").toString());
  m_ui.lineEdit_fitting_peaks->setText(
      qs.value("user-params-fitting-peaks-to-fit", "").toString());

  qs.endGroup();
}

void EnggDiffFittingViewQtWidget::saveSettings() const {
  QSettings qs;
  qs.beginGroup(QString::fromStdString(g_settingsGroup));

  qs.setValue("user-params-fitting-focused-file",
              m_ui.lineEdit_pushButton_run_num->text());
  qs.setValue("user-params-fitting-peaks-to-fit",
              m_ui.lineEdit_fitting_peaks->text());

  qs.endGroup();
}

void EnggDiffFittingViewQtWidget::enable(bool enable) {
  m_ui.pushButton_fitting_browse_run_num->setEnabled(enable);
  m_ui.pushButton_load->setEnabled(enable);
  m_ui.lineEdit_pushButton_run_num->setEnabled(enable);
  m_ui.pushButton_fitting_browse_peaks->setEnabled(enable);
  m_ui.lineEdit_fitting_peaks->setEnabled(enable);
  m_ui.pushButton_fit->setEnabled(enable);
  m_ui.pushButton_clear_peak_list->setEnabled(enable);
  m_ui.pushButton_save_peak_list->setEnabled(enable);
  m_ui.groupBox_fititng_preview->setEnabled(enable);
}

void EnggDiffFittingViewQtWidget::showStatus(const std::string &sts) {
  m_mainMsgProvider->showStatus(sts);
}

void EnggDiffFittingViewQtWidget::userWarning(const std::string &err,
                                              const std::string &description) {
  m_mainMsgProvider->userWarning(err, description);
}

void EnggDiffFittingViewQtWidget::userError(const std::string &err,
                                            const std::string &description) {
  m_mainMsgProvider->userError(err, description);
}

void EnggDiffFittingViewQtWidget::enableCalibrateFocusFitUserActions(
    bool enable) {
  m_mainMsgProvider->enableCalibrateFocusFitUserActions(enable);
}

EnggDiffCalibSettings
EnggDiffFittingViewQtWidget::currentCalibSettings() const {
  return m_mainSettings->currentCalibSettings();
}

std::string
EnggDiffFittingViewQtWidget::enggRunPythonCode(const std::string &pyCode) {
  return m_mainPythonRunner->enggRunPythonCode(pyCode);
}

void EnggDiffFittingViewQtWidget::loadClicked() {
  m_presenter->notify(IEnggDiffFittingPresenter::Load);
}

void EnggDiffFittingViewQtWidget::fitClicked() {
  m_presenter->notify(IEnggDiffFittingPresenter::FitPeaks);
}

void EnggDiffFittingViewQtWidget::fitAllClicked() {
  m_presenter->notify(IEnggDiffFittingPresenter::FitAllPeaks);
}

void EnggDiffFittingViewQtWidget::addClicked() {
  m_presenter->notify(IEnggDiffFittingPresenter::addPeaks);
}

void EnggDiffFittingViewQtWidget::browseClicked() {
  m_presenter->notify(IEnggDiffFittingPresenter::browsePeaks);
}

void EnggDiffFittingViewQtWidget::saveClicked() {
  m_presenter->notify(IEnggDiffFittingPresenter::savePeaks);
}

void EnggDiffFittingViewQtWidget::plotFittedPeaksStateChanged() {
  m_presenter->notify(IEnggDiffFittingPresenter::updatePlotFittedPeaks);
}

void EnggDiffFittingViewQtWidget::listWidget_fitting_run_num_clicked(
    QListWidgetItem *clickedItem) {
  const auto label = clickedItem->text();
  m_presenter->notify(IEnggDiffFittingPresenter::selectRun);
}

void EnggDiffFittingViewQtWidget::removeRunClicked() {
  m_presenter->notify(IEnggDiffFittingPresenter::removeRun);
}

void EnggDiffFittingViewQtWidget::resetCanvas() {
  // clear vector and detach curves to avoid plot crash
  // when only plotting focused workspace
  for (auto curves : m_fittedDataVector) {
    if (curves) {
      curves->detach();
      delete curves;
    }
  }

  if (m_fittedDataVector.size() > 0)
    m_fittedDataVector.clear();

  // set it as false as there will be no valid workspace to plot
  m_ui.pushButton_plot_separate_window->setEnabled(false);
}

void EnggDiffFittingViewQtWidget::setDataVector(
    std::vector<boost::shared_ptr<QwtData>> &data, bool focused,
    bool plotSinglePeaks, const std::string &xAxisLabel) {

  if (!plotSinglePeaks) {
    // clear vector and detach curves to avoid plot crash
    resetCanvas();
  }
  m_ui.dataPlot->setAxisTitle(QwtPlot::xBottom, xAxisLabel.c_str());

  // when only plotting focused workspace
  if (focused) {
    dataCurvesFactory(data, m_focusedDataVector, focused);
  } else {
    dataCurvesFactory(data, m_fittedDataVector, focused);
  }
}

void EnggDiffFittingViewQtWidget::dataCurvesFactory(
    std::vector<boost::shared_ptr<QwtData>> &data,
    std::vector<QwtPlotCurve *> &dataVector, bool focused) {

  // clear vector
  for (auto curves : dataVector) {
    if (curves) {
      curves->detach();
      delete curves;
    }
  }

  if (dataVector.size() > 0)
    dataVector.clear();
  resetView();

  // dark colours could be removed so that the coloured peaks stand out more
  const std::array<QColor, 16> QPenList{
      {Qt::white, Qt::red, Qt::darkRed, Qt::green, Qt::darkGreen, Qt::blue,
       Qt::darkBlue, Qt::cyan, Qt::darkCyan, Qt::magenta, Qt::darkMagenta,
       Qt::yellow, Qt::darkYellow, Qt::gray, Qt::lightGray, Qt::black}};

  std::mt19937 gen;
  std::uniform_int_distribution<std::size_t> dis(0, QPenList.size() - 1);

  for (size_t i = 0; i < data.size(); i++) {
    auto *peak = data[i].get();

    QwtPlotCurve *dataCurve = new QwtPlotCurve();
    if (!focused) {
      dataCurve->setStyle(QwtPlotCurve::Lines);
      auto randIndex = dis(gen);
      dataCurve->setPen(QPen(QPenList[randIndex], 2));

      // only set enabled when single peak workspace plotted
      m_ui.pushButton_plot_separate_window->setEnabled(true);
    } else {
      dataCurve->setStyle(QwtPlotCurve::NoCurve);
      // focused workspace in bg set as darkGrey crosses insted of line
      dataCurve->setSymbol(QwtSymbol(QwtSymbol::XCross, QBrush(),
                                     QPen(Qt::darkGray, 1), QSize(3, 3)));
    }
    dataCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);

    dataVector.push_back(dataCurve);

    dataVector[i]->setData(*peak);
    dataVector[i]->attach(m_ui.dataPlot);
  }

  m_ui.dataPlot->replot();
  m_zoomTool->setZoomBase();
  // enable zoom & select peak btn after the plotting on graph
  setZoomTool(true);
  m_ui.pushButton_select_peak->setEnabled(true);
  data.clear();
}

void EnggDiffFittingViewQtWidget::setPeakPickerEnabled(bool enabled) {
  m_peakPicker->setEnabled(enabled);
  m_peakPicker->setVisible(enabled);
  m_ui.dataPlot->replot(); // PeakPicker might get hidden/shown
  m_ui.pushButton_add_peak->setEnabled(enabled);
  if (enabled) {
    QString btnText = "Reset Peak Selector";
    m_ui.pushButton_select_peak->setText(btnText);
  }
}

void EnggDiffFittingViewQtWidget::setPeakPicker(
    const IPeakFunction_const_sptr &peak) {
  m_peakPicker->setPeak(peak);
  m_ui.dataPlot->replot();
}

double EnggDiffFittingViewQtWidget::getPeakCentre() const {
  auto peak = m_peakPicker->peak();
  auto centre = peak->centre();
  return centre;
}

bool EnggDiffFittingViewQtWidget::peakPickerEnabled() const {
  return m_peakPicker->isEnabled();
}

void EnggDiffFittingViewQtWidget::setZoomTool(bool enabled) {
  m_zoomTool->setEnabled(enabled);
}

void EnggDiffFittingViewQtWidget::resetView() {
  // Resets the view to a sensible default
  // Auto scale the axis
  m_ui.dataPlot->setAxisAutoScale(QwtPlot::xBottom);
  m_ui.dataPlot->setAxisAutoScale(QwtPlot::yLeft);

  // Set this as the default zoom level
  m_zoomTool->setZoomBase(true);
}

std::string EnggDiffFittingViewQtWidget::getPreviousDir() const {

  QString prevPath =
      MantidQt::API::AlgorithmInputHistory::Instance().getPreviousDirectory();

  return prevPath.toStdString();
}

void EnggDiffFittingViewQtWidget::setPreviousDir(const std::string &path) {
  QString qPath = QString::fromStdString(path);
  MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(qPath);
}

std::string
EnggDiffFittingViewQtWidget::getOpenFile(const std::string &prevPath) {

  QString path(QFileDialog::getOpenFileName(
      this, tr("Open Peaks To Fit"), QString::fromStdString(prevPath),
      QString::fromStdString(g_peaksListExt)));

  return path.toStdString();
}

std::string
EnggDiffFittingViewQtWidget::getSaveFile(const std::string &prevPath) {

  QString path(QFileDialog::getSaveFileName(
      this, tr("Save Expected Peaks List"), QString::fromStdString(prevPath),
      QString::fromStdString(g_peaksListExt)));

  return path.toStdString();
}

void EnggDiffFittingViewQtWidget::browseFitFocusedRun() {
  const auto &focusDir = m_fileSettings->outFilesUserDir("Focus").toString();
  std::string nexusFormat = "Nexus file with calibration table: NXS, NEXUS"
                            "(*.nxs *.nexus);;";

  QStringList paths(QFileDialog::getOpenFileNames(
      this, tr("Open Focused File "), QString::fromStdString(focusDir),
      QString::fromStdString(nexusFormat)));

  if (paths.isEmpty()) {
    return;
  }

  setFocusedFileNames(paths.join(",").toStdString());
}

void EnggDiffFittingViewQtWidget::setFocusedFileNames(
    const std::string &paths) {
  m_ui.lineEdit_pushButton_run_num->setText(QString::fromStdString(paths));
}

std::string EnggDiffFittingViewQtWidget::getFocusedFileNames() const {
  return m_ui.lineEdit_pushButton_run_num->text().toStdString();
}

void EnggDiffFittingViewQtWidget::enableFitAllButton(bool enable) const {
  m_ui.pushButton_fit_all->setEnabled(enable);
}

void EnggDiffFittingViewQtWidget::clearFittingListWidget() const {
  m_ui.listWidget_fitting_run_num->clear();
}

void EnggDiffFittingViewQtWidget::enableFittingListWidget(bool enable) const {
  m_ui.listWidget_fitting_run_num->setEnabled(enable);
}

int EnggDiffFittingViewQtWidget::getFittingListWidgetCurrentRow() const {
  return m_ui.listWidget_fitting_run_num->currentRow();
}

boost::optional<std::string>
EnggDiffFittingViewQtWidget::getFittingListWidgetCurrentValue() const {
  if (listWidgetHasSelectedRow()) {
    return m_ui.listWidget_fitting_run_num->currentItem()->text().toStdString();
  }
  return boost::none;
}

bool EnggDiffFittingViewQtWidget::listWidgetHasSelectedRow() const {
  return m_ui.listWidget_fitting_run_num->selectedItems().size() != 0;
}

void EnggDiffFittingViewQtWidget::updateFittingListWidget(
    const std::vector<std::string> &rows) {
  clearFittingListWidget();

  for (const auto &rowLabel : rows) {
    this->addRunNoItem(rowLabel);
  }
}

void EnggDiffFittingViewQtWidget::setFittingListWidgetCurrentRow(
    int idx) const {
  m_ui.listWidget_fitting_run_num->setCurrentRow(idx);
}

bool EnggDiffFittingViewQtWidget::plotFittedPeaksEnabled() const {
  return m_ui.checkBox_plotFittedPeaks->isChecked();
}

void EnggDiffFittingViewQtWidget::plotSeparateWindow() {
  std::string pyCode =

      "fitting_single_peaks_twin_ws = \"__engggui_fitting_single_peaks_twin\"\n"
      "if (mtd.doesExist(fitting_single_peaks_twin_ws)):\n"
      " DeleteWorkspace(fitting_single_peaks_twin_ws)\n"

      "single_peak_ws = CloneWorkspace(InputWorkspace = "
      "\"engggui_fitting_single_peaks\", OutputWorkspace = "
      "fitting_single_peaks_twin_ws)\n"
      "tot_spec = single_peak_ws.getNumberHistograms()\n"

      "spec_list = []\n"
      "for i in range(0, tot_spec):\n"
      " spec_list.append(i)\n"

      "fitting_plot = plotSpectrum(single_peak_ws, spec_list).activeLayer()\n"
      "fitting_plot.setTitle(\"Engg GUI Single Peaks Fitting Workspace\")\n";

  std::string status = m_mainPythonRunner->enggRunPythonCode(pyCode);
  m_logMsgs.emplace_back("Plotted output focused data, with status string " +
                         status);
  m_presenter->notify(IEnggDiffFittingPresenter::LogMsg);
}

void EnggDiffFittingViewQtWidget::showToolTipHelp() {
  // We need a the mouse click position relative to the widget
  // and relative to the screen. We will set the mouse click position
  // relative to widget to 0 as the global position of the mouse
  // is what is considered when the tool tip is displayed
  const QPoint relWidgetPosition(0, 0);
  const QPoint mousePos = QCursor::pos();
  // Now fire the generated event to show a tool tip at the cursor
  QEvent *toolTipEvent =
      new QHelpEvent(QEvent::ToolTip, relWidgetPosition, mousePos);
  QCoreApplication::sendEvent(m_ui.pushButton_tooltip, toolTipEvent);
}

std::string EnggDiffFittingViewQtWidget::getExpectedPeaksInput() const {

  return m_ui.lineEdit_fitting_peaks->text().toStdString();
}

void EnggDiffFittingViewQtWidget::setPeakList(
    const std::string &peakList) const {
  m_ui.lineEdit_fitting_peaks->setText(QString::fromStdString(peakList));
}

void EnggDiffFittingViewQtWidget::addRunNoItem(std::string runNo) {
  m_ui.listWidget_fitting_run_num->addItem(QString::fromStdString(runNo));
}

std::vector<std::string> EnggDiffFittingViewQtWidget::getFittingRunNumVec() {
  return m_fitting_runno_dir_vec;
}

void EnggDiffFittingViewQtWidget::setFittingRunNumVec(
    std::vector<std::string> assignVec) {
  // holds all the directories required
  m_fitting_runno_dir_vec.clear();
  m_fitting_runno_dir_vec = assignVec;
}

void EnggDiffFittingViewQtWidget::setPeakPick() {
  auto bk2bk =
      FunctionFactory::Instance().createFunction("BackToBackExponential");
  auto bk2bkFunc = boost::dynamic_pointer_cast<IPeakFunction>(bk2bk);
  // set the peak to BackToBackExponential function
  setPeakPicker(bk2bkFunc);
  setPeakPickerEnabled(true);
}

void EnggDiffFittingViewQtWidget::clearPeakList() {
  m_ui.lineEdit_fitting_peaks->clear();
}

} // namespace CustomInterfaces
} // namespace MantidQt
