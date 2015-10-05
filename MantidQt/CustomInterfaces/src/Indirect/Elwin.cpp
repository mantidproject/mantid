#include "MantidQtCustomInterfaces/Indirect/Elwin.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"

#include "MantidQtMantidWidgets/RangeSelector.h"

#include <QFileInfo>

#include <qwt_plot.h>

using namespace Mantid::API;
using namespace MantidQt::API;

namespace
{
  Mantid::Kernel::Logger g_log("Elwin");
}

namespace MantidQt
{
namespace CustomInterfaces
{
namespace IDA
{
  Elwin::Elwin(QWidget * parent) :
    IndirectDataAnalysisTab(parent),
    m_elwTree(NULL)
  {
    m_uiForm.setupUi(parent);
  }

  void Elwin::setup()
  {
    // Create QtTreePropertyBrowser object
    m_elwTree = new QtTreePropertyBrowser();
    m_uiForm.properties->addWidget(m_elwTree);

    // Editor Factories
    m_elwTree->setFactoryForManager(m_dblManager, m_dblEdFac);
    m_elwTree->setFactoryForManager(m_blnManager, m_blnEdFac);

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
    connect(integrationRangeSelector, SIGNAL(minValueChanged(double)), this, SLOT(minChanged(double)));
    connect(integrationRangeSelector, SIGNAL(maxValueChanged(double)), this, SLOT(maxChanged(double)));
    // create the second range
    auto backgroundRangeSelector = m_uiForm.ppPlot->addRangeSelector("ElwinBackgroundRange");
    backgroundRangeSelector->setColour(Qt::darkGreen); // dark green for background
    connect(integrationRangeSelector, SIGNAL(rangeChanged(double, double)),
            backgroundRangeSelector, SLOT(setRange(double, double)));
    connect(backgroundRangeSelector, SIGNAL(minValueChanged(double)), this, SLOT(minChanged(double)));
    connect(backgroundRangeSelector, SIGNAL(maxValueChanged(double)), this, SLOT(maxChanged(double)));
    backgroundRangeSelector->setRange(integrationRangeSelector->getRange());

    connect(m_dblManager, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(updateRS(QtProperty*, double)));
    connect(m_blnManager, SIGNAL(valueChanged(QtProperty*, bool)), this, SLOT(twoRanges(QtProperty*, bool)));
    twoRanges(m_properties["BackgroundSubtraction"], false);

    connect(m_uiForm.dsInputFiles, SIGNAL(filesFound()), this, SLOT(newInputFiles()));
    connect(m_uiForm.cbPreviewFile, SIGNAL(currentIndexChanged(int)), this, SLOT(newPreviewFileSelected(int)));
    connect(m_uiForm.spPreviewSpec, SIGNAL(valueChanged(int)), this, SLOT(plotInput()));

    // Set any default values
    m_dblManager->setValue(m_properties["IntegrationStart"], -0.02);
    m_dblManager->setValue(m_properties["IntegrationEnd"], 0.02);

    m_dblManager->setValue(m_properties["BackgroundStart"], -0.24);
    m_dblManager->setValue(m_properties["BackgroundEnd"], -0.22);
  }

  void Elwin::run()
  {
    QStringList inputFilenames = m_uiForm.dsInputFiles->getFilenames();
    inputFilenames.sort();

    // Get workspace names
    std::string inputGroupWsName = "IDA_Elwin_Input";

    QFileInfo firstFileInfo(inputFilenames[0]);
    QString filename = firstFileInfo.baseName();
    QString workspaceBaseName = filename.left(filename.lastIndexOf("_")) + "_elwin_";

    QString qWorkspace = workspaceBaseName + "eq";
    QString qSquaredWorkspace = workspaceBaseName + "eq2";
    QString elfWorkspace = workspaceBaseName + "elf";
    QString eltWorkspace = workspaceBaseName + "elt";

    // Load input files
    std::vector<std::string> inputWorkspaceNames;

    for(auto it = inputFilenames.begin(); it != inputFilenames.end(); ++it)
    {
      QFileInfo inputFileInfo(*it);
      std::string workspaceName = inputFileInfo.baseName().toStdString();

      IAlgorithm_sptr loadAlg = AlgorithmManager::Instance().create("LoadNexus");
      loadAlg->initialize();
      loadAlg->setProperty("Filename", (*it).toStdString());
      loadAlg->setProperty("OutputWorkspace", workspaceName);

      m_batchAlgoRunner->addAlgorithm(loadAlg);
      inputWorkspaceNames.push_back(workspaceName);
    }

    // Group input workspaces
    IAlgorithm_sptr groupWsAlg = AlgorithmManager::Instance().create("GroupWorkspaces");
    groupWsAlg->initialize();
    groupWsAlg->setProperty("InputWorkspaces", inputWorkspaceNames);
    groupWsAlg->setProperty("OutputWorkspace", inputGroupWsName);

    m_batchAlgoRunner->addAlgorithm(groupWsAlg);

    // Configure ElasticWindowMultiple algorithm
    IAlgorithm_sptr elwinMultAlg = AlgorithmManager::Instance().create("ElasticWindowMultiple");
    elwinMultAlg->initialize();

    elwinMultAlg->setProperty("Plot", m_uiForm.ckPlot->isChecked());

    elwinMultAlg->setProperty("OutputInQ", qWorkspace.toStdString());
    elwinMultAlg->setProperty("OutputInQSquared", qSquaredWorkspace.toStdString());
    elwinMultAlg->setProperty("OutputELF", elfWorkspace.toStdString());

    elwinMultAlg->setProperty("SampleEnvironmentLogName", m_uiForm.leLogName->text().toStdString());
    elwinMultAlg->setProperty("SampleEnvironmentLogValue", m_uiForm.leLogValue->currentText().toStdString());

    elwinMultAlg->setProperty("IntegrationRangeStart", m_dblManager->value(m_properties["IntegrationStart"]));
    elwinMultAlg->setProperty("IntegrationRangeEnd", m_dblManager->value(m_properties["IntegrationEnd"]));

    if(m_blnManager->value(m_properties["BackgroundSubtraction"]))
    {
      elwinMultAlg->setProperty("BackgroundRangeStart", m_dblManager->value(m_properties["BackgroundStart"]));
      elwinMultAlg->setProperty("BackgroundRangeEnd", m_dblManager->value(m_properties["BackgroundEnd"]));
    }

    if(m_blnManager->value(m_properties["Normalise"]))
    {
      elwinMultAlg->setProperty("OutputELT", eltWorkspace.toStdString());
    }

    BatchAlgorithmRunner::AlgorithmRuntimeProps elwinInputProps;
    elwinInputProps["InputWorkspaces"] = inputGroupWsName;

    m_batchAlgoRunner->addAlgorithm(elwinMultAlg, elwinInputProps);

    // Configure Save algorithms
    if(m_uiForm.ckSave->isChecked())
    {
      addSaveAlgorithm(qWorkspace);
      addSaveAlgorithm(qSquaredWorkspace);
      addSaveAlgorithm(elfWorkspace);

      if(m_blnManager->value(m_properties["Normalise"]))
        addSaveAlgorithm(eltWorkspace);
    }

    m_batchAlgoRunner->executeBatchAsync();

    // Set the result workspace for Python script export
    m_pythonExportWsName = qSquaredWorkspace.toStdString();
  }

  /**
   * Configures and adds a SaveNexus algorithm to the batch runner.
   *
   * @param workspaceName Name of the workspace to save
   * @param filename Name of the file to save it as
   */
  void Elwin::addSaveAlgorithm(QString workspaceName, QString filename)
  {
    // Set a default filename if none provided
    if(filename.isEmpty())
      filename = workspaceName + ".nxs";

    // Configure the algorithm
    IAlgorithm_sptr loadAlg = AlgorithmManager::Instance().create("SaveNexus");
    loadAlg->initialize();
    loadAlg->setProperty("Filename", filename.toStdString());

    BatchAlgorithmRunner::AlgorithmRuntimeProps saveAlgProps;
    saveAlgProps["InputWorkspace"] = workspaceName.toStdString();

    // Add it to the batch runner
    m_batchAlgoRunner->addAlgorithm(loadAlg, saveAlgProps);
  }

  bool Elwin::validate()
  {
    UserInputValidator uiv;

    uiv.checkMWRunFilesIsValid("Input", m_uiForm.dsInputFiles);

    auto rangeOne = std::make_pair(m_dblManager->value(m_properties["IntegrationStart"]), m_dblManager->value(m_properties["IntegrationEnd"]));
    uiv.checkValidRange("Range One", rangeOne);

    bool useTwoRanges = m_blnManager->value(m_properties["BackgroundSubtraction"]);
    if( useTwoRanges )
    {
      auto rangeTwo = std::make_pair(m_dblManager->value(m_properties["BackgroundStart"]), m_dblManager->value(m_properties["BackgroundEnd"]));
      uiv.checkValidRange("Range Two", rangeTwo);
      uiv.checkRangesDontOverlap(rangeOne, rangeTwo);
    }

    QString error = uiv.generateErrorMessage();
    showMessageBox(error);

    return error.isEmpty();
  }

  void Elwin::loadSettings(const QSettings & settings)
  {
    m_uiForm.dsInputFiles->readSettings(settings.group());
  }

  void Elwin::setDefaultResolution(Mantid::API::MatrixWorkspace_const_sptr ws)
  {
    auto inst = ws->getInstrument();
    auto analyser = inst->getStringParameter("analyser");

    if(analyser.size() > 0)
    {
      auto comp = inst->getComponentByName(analyser[0]);
      auto params = comp->getNumberParameter("resolution", true);

      //set the default instrument resolution
      if(params.size() > 0)
      {
        double res = params[0];
        m_dblManager->setValue(m_properties["IntegrationStart"], -res);
        m_dblManager->setValue(m_properties["IntegrationEnd"], res);

        m_dblManager->setValue(m_properties["BackgroundStart"], -10*res);
        m_dblManager->setValue(m_properties["BackgroundEnd"], -9*res);
      }
    }
  }

  void Elwin::setDefaultSampleLog(Mantid::API::MatrixWorkspace_const_sptr ws)
  {
    auto inst = ws->getInstrument();
    // Set sample environment log name
    auto log = inst->getStringParameter("Workflow.SE-log");
    QString logName("sample");
    if(log.size() > 0)
    {
      logName = QString::fromStdString(log[0]);
    }
    m_uiForm.leLogName->setText(logName);
    // Set sample environment log value
    auto logval = inst->getStringParameter("Workflow.SE-log-value");
    if(logval.size() > 0)
    {
      auto logValue = QString::fromStdString(logval[0]);
      int  index = m_uiForm.leLogValue->findText(logValue);
      if (index >= 0)
      {
        m_uiForm.leLogValue->setCurrentIndex(index);
      }
    }
  }

  /**
   * Handles a new set of input files being entered.
   *
   * Updates preview seletcion combo box.
   */
  void Elwin::newInputFiles()
  {
    // Clear the existing list of files
    m_uiForm.cbPreviewFile->clear();

    // Populate the combo box with the filenames
    QStringList filenames = m_uiForm.dsInputFiles->getFilenames();
    for(auto it = filenames.begin(); it != filenames.end(); ++it)
    {
      QString rawFilename = *it;
      QFileInfo inputFileInfo(rawFilename);
      QString sampleName = inputFileInfo.baseName();

      // Add the item using the base filename as the display string and the raw filename as the data value
      m_uiForm.cbPreviewFile->addItem(sampleName, rawFilename);
    }

    // Default to the first file
    m_uiForm.cbPreviewFile->setCurrentIndex(0);
  }

  /**
   * Handles a new input file being selected for preview.
   *
   * Loads the file and resets the spectra selection spinner.
   *
   * @param index Index of the new selected file
   */
  void Elwin::newPreviewFileSelected(int index)
  {
    QString wsName = m_uiForm.cbPreviewFile->itemText(index);
    QString filename = m_uiForm.cbPreviewFile->itemData(index).toString();

    // Ignore empty filenames (can happen when new files are loaded and the widget is being populated)
    if(filename.isEmpty())
      return;

    if(!loadFile(filename, wsName))
    {
      g_log.error("Failed to load input workspace.");
      return;
    }

    auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName.toStdString());
    int numHist = static_cast<int>(ws->getNumberHistograms()) - 1;

    m_uiForm.spPreviewSpec->setMaximum(numHist);
    m_uiForm.spPreviewSpec->setValue(0);

    plotInput();
  }

  /**
   * Replots the preview plot.
   */
  void Elwin::plotInput()
  {
    QString wsName = m_uiForm.cbPreviewFile->currentText();

    if(!AnalysisDataService::Instance().doesExist(wsName.toStdString()))
    {
      g_log.error("Workspace not found in ADS. Try reloading input files.");
      return;
    }

    auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName.toStdString());

    if(!ws)
    {
      g_log.error("Failed to get input workspace from ADS.");
      return;
    }

    int specNo = m_uiForm.spPreviewSpec->value();

    setDefaultResolution(ws);
    setDefaultSampleLog(ws);

    m_uiForm.ppPlot->clear();
    m_uiForm.ppPlot->addSpectrum("Sample", ws, specNo);

    try
    {
      QPair<double, double> range = m_uiForm.ppPlot->getCurveRange("Sample");
      m_uiForm.ppPlot->getRangeSelector("ElwinIntegrationRange")->setRange(range.first, range.second);
    }
    catch(std::invalid_argument & exc)
    {
      showMessageBox(exc.what());
    }
  }

  void Elwin::twoRanges(QtProperty* prop, bool val)
  {
    if(prop == m_properties["BackgroundSubtraction"])
      m_uiForm.ppPlot->getRangeSelector("ElwinBackgroundRange")->setVisible(val);
  }

  void Elwin::minChanged(double val)
  {
    auto integrationRangeSelector = m_uiForm.ppPlot->getRangeSelector("ElwinIntegrationRange");
    auto backgroundRangeSelector = m_uiForm.ppPlot->getRangeSelector("ElwinBackgroundRange");

    MantidWidgets::RangeSelector* from = qobject_cast<MantidWidgets::RangeSelector*>(sender());

    if(from == integrationRangeSelector)
    {
      m_dblManager->setValue(m_properties["IntegrationStart"], val);
    }
    else if(from == backgroundRangeSelector)
    {
      m_dblManager->setValue(m_properties["BackgroundStart"], val);
    }
  }

  void Elwin::maxChanged(double val)
  {
    auto integrationRangeSelector = m_uiForm.ppPlot->getRangeSelector("ElwinIntegrationRange");
    auto backgroundRangeSelector = m_uiForm.ppPlot->getRangeSelector("ElwinBackgroundRange");

    MantidWidgets::RangeSelector* from = qobject_cast<MantidWidgets::RangeSelector*>(sender());

    if(from == integrationRangeSelector)
    {
      m_dblManager->setValue(m_properties["IntegrationEnd"], val);
    }
    else if(from == backgroundRangeSelector)
    {
      m_dblManager->setValue(m_properties["BackgroundEnd"], val);
    }
  }

  void Elwin::updateRS(QtProperty* prop, double val)
  {
    auto integrationRangeSelector = m_uiForm.ppPlot->getRangeSelector("ElwinIntegrationRange");
    auto backgroundRangeSelector = m_uiForm.ppPlot->getRangeSelector("ElwinBackgroundRange");

    if ( prop == m_properties["IntegrationStart"] )     integrationRangeSelector->setMinimum(val);
    else if ( prop == m_properties["IntegrationEnd"] )  integrationRangeSelector->setMaximum(val);
    else if ( prop == m_properties["BackgroundStart"] ) backgroundRangeSelector->setMinimum(val);
    else if ( prop == m_properties["BackgroundEnd"] )   backgroundRangeSelector->setMaximum(val);
  }

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
