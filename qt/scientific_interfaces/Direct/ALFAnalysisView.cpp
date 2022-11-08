// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFAnalysisView.h"
#include "ALFAnalysisPresenter.h"

#include "MantidQtWidgets/Plotting/PreviewPlot.h"

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

std::tuple<QString, QString> getPeakCentreUIProperties(QString const &fitStatus) {
  QString color("black"), status("");
  if (fitStatus.contains("success")) {
    color = "green", status = "Fit success";
  } else if (fitStatus.contains("Failed to converge")) {
    color = "darkOrange", status = fitStatus;
  } else if (!fitStatus.isEmpty()) {
    color = "red", status = fitStatus;
  }
  return {"QLabel { color: " + color + "; }", status};
}

} // namespace

namespace MantidQt::CustomInterfaces {

ALFAnalysisView::ALFAnalysisView(double const start, double const end, QWidget *parent)
    : QWidget(parent), m_plot(nullptr), m_start(nullptr), m_end(nullptr), m_fitButton(nullptr) {
  setupPlotFitSplitter(start, end);
}

QWidget *ALFAnalysisView::getView() { return this; }

void ALFAnalysisView::subscribePresenter(IALFAnalysisPresenter *presenter) { m_presenter = presenter; }

void ALFAnalysisView::setupPlotFitSplitter(double const start, double const end) {
  auto layout = new QHBoxLayout(this);
  auto splitter = new QSplitter(Qt::Vertical);

  m_plot = new MantidWidgets::PreviewPlot();
  m_plot->setCanvasColour(Qt::white);
  splitter->addWidget(m_plot);

  splitter->addWidget(createFitPane(start, end));

  layout->addWidget(splitter);
}

QWidget *ALFAnalysisView::createFitPane(double const start, double const end) {
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

QWidget *ALFAnalysisView::setupFitRangeWidget(double const start, double const end) {
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

QWidget *ALFAnalysisView::setupFitButtonsWidget() {
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

QWidget *ALFAnalysisView::setupPeakCentreWidget(double const centre) {
  auto *peakCentreWidget = new QWidget();
  auto *peakCentreLayout = new QGridLayout(peakCentreWidget);

  m_peakCentre = new QLineEdit(QString::number(centre));
  m_peakCentre->setValidator(new QDoubleValidator(m_peakCentre));

  connect(m_peakCentre, SIGNAL(editingFinished()), this, SLOT(notifyPeakCentreEditingFinished()));

  peakCentreLayout->addWidget(new QLabel("Peak Centre:"), 0, 0);
  peakCentreLayout->addWidget(m_peakCentre, 0, 1);

  m_fitStatus = new QLabel("");
  m_fitStatus->setAlignment(Qt::AlignRight);
  setPeakCentreStatus("");

  peakCentreLayout->addWidget(m_fitStatus, 1, 0, 1, 2);

  return peakCentreWidget;
}

void ALFAnalysisView::notifyPeakCentreEditingFinished() { m_presenter->notifyPeakCentreEditingFinished(); }

void ALFAnalysisView::notifyFitClicked() { m_presenter->notifyFitClicked(); }

void ALFAnalysisView::notifyUpdateEstimateClicked() { m_presenter->notifyUpdateEstimateClicked(); }

void ALFAnalysisView::addSpectrum(std::string const &wsName) {
  m_plot->addSpectrum("Extracted Data", wsName.c_str(), 0, Qt::black);
}

void ALFAnalysisView::addFitSpectrum(std::string const &wsName) {
  m_plot->addSpectrum("Fitted Data", wsName.c_str(), 1, Qt::red);
}

std::pair<double, double> ALFAnalysisView::getRange() const {
  return std::make_pair(m_start->text().toDouble(), m_end->text().toDouble());
}

void ALFAnalysisView::setPeakCentre(double const centre) { m_peakCentre->setText(QString::number(centre)); }

double ALFAnalysisView::peakCentre() const { return m_peakCentre->text().toDouble(); }

void ALFAnalysisView::setPeakCentreStatus(std::string const &status) {
  const auto [stylesheet, tooltip] = getPeakCentreUIProperties(QString::fromStdString(status));
  m_fitStatus->setStyleSheet(stylesheet);
  m_fitStatus->setText(tooltip);
  m_fitStatus->setToolTip(tooltip);
}

void ALFAnalysisView::displayWarning(std::string const &message) {
  QMessageBox::warning(this, "Warning!", message.c_str());
}

} // namespace MantidQt::CustomInterfaces
