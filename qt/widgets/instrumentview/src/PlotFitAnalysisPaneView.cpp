// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPaneView.h"

#include <tuple>
#include <utility>

#include <QLabel>
#include <QMessageBox>
#include <QRegExpValidator>
#include <QSizePolicy>
#include <QSpacerItem>
#include <QSplitter>
#include <QVBoxLayout>

namespace {

std::tuple<QString, QString> getPeakCentreUIProperties(const QString &fitStatus) {
  if (fitStatus.contains("success")) {
    return {"QLineEdit { background: rgb(179, 240, 153) }", "PeakCentre value is from a successful fit."};
  } else if (fitStatus.contains("Failed to converge")) {
    return {"QLineEdit { background: rgb(251, 210, 121) }", "There was a fitting warning: " + fitStatus};
  } else if (!fitStatus.isEmpty()) {
    return {"QLineEdit { background: rgb(251, 139, 131) }", "There was a fitting error: " + fitStatus};
  }
  return {"", "PeakCentre value is not from a fit."};
}

} // namespace

namespace MantidQt::MantidWidgets {

PlotFitAnalysisPaneView::PlotFitAnalysisPaneView(const double &start, const double &end, QWidget *parent)
    : QWidget(parent), m_plot(nullptr), m_start(nullptr), m_end(nullptr), m_fitButton(nullptr),
      m_peakCentreObservable(new Observable()), m_fitObservable(new Observable()),
      m_updateEstimateObservable(new Observable()) {
  setupPlotFitSplitter(start, end);
}

void PlotFitAnalysisPaneView::setupPlotFitSplitter(const double &start, const double &end) {
  auto layout = new QHBoxLayout(this);
  auto splitter = new QSplitter(Qt::Vertical);

  m_plot = new MantidWidgets::PreviewPlot();
  m_plot->setCanvasColour(Qt::white);
  splitter->addWidget(m_plot);

  splitter->addWidget(createFitPane(start, end));

  layout->addWidget(splitter);
}

QWidget *PlotFitAnalysisPaneView::createFitPane(const double &start, const double &end) {
  auto fitPane = new QWidget();
  auto fitPaneLayout = new QVBoxLayout(fitPane);

  auto fitRangeWidget = setupFitRangeWidget(start, end);
  fitPaneLayout->addWidget(fitRangeWidget);

  auto fitButtonsWidget = setupFitButtonsWidget();
  fitPaneLayout->addWidget(fitButtonsWidget);

  auto peakCentreWidget = setupPeakCentreWidget((start + end) / 2.0);
  fitPaneLayout->addWidget(peakCentreWidget);

  return fitPane;
}

QWidget *PlotFitAnalysisPaneView::setupFitRangeWidget(const double start, const double end) {
  auto *rangeWidget = new QWidget();
  auto *rangeLayout = new QHBoxLayout(rangeWidget);

  m_start = new QLineEdit(QString::number(start));
  m_start->setValidator(new QDoubleValidator(m_start));

  m_end = new QLineEdit(QString::number(end));
  m_end->setValidator(new QDoubleValidator(m_end));

  rangeLayout->addWidget(new QLabel("Fit from:"));
  rangeLayout->addWidget(m_start);
  rangeLayout->addWidget(new QLabel("to:"));
  rangeLayout->addWidget(m_end);
  return rangeWidget;
}

QWidget *PlotFitAnalysisPaneView::setupFitButtonsWidget() {
  auto fitButtonsWidget = new QWidget();
  auto fitButtonsLayout = new QHBoxLayout(fitButtonsWidget);

  m_fitButton = new QPushButton("Fit");
  m_updateEstimateButton = new QPushButton("Update Estimate");

  connect(m_fitButton, SIGNAL(clicked()), this, SLOT(notifyFitClicked()));
  connect(m_updateEstimateButton, SIGNAL(clicked()), this, SLOT(notifyUpdateEstimateClicked()));

  fitButtonsLayout->addWidget(m_fitButton);
  fitButtonsLayout->addItem(new QSpacerItem(80, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
  fitButtonsLayout->addWidget(m_updateEstimateButton);
  return fitButtonsWidget;
}

QWidget *PlotFitAnalysisPaneView::setupPeakCentreWidget(const double centre) {
  auto *peakCentreWidget = new QWidget();
  auto *peakCentreLayout = new QHBoxLayout(peakCentreWidget);

  m_peakCentre = new QLineEdit(QString::number(centre));
  m_peakCentre->setValidator(new QDoubleValidator(m_peakCentre));

  connect(m_peakCentre, SIGNAL(editingFinished()), this, SLOT(notifyPeakCentreEditingFinished()));

  peakCentreLayout->addWidget(new QLabel("Peak Centre:"));
  peakCentreLayout->addWidget(m_peakCentre);

  return peakCentreWidget;
}

void PlotFitAnalysisPaneView::notifyPeakCentreEditingFinished() { m_peakCentreObservable->notify(); }

void PlotFitAnalysisPaneView::notifyFitClicked() { m_fitObservable->notify(); }

void PlotFitAnalysisPaneView::notifyUpdateEstimateClicked() { m_updateEstimateObservable->notify(); }

void PlotFitAnalysisPaneView::addSpectrum(const std::string &wsName) {
  m_plot->addSpectrum("Extracted Data", wsName.c_str(), 0, Qt::black);
}
void PlotFitAnalysisPaneView::addFitSpectrum(const std::string &wsName) {
  m_plot->addSpectrum("Fitted Data", wsName.c_str(), 1, Qt::red);
}

std::pair<double, double> PlotFitAnalysisPaneView::getRange() {
  return std::make_pair(m_start->text().toDouble(), m_end->text().toDouble());
}

void PlotFitAnalysisPaneView::setPeakCentre(const double centre) { m_peakCentre->setText(QString::number(centre)); }

double PlotFitAnalysisPaneView::peakCentre() const { return m_peakCentre->text().toDouble(); }

void PlotFitAnalysisPaneView::setPeakCentreStatus(const std::string &status) {
  const auto [stylesheet, tooltip] = getPeakCentreUIProperties(QString::fromStdString(status));
  m_peakCentre->setStyleSheet(stylesheet);
  m_peakCentre->setToolTip(tooltip);
}

void PlotFitAnalysisPaneView::displayWarning(const std::string &message) {
  QMessageBox::warning(this, "Warning!", message.c_str());
}

} // namespace MantidQt::MantidWidgets
