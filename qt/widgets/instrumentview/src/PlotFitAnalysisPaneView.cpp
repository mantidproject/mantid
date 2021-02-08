// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPaneView.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionFactory.h"

#include <QLabel>
#include <QMessageBox>
#include <QRegExpValidator>
#include <QSizePolicy>
#include <QSpacerItem>
#include <QSplitter>
#include <QVBoxLayout>
#include <utility>

namespace MantidQt {
namespace MantidWidgets {

PlotFitAnalysisPaneView::PlotFitAnalysisPaneView(const double &start,
                                                 const double &end,
                                                 QWidget *parent)
    : QWidget(parent), m_plot(nullptr), m_fitBrowser(nullptr), m_start(nullptr),
      m_end(nullptr), m_fitButton(nullptr), m_fitObservable(nullptr),
      m_updateEstimateObservable(nullptr) {
  setupPlotFitSplitter(start, end);
}

void PlotFitAnalysisPaneView::setupPlotFitSplitter(const double &start,
                                                   const double &end) {
  auto layout = new QHBoxLayout(this);
  auto splitter = new QSplitter(Qt::Vertical);

  m_plot = new MantidWidgets::PreviewPlot();
  m_plot->setCanvasColour(Qt::white);
  splitter->addWidget(m_plot);

  splitter->addWidget(createFitPane(start, end));

  layout->addWidget(splitter);
}

QWidget *PlotFitAnalysisPaneView::createFitPane(const double &start,
                                                const double &end) {
  auto fitPane = new QWidget();
  auto fitPaneLayout = new QVBoxLayout(fitPane);

  auto fitButtons = new QWidget();
  auto layout = new QHBoxLayout(fitButtons);
  m_fitButton = new QPushButton("Fit");
  m_updateEstimateButton = new QPushButton("Update Estimate");
  m_fitObservable = new Observable();
  m_updateEstimateObservable = new Observable();
  connect(m_fitButton, SIGNAL(clicked()), this, SLOT(doFit()));
  connect(m_updateEstimateButton, SIGNAL(clicked()), this,
          SLOT(updateEstimate()));

  layout->addWidget(m_fitButton);
  layout->addItem(
      new QSpacerItem(80, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
  layout->addWidget(m_updateEstimateButton);

  fitPaneLayout->addWidget(fitButtons);

  m_fitBrowser = new MantidWidgets::FunctionBrowser(this);
  fitPaneLayout->addWidget(m_fitBrowser);

  auto *startText = new QLabel("Fit from:");
  m_start = new QLineEdit(QString::number(start));
  auto startValidator = new QDoubleValidator(m_start);
  auto endValidator = new QDoubleValidator(m_start);
  m_start->setValidator(startValidator);
  auto *endText = new QLabel("to:");
  m_end = new QLineEdit(QString::number(end));
  m_end->setValidator(endValidator);
  auto *range = new QWidget();
  auto *rangeLayout = new QHBoxLayout(range);
  rangeLayout->addWidget(startText);
  rangeLayout->addWidget(m_start);
  rangeLayout->addWidget(endText);
  rangeLayout->addWidget(m_end);
  fitPaneLayout->addWidget(range);

  return fitPane;
}

void PlotFitAnalysisPaneView::doFit() {
  auto function = m_fitBrowser->getFunction();
  if (function) {
    m_fitObservable->notify();
  }
}

void PlotFitAnalysisPaneView::updateEstimate() {
  m_updateEstimateObservable->notify();
}

void PlotFitAnalysisPaneView::addSpectrum(const std::string &wsName) {
  m_plot->addSpectrum("Extracted Data", wsName.c_str(), 0, Qt::black);
}
void PlotFitAnalysisPaneView::addFitSpectrum(const std::string &wsName) {
  m_plot->addSpectrum("Fitted Data", wsName.c_str(), 1, Qt::red);
}

std::pair<double, double> PlotFitAnalysisPaneView::getRange() {
  double start = m_start->text().toDouble();
  double end = m_end->text().toDouble();
  return std::make_pair(start, end);
}

Mantid::API::IFunction_sptr PlotFitAnalysisPaneView::getFunction() {
  return m_fitBrowser->getFunction();
}

void PlotFitAnalysisPaneView::updateFunction(
    const Mantid::API::IFunction_sptr func) {
  m_fitBrowser->updateMultiDatasetParameters(*func);
}

void PlotFitAnalysisPaneView::addFunction(Mantid::API::IFunction_sptr func) {
  m_fitBrowser->setFunction(std::move(func));
}

void PlotFitAnalysisPaneView::displayWarning(const std::string &message) {
  QMessageBox::warning(this, "Warning!", message.c_str());
}

} // namespace MantidWidgets
} // namespace MantidQt
