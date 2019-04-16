// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALCPeakFittingView.h"

#include "MantidQtWidgets/Common/HelpWindow.h"
#include "MantidQtWidgets/LegacyQwt/ErrorCurve.h"

#include <QMessageBox>

#include <qwt_symbol.h>

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {

ALCPeakFittingView::ALCPeakFittingView(QWidget *widget)
    : m_widget(widget), m_ui(), m_dataCurve(new QwtPlotCurve()),
      m_fittedCurve(new QwtPlotCurve()), m_dataErrorCurve(nullptr),
      m_peakPicker(nullptr) {}

ALCPeakFittingView::~ALCPeakFittingView() {
  m_dataCurve->detach();
  delete m_dataCurve;
  if (m_dataErrorCurve) {
    m_dataErrorCurve->detach();
    delete m_dataErrorCurve;
  }
}

IFunction_const_sptr ALCPeakFittingView::function(QString index) const {
  return m_ui.peaks->getFunctionByIndex(index);
}

boost::optional<QString> ALCPeakFittingView::currentFunctionIndex() const {
  return m_ui.peaks->currentFunctionIndex();
}

IPeakFunction_const_sptr ALCPeakFittingView::peakPicker() const {
  return m_peakPicker->peak();
}

void ALCPeakFittingView::initialize() {
  m_ui.setupUi(m_widget);

  connect(m_ui.fit, SIGNAL(clicked()), this, SIGNAL(fitRequested()));

  m_ui.plot->setCanvasBackground(Qt::white);
  m_ui.plot->setAxisFont(QwtPlot::xBottom, m_widget->font());
  m_ui.plot->setAxisFont(QwtPlot::yLeft, m_widget->font());

  m_dataCurve->setStyle(QwtPlotCurve::NoCurve);
  m_dataCurve->setSymbol(
      QwtSymbol(QwtSymbol::Ellipse, QBrush(), QPen(), QSize(7, 7)));
  m_dataCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
  m_dataCurve->attach(m_ui.plot);

  m_fittedCurve->setPen(QPen(Qt::red, 1.5));
  m_fittedCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
  m_fittedCurve->attach(m_ui.plot);

  // XXX: Being a QwtPlotItem, should get deleted when m_ui.plot gets deleted
  // (auto-delete option)
  m_peakPicker = new MantidWidgets::PeakPicker(m_ui.plot, Qt::red);

  connect(m_peakPicker, SIGNAL(changed()), SIGNAL(peakPickerChanged()));

  connect(m_ui.peaks, SIGNAL(currentFunctionChanged()),
          SIGNAL(currentFunctionChanged()));
  connect(m_ui.peaks, SIGNAL(parameterChanged(QString, QString)),
          SIGNAL(parameterChanged(QString, QString)));

  connect(m_ui.help, SIGNAL(clicked()), this, SLOT(help()));
  connect(m_ui.plotGuess, SIGNAL(clicked()), this, SLOT(plotGuess()));
}

void ALCPeakFittingView::setDataCurve(const QwtData &data,
                                      const std::vector<double> &errors) {

  // Set data
  m_dataCurve->setData(data);

  // Set errors
  if (m_dataErrorCurve) {
    m_dataErrorCurve->detach();
    delete m_dataErrorCurve;
  }
  m_dataErrorCurve =
      new MantidQt::MantidWidgets::ErrorCurve(m_dataCurve, errors);
  m_dataErrorCurve->attach(m_ui.plot);

  // Replot
  m_ui.plot->replot();
}

void ALCPeakFittingView::setFittedCurve(const QwtData &data) {
  m_fittedCurve->setData(data);
  m_ui.plot->replot();
}

void ALCPeakFittingView::setFunction(const IFunction_const_sptr &newFunction) {
  if (newFunction) {
    size_t nParams = newFunction->nParams();
    for (size_t i = 0; i < nParams; i++) {

      QString name = QString::fromStdString(newFunction->parameterName(i));
      double value = newFunction->getParameter(i);
      double error = newFunction->getError(i);

      m_ui.peaks->setParameter(name, value);
      m_ui.peaks->setParamError(name, error);
    }
  } else {
    m_ui.peaks->clear();
  }
}

void ALCPeakFittingView::setParameter(const QString &funcIndex,
                                      const QString &paramName, double value) {
  m_ui.peaks->setParameter(funcIndex + paramName, value);
}

void ALCPeakFittingView::setPeakPickerEnabled(bool enabled) {
  m_peakPicker->setEnabled(enabled);
  m_peakPicker->setVisible(enabled);
  m_ui.plot->replot(); // PeakPicker might get hidden/shown
}

void ALCPeakFittingView::setPeakPicker(const IPeakFunction_const_sptr &peak) {
  m_peakPicker->setPeak(peak);
  m_ui.plot->replot();
}

void ALCPeakFittingView::help() {
  MantidQt::API::HelpWindow::showCustomInterface(nullptr, QString("Muon ALC"));
}

void ALCPeakFittingView::displayError(const QString &message) {
  QMessageBox::critical(m_widget, "Error", message);
}

void ALCPeakFittingView::emitFitRequested() {
  // Fit requested: reset "plot guess"
  emit fitRequested();
}

/**
 * Emit signal that "plot/remove guess" has been clicked
 */
void ALCPeakFittingView::plotGuess() { emit plotGuessClicked(); }

/**
 * Changes the text on the "Plot guess" button
 * @param plotted :: [input] Whether guess is plotted or not
 */
void ALCPeakFittingView::changePlotGuessState(bool plotted) {
  m_ui.plotGuess->setText(plotted ? "Remove guess" : "Plot guess");
}

} // namespace CustomInterfaces
} // namespace MantidQt
