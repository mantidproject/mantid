#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TextAxis.h"
#include "MantidQtCustomInterfaces/Indirect/JumpFit.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"

#include <string>
#include <boost/lexical_cast.hpp>

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {
JumpFit::JumpFit(QWidget *parent) : IndirectBayesTab(parent) {
  m_uiForm.setupUi(parent);

  // Create range selector
  auto qRangeSelector = m_uiForm.ppPlot->addRangeSelector("JumpFitQ");
  connect(qRangeSelector, SIGNAL(selectionChangedLazy(double, double)), this,
          SLOT(qRangeChanged(double, double)));

  // Add the properties browser to the ui form
  m_uiForm.treeSpace->addWidget(m_propTree);

  // Fitting range
  m_properties["QMin"] = m_dblManager->addProperty("QMin");
  m_properties["QMax"] = m_dblManager->addProperty("QMax");

  m_dblManager->setDecimals(m_properties["QMin"], NUM_DECIMALS);
  m_dblManager->setDecimals(m_properties["QMax"], NUM_DECIMALS);

  m_propTree->addProperty(m_properties["QMin"]);
  m_propTree->addProperty(m_properties["QMax"]);

  // Fitting function
  m_properties["FitFunction"] = m_grpManager->addProperty("Fitting Parameters");
  m_propTree->addProperty(m_properties["FitFunction"]);

  m_uiForm.cbWidth->setEnabled(false);

  // Connect data selector to handler method
  connect(m_uiForm.dsSample, SIGNAL(dataReady(const QString &)), this,
          SLOT(handleSampleInputReady(const QString &)));
  // Connect width selector to handler method
  connect(m_uiForm.cbWidth, SIGNAL(currentIndexChanged(const QString &)), this,
          SLOT(handleWidthChange(const QString &)));

  // Connect algorithm runner to completion handler function
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(fitAlgDone(bool)));

  // Update fit parameters in browser when function is selected
  connect(m_uiForm.cbFunction, SIGNAL(currentIndexChanged(const QString &)),
          this, SLOT(fitFunctionSelected(const QString &)));

  fitFunctionSelected(m_uiForm.cbFunction->currentText());
}

void JumpFit::setup() {}

/**
 * Validate the form to check the program can be run
 *
 * @return :: Whether the form was valid
 */
bool JumpFit::validate() {
  UserInputValidator uiv;
  uiv.checkDataSelectorIsValid("Sample", m_uiForm.dsSample);

  // this workspace doesn't have any valid widths
  if (m_spectraList.size() == 0) {
    uiv.addErrorMessage(
        "Input workspace doesn't appear to contain any width data.");
  }

  QString errors = uiv.generateErrorMessage();
  if (!errors.isEmpty()) {
    emit showMessageBox(errors);
    return false;
  }

  return true;
}

/**
 * Collect the settings on the GUI and build a python
 * script that runs JumpFit
 */
void JumpFit::run() {
  bool plot = m_uiForm.chkPlot->isChecked();
  bool save = m_uiForm.chkSave->isChecked();
  runImpl(plot, save);
}

/**
 * Runs the JumpFit algorithm with preview parameters to update the preview
 * plot.
 */
void JumpFit::runPreviewAlgorithm() { runImpl(); }

/**
 * Runs algorithm.
 *
 * @param plot Enable/disable plotting
 * @param save Enable/disable saving
 */
void JumpFit::runImpl(bool plot, bool save) {
  // Do noting with invalid data
  if (!m_uiForm.dsSample->isValid())
    return;

  if (m_batchAlgoRunner->queueLength() > 0)
    return;

  // Fit function to use
  QString functionName = m_uiForm.cbFunction->currentText();
  QString functionString = "name=" + functionName;

  // Build function string
  QStringList parameters = getFunctionParameters(functionName);
  for (auto it = parameters.begin(); it != parameters.end(); ++it) {
    QString parameterName = *it;

    // Get the value form double manager
    QString name = "parameter_" + *it;
    double value = m_dblManager->value(m_properties[name]);
    QString parameterValue = QString::number(value);

    functionString += "," + parameterName + "=" + parameterValue;
  }

  std::string widthText = m_uiForm.cbWidth->currentText().toStdString();
  int width = m_spectraList[widthText];
  QString sample = m_uiForm.dsSample->getCurrentDataName();
  QString outputName =
      getWorkspaceBasename(sample) + "_" + functionName + "_fit";

  // Setup fit algorithm
  m_fitAlg = AlgorithmManager::Instance().create("Fit");
  m_fitAlg->initialize();

  m_fitAlg->setProperty("Function", functionString.toStdString());
  m_fitAlg->setProperty("InputWorkspace", sample.toStdString());
  m_fitAlg->setProperty("WorkspaceIndex", width);
  m_fitAlg->setProperty("IgnoreInvalidData", true);
  m_fitAlg->setProperty("StartX", m_dblManager->value(m_properties["QMin"]));
  m_fitAlg->setProperty("EndX", m_dblManager->value(m_properties["QMax"]));
  m_fitAlg->setProperty("CreateOutput", true);
  m_fitAlg->setProperty("Output", outputName.toStdString());

  m_batchAlgoRunner->addAlgorithm(m_fitAlg);

  // Add save step if required
  if (save) {
    QString outWsName = outputName + "_Workspace";
    addSaveWorkspaceToQueue(outWsName);
  }

  // Process plotting in MantidPlot
  if (plot)
    connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
            SLOT(plotFitResult(bool)));

  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Handles the JumpFit algorithm finishing, used to plot fit in miniplot.
 *
 * @param error True if the algorithm failed, false otherwise
 */
void JumpFit::fitAlgDone(bool error) {
  // Ignore errors
  if (error)
    return;

  std::string outName = m_fitAlg->getPropertyValue("Output");

  // Get output workspace name
  std::string outWsName = outName + "_Workspace";

  // Get the output workspace group
  MatrixWorkspace_sptr outputWorkspace =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWsName);
  TextAxis *axis = dynamic_cast<TextAxis *>(outputWorkspace->getAxis(1));

  // Find the fit and diff curves (data should already be plotted)
  for (unsigned int histIndex = 0;
       histIndex < outputWorkspace->getNumberHistograms(); histIndex++) {
    QString specName = QString::fromStdString(axis->label(histIndex));

    // Fit curve is red
    if (specName == "Calc")
      m_uiForm.ppPlot->addSpectrum("Fit", outputWorkspace, histIndex, Qt::red);

    // Difference curve is green
    if (specName == "Diff")
      m_uiForm.ppPlot->addSpectrum("Diff", outputWorkspace, histIndex,
                                   Qt::green);
  }

  // Update parameters in UI
  std::string paramTableName = outName + "_Parameters";

  ITableWorkspace_sptr paramTable =
      AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
          paramTableName);

  // Don't run the algorithm when updating parameter values
  disconnect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
             SLOT(runPreviewAlgorithm()));

  for (auto it = m_properties.begin(); it != m_properties.end(); ++it) {
    QString propName(it.key());
    if (propName.startsWith("parameter_")) {
      size_t row(0), col(0);
      paramTable->find(propName.split("_")[1].toStdString(), row, col);
      col++;
      double value = paramTable->cell<double>(row, col);
      m_dblManager->setValue(m_properties[propName], value);
    }
  }

  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(runPreviewAlgorithm()));
}

/**
 * Handles plotting of results within MantidPlot plots
 *
 * @param error If the algorithm failed
 */
void JumpFit::plotFitResult(bool error) {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
             SLOT(plotFitResult(bool)));

  // Ignore errors
  if (error)
    return;

  // Get output workspace name
  std::string outWsName = m_fitAlg->getPropertyValue("Output") + "_Workspace";

  // Plot in MantidPlot
  plotSpectrum(QString::fromStdString(outWsName), 0, 2);
}

/**
 * Set the data selectors to use the default save directory
 * when browsing for input files.
 *
* @param settings :: The current settings
 */
void JumpFit::loadSettings(const QSettings &settings) {
  m_uiForm.dsSample->readSettings(settings.group());
}

/**
 * Plots the loaded file to the miniplot and sets the guides
 * and the range
 *
 * @param filename :: The name of the workspace to plot
 */
void JumpFit::handleSampleInputReady(const QString &filename) {
  // Disable things that run the preview algorithm
  disconnect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
             SLOT(runPreviewAlgorithm()));
  disconnect(m_uiForm.cbWidth, SIGNAL(currentIndexChanged(const QString &)),
             this, SLOT(runPreviewAlgorithm()));

  // Scale to convert to HWHM
  IAlgorithm_sptr scaleAlg = AlgorithmManager::Instance().create("Scale");
  scaleAlg->initialize();
  scaleAlg->setProperty("InputWorkspace", filename.toStdString());
  scaleAlg->setProperty("OutputWorkspace", filename.toStdString());
  scaleAlg->setProperty("Factor", 0.5);
  scaleAlg->execute();

  auto ws = Mantid::API::AnalysisDataService::Instance().retrieve(
      filename.toStdString());
  auto mws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ws);

  findAllWidths(mws);

  auto qRangeSelector = m_uiForm.ppPlot->getRangeSelector("JumpFitQ");

  if (m_spectraList.size() > 0) {
    m_uiForm.cbWidth->setEnabled(true);

    std::string currentWidth = m_uiForm.cbWidth->currentText().toStdString();

    m_uiForm.ppPlot->clear();
    m_uiForm.ppPlot->addSpectrum("Sample", filename,
                                 m_spectraList[currentWidth]);

    QPair<double, double> res;
    QPair<double, double> range = m_uiForm.ppPlot->getCurveRange("Sample");

    // Use the values from the instrument parameter file if we can
    if (getInstrumentResolution(filename, res))
      setRangeSelector(qRangeSelector, m_properties["QMin"],
                       m_properties["QMax"], res);
    else
      setRangeSelector(qRangeSelector, m_properties["QMin"],
                       m_properties["QMax"], range);

    setPlotPropertyRange(qRangeSelector, m_properties["QMin"],
                         m_properties["QMax"], range);
  } else {
    m_uiForm.cbWidth->setEnabled(false);
    emit showMessageBox("Workspace doesn't appear to contain any width data");
  }

  // Update preview plot
  runPreviewAlgorithm();

  // Re-enable things that run the preview algorithm
  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(runPreviewAlgorithm()));
  connect(m_uiForm.cbWidth, SIGNAL(currentIndexChanged(const QString &)), this,
          SLOT(runPreviewAlgorithm()));
}

/**
 * Find all of the spectra in the workspace that have width data
 *
 * @param ws :: The workspace to search
 */
void JumpFit::findAllWidths(Mantid::API::MatrixWorkspace_const_sptr ws) {
  m_uiForm.cbWidth->blockSignals(true);
  m_uiForm.cbWidth->clear();
  m_spectraList.clear();

  for (size_t i = 0; i < ws->getNumberHistograms(); ++i) {
    auto axis = dynamic_cast<Mantid::API::TextAxis *>(ws->getAxis(1));
    if (!axis)
      return;

    std::string title = axis->label(i);

    // check if the axis labels indicate this spectrum is width data
    size_t qLinesWidthIndex = title.find(".Width");
    size_t convFitWidthIndex = title.find(".FWHM");

    bool qLinesWidth = qLinesWidthIndex != std::string::npos;
    bool convFitWidth = convFitWidthIndex != std::string::npos;

    // if we get a match, add this spectrum to the combobox
    if (convFitWidth || qLinesWidth) {
      std::string cbItemName = "";
      size_t substrIndex = 0;

      if (qLinesWidth) {
        substrIndex = qLinesWidthIndex;
      } else if (convFitWidth) {
        substrIndex = convFitWidthIndex;
      }

      cbItemName = title.substr(0, substrIndex);
      m_spectraList[cbItemName] = static_cast<int>(i);
      m_uiForm.cbWidth->addItem(QString(cbItemName.c_str()));

      // display widths f1.f1, f2.f1 and f2.f2
      if (m_uiForm.cbWidth->count() == 3) {
        return;
      }
    }
  }
  m_uiForm.cbWidth->blockSignals(false);
}

/**
 * Plots the loaded file to the miniplot when the selected spectrum changes
 *
 * @param text :: The name spectrum index to plot
 */
void JumpFit::handleWidthChange(const QString &text) {
  QString sampleName = m_uiForm.dsSample->getCurrentDataName();
  QString samplePath = m_uiForm.dsSample->getFullFilePath();

  if (!sampleName.isEmpty() && m_spectraList.size() > 0) {
    if (validate()) {
      m_uiForm.ppPlot->clear();
      m_uiForm.ppPlot->addSpectrum("Sample", sampleName,
                                   m_spectraList[text.toStdString()]);
    }
  }
}

/**
 * Updates the property manager when the range selector is moved on the mini
 *plot.
 *
 * @param min :: The new value of the lower guide
 * @param max :: The new value of the upper guide
 */
void JumpFit::qRangeChanged(double min, double max) {
  m_dblManager->setValue(m_properties["QMin"], min);
  m_dblManager->setValue(m_properties["QMax"], max);
}

/**
 * Handles when properties in the property manager are updated.
 *
 * @param prop :: The property being updated
 * @param val :: The new value for the property
 */
void JumpFit::updateProperties(QtProperty *prop, double val) {
  auto qRangeSelector = m_uiForm.ppPlot->getRangeSelector("JumpFitQ");

  if (prop == m_properties["QMin"]) {
    updateLowerGuide(qRangeSelector, m_properties["QMin"], m_properties["QMax"],
                     val);
  } else if (prop == m_properties["QMax"]) {
    updateUpperGuide(qRangeSelector, m_properties["QMin"], m_properties["QMax"],
                     val);
  }
}

/**
 * Gets a list of parameters for a given fit function.
 *
 * @return List fo parameters
 */
QStringList JumpFit::getFunctionParameters(const QString &functionName) {
  QStringList parameters;

  IFunction_sptr func =
      FunctionFactory::Instance().createFunction(functionName.toStdString());

  for (size_t i = 0; i < func->nParams(); i++)
    parameters << QString::fromStdString(func->parameterName(i));

  return parameters;
}

/**
 * Handles a new fit function being selected.
 *
 * @param functionName Name of new fit function
 */
void JumpFit::fitFunctionSelected(const QString &functionName) {
  // Remove current parameter elements
  for (auto it = m_properties.begin(); it != m_properties.end();) {
    if (it.key().startsWith("parameter_")) {
      delete it.value();
      it = m_properties.erase(it);
    } else {
      ++it;
    }
  }

  // Don't run the algorithm when updating parameter values
  disconnect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
             SLOT(runPreviewAlgorithm()));

  // Add new parameter elements
  QStringList parameters = getFunctionParameters(functionName);
  for (auto it = parameters.begin(); it != parameters.end(); ++it) {
    QString name = "parameter_" + *it;
    m_properties[name] = m_dblManager->addProperty(*it);
    m_dblManager->setValue(m_properties[name], 1.0);
    m_properties["FitFunction"]->addSubProperty(m_properties[name]);
  }

  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(runPreviewAlgorithm()));

  runPreviewAlgorithm();
}

} // namespace CustomInterfaces
} // namespace MantidQt
