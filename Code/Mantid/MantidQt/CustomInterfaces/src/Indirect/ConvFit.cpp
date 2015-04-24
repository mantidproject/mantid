#include "MantidQtCustomInterfaces/Indirect/ConvFit.h"

#include "MantidQtCustomInterfaces/UserInputValidator.h"
#include "MantidQtMantidWidgets/RangeSelector.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionDomain1D.h"

#include <QDoubleValidator>
#include <QFileInfo>
#include <QMenu>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>

using namespace Mantid::API;

namespace
{
  Mantid::Kernel::Logger g_log("ConvFit");
}

namespace MantidQt
{
namespace CustomInterfaces
{
namespace IDA
{
  ConvFit::ConvFit(QWidget * parent) :
    IDATab(parent),
    m_stringManager(NULL), m_cfTree(NULL),
    m_fixedProps(),
    m_cfInputWS(), m_cfInputWSName(),
    m_confitResFileType()
  {
    m_uiForm.setupUi(parent);
  }

  void ConvFit::setup()
  {
    // Create Property Managers
    m_stringManager = new QtStringPropertyManager();

    // Create TreeProperty Widget
    m_cfTree = new QtTreePropertyBrowser();
    m_uiForm.properties->addWidget(m_cfTree);

    // add factories to managers
    m_cfTree->setFactoryForManager(m_blnManager, m_blnEdFac);
    m_cfTree->setFactoryForManager(m_dblManager, m_dblEdFac);

    // Create Range Selectors
    auto fitRangeSelector = m_uiForm.ppPlot->addRangeSelector("ConvFitRange");
    auto backRangeSelector = m_uiForm.ppPlot->addRangeSelector("ConvFitBackRange",
                                                               MantidWidgets::RangeSelector::YSINGLE);
    auto hwhmRangeSelector = m_uiForm.ppPlot->addRangeSelector("ConvFitHWHM");
    backRangeSelector->setColour(Qt::darkGreen);
    backRangeSelector->setRange(0.0, 1.0);
    hwhmRangeSelector->setColour(Qt::red);

    // Populate Property Widget

    // Option to convolve members
    m_properties["Convolve"] = m_blnManager->addProperty("Convolve");
    m_cfTree->addProperty(m_properties["Convolve"]);
    m_blnManager->setValue(m_properties["Convolve"], true);

    m_properties["FitRange"] = m_grpManager->addProperty("Fitting Range");
    m_properties["StartX"] = m_dblManager->addProperty("StartX");
    m_dblManager->setDecimals(m_properties["StartX"], NUM_DECIMALS);
    m_properties["EndX"] = m_dblManager->addProperty("EndX");
    m_dblManager->setDecimals(m_properties["EndX"], NUM_DECIMALS);
    m_properties["FitRange"]->addSubProperty(m_properties["StartX"]);
    m_properties["FitRange"]->addSubProperty(m_properties["EndX"]);
    m_cfTree->addProperty(m_properties["FitRange"]);

    m_properties["LinearBackground"] = m_grpManager->addProperty("Background");
    m_properties["BGA0"] = m_dblManager->addProperty("A0");
    m_dblManager->setDecimals(m_properties["BGA0"], NUM_DECIMALS);
    m_properties["BGA1"] = m_dblManager->addProperty("A1");
    m_dblManager->setDecimals(m_properties["BGA1"], NUM_DECIMALS);
    m_properties["LinearBackground"]->addSubProperty(m_properties["BGA0"]);
    m_properties["LinearBackground"]->addSubProperty(m_properties["BGA1"]);
    m_cfTree->addProperty(m_properties["LinearBackground"]);

    // Delta Function
    m_properties["DeltaFunction"] = m_grpManager->addProperty("Delta Function");
    m_properties["UseDeltaFunc"] = m_blnManager->addProperty("Use");
    m_properties["DeltaHeight"] = m_dblManager->addProperty("Height");
    m_dblManager->setDecimals(m_properties["DeltaHeight"], NUM_DECIMALS);
    m_properties["DeltaFunction"]->addSubProperty(m_properties["UseDeltaFunc"]);
    m_cfTree->addProperty(m_properties["DeltaFunction"]);

    m_properties["Lorentzian1"] = createLorentzian("Lorentzian 1");
    m_properties["Lorentzian2"] = createLorentzian("Lorentzian 2");
    m_properties["DiffSphere"] = createDiffSphere("Diffusion Sphere");
    m_properties["DiffRotDiscreteCircle"] = createDiffRotDiscreteCircle("Diffusion Circle");

    m_uiForm.leTempCorrection->setValidator(new QDoubleValidator(m_parentWidget));

    // Connections
    connect(fitRangeSelector, SIGNAL(minValueChanged(double)), this, SLOT(minChanged(double)));
    connect(fitRangeSelector, SIGNAL(maxValueChanged(double)), this, SLOT(maxChanged(double)));
    connect(backRangeSelector, SIGNAL(minValueChanged(double)), this, SLOT(backgLevel(double)));
    connect(hwhmRangeSelector, SIGNAL(minValueChanged(double)), this, SLOT(hwhmChanged(double)));
    connect(hwhmRangeSelector, SIGNAL(maxValueChanged(double)), this, SLOT(hwhmChanged(double)));
    connect(m_dblManager, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(updateRS(QtProperty*, double)));
    connect(m_blnManager, SIGNAL(valueChanged(QtProperty*, bool)), this, SLOT(checkBoxUpdate(QtProperty*, bool)));
    connect(m_uiForm.ckTempCorrection, SIGNAL(toggled(bool)), m_uiForm.leTempCorrection, SLOT(setEnabled(bool)));

    // Update guess curve when certain things happen
    connect(m_dblManager, SIGNAL(propertyChanged(QtProperty*)), this, SLOT(plotGuess()));
    connect(m_uiForm.cbFitType, SIGNAL(currentIndexChanged(int)), this, SLOT(plotGuess()));
    connect(m_uiForm.ckPlotGuess, SIGNAL(stateChanged(int)), this, SLOT(plotGuess()));

    // Have FWHM Range linked to Fit Start/End Range
    connect(fitRangeSelector, SIGNAL(rangeChanged(double, double)),
            hwhmRangeSelector, SLOT(setRange(double, double)));
    hwhmRangeSelector->setRange(-1.0, 1.0);
    hwhmUpdateRS(0.02);

    typeSelection(m_uiForm.cbFitType->currentIndex());
    bgTypeSelection(m_uiForm.cbBackground->currentIndex());

    // Replot input automatically when file / spec no changes
    connect(m_uiForm.spPlotSpectrum, SIGNAL(valueChanged(int)), this, SLOT(updatePlot()));
    connect(m_uiForm.dsSampleInput, SIGNAL(dataReady(const QString&)), this, SLOT(newDataLoaded(const QString&)));

    connect(m_uiForm.dsSampleInput, SIGNAL(dataReady(const QString&)), this, SLOT(extendResolutionWorkspace()));
    connect(m_uiForm.dsResInput, SIGNAL(dataReady(const QString&)), this, SLOT(extendResolutionWorkspace()));

    connect(m_uiForm.spSpectraMin, SIGNAL(valueChanged(int)), this, SLOT(specMinChanged(int)));
    connect(m_uiForm.spSpectraMax, SIGNAL(valueChanged(int)), this, SLOT(specMaxChanged(int)));

    connect(m_uiForm.cbFitType, SIGNAL(currentIndexChanged(int)), this, SLOT(typeSelection(int)));
    connect(m_uiForm.cbBackground, SIGNAL(currentIndexChanged(int)), this, SLOT(bgTypeSelection(int)));
    connect(m_uiForm.pbSingleFit, SIGNAL(clicked()), this, SLOT(singleFit()));

    // Context menu
    m_cfTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_cfTree, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(fitContextMenu(const QPoint &)));

    // Tie
    connect(m_uiForm.cbFitType,SIGNAL(currentIndexChanged(QString)),SLOT(showTieCheckbox(QString)));
    showTieCheckbox( m_uiForm.cbFitType->currentText() );

    updatePlotOptions();
  }

  void ConvFit::run()
  {
    if ( m_cfInputWS == NULL )
    {
      g_log.error("No workspace loaded");
      return;
    }

    QString fitType = fitTypeString();
    QString bgType = backgroundString();

    if(fitType == "")
    {
      g_log.error("No fit type defined");
    }

    bool useTies = m_uiForm.ckTieCentres->isChecked();
    QString ties = (useTies ? "True" : "False");

    CompositeFunction_sptr func = createFunction(useTies);
    std::string function = std::string(func->asString());
    QString stX = m_properties["StartX"]->valueText();
    QString enX = m_properties["EndX"]->valueText();
    QString specMin = m_uiForm.spSpectraMin->text();
    QString specMax = m_uiForm.spSpectraMax->text();

    QString pyInput =
      "from IndirectDataAnalysis import confitSeq\n"
      "input = '" + m_cfInputWSName + "'\n"
      "func = r'" + QString::fromStdString(function) + "'\n"
      "startx = " + stX + "\n"
      "endx = " + enX + "\n"
      "plot = '" + m_uiForm.cbPlotType->currentText() + "'\n"
      "ties = " + ties + "\n"
      "specMin = " + specMin + "\n"
      "specMax = " + specMax + "\n"
      "save = " + (m_uiForm.ckSave->isChecked() ? "True\n" : "False\n");

    if ( m_blnManager->value(m_properties["Convolve"]) ) pyInput += "convolve = True\n";
    else pyInput += "convolve = False\n";

    QString temperature = m_uiForm.leTempCorrection->text();
    bool useTempCorrection = (!temperature.isEmpty() && m_uiForm.ckTempCorrection->isChecked());
    if ( useTempCorrection )
    {
      pyInput += "temp=" + temperature + "\n";
    }
    else
    {
      pyInput += "temp=None\n";
    }

    pyInput +=
      "bg = '" + bgType + "'\n"
      "ftype = '" + fitType + "'\n"
      "confitSeq(input, func, startx, endx, ftype, bg, temp, specMin, specMax, convolve, Plot=plot, Save=save)\n";

    QString pyOutput = runPythonCode(pyInput);

    // Set the result workspace for Python script export
    QString inputWsName = QString::fromStdString(m_cfInputWS->getName());
    QString resultWsName = inputWsName.left(inputWsName.lastIndexOf("_")) + "_conv_" + fitType + bgType + specMin + "_to_" + specMax + "_Workspaces";
    m_pythonExportWsName = resultWsName.toStdString();

    updatePlot();
  }

  /**
   * Validates the user's inputs in the ConvFit tab.
   */
  bool ConvFit::validate()
  {
    UserInputValidator uiv;

    uiv.checkDataSelectorIsValid("Sample", m_uiForm.dsSampleInput);
    uiv.checkDataSelectorIsValid("Resolution", m_uiForm.dsResInput);

    auto range = std::make_pair(m_dblManager->value(m_properties["StartX"]), m_dblManager->value(m_properties["EndX"]));
    uiv.checkValidRange("Fitting Range", range);

    // Enforce the rule that at least one fit is needed; either a delta function, one or two lorentzian functions,
    // or both.  (The resolution function must be convolved with a model.)
    if ( m_uiForm.cbFitType->currentIndex() == 0 && ! m_blnManager->value(m_properties["UseDeltaFunc"]) )
      uiv.addErrorMessage("No fit function has been selected.");

    QString error = uiv.generateErrorMessage();
    showMessageBox(error);

    return error.isEmpty();
  }

  void ConvFit::loadSettings(const QSettings & settings)
  {
    m_uiForm.dsSampleInput->readSettings(settings.group());
    m_uiForm.dsResInput->readSettings(settings.group());
  }

  /**
   * Called when new data has been loaded by the data selector.
   *
   * Configures ranges for spin boxes before raw plot is done.
   *
   * @param wsName Name of new workspace loaded
   */
  void ConvFit::newDataLoaded(const QString wsName)
  {
    m_cfInputWSName = wsName;
    m_cfInputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_cfInputWSName.toStdString());

    int maxSpecIndex = static_cast<int>(m_cfInputWS->getNumberHistograms()) - 1;

    m_uiForm.spPlotSpectrum->setMaximum(maxSpecIndex);
    m_uiForm.spPlotSpectrum->setMinimum(0);
    m_uiForm.spPlotSpectrum->setValue(0);

    m_uiForm.spSpectraMin->setMaximum(maxSpecIndex);
    m_uiForm.spSpectraMin->setMinimum(0);

    m_uiForm.spSpectraMax->setMaximum(maxSpecIndex);
    m_uiForm.spSpectraMax->setMinimum(0);
    m_uiForm.spSpectraMax->setValue(maxSpecIndex);

    updatePlot();
  }

  /**
   * Create a resolution workspace with the same number of histograms as in the sample.
   *
   * Needed to allow DiffSphere and DiffRotDiscreteCircle fit functions to work as they need
   * to have the WorkspaceIndex attribute set.
   */
  void ConvFit::extendResolutionWorkspace()
  {
    if(m_cfInputWS && m_uiForm.dsResInput->isValid())
    {
      const QString resWsName = m_uiForm.dsResInput->getCurrentDataName();

      API::BatchAlgorithmRunner::AlgorithmRuntimeProps appendProps;
      appendProps["InputWorkspace1"] = "__ConvFit_Resolution";

      size_t numHist = m_cfInputWS->getNumberHistograms();
      for(size_t i = 0; i < numHist; i++)
      {
        IAlgorithm_sptr appendAlg = AlgorithmManager::Instance().create("AppendSpectra");
        appendAlg->initialize();
        appendAlg->setProperty("InputWorkspace2", resWsName.toStdString());
        appendAlg->setProperty("OutputWorkspace", "__ConvFit_Resolution");

        if(i == 0)
        {
          appendAlg->setProperty("InputWorkspace1", resWsName.toStdString());
          m_batchAlgoRunner->addAlgorithm(appendAlg);
        }
        else
        {
          m_batchAlgoRunner->addAlgorithm(appendAlg, appendProps);
        }
      }

      m_batchAlgoRunner->executeBatchAsync();
    }
  }

  namespace
  {
    ////////////////////////////
    // Anon Helper functions. //
    ////////////////////////////

    /**
     * Takes an index and a name, and constructs a single level parameter name
     * for use with function ties, etc.
     *
     * @param index :: the index of the function in the first level.
     * @param name  :: the name of the parameter inside the function.
     *
     * @returns the constructed function parameter name.
     */
    std::string createParName(size_t index, const std::string & name = "")
    {
      std::stringstream prefix;
      prefix << "f" << index << "." << name;
      return prefix.str();
    }

    /**
     * Takes an index, a sub index and a name, and constructs a double level
     * (nested) parameter name for use with function ties, etc.
     *
     * @param index    :: the index of the function in the first level.
     * @param subIndex :: the index of the function in the second level.
     * @param name     :: the name of the parameter inside the function.
     *
     * @returns the constructed function parameter name.
     */
    std::string createParName(size_t index, size_t subIndex, const std::string & name = "")
    {
      std::stringstream prefix;
      prefix << "f" << index << ".f" << subIndex << "." << name;
      return prefix.str();
    }
  }

  /**
   * Creates a function to carry out the fitting in the "ConvFit" tab.  The function consists
   * of various sub functions, with the following structure:
   *
   * Composite
   *  |
   *  +-- LinearBackground
   *  +-- Convolution
   *      |
   *      +-- Resolution
   *      +-- Model (AT LEAST one delta function or one/two lorentzians.)
   *          |
   *          +-- DeltaFunction (yes/no)
   *					+-- ProductFunction
   *							|
   *							+-- Lorentzian 1 (yes/no)
   *							+-- Temperature Correction (yes/no)
   *					+-- ProductFunction
   *							|
   *							+-- Lorentzian 2 (yes/no)
   *							+-- Temperature Correction (yes/no)
   *					+-- ProductFunction
   *							|
   *							+-- InelasticDiffSphere (yes/no)
   *							+-- Temperature Correction (yes/no)
   *					+-- ProductFunction
   *							|
   *							+-- InelasticDiffRotDiscreteCircle (yes/no)
   *							+-- Temperature Correction (yes/no)
   *
   * @param tieCentres :: whether to tie centres of the two lorentzians.
   *
   * @returns the composite fitting function.
   */
  CompositeFunction_sptr ConvFit::createFunction(bool tieCentres)
  {
    auto conv = boost::dynamic_pointer_cast<CompositeFunction>(FunctionFactory::Instance().createFunction("Convolution"));
    CompositeFunction_sptr comp( new CompositeFunction );

    IFunction_sptr func;
    size_t index = 0;

    // -------------------------------------
    // --- Composite / Linear Background ---
    // -------------------------------------
    func = FunctionFactory::Instance().createFunction("LinearBackground");
    comp->addFunction(func);

    const int bgType = m_uiForm.cbBackground->currentIndex(); // 0 = Fixed Flat, 1 = Fit Flat, 2 = Fit all

    if ( bgType == 0 || ! m_properties["BGA0"]->subProperties().isEmpty() )
    {
      comp->tie("f0.A0", m_properties["BGA0"]->valueText().toStdString() );
    }
    else
    {
      func->setParameter("A0", m_properties["BGA0"]->valueText().toDouble());
    }

    if ( bgType != 2 )
    {
      comp->tie("f0.A1", "0.0");
    }
    else
    {
      if ( ! m_properties["BGA1"]->subProperties().isEmpty() )
      {
        comp->tie("f0.A1", m_properties["BGA1"]->valueText().toStdString() );
      }
      else { func->setParameter("A1", m_properties["BGA1"]->valueText().toDouble()); }
    }

    // --------------------------------------------
    // --- Composite / Convolution / Resolution ---
    // --------------------------------------------
    func = FunctionFactory::Instance().createFunction("Resolution");
    conv->addFunction(func);

    //add resolution file
    IFunction::Attribute attr("__ConvFit_Resolution");
    func->setAttribute("Workspace", attr);

    // --------------------------------------------------------
    // --- Composite / Convolution / Model / Delta Function ---
    // --------------------------------------------------------
    CompositeFunction_sptr model( new CompositeFunction );

    bool useDeltaFunc = m_blnManager->value(m_properties["UseDeltaFunc"]);

    size_t subIndex = 0;

    if ( useDeltaFunc )
    {
      func = FunctionFactory::Instance().createFunction("DeltaFunction");
      index = model->addFunction(func);
      std::string parName = createParName(index);
      populateFunction(func, model, m_properties["DeltaFunction"], parName, false);
    }

    // ------------------------------------------------------------
    // --- Composite / Convolution / Model / Temperature Factor ---
    // ------------------------------------------------------------

    //create temperature correction function to multiply with the lorentzians
    IFunction_sptr tempFunc;
    QString temperature = m_uiForm.leTempCorrection->text();
    bool useTempCorrection = (!temperature.isEmpty() && m_uiForm.ckTempCorrection->isChecked());

    // -----------------------------------------------------
    // --- Composite / Convolution / Model / Lorentzians ---
    // -----------------------------------------------------
    std::string prefix1;
    std::string prefix2;

    int fitTypeIndex = m_uiForm.cbFitType->currentIndex();

    // Add 1st Lorentzian
    if(fitTypeIndex == 1 || fitTypeIndex == 2)
    {
      //if temperature not included then product is lorentzian * 1
      //create product function for temp * lorentzian
      auto product = boost::dynamic_pointer_cast<CompositeFunction>(FunctionFactory::Instance().createFunction("ProductFunction"));

      if(useTempCorrection)
      {
        createTemperatureCorrection(product);
      }

      func = FunctionFactory::Instance().createFunction("Lorentzian");
      subIndex = product->addFunction(func);
      index = model->addFunction(product);
      prefix1 = createParName(index, subIndex);

      populateFunction(func, model, m_properties["Lorentzian1"], prefix1, false);
    }

    // Add 2nd Lorentzian
    if(fitTypeIndex == 2)
    {
      //if temperature not included then product is lorentzian * 1
      //create product function for temp * lorentzian
      auto product = boost::dynamic_pointer_cast<CompositeFunction>(FunctionFactory::Instance().createFunction("ProductFunction"));

      if(useTempCorrection)
      {
        createTemperatureCorrection(product);
      }

      func = FunctionFactory::Instance().createFunction("Lorentzian");
      subIndex = product->addFunction(func);
      index = model->addFunction(product);
      prefix2 = createParName(index, subIndex);

      populateFunction(func, model, m_properties["Lorentzian2"], prefix2, false);
    }

    // -------------------------------------------------------------
    // --- Composite / Convolution / Model / InelasticDiffSphere ---
    // -------------------------------------------------------------
    if(fitTypeIndex == 3)
    {
      auto product = boost::dynamic_pointer_cast<CompositeFunction>(FunctionFactory::Instance().createFunction("ProductFunction"));

      if(useTempCorrection)
      {
        createTemperatureCorrection(product);
      }

      func = FunctionFactory::Instance().createFunction("InelasticDiffSphere");
      subIndex = product->addFunction(func);
      index = model->addFunction(product);
      prefix2 = createParName(index, subIndex);

      populateFunction(func, model, m_properties["DiffSphere"], prefix2, false);
    }

    // ------------------------------------------------------------------------
    // --- Composite / Convolution / Model / InelasticDiffRotDiscreteCircle ---
    // ------------------------------------------------------------------------
    if(fitTypeIndex == 4)
    {
      auto product = boost::dynamic_pointer_cast<CompositeFunction>(FunctionFactory::Instance().createFunction("ProductFunction"));

      if(useTempCorrection)
      {
        createTemperatureCorrection(product);
      }

      func = FunctionFactory::Instance().createFunction("InelasticDiffRotDiscreteCircle");
      subIndex = product->addFunction(func);
      index = model->addFunction(product);
      prefix2 = createParName(index, subIndex);

      populateFunction(func, model, m_properties["DiffRotDiscreteCircle"], prefix2, false);
    }

    conv->addFunction(model);
    comp->addFunction(conv);

    // Tie PeakCentres together
    if ( tieCentres )
    {
      std::string tieL = prefix1 + "PeakCentre";
      std::string tieR = prefix2 + "PeakCentre";
      model->tie(tieL, tieR);
    }

    comp->applyTies();
    return comp;
  }

  void ConvFit::createTemperatureCorrection(CompositeFunction_sptr product)
  {
    //create temperature correction function to multiply with the lorentzians
    IFunction_sptr tempFunc;
    QString temperature = m_uiForm.leTempCorrection->text();

    //create user function for the exponential correction
    // (x*temp) / 1-exp(-(x*temp))
    tempFunc = FunctionFactory::Instance().createFunction("UserFunction");
    //11.606 is the conversion factor from meV to K
    std::string formula = "((x*11.606)/Temp) / (1 - exp(-((x*11.606)/Temp)))";
    IFunction::Attribute att(formula);
    tempFunc->setAttribute("Formula", att);
    tempFunc->setParameter("Temp", temperature.toDouble());

    product->addFunction(tempFunc);
    product->tie("f0.Temp", temperature.toStdString());
    product->applyTies();
  }

  double ConvFit::getInstrumentResolution(std::string workspaceName)
  {
    using namespace Mantid::API;

    double resolution = 0.0;
    try
    {
      Mantid::Geometry::Instrument_const_sptr inst =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(workspaceName)->getInstrument();
      std::string analyser = inst->getStringParameter("analyser")[0];
      std::string idfDirectory = Mantid::Kernel::ConfigService::Instance().getString("instrumentDefinition.directory");

      // If the analyser component is not already in the data file the laod it from the parameter file
      if(inst->getComponentByName(analyser)->getNumberParameter("resolution").size() == 0)
      {
        std::string reflection = inst->getStringParameter("reflection")[0];

        IAlgorithm_sptr loadParamFile = AlgorithmManager::Instance().create("LoadParameterFile");
        loadParamFile->initialize();
        loadParamFile->setProperty("Workspace", workspaceName);
        loadParamFile->setProperty("Filename", idfDirectory + inst->getName() + "_"+analyser + "_" + reflection + "_Parameters.xml");
        loadParamFile->execute();

        if(!loadParamFile->isExecuted())
        {
          g_log.error("Could not load parameter file, ensure instrument directory is in data search paths.");
          return 0.0;
        }

        inst = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(workspaceName)->getInstrument();
      }

      resolution = inst->getComponentByName(analyser)->getNumberParameter("resolution")[0];
    }
    catch(Mantid::Kernel::Exception::NotFoundError &e)
    {
      UNUSED_ARG(e);

      g_log.error("Could not load instrument resolution from parameter file");
      resolution = 0.0;
    }

    return resolution;
  }

  QtProperty* ConvFit::createLorentzian(const QString & name)
  {
    QtProperty* lorentzGroup = m_grpManager->addProperty(name);

    m_properties[name+".Amplitude"] = m_dblManager->addProperty("Amplitude");
    // m_dblManager->setRange(m_properties[name+".Amplitude"], 0.0, 1.0); // 0 < Amplitude < 1
    m_properties[name+".PeakCentre"] = m_dblManager->addProperty("PeakCentre");
    m_properties[name+".FWHM"] = m_dblManager->addProperty("FWHM");

    m_dblManager->setDecimals(m_properties[name+".Amplitude"], NUM_DECIMALS);
    m_dblManager->setDecimals(m_properties[name+".PeakCentre"], NUM_DECIMALS);
    m_dblManager->setDecimals(m_properties[name+".FWHM"], NUM_DECIMALS);
    m_dblManager->setValue(m_properties[name+".FWHM"], 0.02);

    lorentzGroup->addSubProperty(m_properties[name+".Amplitude"]);
    lorentzGroup->addSubProperty(m_properties[name+".PeakCentre"]);
    lorentzGroup->addSubProperty(m_properties[name+".FWHM"]);

    return lorentzGroup;
  }

  QtProperty* ConvFit::createDiffSphere(const QString & name)
  {
    QtProperty* diffSphereGroup = m_grpManager->addProperty(name);

    m_properties[name+".Intensity"] = m_dblManager->addProperty("Intensity");
    m_properties[name+".Radius"] = m_dblManager->addProperty("Radius");
    m_properties[name+".Diffusion"] = m_dblManager->addProperty("Diffusion");
    m_properties[name+".Shift"] = m_dblManager->addProperty("Shift");

    m_dblManager->setDecimals(m_properties[name+".Intensity"], NUM_DECIMALS);
    m_dblManager->setDecimals(m_properties[name+".Radius"], NUM_DECIMALS);
    m_dblManager->setDecimals(m_properties[name+".Diffusion"], NUM_DECIMALS);
    m_dblManager->setDecimals(m_properties[name+".Shift"], NUM_DECIMALS);

    diffSphereGroup->addSubProperty(m_properties[name+".Intensity"]);
    diffSphereGroup->addSubProperty(m_properties[name+".Radius"]);
    diffSphereGroup->addSubProperty(m_properties[name+".Diffusion"]);
    diffSphereGroup->addSubProperty(m_properties[name+".Shift"]);

    return diffSphereGroup;
  }

  QtProperty* ConvFit::createDiffRotDiscreteCircle(const QString & name)
  {
    QtProperty* diffRotDiscreteCircleGroup = m_grpManager->addProperty(name);

    m_properties[name+".N"] = m_dblManager->addProperty("N");
    m_dblManager->setValue(m_properties[name+".N"], 3.0);

    m_properties[name+".Intensity"] = m_dblManager->addProperty("Intensity");
    m_properties[name+".Radius"] = m_dblManager->addProperty("Radius");
    m_properties[name+".Decay"] = m_dblManager->addProperty("Decay");
    m_properties[name+".Shift"] = m_dblManager->addProperty("Shift");

    m_dblManager->setDecimals(m_properties[name+".N"], 0);
    m_dblManager->setDecimals(m_properties[name+".Intensity"], NUM_DECIMALS);
    m_dblManager->setDecimals(m_properties[name+".Radius"], NUM_DECIMALS);
    m_dblManager->setDecimals(m_properties[name+".Decay"], NUM_DECIMALS);
    m_dblManager->setDecimals(m_properties[name+".Shift"], NUM_DECIMALS);

    diffRotDiscreteCircleGroup->addSubProperty(m_properties[name+".N"]);
    diffRotDiscreteCircleGroup->addSubProperty(m_properties[name+".Intensity"]);
    diffRotDiscreteCircleGroup->addSubProperty(m_properties[name+".Radius"]);
    diffRotDiscreteCircleGroup->addSubProperty(m_properties[name+".Decay"]);
    diffRotDiscreteCircleGroup->addSubProperty(m_properties[name+".Shift"]);

    return diffRotDiscreteCircleGroup;
  }

  void ConvFit::populateFunction(IFunction_sptr func, IFunction_sptr comp, QtProperty* group, const std::string & pref, bool tie)
  {
    // Get subproperties of group and apply them as parameters on the function object
    QList<QtProperty*> props = group->subProperties();

    for ( int i = 0; i < props.size(); i++ )
    {
      if ( tie || ! props[i]->subProperties().isEmpty() )
      {
        std::string name = pref + props[i]->propertyName().toStdString();
        std::string value = props[i]->valueText().toStdString();
        comp->tie(name, value);
      }
      else
      {
        std::string propName = props[i]->propertyName().toStdString();
        double propValue = props[i]->valueText().toDouble();
        if(propValue)
        {
          if(func->hasAttribute(propName))
            func->setAttributeValue(propName, propValue);
          else
            func->setParameter(propName, propValue);
        }
      }
    }
  }

  /**
   * Generate a string to describe the fit type selected by the user.
   * Used when naming the resultant workspaces.
   *
   * Assertions used to guard against any future changes that dont take
   * workspace naming into account.
   *
   * @returns the generated QString.
   */
  QString ConvFit::fitTypeString() const
  {
    QString fitType("");

    if( m_blnManager->value(m_properties["UseDeltaFunc"]) )
      fitType += "Delta";

    switch ( m_uiForm.cbFitType->currentIndex() )
    {
      case 0:
        break;
      case 1:
        fitType += "1L"; break;
      case 2:
        fitType += "2L"; break;
      case 3:
        fitType += "DS"; break;
      case 4:
        fitType += "DC"; break;
    }

    return fitType;
  }

  /**
   * Generate a string to describe the background selected by the user.
   * Used when naming the resultant workspaces.
   *
   * Assertions used to guard against any future changes that dont take
   * workspace naming into account.
   *
   * @returns the generated QString.
   */
  QString ConvFit::backgroundString() const
  {
    switch ( m_uiForm.cbBackground->currentIndex() )
    {
      case 0:
        return "FixF_s";
      case 1:
        return "FitF_s";
      case 2:
        return "FitL_s";
      default:
        return "";
    }
  }

  void ConvFit::typeSelection(int index)
  {
    m_cfTree->removeProperty(m_properties["Lorentzian1"]);
    m_cfTree->removeProperty(m_properties["Lorentzian2"]);
    m_cfTree->removeProperty(m_properties["DiffSphere"]);
    m_cfTree->removeProperty(m_properties["DiffRotDiscreteCircle"]);

    auto hwhmRangeSelector = m_uiForm.ppPlot->getRangeSelector("ConvFitHWHM");

    switch ( index )
    {
      case 0:
        hwhmRangeSelector->setVisible(false);
        break;
      case 1:
        m_cfTree->addProperty(m_properties["Lorentzian1"]);
        hwhmRangeSelector->setVisible(true);
        break;
      case 2:
        m_cfTree->addProperty(m_properties["Lorentzian1"]);
        m_cfTree->addProperty(m_properties["Lorentzian2"]);
        hwhmRangeSelector->setVisible(true);
        break;
      case 3:
        m_cfTree->addProperty(m_properties["DiffSphere"]);
        hwhmRangeSelector->setVisible(false);
        m_uiForm.ckPlotGuess->setChecked(false);
        m_blnManager->setValue(m_properties["UseDeltaFunc"], false);
        break;
      case 4:
        m_cfTree->addProperty(m_properties["DiffRotDiscreteCircle"]);
        hwhmRangeSelector->setVisible(false);
        m_uiForm.ckPlotGuess->setChecked(false);
        m_blnManager->setValue(m_properties["UseDeltaFunc"], false);
        break;
    }

    // Disable Plot Guess and Use Delta Function for DiffSphere and DiffRotDiscreteCircle
    m_uiForm.ckPlotGuess->setEnabled(index < 3);
    m_properties["UseDeltaFunc"]->setEnabled(index < 3);

    updatePlotOptions();
  }

  void ConvFit::bgTypeSelection(int index)
  {
    if ( index == 2 )
    {
      m_properties["LinearBackground"]->addSubProperty(m_properties["BGA1"]);
    }
    else
    {
      m_properties["LinearBackground"]->removeSubProperty(m_properties["BGA1"]);
    }
  }

  void ConvFit::updatePlot()
  {
    using Mantid::Kernel::Exception::NotFoundError;

    if(!m_cfInputWS)
    {
      g_log.error("No workspace loaded, cannot create preview plot.");
      return;
    }

    const bool plotGuess = m_uiForm.ckPlotGuess->isChecked();
    m_uiForm.ckPlotGuess->setChecked(false);

    int specNo = m_uiForm.spPlotSpectrum->text().toInt();

    m_uiForm.ppPlot->clear();
    m_uiForm.ppPlot->addSpectrum("Sample", m_cfInputWS, specNo);

    try
    {
      const QPair<double, double> curveRange = m_uiForm.ppPlot->getCurveRange("Sample");
      const std::pair<double, double> range(curveRange.first, curveRange.second);
      m_uiForm.ppPlot->getRangeSelector("ConvFitRange")->setRange(range.first, range.second);
      m_uiForm.ckPlotGuess->setChecked(plotGuess);
    }
    catch(std::invalid_argument & exc)
    {
      showMessageBox(exc.what());
    }

    // Default FWHM to resolution of instrument
    double resolution = getInstrumentResolution(m_cfInputWSName.toStdString());
    if(resolution > 0)
    {
      m_dblManager->setValue(m_properties["Lorentzian 1.FWHM"], resolution);
      m_dblManager->setValue(m_properties["Lorentzian 2.FWHM"], resolution);
    }

    // If there is a result plot then plot it
    if(AnalysisDataService::Instance().doesExist(m_pythonExportWsName))
    {
      WorkspaceGroup_sptr outputGroup = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(m_pythonExportWsName);
      if(specNo >= static_cast<int>(outputGroup->size()))
        return;
      MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(outputGroup->getItem(specNo));
      if(ws)
        m_uiForm.ppPlot->addSpectrum("Fit", ws, 1, Qt::red);
    }
  }

  void ConvFit::plotGuess()
  {
    m_uiForm.ppPlot->removeSpectrum("Guess");

    // Do nothing if there is not a sample and resolution
    if(!(m_uiForm.dsSampleInput->isValid() && m_uiForm.dsResInput->isValid()
          && m_uiForm.ckPlotGuess->isChecked()))
      return;

    bool tieCentres = (m_uiForm.cbFitType->currentIndex() > 1);
    CompositeFunction_sptr function = createFunction(tieCentres);

    if ( m_cfInputWS == NULL )
    {
      updatePlot();
    }

    const size_t binIndexLow = m_cfInputWS->binIndexOf(m_dblManager->value(m_properties["StartX"]));
    const size_t binIndexHigh = m_cfInputWS->binIndexOf(m_dblManager->value(m_properties["EndX"]));
    const size_t nData = binIndexHigh - binIndexLow;

    std::vector<double> inputXData(nData);
    const Mantid::MantidVec& XValues = m_cfInputWS->readX(0);
    const bool isHistogram = m_cfInputWS->isHistogramData();

    for ( size_t i = 0; i < nData; i++ )
    {
      if ( isHistogram )
      {
        inputXData[i] = 0.5 * ( XValues[binIndexLow+i] + XValues[binIndexLow+i+1] );
      }
      else
      {
        inputXData[i] = XValues[binIndexLow+i];
      }
    }

    FunctionDomain1DVector domain(inputXData);
    FunctionValues outputData(domain);
    function->function(domain, outputData);

    QVector<double> dataX, dataY;

    for ( size_t i = 0; i < nData; i++ )
    {
      dataX.append(inputXData[i]);
      dataY.append(outputData.getCalculated(i));
    }

    IAlgorithm_sptr createWsAlg = AlgorithmManager::Instance().create("CreateWorkspace");
    createWsAlg->initialize();
    createWsAlg->setChild(true);
    createWsAlg->setLogging(false);
    createWsAlg->setProperty("OutputWorkspace", "__GuessAnon");
    createWsAlg->setProperty("NSpec", 1);
    createWsAlg->setProperty("DataX", dataX.toStdVector());
    createWsAlg->setProperty("DataY", dataY.toStdVector());
    createWsAlg->execute();
    MatrixWorkspace_sptr guessWs = createWsAlg->getProperty("OutputWorkspace");

    m_uiForm.ppPlot->addSpectrum("Guess", guessWs, 0, Qt::green);
  }

  void ConvFit::singleFit()
  {
    if(!validate())
      return;

    updatePlot();

    m_uiForm.ckPlotGuess->setChecked(false);

    CompositeFunction_sptr function = createFunction(m_uiForm.ckTieCentres->isChecked());

    // get output name
    QString fitType = fitTypeString();
    QString bgType = backgroundString();

    if(fitType == "")
    {
      g_log.error("No fit type defined!");
    }

    QString outputNm = runPythonCode(QString("from IndirectCommon import getWSprefix\nprint getWSprefix('") + m_cfInputWSName + QString("')\n")).trimmed();
    outputNm += QString("conv_") + fitType + bgType + m_uiForm.spPlotSpectrum->text();
    std::string output = outputNm.toStdString();

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("Fit");
    alg->initialize();
    alg->setPropertyValue("Function", function->asString());
    alg->setPropertyValue("InputWorkspace", m_cfInputWSName.toStdString());
    alg->setProperty<int>("WorkspaceIndex", m_uiForm.spPlotSpectrum->text().toInt());
    alg->setProperty<double>("StartX", m_dblManager->value(m_properties["StartX"]));
    alg->setProperty<double>("EndX", m_dblManager->value(m_properties["EndX"]));
    alg->setProperty("Output", output);
    alg->setProperty("CreateOutput", true);
    alg->setProperty("OutputCompositeMembers", true);
    alg->setProperty("ConvolveMembers", true);
    alg->execute();

    if ( ! alg->isExecuted() )
    {
      showMessageBox("Fit algorithm failed.");
      return;
    }

    // Plot the line on the mini plot
    m_uiForm.ppPlot->removeSpectrum("Guess");
    m_uiForm.ppPlot->addSpectrum("Fit", outputNm+"_Workspace", 1, Qt::red);

    IFunction_sptr outputFunc = alg->getProperty("Function");

    // Get params.
    QMap<QString,double> parameters;
    std::vector<std::string> parNames = outputFunc->getParameterNames();
    std::vector<double> parVals;

    for( size_t i = 0; i < parNames.size(); ++i )
      parVals.push_back(outputFunc->getParameter(parNames[i]));

    for ( size_t i = 0; i < parNames.size(); ++i )
      parameters[QString(parNames[i].c_str())] = parVals[i];

    // Populate Tree widget with values
    // Background should always be f0
    m_dblManager->setValue(m_properties["BGA0"], parameters["f0.A0"]);
    m_dblManager->setValue(m_properties["BGA1"], parameters["f0.A1"]);

    int fitTypeIndex = m_uiForm.cbFitType->currentIndex();

    int funcIndex = 0;
    int subIndex = 0;

    //check if we're using a temperature correction
    if (m_uiForm.ckTempCorrection->isChecked() &&
        !m_uiForm.leTempCorrection->text().isEmpty())
    {
      subIndex++;
    }

    bool usingDeltaFunc = m_blnManager->value(m_properties["UseDeltaFunc"]);

    // If using a delta function with any fit type or using two Lorentzians
    bool usingCompositeFunc = ((usingDeltaFunc && fitTypeIndex > 0) || fitTypeIndex == 2);

    QString prefBase = "f1.f1.";

    if ( usingDeltaFunc )
    {
      QString key = prefBase;
      if (usingCompositeFunc)
      {
        key += "f0.";
      }

      key += "Height";

      m_dblManager->setValue(m_properties["DeltaHeight"], parameters[key]);
      funcIndex++;
    }

    if ( fitTypeIndex == 1 || fitTypeIndex == 2 )
    {
      // One Lorentz
      QString pref = prefBase;

      if ( usingCompositeFunc )
      {
        pref += "f" + QString::number(funcIndex) + ".f" + QString::number(subIndex) + ".";
      }
      else
      {
        pref += "f" + QString::number(subIndex) + ".";
      }

      m_dblManager->setValue(m_properties["Lorentzian 1.Amplitude"], parameters[pref+"Amplitude"]);
      m_dblManager->setValue(m_properties["Lorentzian 1.PeakCentre"], parameters[pref+"PeakCentre"]);
      m_dblManager->setValue(m_properties["Lorentzian 1.FWHM"], parameters[pref+"FWHM"]);
      funcIndex++;
    }

    if ( fitTypeIndex == 2 )
    {
      // Two Lorentz
      QString pref = prefBase;
      pref += "f" + QString::number(funcIndex) + ".f" + QString::number(subIndex) + ".";

      m_dblManager->setValue(m_properties["Lorentzian 2.Amplitude"], parameters[pref+"Amplitude"]);
      m_dblManager->setValue(m_properties["Lorentzian 2.PeakCentre"], parameters[pref+"PeakCentre"]);
      m_dblManager->setValue(m_properties["Lorentzian 2.FWHM"], parameters[pref+"FWHM"]);
    }

    if ( fitTypeIndex == 3 )
    {
      // DiffSphere
      QString pref = prefBase;

      if ( usingCompositeFunc )
      {
        pref += "f" + QString::number(funcIndex) + ".f" + QString::number(subIndex) + ".";
      }
      else
      {
        pref += "f" + QString::number(subIndex) + ".";
      }

      m_dblManager->setValue(m_properties["Diffusion Sphere.Intensity"], parameters[pref+"Intensity"]);
      m_dblManager->setValue(m_properties["Diffusion Sphere.Radius"], parameters[pref+"Radius"]);
      m_dblManager->setValue(m_properties["Diffusion Sphere.Diffusion"], parameters[pref+"Diffusion"]);
      m_dblManager->setValue(m_properties["Diffusion Sphere.Shift"], parameters[pref+"Shift"]);
    }

    if ( fitTypeIndex == 4 )
    {
      // DiffSphere
      QString pref = prefBase;

      if ( usingCompositeFunc )
      {
        pref += "f" + QString::number(funcIndex) + ".f" + QString::number(subIndex) + ".";
      }
      else
      {
        pref += "f" + QString::number(subIndex) + ".";
      }

      m_dblManager->setValue(m_properties["Diffusion Circle.Intensity"], parameters[pref+"Intensity"]);
      m_dblManager->setValue(m_properties["Diffusion Circle.Radius"], parameters[pref+"Radius"]);
      m_dblManager->setValue(m_properties["Diffusion Circle.Decay"], parameters[pref+"Decay"]);
      m_dblManager->setValue(m_properties["Diffusion Circle.Shift"], parameters[pref+"Shift"]);
    }

    m_pythonExportWsName = "";
  }

  /**
   * Handles the user entering a new minimum spectrum index.
   *
   * Prevents the user entering an overlapping spectra range.
   *
   * @param value Minimum spectrum index
   */
  void ConvFit::specMinChanged(int value)
  {
    m_uiForm.spSpectraMax->setMinimum(value);
  }

  /**
   * Handles the user entering a new maximum spectrum index.
   *
   * Prevents the user entering an overlapping spectra range.
   *
   * @param value Maximum spectrum index
   */
  void ConvFit::specMaxChanged(int value)
  {
    m_uiForm.spSpectraMin->setMaximum(value);
  }

  void ConvFit::minChanged(double val)
  {
    m_dblManager->setValue(m_properties["StartX"], val);
  }

  void ConvFit::maxChanged(double val)
  {
    m_dblManager->setValue(m_properties["EndX"], val);
  }

  void ConvFit::hwhmChanged(double val)
  {
    const double peakCentre = m_dblManager->value(m_properties["Lorentzian 1.PeakCentre"]);
    // Always want FWHM to display as positive.
    const double hwhm = std::fabs(val-peakCentre);
    // Update the property
    auto hwhmRangeSelector = m_uiForm.ppPlot->getRangeSelector("ConvFitHWHM");
    hwhmRangeSelector->blockSignals(true);
    m_dblManager->setValue(m_properties["Lorentzian 1.FWHM"], hwhm*2);
    hwhmRangeSelector->blockSignals(false);
  }

  void ConvFit::backgLevel(double val)
  {
    m_dblManager->setValue(m_properties["BGA0"], val);
  }

  void ConvFit::updateRS(QtProperty* prop, double val)
  {
    auto fitRangeSelector = m_uiForm.ppPlot->getRangeSelector("ConvFitRange");
    auto backRangeSelector = m_uiForm.ppPlot->getRangeSelector("ConvFitBackRange");

    if ( prop == m_properties["StartX"] ) { fitRangeSelector->setMinimum(val); }
    else if ( prop == m_properties["EndX"] ) { fitRangeSelector->setMaximum(val); }
    else if ( prop == m_properties["BGA0"] ) { backRangeSelector->setMinimum(val); }
    else if ( prop == m_properties["Lorentzian 1.FWHM"] ) { hwhmUpdateRS(val); }
    else if ( prop == m_properties["Lorentzian 1.PeakCentre"] )
    {
      hwhmUpdateRS(m_dblManager->value(m_properties["Lorentzian 1.FWHM"]));
    }
  }

  void ConvFit::hwhmUpdateRS(double val)
  {
    const double peakCentre = m_dblManager->value(m_properties["Lorentzian 1.PeakCentre"]);
    auto hwhmRangeSelector = m_uiForm.ppPlot->getRangeSelector("ConvFitHWHM");
    hwhmRangeSelector->setMinimum(peakCentre-val/2);
    hwhmRangeSelector->setMaximum(peakCentre+val/2);
  }

  void ConvFit::checkBoxUpdate(QtProperty* prop, bool checked)
  {
    UNUSED_ARG(checked);

    if(prop == m_properties["UseDeltaFunc"])
      updatePlotOptions();
  }

  void ConvFit::fitContextMenu(const QPoint &)
  {
    QtBrowserItem* item(NULL);

    item = m_cfTree->currentItem();

    if ( ! item )
      return;

    // is it a fit property ?
    QtProperty* prop = item->property();

    if ( prop == m_properties["StartX"] || prop == m_properties["EndX"] )
      return;

    // is it already fixed?
    bool fixed = prop->propertyManager() != m_dblManager;

    if ( fixed && prop->propertyManager() != m_stringManager )
      return;

    // Create the menu
    QMenu* menu = new QMenu("ConvFit", m_cfTree);
    QAction* action;

    if ( ! fixed )
    {
      action = new QAction("Fix", m_parentWidget);
      connect(action, SIGNAL(triggered()), this, SLOT(fixItem()));
    }
    else
    {
      action = new QAction("Remove Fix", m_parentWidget);
      connect(action, SIGNAL(triggered()), this, SLOT(unFixItem()));
    }

    menu->addAction(action);

    // Show the menu
    menu->popup(QCursor::pos());
  }

  void ConvFit::fixItem()
  {
    QtBrowserItem* item = m_cfTree->currentItem();

    // Determine what the property is.
    QtProperty* prop = item->property();
    QtProperty* fixedProp = m_stringManager->addProperty( prop->propertyName() );
    QtProperty* fprlbl = m_stringManager->addProperty("Fixed");
    fixedProp->addSubProperty(fprlbl);
    m_stringManager->setValue(fixedProp, prop->valueText());

    item->parent()->property()->addSubProperty(fixedProp);

    m_fixedProps[fixedProp] = prop;

    item->parent()->property()->removeSubProperty(prop);
  }

  void ConvFit::unFixItem()
  {
    QtBrowserItem* item = m_cfTree->currentItem();

    QtProperty* prop = item->property();
    if ( prop->subProperties().empty() )
    {
      item = item->parent();
      prop = item->property();
    }

    item->parent()->property()->addSubProperty(m_fixedProps[prop]);
    item->parent()->property()->removeSubProperty(prop);
    m_fixedProps.remove(prop);
    QtProperty* proplbl = prop->subProperties()[0];
    delete proplbl;
    delete prop;
  }

  void ConvFit::showTieCheckbox(QString fitType)
  {
    m_uiForm.ckTieCentres->setVisible( fitType == "Two Lorentzians" );
  }

  void ConvFit::updatePlotOptions()
  {
    m_uiForm.cbPlotType->clear();

    bool deltaFunction = m_blnManager->value(m_properties["UseDeltaFunc"]);

    QStringList plotOptions;
    plotOptions << "None";

    if(deltaFunction)
      plotOptions << "Height";

    switch(m_uiForm.cbFitType->currentIndex())
    {
      // Lorentzians
      case 1:
      case 2:
        plotOptions << "Amplitude" << "FWHM";
        if(deltaFunction)
          plotOptions << "EISF";
        break;

      // DiffSphere
      case 3:
        plotOptions << "Intensity" << "Radius" << "Diffusion" << "Shift";
        break;

      // DiffRotDiscreteCircle
      case 4:
        plotOptions << "Intensity" << "Radius" << "Decay" << "Shift";
        break;

      default:
        break;
    }

    plotOptions << "All";
    m_uiForm.cbPlotType->addItems(plotOptions);
  }

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
