// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "PlotFitAnalysisPaneView.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionFactory.h"

#include <QLabel>
#include <QMessageBox>
#include <QRegExpValidator>
#include <QSizePolicy>
#include <QSpacerItem>
#include <QSplitter>
#include <QVBoxLayout>
namespace MantidQt {
namespace CustomInterfaces {

PlotFitAnalysisPaneView::PlotFitAnalysisPaneView(const double &start,
                                                 const double &end,
                                                 QWidget *parent)
    : QWidget(parent), m_plot(nullptr), m_fitBrowser(nullptr), m_start(nullptr),
      m_end(nullptr), m_fitButton(nullptr), m_fitObservable(nullptr) {
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
  m_fitObservable = new Observable();
  connect(m_fitButton, SIGNAL(clicked()), this, SLOT(doFit()));

  layout->addWidget(m_fitButton);
  layout->addItem(
      new QSpacerItem(80, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));

  fitPaneLayout->addWidget(fitButtons);

  m_fitBrowser = new MantidWidgets::FunctionBrowser(this);
  fitPaneLayout->addWidget(m_fitBrowser);

  QLabel *startText = new QLabel("Fit from:");
  m_start = new QLineEdit(QString::number(start));
  auto startValidator = new QDoubleValidator(m_start);
  auto endValidator = new QDoubleValidator(m_start);
  m_start->setValidator(startValidator);
  QLabel *endText = new QLabel("to:");
  m_end = new QLineEdit(QString::number(end));
  m_end->setValidator(endValidator);
  QWidget *range = new QWidget();
  QHBoxLayout *rangeLayout = new QHBoxLayout(range);
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

void PlotFitAnalysisPaneView::addSpectrum(std::string wsName) {
  m_plot->addSpectrum("Extracted Data", wsName.c_str(), 0, Qt::black);
}
void PlotFitAnalysisPaneView::addFitSpectrum(std::string wsName) {
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

void PlotFitAnalysisPaneView::updateFunction(Mantid::API::IFunction_sptr func) {
  m_fitBrowser->updateMultiDatasetParameters(*func);
}

void PlotFitAnalysisPaneView::addFunction(Mantid::API::IFunction_sptr func) {
  m_fitBrowser->setFunction(func);
}

void PlotFitAnalysisPaneView::fitWarning(const std::string &message) {
  QMessageBox::warning(this, "Fit error", message.c_str());
}

} // namespace CustomInterfaces
} // namespace MantidQt
