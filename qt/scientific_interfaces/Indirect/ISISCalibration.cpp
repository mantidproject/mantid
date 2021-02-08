// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ISISCalibration.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Logger.h"

#include <QDebug>
#include <QFileInfo>
#include <stdexcept>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

namespace {
Mantid::Kernel::Logger g_log("ISISCalibration");

template <typename Map, typename Key, typename Value>
Value getValueOr(const Map &map, const Key &key, const Value &defaultValue) {
  try {
    return map.at(key);
  } catch (std::out_of_range &) {
    return defaultValue;
  }
}

} // namespace

using namespace Mantid::API;
using MantidQt::API::BatchAlgorithmRunner;

namespace MantidQt {
namespace CustomInterfaces {
//----------------------------------------------------------------------------------------------
/** Constructor
 */
ISISCalibration::ISISCalibration(IndirectDataReduction *idrUI, QWidget *parent)
    : IndirectDataReductionTab(idrUI, parent), m_lastCalPlotFilename("") {
  m_uiForm.setupUi(parent);
  setOutputPlotOptionsPresenter(std::make_unique<IndirectPlotOptionsPresenter>(
      m_uiForm.ipoPlotOptions, this, PlotWidget::SpectraBin));

  m_uiForm.ppCalibration->setCanvasColour(QColor(240, 240, 240));
  m_uiForm.ppResolution->setCanvasColour(QColor(240, 240, 240));
  m_uiForm.ppCalibration->watchADS(false);
  m_uiForm.ppResolution->watchADS(false);

  auto *doubleEditorFactory = new DoubleEditorFactory();

  // CAL PROPERTY TREE
  m_propTrees["CalPropTree"] = new QtTreePropertyBrowser();
  m_propTrees["CalPropTree"]->setFactoryForManager(m_dblManager,
                                                   doubleEditorFactory);
  m_uiForm.propertiesCalibration->addWidget(m_propTrees["CalPropTree"]);

  // Cal Property Tree: Peak/Background
  m_properties["CalPeakMin"] = m_dblManager->addProperty("Peak Min");
  m_properties["CalPeakMax"] = m_dblManager->addProperty("Peak Max");
  m_properties["CalBackMin"] = m_dblManager->addProperty("Back Min");
  m_properties["CalBackMax"] = m_dblManager->addProperty("Back Max");

  m_propTrees["CalPropTree"]->addProperty(m_properties["CalPeakMin"]);
  m_propTrees["CalPropTree"]->addProperty(m_properties["CalPeakMax"]);
  m_propTrees["CalPropTree"]->addProperty(m_properties["CalBackMin"]);
  m_propTrees["CalPropTree"]->addProperty(m_properties["CalBackMax"]);

  // Cal plot range selectors
  auto calPeak = m_uiForm.ppCalibration->addRangeSelector("CalPeak");
  calPeak->setColour(Qt::red);
  auto calBackground =
      m_uiForm.ppCalibration->addRangeSelector("CalBackground");
  calBackground->setColour(Qt::blue); // blue to be consistent with fit wizard

  // RES PROPERTY TREE
  m_propTrees["ResPropTree"] = new QtTreePropertyBrowser();
  m_propTrees["ResPropTree"]->setFactoryForManager(m_dblManager,
                                                   doubleEditorFactory);
  m_uiForm.loResolutionOptions->addWidget(m_propTrees["ResPropTree"]);

  // Res Property Tree: Spectra Selection
  m_properties["ResSpecMin"] = m_dblManager->addProperty("Spectra Min");
  m_propTrees["ResPropTree"]->addProperty(m_properties["ResSpecMin"]);
  m_dblManager->setDecimals(m_properties["ResSpecMin"], 0);

  m_properties["ResSpecMax"] = m_dblManager->addProperty("Spectra Max");
  m_propTrees["ResPropTree"]->addProperty(m_properties["ResSpecMax"]);
  m_dblManager->setDecimals(m_properties["ResSpecMax"], 0);

  // Res Property Tree: Background Properties
  QtProperty *resBG = m_grpManager->addProperty("Background");
  m_propTrees["ResPropTree"]->addProperty(resBG);

  m_properties["ResStart"] = m_dblManager->addProperty("Start");
  resBG->addSubProperty(m_properties["ResStart"]);

  m_properties["ResEnd"] = m_dblManager->addProperty("End");
  resBG->addSubProperty(m_properties["ResEnd"]);

  // Res Property Tree: Rebinning
  const int NUM_DECIMALS = 3;
  QtProperty *resRB = m_grpManager->addProperty("Rebinning");
  m_propTrees["ResPropTree"]->addProperty(resRB);

  m_properties["ResELow"] = m_dblManager->addProperty("Low");
  m_dblManager->setDecimals(m_properties["ResELow"], NUM_DECIMALS);
  m_dblManager->setValue(m_properties["ResELow"], -0.2);
  resRB->addSubProperty(m_properties["ResELow"]);

  m_properties["ResEWidth"] = m_dblManager->addProperty("Width");
  m_dblManager->setDecimals(m_properties["ResEWidth"], NUM_DECIMALS);
  m_dblManager->setValue(m_properties["ResEWidth"], 0.002);
  m_dblManager->setMinimum(m_properties["ResEWidth"], 0.001);
  resRB->addSubProperty(m_properties["ResEWidth"]);

  m_properties["ResEHigh"] = m_dblManager->addProperty("High");
  m_dblManager->setDecimals(m_properties["ResEHigh"], NUM_DECIMALS);
  m_dblManager->setValue(m_properties["ResEHigh"], 0.2);
  resRB->addSubProperty(m_properties["ResEHigh"]);

  // Res plot range selectors
  // Create ResBackground first so ResPeak is drawn above it
  auto resBackground = m_uiForm.ppResolution->addRangeSelector("ResBackground");
  resBackground->setColour(Qt::blue);
  auto resPeak = m_uiForm.ppResolution->addRangeSelector("ResPeak");
  resPeak->setColour(Qt::red);

  // SIGNAL/SLOT CONNECTIONS
  // Update instrument information when a new instrument config is selected
  connect(this, SIGNAL(newInstrumentConfiguration()), this,
          SLOT(setDefaultInstDetails()));

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  connect(resPeak, SIGNAL(rangeChanged(double, double)), resBackground,
          SLOT(setRange(double, double)));
#endif

  // Update property map when a range selector is moved
  connect(calPeak, SIGNAL(minValueChanged(double)), this,
          SLOT(calMinChanged(double)));
  connect(calPeak, SIGNAL(maxValueChanged(double)), this,
          SLOT(calMaxChanged(double)));
  connect(calBackground, SIGNAL(minValueChanged(double)), this,
          SLOT(calMinChanged(double)));
  connect(calBackground, SIGNAL(maxValueChanged(double)), this,
          SLOT(calMaxChanged(double)));
  connect(resPeak, SIGNAL(minValueChanged(double)), this,
          SLOT(calMinChanged(double)));
  connect(resPeak, SIGNAL(maxValueChanged(double)), this,
          SLOT(calMaxChanged(double)));
  connect(resBackground, SIGNAL(minValueChanged(double)), this,
          SLOT(calMinChanged(double)));
  connect(resBackground, SIGNAL(maxValueChanged(double)), this,
          SLOT(calMaxChanged(double)));

  // Update range selector positions when a value in the double manager changes
  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(calUpdateRS(QtProperty *, double)));
  // Plot miniplots after a file has loaded
  connect(m_uiForm.leRunNo, SIGNAL(filesFound()), this, SLOT(calPlotRaw()));
  // Toggle RES file options when user toggles Create RES File checkbox
  connect(m_uiForm.ckCreateResolution, SIGNAL(toggled(bool)), this,
          SLOT(resCheck(bool)));

  // Shows message on run button when user is inputting a run number
  connect(m_uiForm.leRunNo, SIGNAL(fileTextChanged(const QString &)), this,
          SLOT(pbRunEditing()));
  // Shows message on run button when Mantid is finding the file for a given run
  // number
  connect(m_uiForm.leRunNo, SIGNAL(findingFiles()), this, SLOT(pbRunFinding()));
  // Reverts run button back to normal when file finding has finished
  connect(m_uiForm.leRunNo, SIGNAL(fileFindingFinished()), this,
          SLOT(pbRunFinished()));

  // Nudge resCheck to ensure res range selectors are only shown when Create RES
  // file is checked
  resCheck(m_uiForm.ckCreateResolution->isChecked());

  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(algorithmComplete(bool)));
  // Handle running, plotting and saving
  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));

  connect(this,
          SIGNAL(updateRunButton(bool, std::string const &, QString const &,
                                 QString const &)),
          this,
          SLOT(updateRunButton(bool, std::string const &, QString const &,
                               QString const &)));
}

ISISCalibration::~ISISCalibration() {
  m_propTrees["CalPropTree"]->unsetFactoryForManager(m_dblManager);
  m_propTrees["ResPropTree"]->unsetFactoryForManager(m_dblManager);
}

std::pair<double, double> ISISCalibration::peakRange() const {
  return std::make_pair(m_dblManager->value(m_properties["CalPeakMin"]),
                        m_dblManager->value(m_properties["CalPeakMax"]));
}

std::pair<double, double> ISISCalibration::backgroundRange() const {
  return std::make_pair(m_dblManager->value(m_properties["CalBackMin"]),
                        m_dblManager->value(m_properties["CalBackMax"]));
}

std::pair<double, double> ISISCalibration::resolutionRange() const {
  return std::make_pair(m_dblManager->value(m_properties["ResStart"]),
                        m_dblManager->value(m_properties["ResEnd"]));
}

QString ISISCalibration::peakRangeString() const {
  return m_properties["CalPeakMin"]->valueText() + "," +
         m_properties["CalPeakMax"]->valueText();
}

QString ISISCalibration::backgroundRangeString() const {
  return m_properties["CalBackMin"]->valueText() + "," +
         m_properties["CalBackMax"]->valueText();
}

QString ISISCalibration::instrumentDetectorRangeString() {
  return getInstrumentDetail("spectra-min") + "," +
         getInstrumentDetail("spectra-max");
}

QString ISISCalibration::outputWorkspaceName() const {
  auto name = QFileInfo(m_uiForm.leRunNo->getFirstFilename()).baseName();
  if (m_uiForm.leRunNo->getFilenames().size() > 1)
    name += "_multi";

  return name + QString::fromStdString("_") + getAnalyserName() +
         getReflectionName();
}

QString ISISCalibration::resolutionDetectorRangeString() const {
  return QString::number(m_dblManager->value(m_properties["ResSpecMin"])) +
         "," + QString::number(m_dblManager->value(m_properties["ResSpecMax"]));
}

QString ISISCalibration::rebinString() const {
  return QString::number(m_dblManager->value(m_properties["ResELow"])) + "," +
         QString::number(m_dblManager->value(m_properties["ResEWidth"])) + "," +
         QString::number(m_dblManager->value(m_properties["ResEHigh"]));
}

QString ISISCalibration::backgroundString() const {
  return QString::number(m_dblManager->value(m_properties["ResStart"])) + "," +
         QString::number(m_dblManager->value(m_properties["ResEnd"]));
}

void ISISCalibration::setPeakRange(const double &minimumTof,
                                   const double &maximumTof) {
  auto calibrationPeak = m_uiForm.ppCalibration->getRangeSelector("CalPeak");
  setRangeSelector(calibrationPeak, m_properties["CalPeakMin"],
                   m_properties["CalPeakMax"],
                   qMakePair(minimumTof, maximumTof));
}

void ISISCalibration::setBackgroundRange(const double &minimumTof,
                                         const double &maximumTof) {
  auto background = m_uiForm.ppCalibration->getRangeSelector("CalBackground");
  setRangeSelector(background, m_properties["CalBackMin"],
                   m_properties["CalBackMax"],
                   qMakePair(minimumTof, maximumTof));
}

void ISISCalibration::setRangeLimits(
    MantidWidgets::RangeSelector *rangeSelector, const double &minimum,
    const double &maximum, const QString &minPropertyName,
    const QString &maxPropertyName) {
  setPlotPropertyRange(rangeSelector, m_properties[minPropertyName],
                       m_properties[maxPropertyName],
                       qMakePair(minimum, maximum));
}

void ISISCalibration::setPeakRangeLimits(const double &peakMin,
                                         const double &peakMax) {
  auto calibrationPeak = m_uiForm.ppCalibration->getRangeSelector("CalPeak");
  setRangeLimits(calibrationPeak, peakMin, peakMax, "CalELow", "CalEHigh");
}

void ISISCalibration::setBackgroundRangeLimits(const double &backgroundMin,
                                               const double &backgroundMax) {
  auto background = m_uiForm.ppCalibration->getRangeSelector("CalBackground");
  setRangeLimits(background, backgroundMin, backgroundMax, "CalStart",
                 "CalEnd");
}

void ISISCalibration::setResolutionSpectraRange(const double &minimum,
                                                const double &maximum) {
  m_dblManager->setValue(m_properties["ResSpecMin"], minimum);
  m_dblManager->setValue(m_properties["ResSpecMax"], maximum);
}

void ISISCalibration::setup() {}

void ISISCalibration::run() {
  // Get properties
  const auto filenames = m_uiForm.leRunNo->getFilenames().join(",");
  const auto outputWorkspaceNameStem = outputWorkspaceName().toLower();

  m_outputCalibrationName = outputWorkspaceNameStem + "_calib";

  try {
    m_batchAlgoRunner->addAlgorithm(calibrationAlgorithm(filenames));
  } catch (std::exception const &ex) {
    g_log.warning(ex.what());
    return;
  }

  // Initially take the calibration workspace as the result
  m_pythonExportWsName = m_outputCalibrationName.toStdString();
  // Configure the resolution algorithm
  if (m_uiForm.ckCreateResolution->isChecked()) {
    m_outputResolutionName = outputWorkspaceNameStem + "_res";
    m_batchAlgoRunner->addAlgorithm(resolutionAlgorithm(filenames));

    if (m_uiForm.ckSmoothResolution->isChecked())
      addRuntimeSmoothing(m_outputResolutionName);

    // When creating resolution file take the resolution workspace as the result
    m_pythonExportWsName = m_outputResolutionName.toStdString();
  }

  m_batchAlgoRunner->executeBatchAsync();
}

/*
 * Handle completion of the calibration and resolution algorithms.
 *
 * @param error If the algorithms failed.
 */
void ISISCalibration::algorithmComplete(bool error) {
  if (!error) {
    std::vector<std::string> outputWorkspaces{
        m_outputCalibrationName.toStdString()};
    if (m_uiForm.ckCreateResolution->isChecked() &&
        !m_outputResolutionName.isEmpty()) {
      outputWorkspaces.emplace_back(m_outputResolutionName.toStdString());
      if (m_uiForm.ckSmoothResolution->isChecked())
        outputWorkspaces.emplace_back(m_outputResolutionName.toStdString() +
                                      "_pre_smooth");
    }
    setOutputPlotOptionsWorkspaces(outputWorkspaces);

    m_uiForm.pbSave->setEnabled(true);
  }
}

bool ISISCalibration::validate() {
  MantidQt::CustomInterfaces::UserInputValidator uiv;

  uiv.checkFileFinderWidgetIsValid("Run", m_uiForm.leRunNo);

  auto rangeOfPeak = peakRange();
  auto rangeOfBackground = backgroundRange();
  uiv.checkValidRange("Peak Range", rangeOfPeak);
  uiv.checkValidRange("Back Range", rangeOfBackground);
  uiv.checkRangesDontOverlap(rangeOfPeak, rangeOfBackground);

  if (m_uiForm.ckCreateResolution->isChecked()) {
    uiv.checkValidRange("Background", resolutionRange());

    double eLow = m_dblManager->value(m_properties["ResELow"]);
    double eHigh = m_dblManager->value(m_properties["ResEHigh"]);
    double eWidth = m_dblManager->value(m_properties["ResEWidth"]);

    uiv.checkBins(eLow, eWidth, eHigh);
  }

  QString error = uiv.generateErrorMessage();

  if (error != "")
    g_log.warning(error.toStdString());

  return (error == "");
}

/**
 * Sets default spectra, peak and background ranges.
 */
void ISISCalibration::setDefaultInstDetails() {
  try {
    setDefaultInstDetails(getInstrumentDetails());
  } catch (std::exception const &ex) {
    g_log.warning(ex.what());
    showMessageBox(ex.what());
  }
}

void ISISCalibration::setDefaultInstDetails(
    QMap<QString, QString> const &instrumentDetails) {
  auto const instrument = getInstrumentDetail(instrumentDetails, "instrument");
  auto const spectraMin =
      getInstrumentDetail(instrumentDetails, "spectra-min").toDouble();
  auto const spectraMax =
      getInstrumentDetail(instrumentDetails, "spectra-max").toDouble();

  // Set the search instrument for runs
  m_uiForm.leRunNo->setInstrumentOverride(instrument);

  // Set spectra range
  setResolutionSpectraRange(spectraMin, spectraMax);

  // Set peak and background ranges
  const auto ranges = getRangesFromInstrument();
  setPeakRange(getValueOr(ranges, "peak-start-tof", 0.0),
               getValueOr(ranges, "peak-end-tof", 0.0));
  setBackgroundRange(getValueOr(ranges, "back-start-tof", 0.0),
                     getValueOr(ranges, "back-end-tof", 0.0));

  auto const hasResolution =
      hasInstrumentDetail(instrumentDetails, "resolution");
  m_uiForm.ckCreateResolution->setEnabled(hasResolution);
  if (!hasResolution)
    m_uiForm.ckCreateResolution->setChecked(false);
}

/**
 * Replots the raw data mini plot and the energy mini plot
 */
void ISISCalibration::calPlotRaw() {
  QString filename = m_uiForm.leRunNo->getFirstFilename();

  // Don't do anything if the file we would plot has not changed
  if (filename.isEmpty() || filename == m_lastCalPlotFilename)
    return;

  m_lastCalPlotFilename = filename;

  QFileInfo fi(filename);
  QString wsname = fi.baseName();

  int const specMin = hasInstrumentDetail("spectra-min")
                          ? getInstrumentDetail("spectra-min").toInt()
                          : -1;
  int const specMax = hasInstrumentDetail("spectra-max")
                          ? getInstrumentDetail("spectra-max").toInt()
                          : -1;

  if (!loadFile(filename, wsname, specMin, specMax)) {
    emit showMessageBox("Unable to load file.\nCheck whether your file exists "
                        "and matches the selected instrument in the Energy "
                        "Transfer tab.");
    return;
  }

  const auto input = std::dynamic_pointer_cast<MatrixWorkspace>(
      AnalysisDataService::Instance().retrieve(wsname.toStdString()));

  m_uiForm.ppCalibration->clear();
  m_uiForm.ppCalibration->addSpectrum("Raw", input, 0);
  m_uiForm.ppCalibration->resizeX();

  const auto &dataX = input->x(0);
  setPeakRangeLimits(dataX.front(), dataX.back());
  setBackgroundRangeLimits(dataX.front(), dataX.back());

  setDefaultInstDetails();

  m_uiForm.ppCalibration->replot();

  // Also replot the energy
  calPlotEnergy();
}

/**
 * Replots the energy mini plot
 */
void ISISCalibration::calPlotEnergy() {

  const auto files = m_uiForm.leRunNo->getFilenames().join(",");
  auto reductionAlg = energyTransferReductionAlgorithm(files);
  reductionAlg->execute();

  if (!reductionAlg->isExecuted()) {
    g_log.warning("Could not generate energy preview plot.");
    return;
  }

  WorkspaceGroup_sptr reductionOutputGroup =
      AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
          "__IndirectCalibration_reduction");
  if (reductionOutputGroup->isEmpty()) {
    g_log.warning("No result workspaces, cannot plot energy preview.");
    return;
  }

  MatrixWorkspace_sptr energyWs = std::dynamic_pointer_cast<MatrixWorkspace>(
      reductionOutputGroup->getItem(0));
  if (!energyWs) {
    g_log.warning("No result workspaces, cannot plot energy preview.");
    return;
  }

  const auto &dataX = energyWs->x(0);
  QPair<double, double> range(dataX.front(), dataX.back());

  auto resBackground = m_uiForm.ppResolution->getRangeSelector("ResBackground");
  setPlotPropertyRange(resBackground, m_properties["ResStart"],
                       m_properties["ResEnd"], range);

  m_uiForm.ppResolution->clear();
  m_uiForm.ppResolution->addSpectrum("Energy", energyWs, 0);
  m_uiForm.ppResolution->resizeX();

  calSetDefaultResolution(energyWs);

  m_uiForm.ppResolution->replot();
}

/**
 * Set default background and rebinning properties for a given instrument
 * and analyser
 *
 * @param ws :: Mantid workspace containing the loaded instrument
 */
void ISISCalibration::calSetDefaultResolution(
    const MatrixWorkspace_const_sptr &ws) {
  auto inst = ws->getInstrument();
  auto analyser = inst->getStringParameter("analyser");

  if (analyser.size() > 0) {
    auto comp = inst->getComponentByName(analyser[0]);

    if (!comp)
      return;

    auto params = comp->getNumberParameter("resolution", true);

    // Set the default instrument resolution
    if (!params.empty()) {
      double res = params[0];

      const auto energyRange = getXRangeFromWorkspace(ws);
      // Set default rebinning bounds
      QPair<double, double> peakERange(-res * 10, res * 10);
      auto resPeak = m_uiForm.ppResolution->getRangeSelector("ResPeak");
      setPlotPropertyRange(resPeak, m_properties["ResELow"],
                           m_properties["ResEHigh"], energyRange);
      setRangeSelector(resPeak, m_properties["ResELow"],
                       m_properties["ResEHigh"], peakERange);

      // Set default background bounds
      QPair<double, double> backgroundERange(-res * 9, -res * 8);
      auto resBackground =
          m_uiForm.ppResolution->getRangeSelector("ResBackground");
      setRangeSelector(resBackground, m_properties["ResStart"],
                       m_properties["ResEnd"], backgroundERange);
    }
  }
}

/**
 * Handles a range selector having it's minimum value changed.
 * Updates property in property map.
 *
 * @param val :: New minimum value
 */
void ISISCalibration::calMinChanged(double val) {
  auto calPeak = m_uiForm.ppCalibration->getRangeSelector("CalPeak");
  auto calBackground =
      m_uiForm.ppCalibration->getRangeSelector("CalBackground");
  auto resPeak = m_uiForm.ppResolution->getRangeSelector("ResPeak");
  auto resBackground = m_uiForm.ppResolution->getRangeSelector("ResBackground");

  MantidWidgets::RangeSelector *from =
      qobject_cast<MantidWidgets::RangeSelector *>(sender());

  disconnect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
             SLOT(calUpdateRS(QtProperty *, double)));
  if (from == calPeak) {
    m_dblManager->setValue(m_properties["CalPeakMin"], val);
  } else if (from == calBackground) {
    m_dblManager->setValue(m_properties["CalBackMin"], val);
  } else if (from == resPeak) {
    m_dblManager->setValue(m_properties["ResELow"], val);
  } else if (from == resBackground) {
    m_dblManager->setValue(m_properties["ResStart"], val);
  }
  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(calUpdateRS(QtProperty *, double)));
}

/**
 * Handles a range selector having it's maximum value changed.
 * Updates property in property map.
 *
 * @param val :: New maximum value
 */
void ISISCalibration::calMaxChanged(double val) {
  auto calPeak = m_uiForm.ppCalibration->getRangeSelector("CalPeak");
  auto calBackground =
      m_uiForm.ppCalibration->getRangeSelector("CalBackground");
  auto resPeak = m_uiForm.ppResolution->getRangeSelector("ResPeak");
  auto resBackground = m_uiForm.ppResolution->getRangeSelector("ResBackground");

  MantidWidgets::RangeSelector *from =
      qobject_cast<MantidWidgets::RangeSelector *>(sender());

  disconnect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
             SLOT(calUpdateRS(QtProperty *, double)));
  if (from == calPeak) {
    m_dblManager->setValue(m_properties["CalPeakMax"], val);
  } else if (from == calBackground) {
    m_dblManager->setValue(m_properties["CalBackMax"], val);
  } else if (from == resPeak) {
    m_dblManager->setValue(m_properties["ResEHigh"], val);
  } else if (from == resBackground) {
    m_dblManager->setValue(m_properties["ResEnd"], val);
  }
  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(calUpdateRS(QtProperty *, double)));
}

/**
 * Update a range selector given a QtProperty and new value
 *
 * @param prop :: The property to update
 * @param val :: New value for property
 */
void ISISCalibration::calUpdateRS(QtProperty *prop, double val) {
  auto calPeak = m_uiForm.ppCalibration->getRangeSelector("CalPeak");
  auto calBackground =
      m_uiForm.ppCalibration->getRangeSelector("CalBackground");
  auto resPeak = m_uiForm.ppResolution->getRangeSelector("ResPeak");
  auto resBackground = m_uiForm.ppResolution->getRangeSelector("ResBackground");

  disconnect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
             SLOT(calUpdateRS(QtProperty *, double)));

  if (prop == m_properties["CalPeakMin"]) {
    setRangeSelectorMin(m_properties["CalPeakMin"], m_properties["CalPeakMax"],
                        calPeak, val);
  } else if (prop == m_properties["CalPeakMax"]) {
    setRangeSelectorMax(m_properties["CalPeakMin"], m_properties["CalPeakMax"],
                        calPeak, val);
  } else if (prop == m_properties["CalBackMin"]) {
    setRangeSelectorMin(m_properties["CalPeakMin"], m_properties["CalBackMax"],
                        calBackground, val);
  } else if (prop == m_properties["CalBackMax"]) {
    setRangeSelectorMax(m_properties["CalPeakMin"], m_properties["CalBackMax"],
                        calBackground, val);
  } else if (prop == m_properties["ResStart"]) {
    setRangeSelectorMin(m_properties["ResStart"], m_properties["ResEnd"],
                        resBackground, val);
  } else if (prop == m_properties["ResEnd"]) {
    setRangeSelectorMax(m_properties["ResStart"], m_properties["ResEnd"],
                        resBackground, val);
  } else if (prop == m_properties["ResELow"]) {
    setRangeSelectorMin(m_properties["ResELow"], m_properties["ResEHigh"],
                        resPeak, val);
  } else if (prop == m_properties["ResEHigh"]) {
    setRangeSelectorMax(m_properties["ResELow"], m_properties["ResEHigh"],
                        resPeak, val);
  }

  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(calUpdateRS(QtProperty *, double)));
}

/**
 * This function enables/disables the display of the options involved in
 *creating the RES file.
 *
 * @param state :: whether checkbox is checked or unchecked
 */
void ISISCalibration::resCheck(bool state) {
  m_uiForm.ppResolution->getRangeSelector("ResPeak")->setVisible(state);
  m_uiForm.ppResolution->getRangeSelector("ResBackground")->setVisible(state);

  // Toggle scale and smooth options
  m_uiForm.ckResolutionScale->setEnabled(state);
  m_uiForm.ckSmoothResolution->setEnabled(state);
}

/**
 * Called when a user starts to type / edit the runs to load.
 */
void ISISCalibration::pbRunEditing() {
  updateRunButton(false, "unchanged", "Editing...",
                  "Run numbers are currently being edited.");
}

/**
 * Called when the FileFinder starts finding the files.
 */
void ISISCalibration::pbRunFinding() {
  updateRunButton(false, "unchanged", "Finding files...",
                  "Searching for data files for the run numbers entered...");
  m_uiForm.leRunNo->setEnabled(false);
}

/**
 * Called when the FileFinder has finished finding the files.
 */
void ISISCalibration::pbRunFinished() {
  if (!m_uiForm.leRunNo->isValid())
    updateRunButton(
        false, "unchanged", "Invalid Run(s)",
        "Cannot find data files for some of the run numbers entered.");
  else
    updateRunButton();

  m_uiForm.leRunNo->setEnabled(true);
}
/**
 * Handle saving of workspace
 */
void ISISCalibration::saveClicked() {
  checkADSForPlotSaveWorkspace(m_outputCalibrationName.toStdString(), false);
  addSaveWorkspaceToQueue(m_outputCalibrationName);

  if (m_uiForm.ckCreateResolution->isChecked()) {
    checkADSForPlotSaveWorkspace(m_outputResolutionName.toStdString(), false);
    addSaveWorkspaceToQueue(m_outputResolutionName);
  }
  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Handle when Run is clicked
 */
void ISISCalibration::runClicked() { runTab(); }

void ISISCalibration::addRuntimeSmoothing(const QString &workspaceName) {
  auto smoothAlg = AlgorithmManager::Instance().create("WienerSmooth");
  smoothAlg->initialize();
  smoothAlg->setProperty("OutputWorkspace", workspaceName.toStdString());

  BatchAlgorithmRunner::AlgorithmRuntimeProps smoothAlgInputProps;
  smoothAlgInputProps["InputWorkspace"] =
      workspaceName.toStdString() + "_pre_smooth";
  m_batchAlgoRunner->addAlgorithm(smoothAlg, smoothAlgInputProps);
}

IAlgorithm_sptr
ISISCalibration::calibrationAlgorithm(const QString &inputFiles) {
  auto calibrationAlg =
      AlgorithmManager::Instance().create("IndirectCalibration");
  calibrationAlg->initialize();
  calibrationAlg->setProperty("InputFiles", inputFiles.toStdString());
  calibrationAlg->setProperty("OutputWorkspace",
                              m_outputCalibrationName.toStdString());
  calibrationAlg->setProperty("DetectorRange",
                              instrumentDetectorRangeString().toStdString());
  calibrationAlg->setProperty("PeakRange", peakRangeString().toStdString());
  calibrationAlg->setProperty("BackgroundRange",
                              backgroundRangeString().toStdString());
  calibrationAlg->setProperty("LoadLogFiles",
                              m_uiForm.ckLoadLogFiles->isChecked());

  if (m_uiForm.ckScale->isChecked())
    calibrationAlg->setProperty("ScaleFactor", m_uiForm.spScale->value());
  return calibrationAlg;
}

IAlgorithm_sptr
ISISCalibration::resolutionAlgorithm(const QString &inputFiles) const {
  auto resAlg = AlgorithmManager::Instance().create("IndirectResolution", -1);
  resAlg->initialize();
  resAlg->setProperty("InputFiles", inputFiles.toStdString());
  resAlg->setProperty("Instrument", getInstrumentName().toStdString());
  resAlg->setProperty("Analyser", getAnalyserName().toStdString());
  resAlg->setProperty("Reflection", getReflectionName().toStdString());
  resAlg->setProperty("RebinParam", rebinString().toStdString());
  resAlg->setProperty("DetectorRange",
                      resolutionDetectorRangeString().toStdString());
  resAlg->setProperty("BackgroundRange", backgroundString().toStdString());
  resAlg->setProperty("LoadLogFiles", m_uiForm.ckLoadLogFiles->isChecked());

  if (m_uiForm.ckResolutionScale->isChecked())
    resAlg->setProperty("ScaleFactor", m_uiForm.spScale->value());

  if (m_uiForm.ckSmoothResolution->isChecked())
    resAlg->setProperty("OutputWorkspace",
                        m_outputResolutionName.toStdString() + "_pre_smooth");
  else
    resAlg->setProperty("OutputWorkspace",
                        m_outputResolutionName.toStdString());
  return resAlg;
}

IAlgorithm_sptr ISISCalibration::energyTransferReductionAlgorithm(
    const QString &inputFiles) const {
  auto reductionAlg =
      AlgorithmManager::Instance().create("ISISIndirectEnergyTransferWrapper");
  reductionAlg->initialize();
  reductionAlg->setProperty("Instrument", getInstrumentName().toStdString());
  reductionAlg->setProperty("Analyser", getAnalyserName().toStdString());
  reductionAlg->setProperty("Reflection", getReflectionName().toStdString());
  reductionAlg->setProperty("InputFiles", inputFiles.toStdString());
  reductionAlg->setProperty("SumFiles", m_uiForm.ckSumFiles->isChecked());
  reductionAlg->setProperty("OutputWorkspace",
                            "__IndirectCalibration_reduction");
  reductionAlg->setProperty("SpectraRange",
                            resolutionDetectorRangeString().toStdString());
  reductionAlg->setProperty("LoadLogFiles",
                            m_uiForm.ckLoadLogFiles->isChecked());
  return reductionAlg;
}

void ISISCalibration::setRunEnabled(bool enabled) {
  m_uiForm.pbRun->setEnabled(enabled);
}

void ISISCalibration::setSaveEnabled(bool enabled) {
  m_uiForm.pbSave->setEnabled(enabled);
}

void ISISCalibration::updateRunButton(bool enabled,
                                      std::string const &enableOutputButtons,
                                      QString const &message,
                                      QString const &tooltip) {
  setRunEnabled(enabled);
  m_uiForm.pbRun->setText(message);
  m_uiForm.pbRun->setToolTip(tooltip);
  if (enableOutputButtons != "unchanged")
    setSaveEnabled(enableOutputButtons == "enable");
}

} // namespace CustomInterfaces
} // namespace MantidQt
