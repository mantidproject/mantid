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

QString const TWO_THETA_TOOLTIP = "The average two theta of the extracted tubes. The two theta of a tube is taken to "
                                  "be the two theta at which the Out of Plane angle is closest to zero.";

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

QString constructAverageString(std::vector<double> const &twoThetas) {
  QString calculationStr("");
  for (auto i = 0u; i < twoThetas.size(); ++i) {
    if (i != 0)
      calculationStr += " + ";
    calculationStr += QString::number(twoThetas[i]);
  }
  return "\n=(" + calculationStr + ")/" + QString::number(twoThetas.size());
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

  auto resultsWidget = setupResultsWidget((start + end) / 2.0);
  fitPaneLayout->addWidget(resultsWidget);

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

QWidget *ALFAnalysisView::setupResultsWidget(double const centre) {
  auto *resultsWidget = new QWidget();
  auto *resultsLayout = new QGridLayout(resultsWidget);

  m_peakCentre = new QLineEdit(QString::number(centre));
  m_peakCentre->setValidator(new QDoubleValidator(m_peakCentre));

  connect(m_peakCentre, SIGNAL(editingFinished()), this, SLOT(notifyPeakCentreEditingFinished()));

  resultsLayout->addWidget(new QLabel("Peak Centre:"), 0, 0);
  resultsLayout->addWidget(m_peakCentre, 0, 1);

  m_fitStatus = new QLabel("");
  m_fitStatus->setAlignment(Qt::AlignRight);
  setPeakCentreStatus("");

  resultsLayout->addWidget(m_fitStatus, 1, 0, 1, 2);

  m_averageTwoTheta = new QLineEdit("-");
  m_averageTwoTheta->setReadOnly(true);
  m_averageTwoTheta->setToolTip(TWO_THETA_TOOLTIP);

  resultsLayout->addWidget(new QLabel("Two theta:"), 2, 0);
  resultsLayout->addWidget(m_averageTwoTheta, 2, 1);

  return resultsWidget;
}

void ALFAnalysisView::notifyPeakCentreEditingFinished() { m_presenter->notifyPeakCentreEditingFinished(); }

void ALFAnalysisView::notifyFitClicked() { m_presenter->notifyFitClicked(); }

void ALFAnalysisView::notifyUpdateEstimateClicked() { m_presenter->notifyUpdateEstimateClicked(); }

std::pair<double, double> ALFAnalysisView::getRange() const {
  return std::make_pair(m_start->text().toDouble(), m_end->text().toDouble());
}

void ALFAnalysisView::addSpectrum(Mantid::API::MatrixWorkspace_sptr const &workspace) {
  m_plot->clear();
  if (workspace) {
    m_plot->addSpectrum("Extracted Data", workspace, 0, Qt::black);
  }
}

void ALFAnalysisView::addFitSpectrum(Mantid::API::MatrixWorkspace_sptr const &workspace) {
  if (workspace) {
    m_plot->addSpectrum("Fitted Data", workspace, 1, Qt::red);
  }
}

void ALFAnalysisView::setPeakCentre(double const centre) { m_peakCentre->setText(QString::number(centre)); }

double ALFAnalysisView::peakCentre() const { return m_peakCentre->text().toDouble(); }

void ALFAnalysisView::setPeakCentreStatus(std::string const &status) {
  const auto [stylesheet, tooltip] = getPeakCentreUIProperties(QString::fromStdString(status));
  m_fitStatus->setStyleSheet(stylesheet);
  m_fitStatus->setText(tooltip);
  m_fitStatus->setToolTip(tooltip);
}

void ALFAnalysisView::setAverageTwoTheta(std::optional<double> average, std::vector<double> const &all) {
  m_averageTwoTheta->setText(average ? QString::number(*average) : "-");
  m_averageTwoTheta->setToolTip(all.size() == 0u ? TWO_THETA_TOOLTIP : TWO_THETA_TOOLTIP + constructAverageString(all));
}

void ALFAnalysisView::displayWarning(std::string const &message) {
  QMessageBox::warning(this, "Warning!", message.c_str());
}

} // namespace MantidQt::CustomInterfaces
