// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFAnalysisView.h"
#include "ALFAnalysisPresenter.h"
#include "ALFView.h"

#include "MantidAPI/IPeakFunction.h"
#include "MantidQtIcons/Icon.h"
#include "MantidQtWidgets/MplCpp/Plot.h"
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

QString const DEFAULT_TUBES_TOOLTIP = "No tubes have been selected";
QString const FIT_BUTTON_TOOLTIP =
    "Fit to find the Peak Centre. Repeated Fits will attempt to refine the Peak Centre value further.";
QString const PEAK_CENTRE_TOOLTIP = "The centre of the Gaussian peak function, V, in degrees.";
QString const TWO_THETA_TOOLTIP = "The average two theta of the extracted tubes. The two theta of a tube is taken to "
                                  "be the two theta at which the Out of Plane angle is closest to zero.";
QString const ROTATION_ANGLE_TOOLTIP = "The Rotation or tilt angle, R, in degrees. R = V / (2*sin(theta))";

QString const INFO_LABEL_STYLE = "QLabel { border-radius: 5px; border: 2px solid black; }";
QString const ERROR_LABEL_STYLE = "QLabel { color: red; border-radius: 5px; border: 2px solid red; }";
QString const WARNING_LABEL_STYLE = "QLabel { color: darkOrange; border-radius: 5px; border: 2px solid orange; }";
QString const SUCCESS_LABEL_STYLE = "QLabel { color: green; border-radius: 5px; border: 2px solid green; }";

std::tuple<QString, QString, QString> getPeakCentreUIProperties(QString const &fitStatus) {
  QString stylesheet(""), status(""), tooltip("");
  if (fitStatus.contains("success")) {
    stylesheet = SUCCESS_LABEL_STYLE, status = "Success", tooltip = "Fit successful";
  } else if (fitStatus.contains("Failed to converge")) {
    stylesheet = WARNING_LABEL_STYLE, status = "Warning", tooltip = fitStatus;
  } else if (!fitStatus.isEmpty()) {
    stylesheet = ERROR_LABEL_STYLE, status = "Error", tooltip = fitStatus;
  }
  return {stylesheet, status, tooltip};
}

QString constructNumberOfTubesTooltip(std::vector<double> const &twoThetas) {
  if (twoThetas.empty()) {
    return DEFAULT_TUBES_TOOLTIP;
  }
  QString calculationStr("All two thetas:");
  for (auto i = 0u; i < twoThetas.size(); ++i) {
    calculationStr += "\n";
    calculationStr += QString::number(twoThetas[i]);
  }
  return calculationStr;
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

  layout->setContentsMargins(5, 0, 5, 0);
  layout->addWidget(splitter);
}

QWidget *ALFAnalysisView::createPlotWidget() {
  auto *plotWidget = new QWidget();
  auto *plotLayout = new QVBoxLayout(plotWidget);
  plotLayout->setSpacing(0);

  m_plot = new MantidWidgets::PreviewPlot();

  // Set override axis labels to be more concise
  m_plot->setOverrideAxisLabel(MantidWidgets::AxisID::XBottom, "Out of plane angle (degrees)");
  m_plot->setOverrideAxisLabel(MantidWidgets::AxisID::YLeft, "Counts");

  // Remove padding from the preview plot
  QHash<QString, QVariant> kwargs;
  kwargs.insert("pad", 0);
  m_plot->setTightLayout(kwargs);

  // Set the preview plot background as transparent.
  m_plot->canvas()->gcf().setFaceColor("None");
  m_plot->canvas()->setStyleSheet("background-color:transparent;");

  m_peakPicker = new MantidWidgets::PeakPicker(m_plot);
  m_peakPicker->setVisible(false);
  connect(m_peakPicker, SIGNAL(changed()), this, SLOT(notifyPeakPickerChanged()));

  plotLayout->addWidget(createPlotToolbar());
  plotLayout->addWidget(m_plot);

  return plotWidget;
}

QWidget *ALFAnalysisView::createPlotToolbar() {
  m_exportToADS = new QPushButton(MantidQt::Icons::getIcon("mdi.download"), "");
  m_exportToADS->setToolTip("Generate workspace from plot. The workspace is named 'ALFView_exported'");
  connect(m_exportToADS, SIGNAL(clicked()), this, SLOT(notifyExportWorkspaceToADSClicked()));

  m_externalPlot = new QPushButton(MantidQt::Icons::getIcon("mdi.open-in-new"), "");
  m_externalPlot->setToolTip("Open plot in new window. The new window has more plotting options.");
  connect(m_externalPlot, SIGNAL(clicked()), this, SLOT(notifyExternalPlotClicked()));

  m_resetButton = new QPushButton(MantidQt::Icons::getIcon("mdi.replay"), "");
  m_resetButton->setToolTip("Reset extracted plot");
  connect(m_resetButton, SIGNAL(clicked()), this, SLOT(notifyResetClicked()));

  auto toolbarWidget = new QWidget();
  auto *toolbarLayout = new QHBoxLayout(toolbarWidget);
  toolbarLayout->setMargin(0);
  toolbarLayout->addItem(new QSpacerItem(80, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
  toolbarLayout->addWidget(m_exportToADS);
  toolbarLayout->addWidget(m_externalPlot);
  toolbarLayout->addWidget(m_resetButton);

  return toolbarWidget;
}

QWidget *ALFAnalysisView::createFitWidget(double const start, double const end) {
  auto *analysisPane = new QWidget();
  auto *analysisLayout = new QGridLayout(analysisPane);

  setupTwoThetaWidget(analysisLayout);
  setupFitRangeWidget(analysisLayout, start, end);
  setupPeakCentreWidget(analysisLayout, (start + end) / 2.0);
  setupRotationAngleWidget(analysisLayout);

  return analysisPane;
}

void ALFAnalysisView::setupTwoThetaWidget(QGridLayout *layout) {
  m_averageTwoTheta = new QLineEdit("-");
  m_averageTwoTheta->setReadOnly(true);
  m_averageTwoTheta->setToolTip(TWO_THETA_TOOLTIP);
  m_numberOfTubes = new QLabel("0 tubes");
  m_numberOfTubes->setStyleSheet(INFO_LABEL_STYLE);
  m_numberOfTubes->setToolTip(DEFAULT_TUBES_TOOLTIP);
  m_numberOfTubes->setAlignment(Qt::AlignCenter);

  layout->addWidget(new QLabel("Two theta:"), 0, 0);
  layout->addWidget(m_averageTwoTheta, 0, 1, 1, 3);
  layout->addWidget(m_numberOfTubes, 0, 4);

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

  m_fitButton = new QPushButton("Fit");
  m_fitButton->setToolTip(FIT_BUTTON_TOOLTIP);
  connect(m_fitButton, SIGNAL(clicked()), this, SLOT(notifyFitClicked()));
  layout->addWidget(m_fitButton, 2, 4);
}

void ALFAnalysisView::setupPeakCentreWidget(QGridLayout *layout, double const centre) {
  m_peakCentre = new QLineEdit(QString::number(centre));
  m_peakCentre->setValidator(new QDoubleValidator(m_peakCentre));
  m_peakCentre->setToolTip(PEAK_CENTRE_TOOLTIP);

  connect(m_peakCentre, SIGNAL(editingFinished()), this, SLOT(notifyPeakCentreEditingFinished()));

  layout->addWidget(new QLabel("Peak Centre:"), 3, 0);
  layout->addWidget(m_peakCentre, 3, 1, 1, 3);

  m_fitStatus = new QLabel("");
  m_fitStatus->setAlignment(Qt::AlignCenter);
  setPeakCentreStatus("");

  layout->addWidget(m_fitStatus, 3, 4);

  // Add an empty label to act as empty space
  layout->addWidget(new QLabel(""), 4, 4);
}

void ALFAnalysisView::setupRotationAngleWidget(QGridLayout *layout) {
  m_rotationAngle = new QLineEdit("-");
  m_rotationAngle->setReadOnly(true);
  m_rotationAngle->setToolTip(ROTATION_ANGLE_TOOLTIP);
  m_fitRequired = new QLabel("*");
  m_fitRequired->setToolTip("A Fit to find the peak centre is required.");
  m_fitRequired->setStyleSheet("QLabel { color: red; }");

  layout->addWidget(new QLabel("Rotation:"), 5, 0);
  layout->addWidget(m_rotationAngle, 5, 1, 1, 3);
  layout->addWidget(m_fitRequired, 5, 4);
}

void ALFAnalysisView::disable(std::string const &reason) {
  if (auto parent = static_cast<ALFView *>(parentWidget())) {
    parent->disable(reason);
  }
}

void ALFAnalysisView::enable() {
  if (auto parent = static_cast<ALFView *>(parentWidget())) {
    parent->enable();
  }
}

void ALFAnalysisView::notifyPeakPickerChanged() { m_presenter->notifyPeakPickerChanged(); }

void ALFAnalysisView::notifyPeakCentreEditingFinished() { m_presenter->notifyPeakCentreEditingFinished(); }

void ALFAnalysisView::notifyFitClicked() { m_presenter->notifyFitClicked(); }

void ALFAnalysisView::notifyExportWorkspaceToADSClicked() { m_presenter->notifyExportWorkspaceToADSClicked(); }

void ALFAnalysisView::notifyExternalPlotClicked() { m_presenter->notifyExternalPlotClicked(); }

void ALFAnalysisView::notifyResetClicked() { m_presenter->notifyResetClicked(); }

void ALFAnalysisView::replot() { m_plot->replot(); }

void ALFAnalysisView::openExternalPlot(Mantid::API::MatrixWorkspace_sptr const &workspace,
                                       std::vector<int> const &workspaceIndices) const {
  // Externally plot the plotted workspace.
  MantidQt::Widgets::MplCpp::plot({workspace}, std::nullopt, workspaceIndices);
}

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

void ALFAnalysisView::setAverageTwoTheta(std::optional<double> average, std::vector<double> const &all) {
  m_averageTwoTheta->setText(average ? QString::number(*average) : "-");
  m_numberOfTubes->setText(all.size() == 1u ? QString::number(all.size()) + " tube"
                                            : QString::number(all.size()) + " tubes");
  m_numberOfTubes->setToolTip(constructNumberOfTubesTooltip(all));
}

void ALFAnalysisView::setPeak(Mantid::API::IPeakFunction_const_sptr const &peak, double const background) {
  setPeakCentre(peak->getParameter("PeakCentre"));

  m_peakPicker->setPeak(peak, background);
  m_peakPicker->select(true);
}

Mantid::API::IPeakFunction_const_sptr ALFAnalysisView::getPeak() const { return m_peakPicker->peak(); }

void ALFAnalysisView::setPeakCentre(double const centre) { m_peakCentre->setText(QString::number(centre)); }

double ALFAnalysisView::peakCentre() const { return m_peakCentre->text().toDouble(); }

void ALFAnalysisView::setPeakCentreStatus(std::string const &status) {
  const auto [stylesheet, text, tooltip] = getPeakCentreUIProperties(QString::fromStdString(status));
  m_fitStatus->setStyleSheet(stylesheet);
  m_fitStatus->setText(text);
  m_fitStatus->setToolTip(tooltip);
}

void ALFAnalysisView::setRotationAngle(std::optional<double> rotation) {
  m_rotationAngle->setText(rotation ? QString::number(*rotation) : "-");
  m_fitRequired->setVisible(!rotation);
}

void ALFAnalysisView::displayWarning(std::string const &message) {
  QMessageBox::warning(this, "Warning!", message.c_str());
}

} // namespace MantidQt::CustomInterfaces
