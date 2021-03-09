// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALCPeakFittingView.h"

#include "MantidQtWidgets/Common/HelpWindow.h"

#include <QMessageBox>

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {

ALCPeakFittingView::ALCPeakFittingView(QWidget *widget)
    : m_widget(widget), m_ui(), m_peakPicker(nullptr) {}

ALCPeakFittingView::~ALCPeakFittingView() {}

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

  m_ui.plot->setCanvasColour(Qt::white);

  // Error bars on the plot
  QStringList plotsWithErrors{"Corrected"};
  m_ui.plot->setLinesWithErrors(plotsWithErrors);

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

void ALCPeakFittingView::setDataCurve(MatrixWorkspace_sptr workspace,
                                      std::size_t const &workspaceIndex) {
  // These kwargs ensure only the data points are plotted with no line
  QHash<QString, QVariant> kwargs;
  kwargs.insert("linestyle", QString("None").toLatin1().constData());
  kwargs.insert("marker", QString(".").toLatin1().constData());

  m_ui.plot->clear();
  m_ui.plot->addSpectrum("Corrected", workspace, workspaceIndex, Qt::black,
                         kwargs);
}

void ALCPeakFittingView::setFittedCurve(MatrixWorkspace_sptr workspace,
                                        std::size_t const &workspaceIndex) {
  m_ui.plot->addSpectrum("Fit", workspace, workspaceIndex, Qt::red);
  m_ui.plot->replot();
}

void ALCPeakFittingView::setGuessCurve(MatrixWorkspace_sptr workspace,
                                       std::size_t const &workspaceIndex) {
  m_ui.plot->addSpectrum("Guess", workspace, workspaceIndex, Qt::green);
  m_ui.plot->replot();
}

void ALCPeakFittingView::removePlot(QString const &plotName) {
  m_ui.plot->removeSpectrum(plotName);
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
      m_ui.peaks->setParameterError(name, error);
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
  m_peakPicker->select(enabled);
  if (enabled)
    m_peakPicker->redraw();
  else
    m_peakPicker->remove();
  m_ui.plot->replot();
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
