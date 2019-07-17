// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectPlotOptionsPresenter.h"

namespace {

std::string OR(std::string const &lhs, std::string const &rhs) {
  return "(" + lhs + "|" + rhs + ")";
}

std::string NATURAL_NUMBER(std::size_t const &digits) {
  return OR("0", "[1-9][0-9]{," + std::to_string(digits - 1) + "}");
}

namespace Regexes {

std::string const SPACE = "[ ]*";
std::string const COMMA = SPACE + "," + SPACE;
std::string const MINUS = "\\-";

std::string const NUMBER = NATURAL_NUMBER(4);
std::string const NATURAL_RANGE = "(" + NUMBER + MINUS + NUMBER + ")";
std::string const NATURAL_OR_RANGE = OR(NATURAL_RANGE, NUMBER);
std::string const SPECTRA_LIST =
    "(" + NATURAL_OR_RANGE + "(" + COMMA + NATURAL_OR_RANGE + ")*)";

} // namespace Regexes
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectPlotOptionsPresenter::IndirectPlotOptionsPresenter(
    IndirectPlotOptionsView *view, IndirectTab *parent,
    PlotWidget const &plotType)
    : QObject(nullptr), m_view(view),
      m_model(std::make_unique<IndirectPlotOptionsModel>()),
      m_parentTab(parent) {
  m_view->setPlotType(plotType);
  setupPresenter();
}

void IndirectPlotOptionsPresenter::setupPresenter() {
  connect(m_view.get(), SIGNAL(selectedSpectraChanged(std::string const &)),
          this, SLOT(spectraChanged(std::string const &)));

  connect(m_view.get(), SIGNAL(plotSpectraClicked()), this,
          SLOT(plotSpectra()));
  connect(m_view.get(), SIGNAL(plotContourClicked()), this,
          SLOT(plotContour()));
  connect(m_view.get(), SIGNAL(plotTiledClicked()), this, SLOT(plotTiled()));

  connect(&m_pythonRunner, SIGNAL(runAsPythonScript(QString const &, bool)),
          m_parentTab, SIGNAL(runAsPythonScript(QString const &, bool)));

  m_view->setSpectraRegex(QString::fromStdString(Regexes::SPECTRA_LIST));
}

void IndirectPlotOptionsPresenter::setWorkspace(
    std::string const &plotWorkspace) {
  bool success = m_model->setWorkspace(plotWorkspace);
  m_view->setOptionsEnabled(Plotting::None, success);
  if (success)
    setSpectra();
}

void IndirectPlotOptionsPresenter::removeWorkspace() {
  m_model->removeWorkspace();
  m_view->setOptionsEnabled(Plotting::None, false);
}

void IndirectPlotOptionsPresenter::setSpectra() {
  auto const spectra = m_model->spectra();
  if (spectra) {
    m_view->setSpectra(QString::fromStdString(spectra.get()));
    m_view->setSpectraErrorLabelVisible(false);
  } else {
    m_view->setSpectra("0");
    m_view->setSpectraErrorLabelVisible(!m_model->setSpectra("0"));
  }
}

void IndirectPlotOptionsPresenter::spectraChanged(std::string const &spectra) {
  auto const formattedSpectra = m_model->formatSpectra(spectra);
  m_view->setSpectra(QString::fromStdString(formattedSpectra));
  m_view->setSpectraErrorLabelVisible(!m_model->setSpectra(formattedSpectra));

  if (!formattedSpectra.empty())
    m_view->addSpectraSuggestion(QString::fromStdString(formattedSpectra));
}

void IndirectPlotOptionsPresenter::plotSpectra() {
  m_view->setOptionsEnabled(Plotting::Spectrum, false);
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  runPythonCode(m_model->getPlotSpectraString(m_parentTab->errorBars()));
#else
  m_model->plotSpectra(m_parentTab->errorBars());
#endif
  m_view->setOptionsEnabled(Plotting::Spectrum, true);
}

void IndirectPlotOptionsPresenter::plotContour() {
  m_view->setOptionsEnabled(Plotting::Contour, false);
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  runPythonCode(m_model->getPlotContourString());
#else
  m_model->plotContour();
#endif
  m_view->setOptionsEnabled(Plotting::Contour, true);
}

void IndirectPlotOptionsPresenter::plotTiled() {
  m_view->setOptionsEnabled(Plotting::Tiled, false);
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  runPythonCode(m_model->getPlotTiledString());
#else
  m_model->plotTiled();
#endif
  m_view->setOptionsEnabled(Plotting::Tiled, true);
}

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
void IndirectPlotOptionsPresenter::runPythonCode(
    boost::optional<std::string> const &plotString) {
  if (plotString)
    m_pythonRunner.runPythonCode(QString::fromStdString(plotString.get()));
}
#endif

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
