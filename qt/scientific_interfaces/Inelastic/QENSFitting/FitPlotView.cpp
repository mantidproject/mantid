// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FitPlotView.h"
#include "FitPlotPresenter.h"

#include "MantidQtIcons/Icon.h"

#include <boost/numeric/conversion/cast.hpp>

#include <QMessageBox>
#include <QSignalBlocker>
#include <QTimer>

#include <gtest/gtest-param-test.h>
#include <limits>

namespace {

QHash<QString, QVariant> tightLayoutKwargs() {
  QHash<QString, QVariant> kwargs;
  kwargs.insert("pad", 0);
  return kwargs;
}

} // namespace

namespace MantidQt::CustomInterfaces::Inelastic {

using namespace MantidWidgets;

FitPlotView::FitPlotView(QWidget *parent)
    : API::MantidWidget(parent), m_plotForm(new Ui::FitPreviewPlot), m_presenter() {
  m_plotForm->setupUi(this);

  connect(m_plotForm->cbDataSelection, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
          &FitPlotView::notifySelectedFitDataChanged);
  connect(m_plotForm->spPlotSpectrum, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,
          &FitPlotView::notifyDelayedPlotSpectrumChanged);
  connect(m_plotForm->cbPlotSpectrum,
          static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::currentIndexChanged), this,
          static_cast<void (FitPlotView::*)(const QString &)>(&FitPlotView::notifyPlotSpectrumChanged));
  connect(m_plotForm->ckPlotGuess, &QCheckBox::stateChanged, this, &FitPlotView::notifyPlotGuessChanged);
  connect(m_plotForm->pbPlotPreview, &QPushButton::clicked, this, &FitPlotView::notifyPlotCurrentPreview);
  connect(m_plotForm->pbFitSingle, &QPushButton::clicked, this, &FitPlotView::notifyFitSelectedSpectrum);

  // Create a Splitter and place two plots within the splitter layout
  createSplitterWithPlots();

  addFitRangeSelector();
  addBackgroundRangeSelector();
  addHWHMRangeSelector();
}

void FitPlotView::subscribePresenter(IFitPlotPresenter *presenter) { m_presenter = presenter; }

void FitPlotView::createSplitterWithPlots() {
  createSplitter();
  m_splitter->addWidget(createTopPlot());
  m_splitter->addWidget(createBottomPlot());

  m_plotForm->gridLayout->addWidget(m_splitter.get(), 0, 0, 1, 1);
}

void FitPlotView::createSplitter() {
  auto const dragIcon = Icons::getIcon("mdi.dots-horizontal");
  m_splitter = std::make_unique<Splitter>(dragIcon);
  m_splitter->setOrientation(Qt::Vertical);
  m_splitter->setStyleSheet("QSplitter::handle { background-color: transparent; }");
}

PreviewPlot *FitPlotView::createTopPlot() {
  m_topPlot = std::make_unique<PreviewPlot>(m_splitter.get());
  return createPlot(m_topPlot.get(), QSize(0, 125), 0, 10);
}

PreviewPlot *FitPlotView::createBottomPlot() {
  m_bottomPlot = std::make_unique<PreviewPlot>(m_splitter.get());
  return createPlot(m_bottomPlot.get(), QSize(0, 75), 0, 6);
}

PreviewPlot *FitPlotView::createPlot(PreviewPlot *plot, QSize const &minimumSize, unsigned char horizontalStretch,
                                     unsigned char verticalStretch) const {
  setPlotSizePolicy(plot, horizontalStretch, verticalStretch);

  plot->setMinimumSize(minimumSize);
  plot->setProperty("showLegend", QVariant(true));
  plot->setProperty("canvasColour", QVariant(QColor(255, 255, 255)));

  // Avoids squished plots
  plot->setTightLayout(tightLayoutKwargs());

  return plot;
}

void FitPlotView::setPlotSizePolicy(PreviewPlot *plot, unsigned char horizontalStretch,
                                    unsigned char verticalStretch) const {
  QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
  sizePolicy.setHorizontalStretch(horizontalStretch);
  sizePolicy.setVerticalStretch(verticalStretch);
  sizePolicy.setHeightForWidth(plot->sizePolicy().hasHeightForWidth());
  plot->setSizePolicy(sizePolicy);
}

FitPlotView::~FitPlotView() {
  m_topPlot.reset();
  m_bottomPlot.reset();
  m_splitter.reset();
}

void FitPlotView::watchADS(bool watch) {
  m_topPlot->watchADS(watch);
  m_bottomPlot->watchADS(watch);
}

std::string FitPlotView::getSpectrumText() const { return m_plotForm->cbPlotSpectrum->currentText().toStdString(); }

WorkspaceIndex FitPlotView::getSelectedSpectrum() const {
  if (m_plotForm->swPlotSpectrum->currentIndex() == 0)
    return WorkspaceIndex{static_cast<size_t>(m_plotForm->spPlotSpectrum->value())};
  else if (m_plotForm->cbPlotSpectrum->count() != 0)
    return WorkspaceIndex{std::stoul(getSpectrumText())};
  return WorkspaceIndex{0};
}

WorkspaceID FitPlotView::getSelectedDataIndex() const {
  return WorkspaceID{static_cast<size_t>(m_plotForm->cbDataSelection->currentIndex())};
}

WorkspaceID FitPlotView::dataSelectionSize() const {
  return WorkspaceID{static_cast<size_t>(m_plotForm->cbDataSelection->count())};
}

bool FitPlotView::isPlotGuessChecked() const { return m_plotForm->ckPlotGuess->isChecked(); }

void FitPlotView::setAvailableSpectra(WorkspaceIndex minimum, WorkspaceIndex maximum) {
  m_plotForm->swPlotSpectrum->setCurrentIndex(0);
  m_plotForm->spPlotSpectrum->setMinimum(boost::numeric_cast<int>(minimum.value));
  m_plotForm->spPlotSpectrum->setMaximum(boost::numeric_cast<int>(maximum.value));
}

void FitPlotView::setAvailableSpectra(const std::vector<WorkspaceIndex>::const_iterator &from,
                                      const std::vector<WorkspaceIndex>::const_iterator &to) {
  m_plotForm->swPlotSpectrum->setCurrentIndex(1);
  m_plotForm->cbPlotSpectrum->clear();

  for (auto spectrum = from; spectrum < to; ++spectrum)
    m_plotForm->cbPlotSpectrum->addItem(QString::number(spectrum->value));
}

void FitPlotView::setMinimumSpectrum(int minimum) { m_plotForm->spPlotSpectrum->setMinimum(minimum); }

void FitPlotView::setMaximumSpectrum(int maximum) { m_plotForm->spPlotSpectrum->setMaximum(maximum); }

void FitPlotView::setPlotSpectrum(WorkspaceIndex spectrum) {
  QSignalBlocker blocker(m_plotForm->spPlotSpectrum);
  QSignalBlocker comboBlocker(m_plotForm->cbPlotSpectrum);
  m_plotForm->spPlotSpectrum->setValue(static_cast<int>(spectrum.value));
  auto index = m_plotForm->cbPlotSpectrum->findText(QString::number(spectrum.value));
  m_plotForm->cbPlotSpectrum->setCurrentIndex(index);
}

void FitPlotView::setBackgroundLevel(double value) {
  auto selector = m_topPlot->getSingleSelector("Background");
  QSignalBlocker blocker(selector);
  selector->setPosition(value);
}

void FitPlotView::setFitRange(double minimum, double maximum) {
  auto selector = m_topPlot->getRangeSelector("FitRange");
  QSignalBlocker blocker(selector);
  selector->setRange(minimum, maximum);
}

void FitPlotView::setFitRangeMinimum(double minimum) {
  auto selector = m_topPlot->getRangeSelector("FitRange");
  QSignalBlocker blocker(selector);
  selector->setMinimum(minimum);
}

void FitPlotView::setFitRangeMaximum(double maximum) {
  auto selector = m_topPlot->getRangeSelector("FitRange");
  QSignalBlocker blocker(selector);
  selector->setMaximum(maximum);
}

void FitPlotView::setFitRangeBounds(std::pair<double, double> const &bounds) {
  auto selector = m_topPlot->getRangeSelector("FitRange");
  selector->setBounds(bounds.first, bounds.second);
}

void FitPlotView::appendToDataSelection(const std::string &dataName) {
  QSignalBlocker blocker(m_plotForm->cbDataSelection);
  m_plotForm->cbDataSelection->addItem(QString::fromStdString(dataName));
}

void FitPlotView::setNameInDataSelection(const std::string &dataName, WorkspaceID workspaceID) {
  m_plotForm->cbDataSelection->setItemText(static_cast<int>(workspaceID.value), QString::fromStdString(dataName));
}

void FitPlotView::clearDataSelection() {
  QSignalBlocker blocker(m_plotForm->cbDataSelection);
  m_plotForm->cbDataSelection->clear();
}

void FitPlotView::plotInTopPreview(const QString &name, Mantid::API::MatrixWorkspace_sptr workspace,
                                   WorkspaceIndex spectrum, Qt::GlobalColor colour) {
  m_topPlot->addSpectrum(name, workspace, spectrum.value, colour);
}

void FitPlotView::plotInBottomPreview(const QString &name, Mantid::API::MatrixWorkspace_sptr workspace,
                                      WorkspaceIndex spectrum, Qt::GlobalColor colour) {
  m_bottomPlot->addSpectrum(name, workspace, spectrum.value, colour);
}

void FitPlotView::removeFromTopPreview(const QString &name) { m_topPlot->removeSpectrum(name); }

void FitPlotView::removeFromBottomPreview(const QString &name) { m_bottomPlot->removeSpectrum(name); }

void FitPlotView::enablePlotGuess(bool enable) {
  if (!enable)
    m_plotForm->ckPlotGuess->setChecked(enable);
  m_plotForm->ckPlotGuess->setEnabled(enable);
}

void FitPlotView::enableSpectrumSelection(bool enable) {
  if (!enable)
    m_plotForm->spPlotSpectrum->setValue(0);
  m_plotForm->spPlotSpectrum->setEnabled(enable);
}

void FitPlotView::enableFitRangeSelection(bool enable) { m_topPlot->getRangeSelector("FitRange")->setVisible(enable); }

void FitPlotView::setFitSingleSpectrumText(QString const &text) { m_plotForm->pbFitSingle->setText(text); }

void FitPlotView::setFitSingleSpectrumEnabled(bool enable) { m_plotForm->pbFitSingle->setEnabled(enable); }

void FitPlotView::clearTopPreview() { m_topPlot->clear(); }

void FitPlotView::clearBottomPreview() { m_bottomPlot->clear(); }

void FitPlotView::clearPreviews() {
  clearTopPreview();
  clearBottomPreview();
}

void FitPlotView::setHWHMRange(double minimum, double maximum) {
  auto selector = m_topPlot->getRangeSelector("HWHM");
  QSignalBlocker blocker(selector);
  selector->setRange(minimum, maximum);
}

void FitPlotView::setHWHMMinimum(double minimum) {
  auto selector = m_topPlot->getRangeSelector("HWHM");
  QSignalBlocker blocker(selector);
  selector->setMinimum(minimum);
}

void FitPlotView::setHWHMMaximum(double maximum) {
  auto selector = m_topPlot->getRangeSelector("HWHM");
  QSignalBlocker blocker(selector);
  selector->setMaximum(maximum);
}

void FitPlotView::addFitRangeSelector() {
  auto fitRangeSelector = m_topPlot->addRangeSelector("FitRange");
  fitRangeSelector->setBounds(-DBL_MAX, DBL_MAX);

  connect(fitRangeSelector, &RangeSelector::minValueChanged, this, &FitPlotView::notifyStartXChanged);
  connect(fitRangeSelector, &RangeSelector::maxValueChanged, this, &FitPlotView::notifyEndXChanged);
}

void FitPlotView::addBackgroundRangeSelector() {
  auto backRangeSelector = m_topPlot->addSingleSelector("Background", SingleSelector::YSINGLE);
  backRangeSelector->setVisible(false);
  backRangeSelector->setColour(Qt::darkGreen);
  backRangeSelector->setLowerBound(0.0);
  backRangeSelector->setUpperBound(10.0);

  connect(backRangeSelector, &SingleSelector::valueChanged, this, &FitPlotView::notifyBackgroundChanged);
  connect(backRangeSelector, &SingleSelector::resetScientificBounds, this, &FitPlotView::setBackgroundBounds);
}

void FitPlotView::setBackgroundBounds() {
  auto backRangeSelector = m_topPlot->getSingleSelector("Background");
  backRangeSelector->setLowerBound(0.0);
  backRangeSelector->setUpperBound(10.0);
}

void FitPlotView::addHWHMRangeSelector() {
  auto hwhmRangeSelector = m_topPlot->addRangeSelector("HWHM");
  hwhmRangeSelector->setBounds(-DBL_MAX, DBL_MAX);
  hwhmRangeSelector->setColour(Qt::red);
  hwhmRangeSelector->setRange(0.0, 0.0);
  hwhmRangeSelector->setVisible(false);

  connect(hwhmRangeSelector, &RangeSelector::minValueChanged, this, &FitPlotView::notifyHWHMMinimumChanged);
  connect(hwhmRangeSelector, &RangeSelector::maxValueChanged, this, &FitPlotView::notifyHWHMMaximumChanged);
  connect(hwhmRangeSelector, &RangeSelector::selectionChanged, this, &FitPlotView::notifyFWHMChanged);
}

void FitPlotView::setBackgroundRangeVisible(bool visible) {
  m_topPlot->getSingleSelector("Background")->setVisible(visible);
}

void FitPlotView::setHWHMRangeVisible(bool visible) { m_topPlot->getRangeSelector("HWHM")->setVisible(visible); }

void FitPlotView::allowRedraws(bool state) {
  m_topPlot->allowRedraws(state);
  m_bottomPlot->allowRedraws(state);
}

void FitPlotView::redrawPlots() {
  m_topPlot->replot();
  m_bottomPlot->replot();
}

void FitPlotView::displayMessage(const std::string &message) const {
  QMessageBox::information(parentWidget(), "MantidPlot - Warning", QString::fromStdString(message));
}

void FitPlotView::notifySelectedFitDataChanged(int index) {
  if (index >= 0)
    m_presenter->handleSelectedFitDataChanged(WorkspaceID{static_cast<size_t>(index)});
}

// Required due to a bug in qt causing the valueChanged signal to be emitted
// twice due to the long amount of time taken to complete the necessary actions
void FitPlotView::notifyDelayedPlotSpectrumChanged() {
  QTimer::singleShot(150, this, SLOT(notifyPlotSpectrumChanged()));
}

void FitPlotView::notifyPlotSpectrumChanged() {
  m_presenter->handlePlotSpectrumChanged(
      WorkspaceIndex{boost::numeric_cast<size_t>(m_plotForm->spPlotSpectrum->value())});
}

void FitPlotView::notifyPlotSpectrumChanged(const QString &spectrum) {
  bool successState{false};
  int spectrumInt = spectrum.toInt(&successState);
  if (successState)
    m_presenter->handlePlotSpectrumChanged(WorkspaceIndex{static_cast<size_t>(spectrumInt)});
}

void FitPlotView::notifyPlotGuessChanged(int doPlotGuess) { m_presenter->handlePlotGuess(doPlotGuess == Qt::Checked); }

void FitPlotView::notifyPlotCurrentPreview() { m_presenter->handlePlotCurrentPreview(); }

void FitPlotView::notifyFitSelectedSpectrum() { m_presenter->handleFitSingleSpectrum(); }

void FitPlotView::notifyStartXChanged(double value) { m_presenter->handleStartXChanged(value); }

void FitPlotView::notifyEndXChanged(double value) { m_presenter->handleEndXChanged(value); }

void FitPlotView::notifyHWHMMinimumChanged(double value) { m_presenter->handleHWHMMinimumChanged(value); }

void FitPlotView::notifyHWHMMaximumChanged(double value) { m_presenter->handleHWHMMaximumChanged(value); }

void FitPlotView::notifyFWHMChanged(double minimum, double maximum) {
  m_presenter->handleFWHMChanged(minimum, maximum);
}

void FitPlotView::notifyBackgroundChanged(double value) { m_presenter->handleBackgroundChanged(value); }

} // namespace MantidQt::CustomInterfaces::Inelastic
