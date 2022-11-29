// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFAnalysisView.h"
#include "ALFAnalysisPresenter.h"

#include "MantidAPI/IPeakFunction.h"
#include "MantidQtIcons/Icon.h"
#include "MantidQtWidgets/Plotting/PeakPicker.h"
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
    : QWidget(parent), m_plot(nullptr), m_peakPicker(nullptr), m_start(nullptr), m_end(nullptr), m_fitButton(nullptr) {
  setupPlotFitSplitter(start, end);
}

QWidget *ALFAnalysisView::getView() { return this; }

void ALFAnalysisView::subscribePresenter(IALFAnalysisPresenter *presenter) { m_presenter = presenter; }

void ALFAnalysisView::setupPlotFitSplitter(double const start, double const end) {
  auto layout = new QHBoxLayout(this);
  auto splitter = new QSplitter(Qt::Vertical);

  splitter->addWidget(createPlotWidget());
  splitter->addWidget(createFitWidget(start, end));

  layout->addWidget(splitter);
}

QWidget *ALFAnalysisView::createPlotWidget() {
  m_plot = new MantidWidgets::PreviewPlot();

  // Set the preview plot background as transparent.
  m_plot->canvas()->gcf().setFaceColor("None");
  m_plot->canvas()->setStyleSheet("background-color:transparent;");

  m_peakPicker = new MantidWidgets::PeakPicker(m_plot, Qt::red);
  m_peakPicker->setVisible(false);
  connect(m_peakPicker, SIGNAL(changed()), this, SLOT(notifyPeakPickerChanged()));

  return m_plot;
}

QWidget *ALFAnalysisView::createFitWidget(double const start, double const end) {
  auto *analysisPane = new QWidget();
  auto *analysisLayout = new QGridLayout(analysisPane);

  setupTwoThetaWidget(analysisLayout);
  setupFitRangeWidget(analysisLayout, start, end);
  setupPeakCentreWidget(analysisLayout, (start + end) / 2.0);

  return analysisPane;
}

void ALFAnalysisView::setupTwoThetaWidget(QGridLayout *layout) {
  m_averageTwoTheta = new QLineEdit("-");
  m_averageTwoTheta->setReadOnly(true);
  m_averageTwoTheta->setToolTip(TWO_THETA_TOOLTIP);

  layout->addWidget(new QLabel("Two theta:"), 0, 0);
  layout->addWidget(m_averageTwoTheta, 0, 1, 1, 3);
  layout->addWidget(new QLabel("*placehold"), 0, 4);

  // Add an empty label to act as empty space
  layout->addWidget(new QLabel(""), 1, 4);
}

void ALFAnalysisView::setupFitRangeWidget(QGridLayout *layout, double const start, double const end) {
  m_start = new QLineEdit(QString::number(start));
  m_start->setValidator(new QDoubleValidator(m_start));

  m_end = new QLineEdit(QString::number(end));
  m_end->setValidator(new QDoubleValidator(m_end));

  layout->addWidget(new QLabel("Fit from:"), 2, 0);
  layout->addWidget(m_start, 2, 1);
  layout->addWidget(new QLabel("to:"), 2, 2);
  layout->addWidget(m_end, 2, 3);
}

void ALFAnalysisView::setupPeakCentreWidget(QGridLayout *layout, double const centre) {
  m_peakCentre = new QLineEdit(QString::number(centre));
  m_peakCentre->setValidator(new QDoubleValidator(m_peakCentre));

  m_fitButton = new QPushButton("Fit");

  connect(m_peakCentre, SIGNAL(editingFinished()), this, SLOT(notifyPeakCentreEditingFinished()));
  connect(m_fitButton, SIGNAL(clicked()), this, SLOT(notifyFitClicked()));

  layout->addWidget(new QLabel("Peak Centre:"), 3, 0);
  layout->addWidget(m_peakCentre, 3, 1, 1, 3);
  layout->addWidget(m_fitButton, 3, 4);

  m_fitStatus = new QLabel("");
  m_fitStatus->setAlignment(Qt::AlignRight);
  setPeakCentreStatus("");

  layout->addWidget(m_fitStatus, 4, 0, 1, 4);
}

void ALFAnalysisView::notifyPeakPickerChanged() { m_presenter->notifyPeakPickerChanged(); }

void ALFAnalysisView::notifyPeakCentreEditingFinished() { m_presenter->notifyPeakCentreEditingFinished(); }

void ALFAnalysisView::notifyFitClicked() { m_presenter->notifyFitClicked(); }

void ALFAnalysisView::replot() { m_plot->replot(); }

std::pair<double, double> ALFAnalysisView::getRange() const {
  return std::make_pair(m_start->text().toDouble(), m_end->text().toDouble());
}

void ALFAnalysisView::addSpectrum(Mantid::API::MatrixWorkspace_sptr const &workspace) {
  m_peakPicker->setVisible(false);
  m_plot->clear();
  if (workspace) {
    m_peakPicker->setVisible(true);
    m_plot->addSpectrum("Extracted Data", workspace, 0, Qt::black);
  }
}

void ALFAnalysisView::addFitSpectrum(Mantid::API::MatrixWorkspace_sptr const &workspace) {
  if (workspace) {
    m_plot->addSpectrum("Fitted Data", workspace, 1, Qt::red);
  }
}

void ALFAnalysisView::removeFitSpectrum() { m_plot->removeSpectrum("Fitted Data"); }

void ALFAnalysisView::setPeak(Mantid::API::IPeakFunction_const_sptr const &peak) {
  setPeakCentre(peak->getParameter("PeakCentre"));

  m_peakPicker->setPeak(peak);
  m_peakPicker->select(false);
}

Mantid::API::IPeakFunction_const_sptr ALFAnalysisView::getPeak() const { return m_peakPicker->peak(); }

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
