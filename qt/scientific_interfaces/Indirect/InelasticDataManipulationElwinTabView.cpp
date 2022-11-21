// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "InelasticDataManipulationElwinTabView.h"
#include "MantidQtWidgets/Common/AlgorithmRuntimeProps.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include "MantidGeometry/Instrument.h"
#include "MantidQtWidgets/Common/SignalBlocker.h"
#include "MantidQtWidgets/Plotting/RangeSelector.h"

#include <QFileInfo>

#include <algorithm>

#include "IndirectAddWorkspaceDialog.h"

using namespace Mantid::API;
using namespace MantidQt::API;

namespace {
Mantid::Kernel::Logger g_log("Elwin");

MatrixWorkspace_sptr getADSMatrixWorkspace(std::string const &workspaceName) {
  return AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(workspaceName);
}

bool doesExistInADS(std::string const &workspaceName) {
  return AnalysisDataService::Instance().doesExist(workspaceName);
}

std::vector<std::string> getOutputWorkspaceSuffices() { return {"_eq", "_eq2", "_elf", "_elt"}; }

std::string extractLastOf(const std::string &str, const std::string &delimiter) {
  auto const cutIndex = str.rfind(delimiter);
  if (cutIndex != std::string::npos)
    return str.substr(cutIndex + 1, str.size() - cutIndex);
  return str;
}

template <typename Iterator, typename Functor>
std::vector<std::string> transformElements(Iterator const fromIter, Iterator const toIter, Functor const &functor) {
  std::vector<std::string> newVector;
  newVector.reserve(toIter - fromIter);
  std::transform(fromIter, toIter, std::back_inserter(newVector), functor);
  return newVector;
}

template <typename T, typename Predicate> void removeElementsIf(std::vector<T> &vector, Predicate const &filter) {
  auto const iter = std::remove_if(vector.begin(), vector.end(), filter);
  if (iter != vector.end())
    vector.erase(iter, vector.end());
}

std::vector<std::string> extractSuffixes(QStringList const &files, std::string const &delimiter) {
  return transformElements(files.begin(), files.end(), [&](QString const &file) {
    QFileInfo const fileInfo(file);
    return extractLastOf(fileInfo.baseName().toStdString(), delimiter);
  });
}

std::vector<std::string> attachPrefix(std::vector<std::string> const &strings, std::string const &prefix) {
  return transformElements(strings.begin(), strings.end(), [&prefix](std::string const &str) { return prefix + str; });
}

std::vector<std::string> getFilteredSuffixes(QStringList const &files) {
  auto suffixes = extractSuffixes(files, "_");

  removeElementsIf(suffixes, [&](std::string const &suffix) { return suffix != "red" && suffix != "sqw"; });
  return suffixes;
}

IAlgorithm_sptr loadAlgorithm(std::string const &filepath, std::string const &outputName) {
  auto loadAlg = AlgorithmManager::Instance().create("LoadNexus");
  loadAlg->initialize();
  loadAlg->setProperty("Filename", filepath);
  loadAlg->setProperty("OutputWorkspace", outputName);
  return loadAlg;
}

namespace Regexes {
const QString EMPTY = "^$";
const QString SPACE = "(\\s)*";
const QString COMMA = SPACE + "," + SPACE;
const QString NATURAL_NUMBER = "(0|[1-9][0-9]*)";
const QString REAL_NUMBER = "(-?" + NATURAL_NUMBER + "(\\.[0-9]*)?)";
const QString REAL_RANGE = "(" + REAL_NUMBER + COMMA + REAL_NUMBER + ")";
const QString MASK_LIST = "(" + REAL_RANGE + "(" + COMMA + REAL_RANGE + ")*" + ")|" + EMPTY;
} // namespace Regexes

class ExcludeRegionDelegate : public QItemDelegate {
public:
  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem & /*option*/,
                        const QModelIndex & /*index*/) const override {
    auto lineEdit = std::make_unique<QLineEdit>(parent);
    auto validator = std::make_unique<QRegExpValidator>(QRegExp(Regexes::MASK_LIST), parent);
    lineEdit->setValidator(validator.release());
    return lineEdit.release();
  }

  void setEditorData(QWidget *editor, const QModelIndex &index) const override {
    const auto value = index.model()->data(index, Qt::EditRole).toString();
    static_cast<QLineEdit *>(editor)->setText(value);
  }

  void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override {
    auto *lineEdit = static_cast<QLineEdit *>(editor);
    model->setData(index, lineEdit->text(), Qt::EditRole);
  }

  void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                            const QModelIndex & /*index*/) const override {
    editor->setGeometry(option.rect);
  }
};

QStringList defaultHeaders() {
  QStringList headers;
  headers << "Workspace"
          << "WS Index";
  return headers;
}

class ScopedFalse {
  bool &m_ref;
  bool m_oldValue;

public:
  // this sets the input bool to false whilst this object is in scope and then
  // resets it to its old value when this object drops out of scope.
  explicit ScopedFalse(bool &variable) : m_ref(variable), m_oldValue(variable) { m_ref = false; }
  ~ScopedFalse() { m_ref = m_oldValue; }
};

QStringList getSampleWSSuffices() {
  QStringList const wsSampleSuffixes{"red", "sqw"};
  return wsSampleSuffixes;
}

QStringList getSampleFBSuffices() {
  QStringList const fbSampleSuffixes{"red.*", "sqw.*"};
  return fbSampleSuffixes;
}

QPair<double, double> getXRangeFromWorkspace(const Mantid::API::MatrixWorkspace_const_sptr &workspace) {
  auto const xValues = workspace->x(0);
  return QPair<double, double>(xValues.front(), xValues.back());
}
} // namespace

namespace MantidQt::CustomInterfaces {
using namespace IDA;
InelasticDataManipulationElwinTabView::InelasticDataManipulationElwinTabView(QWidget *parent) : m_elwTree(nullptr) {

  // Create Editor Factories
  m_dblEdFac = new DoubleEditorFactory(this);
  m_blnEdFac = new QtCheckBoxFactory(this);
  m_dblManager = new QtDoublePropertyManager();
  m_blnManager = new QtBoolPropertyManager();
  m_grpManager = new QtGroupPropertyManager();

  m_uiForm.setupUi(parent);
  connect(m_uiForm.wkspAdd, SIGNAL(clicked()), this, SIGNAL(addDataClicked()));
  connect(m_uiForm.wkspRemove, SIGNAL(clicked()), this, SIGNAL(removeDataClicked()));

  setup();
}

InelasticDataManipulationElwinTabView::~InelasticDataManipulationElwinTabView() {
  m_elwTree->unsetFactoryForManager(m_dblManager);
  m_elwTree->unsetFactoryForManager(m_blnManager);
}

void InelasticDataManipulationElwinTabView::setup() {
  // Create QtTreePropertyBrowser object
  m_elwTree = new QtTreePropertyBrowser();
  m_uiForm.properties->addWidget(m_elwTree);

  // Editor Factories
  m_elwTree->setFactoryForManager(m_dblManager, m_dblEdFac);
  m_elwTree->setFactoryForManager(m_blnManager, m_blnEdFac);

  // Number of decimal places in property browsers.
  static const unsigned int NUM_DECIMALS = 6;
  // Create Properties
  m_properties["IntegrationStart"] = m_dblManager->addProperty("Start");
  m_dblManager->setDecimals(m_properties["IntegrationStart"], NUM_DECIMALS);
  m_properties["IntegrationEnd"] = m_dblManager->addProperty("End");
  m_dblManager->setDecimals(m_properties["IntegrationEnd"], NUM_DECIMALS);
  m_properties["BackgroundStart"] = m_dblManager->addProperty("Start");
  m_dblManager->setDecimals(m_properties["BackgroundStart"], NUM_DECIMALS);
  m_properties["BackgroundEnd"] = m_dblManager->addProperty("End");
  m_dblManager->setDecimals(m_properties["BackgroundEnd"], NUM_DECIMALS);

  m_properties["BackgroundSubtraction"] = m_blnManager->addProperty("Background Subtraction");
  m_properties["Normalise"] = m_blnManager->addProperty("Normalise to Lowest Temp");

  m_properties["IntegrationRange"] = m_grpManager->addProperty("Integration Range");
  m_properties["IntegrationRange"]->addSubProperty(m_properties["IntegrationStart"]);
  m_properties["IntegrationRange"]->addSubProperty(m_properties["IntegrationEnd"]);
  m_properties["BackgroundRange"] = m_grpManager->addProperty("Background Range");
  m_properties["BackgroundRange"]->addSubProperty(m_properties["BackgroundStart"]);
  m_properties["BackgroundRange"]->addSubProperty(m_properties["BackgroundEnd"]);

  m_elwTree->addProperty(m_properties["IntegrationRange"]);
  m_elwTree->addProperty(m_properties["BackgroundSubtraction"]);
  m_elwTree->addProperty(m_properties["BackgroundRange"]);
  m_elwTree->addProperty(m_properties["Normalise"]);

  // We always want one range selector... the second one can be controlled from
  // within the elwinTwoRanges(bool state) function
  auto integrationRangeSelector = m_uiForm.ppPlot->addRangeSelector("ElwinIntegrationRange");
  integrationRangeSelector->setBounds(-DBL_MAX, DBL_MAX);
  connect(integrationRangeSelector, SIGNAL(minValueChanged(double)), this, SLOT(minChanged(double)));
  connect(integrationRangeSelector, SIGNAL(maxValueChanged(double)), this, SLOT(maxChanged(double)));
  // create the second range
  auto backgroundRangeSelector = m_uiForm.ppPlot->addRangeSelector("ElwinBackgroundRange");
  backgroundRangeSelector->setColour(Qt::darkGreen); // dark green for background
  backgroundRangeSelector->setBounds(-DBL_MAX, DBL_MAX);
  connect(integrationRangeSelector, SIGNAL(selectionChanged(double, double)), backgroundRangeSelector,
          SLOT(setRange(double, double)));
  connect(backgroundRangeSelector, SIGNAL(minValueChanged(double)), this, SLOT(minChanged(double)));
  connect(backgroundRangeSelector, SIGNAL(maxValueChanged(double)), this, SLOT(maxChanged(double)));

  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this, SLOT(updateRS(QtProperty *, double)));
  connect(m_blnManager, SIGNAL(valueChanged(QtProperty *, bool)), this, SLOT(twoRanges(QtProperty *, bool)));
  twoRanges(m_properties["BackgroundSubtraction"], false);

  connect(m_uiForm.dsInputFiles, SIGNAL(filesFound()), this, SIGNAL(filesFound()));
  connect(m_uiForm.cbPreviewFile, SIGNAL(currentIndexChanged(int)), this, SIGNAL(previewIndexChanged(int)));
  connect(m_uiForm.spPlotSpectrum, SIGNAL(valueChanged(int)), this, SIGNAL(selectedSpectrumChanged(int)));
  connect(m_uiForm.cbPlotSpectrum, SIGNAL(currentIndexChanged(int)), this, SIGNAL(selectedSpectrumChanged(int)));

  // Handle plot and save
  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SIGNAL(runClicked()));
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SIGNAL(saveClicked()));
  connect(m_uiForm.pbPlotPreview, SIGNAL(clicked()), this, SIGNAL(plotPreviewClicked()));

  // Set any default values
  m_dblManager->setValue(m_properties["IntegrationStart"], -0.02);
  m_dblManager->setValue(m_properties["IntegrationEnd"], 0.02);

  m_dblManager->setValue(m_properties["BackgroundStart"], -0.24);
  m_dblManager->setValue(m_properties["BackgroundEnd"], -0.22);
}

IndirectPlotOptionsView *InelasticDataManipulationElwinTabView::getPlotOptions() { return m_uiForm.ipoPlotOptions; }

bool InelasticDataManipulationElwinTabView::validate() {
  if (validateFileSuffix()) {
    UserInputValidator uiv;
    auto rangeOne = std::make_pair(m_dblManager->value(m_properties["IntegrationStart"]),
                                   m_dblManager->value(m_properties["IntegrationEnd"]));
    uiv.checkValidRange("Range One", rangeOne);

    bool useTwoRanges = m_blnManager->value(m_properties["BackgroundSubtraction"]);
    if (useTwoRanges) {
      auto rangeTwo = std::make_pair(m_dblManager->value(m_properties["BackgroundStart"]),
                                     m_dblManager->value(m_properties["BackgroundEnd"]));
      uiv.checkValidRange("Range Two", rangeTwo);
      uiv.checkRangesDontOverlap(rangeOne, rangeTwo);
    }

    auto const errorMessage = uiv.generateErrorMessage();
    if (!errorMessage.isEmpty())
      showMessageBox(errorMessage);
    return errorMessage.isEmpty();
  } else
    return false;
}
bool InelasticDataManipulationElwinTabView::validateFileSuffix() {
  UserInputValidator uiv;

  if (m_uiForm.inputChoice->currentIndex() == 0) {
    uiv.checkFileFinderWidgetIsValid("Input", m_uiForm.dsInputFiles);
    auto const suffixes = getFilteredSuffixes(m_uiForm.dsInputFiles->getFilenames());
    if (std::adjacent_find(suffixes.begin(), suffixes.end(), std::not_equal_to<>()) != suffixes.end())
      uiv.addErrorMessage("The input files must be all _red or all _sqw.");
  }
  auto const errorMessage = uiv.generateErrorMessage();
  if (!errorMessage.isEmpty())
    showMessageBox(errorMessage);
  return errorMessage.isEmpty();
}

void InelasticDataManipulationElwinTabView::setFBSuffixes(QStringList const suffix) {
  m_uiForm.dsInputFiles->setFileExtensions(suffix);
}

void InelasticDataManipulationElwinTabView::setDefaultResolution(const Mantid::API::MatrixWorkspace_const_sptr &ws,
                                                                 const QPair<double, double> &range) {
  auto inst = ws->getInstrument();
  auto analyser = inst->getStringParameter("analyser");

  if (analyser.size() > 0) {
    auto comp = inst->getComponentByName(analyser[0]);

    if (comp) {
      auto params = comp->getNumberParameter("resolution", true);

      // set the default instrument resolution
      if (!params.empty()) {
        double res = params[0];
        m_dblManager->setValue(m_properties["IntegrationStart"], -res);
        m_dblManager->setValue(m_properties["IntegrationEnd"], res);

        m_dblManager->setValue(m_properties["BackgroundStart"], -10 * res);
        m_dblManager->setValue(m_properties["BackgroundEnd"], -9 * res);
      } else {
        m_dblManager->setValue(m_properties["IntegrationStart"], range.first);
        m_dblManager->setValue(m_properties["IntegrationEnd"], range.second);
      }
    } else {
      showMessageBox("Warning: The instrument definition file for the input "
                     "workspace contains an invalid value.");
    }
  }
}

void InelasticDataManipulationElwinTabView::setDefaultSampleLog(const Mantid::API::MatrixWorkspace_const_sptr &ws) {
  auto inst = ws->getInstrument();
  // Set sample environment log name
  auto log = inst->getStringParameter("Workflow.SE-log");
  QString logName("sample");
  if (log.size() > 0) {
    logName = QString::fromStdString(log[0]);
  }
  m_uiForm.leLogName->setText(logName);
  // Set sample environment log value
  auto logval = inst->getStringParameter("Workflow.SE-log-value");
  if (logval.size() > 0) {
    auto logValue = QString::fromStdString(logval[0]);
    int index = m_uiForm.leLogValue->findText(logValue);
    if (index >= 0) {
      m_uiForm.leLogValue->setCurrentIndex(index);
    }
  }
}

/**
 * Handles a new set of input files being entered.
 *
 * Updates preview selection combo box.
 */
void InelasticDataManipulationElwinTabView::newInputFiles() {
  // Clear the existing list of files
  m_uiForm.cbPreviewFile->clear();

  // Populate the combo box with the filenames
  QStringList filenames = getInputFilenames();
  for (auto rawFilename : filenames) {
    QFileInfo inputFileInfo(rawFilename);
    QString sampleName = inputFileInfo.baseName();
    // Add the item using the base filename as the display string and the raw
    // filename as the data value
    m_uiForm.cbPreviewFile->addItem(sampleName, rawFilename);
  }

  // Default to the first file
  setPreviewToDefault();
}

/**
 * Handles a new set of input files being entered.
 *
 * Updates preview selection combo box.
 */
void InelasticDataManipulationElwinTabView::newInputFilesFromDialog(IAddWorkspaceDialog const *dialog) {
  // Populate the combo box with the filenames
  QString workspaceNames;
  QString filename;
  if (const auto indirectDialog = dynamic_cast<IndirectAddWorkspaceDialog const *>(dialog)) {
    workspaceNames = QString::fromStdString(indirectDialog->workspaceName());
    filename = QString::fromStdString(indirectDialog->getFileName());
  }
  m_uiForm.cbPreviewFile->addItem(workspaceNames, filename);

  // Default to the first file
  setPreviewToDefault();
}

void InelasticDataManipulationElwinTabView::clearPreviewFile() { m_uiForm.cbPreviewFile->clear(); }

void InelasticDataManipulationElwinTabView::setPreviewToDefault() {
  m_uiForm.cbPreviewFile->setCurrentIndex(0);
  QString const wsname = m_uiForm.cbPreviewFile->currentText();
  auto const inputWs = getADSMatrixWorkspace(wsname.toStdString());
  const auto range = getXRangeFromWorkspace(inputWs);

  setRangeSelector(m_uiForm.ppPlot->getRangeSelector("ElwinIntegrationRange"), m_properties["IntegrationStart"],
                   m_properties["IntegrationEnd"], range);
  setRangeSelector(m_uiForm.ppPlot->getRangeSelector("ElwinBackgroundRange"), m_properties["BackgroundStart"],
                   m_properties["BackgroundEnd"], range);
}

void InelasticDataManipulationElwinTabView::newPreviewFileSelected(const MatrixWorkspace_sptr &workspace) {
  if (m_uiForm.inputChoice->currentIndex() == 0) {
    int const numHist = static_cast<int>(workspace->getNumberHistograms()) - 1;
    m_uiForm.spPlotSpectrum->setMaximum(numHist);
    m_uiForm.spPlotSpectrum->setValue(0);
  }
}

/**
 * Plots the selected spectrum of the input workspace.
 *
 * @param previewPlot The preview plot widget in which to plot the input
 *                    input workspace.
 */
void InelasticDataManipulationElwinTabView::plotInput(MatrixWorkspace_sptr inputWS, int spectrum) {
  m_uiForm.ppPlot->clear();

  if (inputWS && inputWS->x(spectrum).size() > 1) {
    m_uiForm.ppPlot->addSpectrum("Sample", inputWS, spectrum);
  }
  setDefaultSampleLog(inputWS);
}

void InelasticDataManipulationElwinTabView::twoRanges(QtProperty *prop, bool enabled) {
  if (prop == m_properties["BackgroundSubtraction"]) {
    auto integrationRangeSelector = m_uiForm.ppPlot->getRangeSelector("ElwinIntegrationRange");
    auto backgroundRangeSelector = m_uiForm.ppPlot->getRangeSelector("ElwinBackgroundRange");
    backgroundRangeSelector->setVisible(enabled);
    m_properties["BackgroundStart"]->setEnabled(enabled);
    m_properties["BackgroundEnd"]->setEnabled(enabled);

    disconnect(integrationRangeSelector, SIGNAL(selectionChanged(double, double)), backgroundRangeSelector,
               SLOT(setRange(double, double)));
    if (!enabled) {
      backgroundRangeSelector->setRange(integrationRangeSelector->getRange());
      connect(integrationRangeSelector, SIGNAL(selectionChanged(double, double)), backgroundRangeSelector,
              SLOT(setRange(double, double)));
    }
  }
}

void InelasticDataManipulationElwinTabView::minChanged(double val) {
  auto integrationRangeSelector = m_uiForm.ppPlot->getRangeSelector("ElwinIntegrationRange");
  auto backgroundRangeSelector = m_uiForm.ppPlot->getRangeSelector("ElwinBackgroundRange");

  MantidWidgets::RangeSelector *from = qobject_cast<MantidWidgets::RangeSelector *>(sender());

  disconnect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this, SLOT(updateRS(QtProperty *, double)));
  if (from == integrationRangeSelector) {
    m_dblManager->setValue(m_properties["IntegrationStart"], val);
  } else if (from == backgroundRangeSelector) {
    m_dblManager->setValue(m_properties["BackgroundStart"], val);
  }

  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this, SLOT(updateRS(QtProperty *, double)));
}

void InelasticDataManipulationElwinTabView::maxChanged(double val) {
  auto integrationRangeSelector = m_uiForm.ppPlot->getRangeSelector("ElwinIntegrationRange");
  auto backgroundRangeSelector = m_uiForm.ppPlot->getRangeSelector("ElwinBackgroundRange");

  MantidWidgets::RangeSelector *from = qobject_cast<MantidWidgets::RangeSelector *>(sender());

  disconnect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this, SLOT(updateRS(QtProperty *, double)));

  if (from == integrationRangeSelector) {
    m_dblManager->setValue(m_properties["IntegrationEnd"], val);
  } else if (from == backgroundRangeSelector) {
    m_dblManager->setValue(m_properties["BackgroundEnd"], val);
  }

  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this, SLOT(updateRS(QtProperty *, double)));
}

void InelasticDataManipulationElwinTabView::updateRS(QtProperty *prop, double val) {
  auto integrationRangeSelector = m_uiForm.ppPlot->getRangeSelector("ElwinIntegrationRange");
  auto backgroundRangeSelector = m_uiForm.ppPlot->getRangeSelector("ElwinBackgroundRange");

  disconnect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this, SLOT(updateRS(QtProperty *, double)));

  if (prop == m_properties["IntegrationStart"])
    setRangeSelectorMin(m_properties["IntegrationStart"], m_properties["IntegrationEnd"], integrationRangeSelector,
                        val);
  else if (prop == m_properties["IntegrationEnd"])
    setRangeSelectorMax(m_properties["IntegrationStart"], m_properties["IntegrationEnd"], integrationRangeSelector,
                        val);
  else if (prop == m_properties["BackgroundStart"])
    setRangeSelectorMin(m_properties["BackgroundStart"], m_properties["BackgroundEnd"], backgroundRangeSelector, val);
  else if (prop == m_properties["BackgroundEnd"])
    setRangeSelectorMax(m_properties["BackgroundStart"], m_properties["BackgroundEnd"], backgroundRangeSelector, val);

  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this, SLOT(updateRS(QtProperty *, double)));
}

/**
 * Set the position of the range selectors on the mini plot
 *
 * @param rs :: Pointer to the RangeSelector
 * @param lower :: The lower bound property in the property browser
 * @param upper :: The upper bound property in the property browser
 * @param bounds :: The upper and lower bounds to be set
 * @param range :: The range to set the range selector to.
 */
void InelasticDataManipulationElwinTabView::setRangeSelector(RangeSelector *rs, QtProperty *lower, QtProperty *upper,
                                                             const QPair<double, double> &range,
                                                             const boost::optional<QPair<double, double>> &bounds) {
  m_dblManager->setValue(lower, range.first);
  m_dblManager->setValue(upper, range.second);
  rs->setRange(range.first, range.second);
  if (bounds) {
    // clamp the bounds of the selector
    rs->setBounds(bounds.get().first, bounds.get().second);
  }
}

/**
 * Set the minimum of a range selector if it is less than the maximum value.
 * To be used when changing the min or max via the Property table
 *
 * @param minProperty :: The property managing the minimum of the range
 * @param maxProperty :: The property managing the maximum of the range
 * @param rangeSelector :: The range selector
 * @param newValue :: The new value for the minimum
 */
void InelasticDataManipulationElwinTabView::setRangeSelectorMin(QtProperty *minProperty, QtProperty *maxProperty,
                                                                RangeSelector *rangeSelector, double newValue) {
  if (newValue <= maxProperty->valueText().toDouble())
    rangeSelector->setMinimum(newValue);
  else
    m_dblManager->setValue(minProperty, rangeSelector->getMinimum());
}

/**
 * Set the maximum of a range selector if it is greater than the minimum value
 * To be used when changing the min or max via the Property table
 *
 * @param minProperty :: The property managing the minimum of the range
 * @param maxProperty :: The property managing the maximum of the range
 * @param rangeSelector :: The range selector
 * @param newValue :: The new value for the maximum
 */
void InelasticDataManipulationElwinTabView::setRangeSelectorMax(QtProperty *minProperty, QtProperty *maxProperty,
                                                                RangeSelector *rangeSelector, double newValue) {
  if (newValue >= minProperty->valueText().toDouble())
    rangeSelector->setMaximum(newValue);
  else
    m_dblManager->setValue(maxProperty, rangeSelector->getMaximum());
}

void InelasticDataManipulationElwinTabView::setRunIsRunning(const bool &running) {
  m_uiForm.pbRun->setText(running ? "Running..." : "Run");
  setButtonsEnabled(!running);
  m_uiForm.ppPlot->watchADS(!running);
}

void InelasticDataManipulationElwinTabView::setButtonsEnabled(const bool &enabled) {
  setRunEnabled(enabled);
  setSaveResultEnabled(enabled);
}

void InelasticDataManipulationElwinTabView::setRunEnabled(const bool &enabled) { m_uiForm.pbRun->setEnabled(enabled); }

void InelasticDataManipulationElwinTabView::setSaveResultEnabled(const bool &enabled) {
  m_uiForm.pbSave->setEnabled(enabled);
}

std::unique_ptr<IAddWorkspaceDialog>
InelasticDataManipulationElwinTabView::getAddWorkspaceDialog(QWidget *parent) const {
  return std::make_unique<IndirectAddWorkspaceDialog>(parent);
}

void InelasticDataManipulationElwinTabView::clearInputFiles() { m_uiForm.dsInputFiles->clear(); }

void InelasticDataManipulationElwinTabView::showAddWorkspaceDialog() {
  if (!m_addWorkspaceDialog)
    m_addWorkspaceDialog = getAddWorkspaceDialog(m_parent);
  m_addWorkspaceDialog->setWSSuffices(getSampleWSSuffices());
  m_addWorkspaceDialog->setFBSuffices(getSampleFBSuffices());
  m_addWorkspaceDialog->updateSelectedSpectra();
  m_addWorkspaceDialog->show();
  connect(m_addWorkspaceDialog.get(), SIGNAL(addData()), this, SLOT(addData()));
  connect(m_addWorkspaceDialog.get(), SIGNAL(closeDialog()), this, SLOT(closeDialog()));
}

void InelasticDataManipulationElwinTabView::closeDialog() {
  disconnect(m_addWorkspaceDialog.get(), SIGNAL(addData()), this, SLOT(addData()));
  disconnect(m_addWorkspaceDialog.get(), SIGNAL(closeDialog()), this, SLOT(closeDialog()));
  m_addWorkspaceDialog->close();
  m_addWorkspaceDialog = nullptr;
}

void InelasticDataManipulationElwinTabView::addDataToModel(IAddWorkspaceDialog const *dialog) {
  if (const auto indirectDialog = dynamic_cast<IndirectAddWorkspaceDialog const *>(dialog))
    m_dataModel->addWorkspace(indirectDialog->workspaceName(), indirectDialog->workspaceIndices());
}

void InelasticDataManipulationElwinTabView::updateTableFromModel() {
  ScopedFalse _signalBlock(m_emitCellChanged);
  m_dataTable->setRowCount(0);
  for (auto domainIndex = FitDomainIndex{0}; domainIndex < m_dataModel->getNumberOfDomains(); domainIndex++) {
    addTableEntry(domainIndex);
  }
}

QTableWidget *InelasticDataManipulationElwinTabView::getDataTable() const { return m_uiForm.tbElwinData; }

void InelasticDataManipulationElwinTabView::addTableEntry(FitDomainIndex row) {
  m_dataTable->insertRow(static_cast<int>(row.value));
  const auto &name = m_dataModel->getWorkspace(row)->getName();
  auto cell = std::make_unique<QTableWidgetItem>(QString::fromStdString(name));
  auto flags = cell->flags();
  flags ^= Qt::ItemIsEditable;
  cell->setFlags(flags);
  setCell(std::move(cell), row.value, 0);

  cell = std::make_unique<QTableWidgetItem>(QString::number(m_dataModel->getSpectrum(row)));
  cell->setFlags(flags);
  setCell(std::move(cell), row.value, workspaceIndexColumn());
}

void InelasticDataManipulationElwinTabView::setCell(std::unique_ptr<QTableWidgetItem> cell, FitDomainIndex row,
                                                    int column) {
  m_dataTable->setItem(static_cast<int>(row.value), column, cell.release());
}

void InelasticDataManipulationElwinTabView::setCellText(const QString &text, FitDomainIndex row, int column) {
  m_dataTable->item(static_cast<int>(row.value), column)->setText(text);
}

int InelasticDataManipulationElwinTabView::workspaceIndexColumn() const { return 1; }

void InelasticDataManipulationElwinTabView::setHorizontalHeaders(const QStringList &headers) {
  m_dataTable->setColumnCount(headers.size());
  m_dataTable->setHorizontalHeaderLabels(headers);

  auto header = m_dataTable->horizontalHeader();
  header->setSectionResizeMode(0, QHeaderView::Stretch);
}

void InelasticDataManipulationElwinTabView::setAvailableSpectra(WorkspaceIndex minimum, WorkspaceIndex maximum) {
  m_uiForm.elwinPreviewSpec->setCurrentIndex(0);
  m_uiForm.spPlotSpectrum->setMinimum(boost::numeric_cast<int>(minimum.value));
  m_uiForm.spPlotSpectrum->setMaximum(boost::numeric_cast<int>(maximum.value));
}

void InelasticDataManipulationElwinTabView::setAvailableSpectra(const std::vector<WorkspaceIndex>::const_iterator &from,
                                                                const std::vector<WorkspaceIndex>::const_iterator &to) {
  m_uiForm.elwinPreviewSpec->setCurrentIndex(1);
  m_uiForm.cbPlotSpectrum->clear();

  for (auto spectrum = from; spectrum < to; ++spectrum)
    m_uiForm.cbPlotSpectrum->addItem(QString::number(spectrum->value));
}

int InelasticDataManipulationElwinTabView::getCurrentInputIndex() { return m_uiForm.inputChoice->currentIndex(); }

QString InelasticDataManipulationElwinTabView::getPreviewWorkspaceName(int index) {
  return m_uiForm.cbPreviewFile->itemText(index);
}

QString InelasticDataManipulationElwinTabView::getPreviewFilename(int index) {
  return m_uiForm.cbPreviewFile->itemData(index).toString();
}

int InelasticDataManipulationElwinTabView::getPreviewSpec() { return m_uiForm.elwinPreviewSpec->currentIndex(); }

QString InelasticDataManipulationElwinTabView::getCurrentPreview() { return m_uiForm.cbPreviewFile->currentText(); }

QStringList InelasticDataManipulationElwinTabView::getInputFilenames() { return m_uiForm.dsInputFiles->getFilenames(); }

bool InelasticDataManipulationElwinTabView::isLoadHistory() { return m_uiForm.ckLoadHistory->isChecked(); }

bool InelasticDataManipulationElwinTabView::isGroupInput() { return m_uiForm.ckGroupInput->isChecked(); }

double InelasticDataManipulationElwinTabView::getIntegrationStart() {
  return m_dblManager->value(m_properties["IntegrationStart"]);
}

double InelasticDataManipulationElwinTabView::getIntegrationEnd() {
  return m_dblManager->value(m_properties["IntegrationEnd"]);
}

double InelasticDataManipulationElwinTabView::getBackgroundStart() {
  return m_dblManager->value(m_properties["BackgroundStart"]);
}

double InelasticDataManipulationElwinTabView::getBackgroundEnd() {
  return m_dblManager->value(m_properties["BackgroundEnd"]);
}

bool InelasticDataManipulationElwinTabView::getBackgroundSubtraction() {
  return m_blnManager->value(m_properties["BackgroundSubtraction"]);
}

bool InelasticDataManipulationElwinTabView::getNormalise() { return m_blnManager->value(m_properties["Normalise"]); }

std::string InelasticDataManipulationElwinTabView::getLogName() { return m_uiForm.leLogName->text().toStdString(); }

std::string InelasticDataManipulationElwinTabView::getLogValue() {
  return m_uiForm.leLogValue->currentText().toStdString();
}

} // namespace MantidQt::CustomInterfaces