// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectFitPlotView.h"
#include "IndirectFitPlotPresenter.h"

#include "MantidQtIcons/Icon.h"

#include <boost/numeric/conversion/cast.hpp>

#include <QMessageBox>
#include <QSignalBlocker>
#include <QTimer>

#include <limits>

namespace {

QHash<QString, QVariant> tightLayoutKwargs() {
  QHash<QString, QVariant> kwargs;
  kwargs.insert("pad", 0);
  return kwargs;
}

} // namespace

namespace MantidQt::CustomInterfaces::IDA {

using namespace MantidWidgets;

IndirectFitPlotView::IndirectFitPlotView(QWidget *parent)
    : IIndirectFitPlotView(parent), m_plotForm(new Ui::IndirectFitPreviewPlot), m_presenter() {
  m_plotForm->setupUi(this);

  connect(m_plotForm->cbDataSelection, SIGNAL(currentIndexChanged(int)), this, SLOT(notifySelectedFitDataChanged(int)));
  connect(m_plotForm->spPlotSpectrum, SIGNAL(valueChanged(int)), this, SLOT(notifyDelayedPlotSpectrumChanged()));

  connect(m_plotForm->cbPlotSpectrum, SIGNAL(currentIndexChanged(const QString &)), this,
          SLOT(notifyPlotSpectrumChanged(const QString &)));
  connect(m_plotForm->ckPlotGuess, SIGNAL(stateChanged(int)), this, SLOT(notifyPlotGuessChanged(int)));
  connect(m_plotForm->pbPlotPreview, SIGNAL(clicked()), this, SLOT(notifyPlotCurrentPreview()));
  connect(m_plotForm->pbFitSingle, SIGNAL(clicked()), this, SLOT(notifyFitSelectedSpectrum()));

  // Create a Splitter and place two plots within the splitter layout
  createSplitterWithPlots();

  addFitRangeSelector();
  addBackgroundRangeSelector();
  addHWHMRangeSelector();
}

void IndirectFitPlotView::subscribePresenter(IIndirectFitPlotPresenter *presenter) { m_presenter = presenter; }

void IndirectFitPlotView::createSplitterWithPlots() {
  createSplitter();
  m_splitter->addWidget(createTopPlot());
  m_splitter->addWidget(createBottomPlot());

  m_plotForm->gridLayout->addWidget(m_splitter.get(), 0, 0, 1, 1);
}

void IndirectFitPlotView::createSplitter() {
  auto const dragIcon = Icons::getIcon("mdi.dots-horizontal");
  m_splitter = std::make_unique<Splitter>(dragIcon);
  m_splitter->setOrientation(Qt::Vertical);
  m_splitter->setStyleSheet("QSplitter::handle { background-color: transparent; }");
}

PreviewPlot *IndirectFitPlotView::createTopPlot() {
  m_topPlot = std::make_unique<PreviewPlot>(m_splitter.get());
  return createPlot(m_topPlot.get(), QSize(0, 125), 0, 10);
}

PreviewPlot *IndirectFitPlotView::createBottomPlot() {
  m_bottomPlot = std::make_unique<PreviewPlot>(m_splitter.get());
  return createPlot(m_bottomPlot.get(), QSize(0, 75), 0, 6);
}

PreviewPlot *IndirectFitPlotView::createPlot(PreviewPlot *plot, QSize const &minimumSize,
                                             unsigned char horizontalStretch, unsigned char verticalStretch) const {
  setPlotSizePolicy(plot, horizontalStretch, verticalStretch);

  plot->setMinimumSize(minimumSize);
  plot->setProperty("showLegend", QVariant(true));
  plot->setProperty("canvasColour", QVariant(QColor(255, 255, 255)));

  // Avoids squished plots
  plot->setTightLayout(tightLayoutKwargs());

  return plot;
}

void IndirectFitPlotView::setPlotSizePolicy(PreviewPlot *plot, unsigned char horizontalStretch,
                                            unsigned char verticalStretch) const {
  QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
  sizePolicy.setHorizontalStretch(horizontalStretch);
  sizePolicy.setVerticalStretch(verticalStretch);
  sizePolicy.setHeightForWidth(plot->sizePolicy().hasHeightForWidth());
  plot->setSizePolicy(sizePolicy);
}

IndirectFitPlotView::~IndirectFitPlotView() {
  m_topPlot.reset();
  m_bottomPlot.reset();
  m_splitter.reset();
}

void IndirectFitPlotView::watchADS(bool watch) {
  m_topPlot->watchADS(watch);
  m_bottomPlot->watchADS(watch);
}

std::string IndirectFitPlotView::getSpectrumText() const {
  return m_plotForm->cbPlotSpectrum->currentText().toStdString();
}

WorkspaceIndex IndirectFitPlotView::getSelectedSpectrum() const {
  if (m_plotForm->swPlotSpectrum->currentIndex() == 0)
    return WorkspaceIndex{static_cast<size_t>(m_plotForm->spPlotSpectrum->value())};
  else if (m_plotForm->cbPlotSpectrum->count() != 0)
    return WorkspaceIndex{std::stoul(getSpectrumText())};
  return WorkspaceIndex{0};
}

WorkspaceID IndirectFitPlotView::getSelectedDataIndex() const {
  return WorkspaceID{static_cast<size_t>(m_plotForm->cbDataSelection->currentIndex())};
}

WorkspaceID IndirectFitPlotView::dataSelectionSize() const {
  return WorkspaceID{static_cast<size_t>(m_plotForm->cbDataSelection->count())};
}

bool IndirectFitPlotView::isPlotGuessChecked() const { return m_plotForm->ckPlotGuess->isChecked(); }

void IndirectFitPlotView::setAvailableSpectra(WorkspaceIndex minimum, WorkspaceIndex maximum) {
  m_plotForm->swPlotSpectrum->setCurrentIndex(0);
  m_plotForm->spPlotSpectrum->setMinimum(boost::numeric_cast<int>(minimum.value));
  m_plotForm->spPlotSpectrum->setMaximum(boost::numeric_cast<int>(maximum.value));
}

void IndirectFitPlotView::setAvailableSpectra(const std::vector<WorkspaceIndex>::const_iterator &from,
                                              const std::vector<WorkspaceIndex>::const_iterator &to) {
  m_plotForm->swPlotSpectrum->setCurrentIndex(1);
  m_plotForm->cbPlotSpectrum->clear();

  for (auto spectrum = from; spectrum < to; ++spectrum)
    m_plotForm->cbPlotSpectrum->addItem(QString::number(spectrum->value));
}

void IndirectFitPlotView::setMinimumSpectrum(int minimum) { m_plotForm->spPlotSpectrum->setMinimum(minimum); }

void IndirectFitPlotView::setMaximumSpectrum(int maximum) { m_plotForm->spPlotSpectrum->setMaximum(maximum); }

void IndirectFitPlotView::setPlotSpectrum(WorkspaceIndex spectrum) {
  QSignalBlocker blocker(m_plotForm->spPlotSpectrum);
  QSignalBlocker comboBlocker(m_plotForm->cbPlotSpectrum);
  m_plotForm->spPlotSpectrum->setValue(static_cast<int>(spectrum.value));
  auto index = m_plotForm->cbPlotSpectrum->findText(QString::number(spectrum.value));
  m_plotForm->cbPlotSpectrum->setCurrentIndex(index);
}

void IndirectFitPlotView::disableSpectrumPlotSelection() {
  m_plotForm->spPlotSpectrum->setEnabled(false);
  m_plotForm->cbPlotSpectrum->setEnabled(false);
}

void IndirectFitPlotView::setBackgroundLevel(double value) {
  auto selector = m_topPlot->getSingleSelector("Background");
  QSignalBlocker blocker(selector);
  selector->setPosition(value);
}

void IndirectFitPlotView::setFitRange(double minimum, double maximum) {
  auto selector = m_topPlot->getRangeSelector("FitRange");
  QSignalBlocker blocker(selector);
  selector->setRange(minimum, maximum);
}

void IndirectFitPlotView::setFitRangeMinimum(double minimum) {
  auto selector = m_topPlot->getRangeSelector("FitRange");
  QSignalBlocker blocker(selector);
  selector->setMinimum(minimum);
}

void IndirectFitPlotView::setFitRangeMaximum(double maximum) {
  auto selector = m_topPlot->getRangeSelector("FitRange");
  QSignalBlocker blocker(selector);
  selector->setMaximum(maximum);
}

void IndirectFitPlotView::setFitRangeBounds(std::pair<double, double> const &bounds) {
  auto selector = m_topPlot->getRangeSelector("FitRange");
  selector->setBounds(bounds.first, bounds.second);
}

void IndirectFitPlotView::appendToDataSelection(const std::string &dataName) {
  QSignalBlocker blocker(m_plotForm->cbDataSelection);
  m_plotForm->cbDataSelection->addItem(QString::fromStdString(dataName));
}

void IndirectFitPlotView::setNameInDataSelection(const std::string &dataName, WorkspaceID workspaceID) {
  m_plotForm->cbDataSelection->setItemText(static_cast<int>(workspaceID.value), QString::fromStdString(dataName));
}

void IndirectFitPlotView::clearDataSelection() { m_plotForm->cbDataSelection->clear(); }

void IndirectFitPlotView::plotInTopPreview(const QString &name, Mantid::API::MatrixWorkspace_sptr workspace,
                                           WorkspaceIndex spectrum, Qt::GlobalColor colour) {
  m_topPlot->addSpectrum(name, workspace, spectrum.value, colour);
}

void IndirectFitPlotView::plotInBottomPreview(const QString &name, Mantid::API::MatrixWorkspace_sptr workspace,
                                              WorkspaceIndex spectrum, Qt::GlobalColor colour) {
  m_bottomPlot->addSpectrum(name, workspace, spectrum.value, colour);
}

void IndirectFitPlotView::removeFromTopPreview(const QString &name) { m_topPlot->removeSpectrum(name); }

void IndirectFitPlotView::removeFromBottomPreview(const QString &name) { m_bottomPlot->removeSpectrum(name); }

void IndirectFitPlotView::enablePlotGuess(bool enable) {
  if (!enable)
    m_plotForm->ckPlotGuess->setChecked(enable);
  m_plotForm->ckPlotGuess->setEnabled(enable);
}

void IndirectFitPlotView::enableSpectrumSelection(bool enable) {
  if (!enable)
    m_plotForm->spPlotSpectrum->setValue(0);
  m_plotForm->spPlotSpectrum->setEnabled(enable);
}

void IndirectFitPlotView::enableFitRangeSelection(bool enable) {
  m_topPlot->getRangeSelector("FitRange")->setVisible(enable);
}

void IndirectFitPlotView::setFitSingleSpectrumText(QString const &text) { m_plotForm->pbFitSingle->setText(text); }

void IndirectFitPlotView::setFitSingleSpectrumEnabled(bool enable) { m_plotForm->pbFitSingle->setEnabled(enable); }

void IndirectFitPlotView::clearTopPreview() { m_topPlot->clear(); }

void IndirectFitPlotView::clearBottomPreview() { m_bottomPlot->clear(); }

void IndirectFitPlotView::clearPreviews() {
  clearTopPreview();
  clearBottomPreview();
}

void IndirectFitPlotView::setHWHMRange(double minimum, double maximum) {
  auto selector = m_topPlot->getRangeSelector("HWHM");
  QSignalBlocker blocker(selector);
  selector->setRange(minimum, maximum);
}

void IndirectFitPlotView::setHWHMMinimum(double minimum) {
  auto selector = m_topPlot->getRangeSelector("HWHM");
  QSignalBlocker blocker(selector);
  selector->setMinimum(minimum);
}

void IndirectFitPlotView::setHWHMMaximum(double maximum) {
  auto selector = m_topPlot->getRangeSelector("HWHM");
  QSignalBlocker blocker(selector);
  selector->setMaximum(maximum);
}

void IndirectFitPlotView::addFitRangeSelector() {
  auto fitRangeSelector = m_topPlot->addRangeSelector("FitRange");
  fitRangeSelector->setBounds(-DBL_MAX, DBL_MAX);

  connect(fitRangeSelector, SIGNAL(minValueChanged(double)), this, SLOT(notifyStartXChanged(double)));
  connect(fitRangeSelector, SIGNAL(maxValueChanged(double)), this, SLOT(notifyEndXChanged(double)));
}

void IndirectFitPlotView::addBackgroundRangeSelector() {
  auto backRangeSelector = m_topPlot->addSingleSelector("Background", SingleSelector::YSINGLE);
  backRangeSelector->setVisible(false);
  backRangeSelector->setColour(Qt::darkGreen);
  backRangeSelector->setLowerBound(0.0);
  backRangeSelector->setUpperBound(10.0);

  connect(backRangeSelector, SIGNAL(valueChanged(double)), this, SLOT(notifyBackgroundChanged(double)));
  connect(backRangeSelector, SIGNAL(resetScientificBounds()), this, SLOT(setBackgroundBounds()));
}

void IndirectFitPlotView::setBackgroundBounds() {
  auto backRangeSelector = m_topPlot->getSingleSelector("Background");
  backRangeSelector->setLowerBound(0.0);
  backRangeSelector->setUpperBound(10.0);
}

void IndirectFitPlotView::addHWHMRangeSelector() {
  auto hwhmRangeSelector = m_topPlot->addRangeSelector("HWHM");
  hwhmRangeSelector->setBounds(-DBL_MAX, DBL_MAX);
  hwhmRangeSelector->setColour(Qt::red);
  hwhmRangeSelector->setRange(0.0, 0.0);
  hwhmRangeSelector->setVisible(false);

  connect(hwhmRangeSelector, SIGNAL(minValueChanged(double)), this, SLOT(notifyHWHMMinimumChanged(double)));
  connect(hwhmRangeSelector, SIGNAL(maxValueChanged(double)), this, SLOT(notifyHWHMMaximumChanged(double)));
  connect(hwhmRangeSelector, SIGNAL(selectionChanged(double, double)), this, SLOT(notifyFWHMChanged(double, double)));
}

void IndirectFitPlotView::setBackgroundRangeVisible(bool visible) {
  m_topPlot->getSingleSelector("Background")->setVisible(visible);
}

void IndirectFitPlotView::setHWHMRangeVisible(bool visible) {
  m_topPlot->getRangeSelector("HWHM")->setVisible(visible);
}

void IndirectFitPlotView::allowRedraws(bool state) {
  m_topPlot->allowRedraws(state);
  m_bottomPlot->allowRedraws(state);
}

void IndirectFitPlotView::redrawPlots() {
  m_topPlot->replot();
  m_bottomPlot->replot();
}

void IndirectFitPlotView::displayMessage(const std::string &message) const {
  QMessageBox::information(parentWidget(), "MantidPlot - Warning", QString::fromStdString(message));
}

void IndirectFitPlotView::notifySelectedFitDataChanged(int index) {
  if (index >= 0)
    m_presenter->handleSelectedFitDataChanged(WorkspaceID{static_cast<size_t>(index)});
}

// Required due to a bug in qt causing the valueChanged signal to be emitted
// twice due to the long amount of time taken to complete the necessary actions
void IndirectFitPlotView::notifyDelayedPlotSpectrumChanged() {
  QTimer::singleShot(150, this, SLOT(emitPlotSpectrumChanged()));
}

void IndirectFitPlotView::notifyPlotSpectrumChanged() {
  m_presenter->handlePlotSpectrumChanged(
      WorkspaceIndex{boost::numeric_cast<size_t>(m_plotForm->spPlotSpectrum->value())});
}

void IndirectFitPlotView::notifyPlotSpectrumChanged(const QString &spectrum) {
  bool successState{false};
  int spectrumInt = spectrum.toInt(&successState);
  if (successState)
    m_presenter->handlePlotSpectrumChanged(WorkspaceIndex{static_cast<size_t>(spectrumInt)});
}

void IndirectFitPlotView::notifyPlotGuessChanged(int doPlotGuess) {
  m_presenter->handlePlotGuess(doPlotGuess == Qt::Checked);
}

void IndirectFitPlotView::notifyPlotCurrentPreview() { m_presenter->handlePlotCurrentPreview(); }

void IndirectFitPlotView::notifyFitSelectedSpectrum() { m_presenter->handleFitSingleSpectrum(); }

void IndirectFitPlotView::notifyStartXChanged(double value) { m_presenter->handleStartXChanged(value); }

void IndirectFitPlotView::notifyEndXChanged(double value) { m_presenter->handleEndXChanged(value); }

void IndirectFitPlotView::notifyHWHMMinimumChanged(double value) { m_presenter->handleHWHMMinimumChanged(value); }

void IndirectFitPlotView::notifyHWHMMaximumChanged(double value) { m_presenter->handleHWHMMaximumChanged(value); }

void IndirectFitPlotView::notifyFWHMChanged(double minimum, double maximum) {
  m_presenter->handleFWHMChanged(minimum, maximum);
}

void IndirectFitPlotView::notifyBackgroundChanged(double value) { m_presenter->handleBackgroundChanged(value); }

} // namespace MantidQt::CustomInterfaces::IDA
