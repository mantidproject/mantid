#include "MantidQtCustomInterfaces/ConvFit.h"

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
    IDATab(parent), m_intVal(NULL), m_stringManager(NULL), m_cfTree(NULL), 
      m_cfPlot(NULL), m_cfProp(), m_fixedProps(), m_cfRangeS(NULL), m_cfBackgS(NULL), 
      m_cfHwhmRange(NULL), m_cfGrpMng(NULL), m_cfDblMng(NULL), m_cfBlnMng(NULL), m_cfDataCurve(NULL), 
      m_cfCalcCurve(NULL), m_cfInputWS(), m_cfInputWSName(), m_confitResFileType()
  {}
  
  void ConvFit::setup()
  {
    m_intVal = new QIntValidator(this);
    
    // Create Property Managers
    m_cfGrpMng = new QtGroupPropertyManager();
    m_cfBlnMng = new QtBoolPropertyManager();
    m_cfDblMng = new QtDoublePropertyManager();
    m_stringManager = new QtStringPropertyManager();

    // Create TreeProperty Widget
    m_cfTree = new QtTreePropertyBrowser();
    uiForm().confit_properties->addWidget(m_cfTree);

    // add factories to managers
    m_cfTree->setFactoryForManager(m_cfBlnMng, qtCheckBoxFactory());
    m_cfTree->setFactoryForManager(m_cfDblMng, doubleEditorFactory());

    // Create Plot Widget
    m_cfPlot = new QwtPlot(this);
    m_cfPlot->setAxisFont(QwtPlot::xBottom, this->font());
    m_cfPlot->setAxisFont(QwtPlot::yLeft, this->font());
    m_cfPlot->setCanvasBackground(Qt::white);
    uiForm().confit_plot->addWidget(m_cfPlot);

    // Create Range Selectors
    m_cfRangeS = new MantidQt::MantidWidgets::RangeSelector(m_cfPlot);
    m_cfBackgS = new MantidQt::MantidWidgets::RangeSelector(m_cfPlot, 
      MantidQt::MantidWidgets::RangeSelector::YSINGLE);
    m_cfBackgS->setColour(Qt::darkGreen);
    m_cfBackgS->setRange(0.0, 1.0);
    m_cfHwhmRange = new MantidQt::MantidWidgets::RangeSelector(m_cfPlot);
    m_cfHwhmRange->setColour(Qt::red);

    // Populate Property Widget
    m_cfProp["FitRange"] = m_cfGrpMng->addProperty("Fitting Range");
    m_cfProp["StartX"] = m_cfDblMng->addProperty("StartX");
    m_cfDblMng->setDecimals(m_cfProp["StartX"], NUM_DECIMALS);
    m_cfProp["EndX"] = m_cfDblMng->addProperty("EndX");
    m_cfDblMng->setDecimals(m_cfProp["EndX"], NUM_DECIMALS);
    m_cfProp["FitRange"]->addSubProperty(m_cfProp["StartX"]);
    m_cfProp["FitRange"]->addSubProperty(m_cfProp["EndX"]);
    m_cfTree->addProperty(m_cfProp["FitRange"]);

    m_cfProp["LinearBackground"] = m_cfGrpMng->addProperty("Background");
    m_cfProp["BGA0"] = m_cfDblMng->addProperty("A0");
    m_cfDblMng->setDecimals(m_cfProp["BGA0"], NUM_DECIMALS);
    m_cfProp["BGA1"] = m_cfDblMng->addProperty("A1");
    m_cfDblMng->setDecimals(m_cfProp["BGA1"], NUM_DECIMALS);
    m_cfProp["LinearBackground"]->addSubProperty(m_cfProp["BGA0"]);
    m_cfProp["LinearBackground"]->addSubProperty(m_cfProp["BGA1"]);
    m_cfTree->addProperty(m_cfProp["LinearBackground"]);

    // Delta Function
    m_cfProp["DeltaFunction"] = m_cfGrpMng->addProperty("Delta Function");
    m_cfProp["UseDeltaFunc"] = m_cfBlnMng->addProperty("Use");
    m_cfProp["DeltaHeight"] = m_cfDblMng->addProperty("Height");
    m_cfDblMng->setDecimals(m_cfProp["DeltaHeight"], NUM_DECIMALS);
    m_cfProp["DeltaFunction"]->addSubProperty(m_cfProp["UseDeltaFunc"]);
    m_cfTree->addProperty(m_cfProp["DeltaFunction"]);

    m_cfProp["Lorentzian1"] = createLorentzian("Lorentzian 1");
    m_cfProp["Lorentzian2"] = createLorentzian("Lorentzian 2");

    uiForm().confit_leTempCorrection->setValidator(new QDoubleValidator(this));

    // Connections
    connect(m_cfRangeS, SIGNAL(minValueChanged(double)), this, SLOT(minChanged(double)));
    connect(m_cfRangeS, SIGNAL(maxValueChanged(double)), this, SLOT(maxChanged(double)));
    connect(m_cfBackgS, SIGNAL(minValueChanged(double)), this, SLOT(backgLevel(double)));
    connect(m_cfHwhmRange, SIGNAL(minValueChanged(double)), this, SLOT(hwhmChanged(double)));
    connect(m_cfHwhmRange, SIGNAL(maxValueChanged(double)), this, SLOT(hwhmChanged(double)));
    connect(m_cfDblMng, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(updateRS(QtProperty*, double)));
    connect(m_cfBlnMng, SIGNAL(valueChanged(QtProperty*, bool)), this, SLOT(checkBoxUpdate(QtProperty*, bool)));
    connect(m_cfDblMng, SIGNAL(propertyChanged(QtProperty*)), this, SLOT(plotGuess(QtProperty*)));
    connect(uiForm().confit_ckTempCorrection, SIGNAL(toggled(bool)), uiForm().confit_leTempCorrection, SLOT(setEnabled(bool)));

    // Have FWHM Range linked to Fit Start/End Range
    connect(m_cfRangeS, SIGNAL(rangeChanged(double, double)), m_cfHwhmRange, SLOT(setRange(double, double)));
    m_cfHwhmRange->setRange(-1.0,1.0);
    hwhmUpdateRS(0.02);

    typeSelection(uiForm().confit_cbFitType->currentIndex());
    bgTypeSelection(uiForm().confit_cbBackground->currentIndex());

    // Replot input automatically when file / spec no changes
    connect(uiForm().confit_leSpecNo, SIGNAL(editingFinished()), this, SLOT(plotInput()));
    connect(uiForm().confit_dsSampleInput, SIGNAL(dataReady(const QString&)), this, SLOT(plotInput()));
    
    connect(uiForm().confit_cbFitType, SIGNAL(currentIndexChanged(int)), this, SLOT(typeSelection(int)));
    connect(uiForm().confit_cbBackground, SIGNAL(currentIndexChanged(int)), this, SLOT(bgTypeSelection(int)));
    connect(uiForm().confit_pbSingle, SIGNAL(clicked()), this, SLOT(singleFit()));

    uiForm().confit_leSpecNo->setValidator(m_intVal);
    uiForm().confit_leSpecMax->setValidator(m_intVal);

    // Context menu
    m_cfTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_cfTree, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(fitContextMenu(const QPoint &)));

    // Tie
    connect(uiForm().confit_cbFitType,SIGNAL(currentIndexChanged(QString)),SLOT(showTieCheckbox(QString)));
    showTieCheckbox( uiForm().confit_cbFitType->currentText() );
  }

  void ConvFit::run()
  {
    const QString error = validate();
    if( ! error.isEmpty() )
    {
      showInformationBox(error);
      return;
    }

    if ( m_cfInputWS == NULL )
    {
      return;
    }

    QString ftype = fitTypeString();
    QString bg = backgroundString();

    if(ftype == "")
    {
      g_log.error("No fit type defined");
    }

    bool useTies = uiForm().confit_ckTieCentres->isChecked();
    QString ties = (useTies ? "True" : "False");

    Mantid::API::CompositeFunction_sptr func = createFunction(useTies);
    std::string function = std::string(func->asString());
    QString stX = m_cfProp["StartX"]->valueText();
    QString enX = m_cfProp["EndX"]->valueText();

    QString pyInput =
      "from IndirectDataAnalysis import confitSeq\n"
      "input = '" + m_cfInputWSName + "'\n"
      "func = r'" + QString::fromStdString(function) + "'\n"
      "startx = " + stX + "\n"
      "endx = " + enX + "\n"
      "specMin = " + uiForm().confit_leSpecNo->text() + "\n"
      "specMax = " + uiForm().confit_leSpecMax->text() + "\n"
      "plot = '" + uiForm().confit_cbPlotOutput->currentText() + "'\n"
      "ties = " + ties + "\n"
      "save = ";
  
    pyInput += uiForm().confit_ckSaveSeq->isChecked() ? "True\n" : "False\n";

    if ( uiForm().confit_ckVerbose->isChecked() ) pyInput += "verbose = True\n";
    else pyInput += "verbose = False\n";

    QString temperature = uiForm().confit_leTempCorrection->text();
    bool useTempCorrection = (!temperature.isEmpty() && uiForm().confit_ckTempCorrection->isChecked());
    if ( useTempCorrection ) 
    {
      pyInput += "temp=" + temperature + "\n";
    }
    else
    {
      pyInput += "temp=None\n";
    }
  
    pyInput +=    
      "bg = '" + bg + "'\n"
      "ftype = '" + ftype + "'\n"
      "confitSeq(input, func, startx, endx, ftype, bg, temp, specMin, specMax, Verbose=verbose, Plot=plot, Save=save)\n";

    QString pyOutput = runPythonCode(pyInput);
  }

  /**
   * Validates the user's inputs in the ConvFit tab.
   *
   * @returns an string containing an error message if invalid input detected, else an empty string.
   */
  QString ConvFit::validate()
  {
    using Mantid::API::AnalysisDataService;
    
    UserInputValidator uiv;

    uiv.checkDataSelectorIsValid("Sample", uiForm().confit_dsSampleInput);
    uiv.checkDataSelectorIsValid("Resolution", uiForm().confit_dsResInput);

    auto range = std::make_pair(m_cfDblMng->value(m_cfProp["StartX"]), m_cfDblMng->value(m_cfProp["EndX"]));
    uiv.checkValidRange("Fitting Range", range);

    // Enforce the rule that at least one fit is needed; either a delta function, one or two lorentzian functions,
    // or both.  (The resolution function must be convolved with a model.)
    if ( uiForm().confit_cbFitType->currentIndex() == 0 && ! m_cfBlnMng->value(m_cfProp["UseDeltaFunc"]) )
      uiv.addErrorMessage("No fit function has been selected.");

    return uiv.generateErrorMessage();
  }

  void ConvFit::loadSettings(const QSettings & settings)
  {
    uiForm().confit_dsSampleInput->readSettings(settings.group());
    uiForm().confit_dsResInput->readSettings(settings.group());
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
   *
   * @param tieCentres :: whether to tie centres of the two lorentzians.
   *
   * @returns the composite fitting function.
   */
  Mantid::API::CompositeFunction_sptr ConvFit::createFunction(bool tieCentres)
  {
    auto conv = boost::dynamic_pointer_cast<Mantid::API::CompositeFunction>(Mantid::API::FunctionFactory::Instance().createFunction("Convolution"));
    Mantid::API::CompositeFunction_sptr comp( new Mantid::API::CompositeFunction );

    Mantid::API::IFunction_sptr func;
    size_t index = 0;

    // -------------------------------------
    // --- Composite / Linear Background ---
    // -------------------------------------
    func = Mantid::API::FunctionFactory::Instance().createFunction("LinearBackground");
    comp->addFunction(func); 

    const int bgType = uiForm().confit_cbBackground->currentIndex(); // 0 = Fixed Flat, 1 = Fit Flat, 2 = Fit all
  
    if ( bgType == 0 || ! m_cfProp["BGA0"]->subProperties().isEmpty() )
    {
      comp->tie("f0.A0", m_cfProp["BGA0"]->valueText().toStdString() );
    }
    else
    {
      func->setParameter("A0", m_cfProp["BGA0"]->valueText().toDouble());
    }

    if ( bgType != 2 )
    {
      comp->tie("f0.A1", "0.0");
    }
    else
    {
      if ( ! m_cfProp["BGA1"]->subProperties().isEmpty() )
      {
        comp->tie("f0.A1", m_cfProp["BGA1"]->valueText().toStdString() );
      }
      else { func->setParameter("A1", m_cfProp["BGA1"]->valueText().toDouble()); }
    }

    // --------------------------------------------
    // --- Composite / Convolution / Resolution ---
    // --------------------------------------------
    func = Mantid::API::FunctionFactory::Instance().createFunction("Resolution");
    conv->addFunction(func);
    
    //add resolution file
    if (uiForm().confit_dsResInput->isFileSelectorVisible())
    {    
      std::string resfilename = uiForm().confit_dsResInput->getFullFilePath().toStdString();
      Mantid::API::IFunction::Attribute attr(resfilename);
      func->setAttribute("FileName", attr);
    }
    else
    {
      std::string resWorkspace = uiForm().confit_dsResInput->getCurrentDataName().toStdString();
      Mantid::API::IFunction::Attribute attr(resWorkspace);
      func->setAttribute("Workspace", attr);
    }

    // --------------------------------------------------------
    // --- Composite / Convolution / Model / Delta Function ---
    // --------------------------------------------------------
    Mantid::API::CompositeFunction_sptr model( new Mantid::API::CompositeFunction );

    bool useDeltaFunc = m_cfBlnMng->value(m_cfProp["UseDeltaFunc"]);

    size_t subIndex = 0;

    if ( useDeltaFunc )
    {
      func = Mantid::API::FunctionFactory::Instance().createFunction("DeltaFunction");
			index = model->addFunction(func);
			std::string parName = createParName(index);
			populateFunction(func, model, m_cfProp["DeltaFunction"], parName, false);
    }

    // ------------------------------------------------------------
    // --- Composite / Convolution / Model / Temperature Factor ---
    // ------------------------------------------------------------

    //create temperature correction function to multiply with the lorentzians
    Mantid::API::IFunction_sptr tempFunc;
    QString temperature = uiForm().confit_leTempCorrection->text();
    bool useTempCorrection = (!temperature.isEmpty() && uiForm().confit_ckTempCorrection->isChecked());

    // -----------------------------------------------------
    // --- Composite / Convolution / Model / Lorentzians ---
    // -----------------------------------------------------
    std::string prefix1;
    std::string prefix2;

    int fitTypeIndex = uiForm().confit_cbFitType->currentIndex();  

    // Add 1st Lorentzian
    if(fitTypeIndex > 0)
    {
      //if temperature not included then product is lorentzian * 1
      //create product function for temp * lorentzian
      auto product = boost::dynamic_pointer_cast<Mantid::API::CompositeFunction>(Mantid::API::FunctionFactory::Instance().createFunction("ProductFunction"));
      
      if(useTempCorrection)
      {
        createTemperatureCorrection(product);
      }

      func = Mantid::API::FunctionFactory::Instance().createFunction("Lorentzian");
      subIndex = product->addFunction(func);
      index = model->addFunction(product);
      prefix1 = createParName(index, subIndex);

      populateFunction(func, model, m_cfProp["Lorentzian1"], prefix1, false);
    }

    // Add 2nd Lorentzian
    if(fitTypeIndex == 2)
    {
      //if temperature not included then product is lorentzian * 1
      //create product function for temp * lorentzian
      auto product = boost::dynamic_pointer_cast<Mantid::API::CompositeFunction>(Mantid::API::FunctionFactory::Instance().createFunction("ProductFunction"));
    
      if(useTempCorrection)
      {
        createTemperatureCorrection(product);
      }

      func = Mantid::API::FunctionFactory::Instance().createFunction("Lorentzian");
      subIndex = product->addFunction(func);
      index = model->addFunction(product);
      prefix2 = createParName(index, subIndex);
      
      populateFunction(func, model, m_cfProp["Lorentzian2"], prefix2, false);
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

  void ConvFit::createTemperatureCorrection(Mantid::API::CompositeFunction_sptr product)
  {
    //create temperature correction function to multiply with the lorentzians
    Mantid::API::IFunction_sptr tempFunc;
    QString temperature = uiForm().confit_leTempCorrection->text();
    
    //create user function for the exponential correction
    // (x*temp) / 1-exp(-(x*temp))
    tempFunc = Mantid::API::FunctionFactory::Instance().createFunction("UserFunction");
    //11.606 is the conversion factor from meV to K
    std::string formula = "((x*11.606)/Temp) / (1 - exp(-((x*11.606)/Temp)))";
    Mantid::API::IFunction::Attribute att(formula);
    tempFunc->setAttribute("Formula", att);
    tempFunc->setParameter("Temp", temperature.toDouble());

    product->addFunction(tempFunc);
    product->tie("f0.Temp", temperature.toStdString());
    product->applyTies();
  }

  QtProperty* ConvFit::createLorentzian(const QString & name)
  {
    QtProperty* lorentzGroup = m_cfGrpMng->addProperty(name);
    m_cfProp[name+".Amplitude"] = m_cfDblMng->addProperty("Amplitude");
    // m_cfDblMng->setRange(m_cfProp[name+".Amplitude"], 0.0, 1.0); // 0 < Amplitude < 1
    m_cfProp[name+".PeakCentre"] = m_cfDblMng->addProperty("PeakCentre");
    m_cfProp[name+".FWHM"] = m_cfDblMng->addProperty("FWHM");
    m_cfDblMng->setDecimals(m_cfProp[name+".Amplitude"], NUM_DECIMALS);
    m_cfDblMng->setDecimals(m_cfProp[name+".PeakCentre"], NUM_DECIMALS);
    m_cfDblMng->setDecimals(m_cfProp[name+".FWHM"], NUM_DECIMALS);
    m_cfDblMng->setValue(m_cfProp[name+".FWHM"], 0.02);
    lorentzGroup->addSubProperty(m_cfProp[name+".Amplitude"]);
    lorentzGroup->addSubProperty(m_cfProp[name+".PeakCentre"]);
    lorentzGroup->addSubProperty(m_cfProp[name+".FWHM"]);
    return lorentzGroup;
  }

  void ConvFit::populateFunction(Mantid::API::IFunction_sptr func, Mantid::API::IFunction_sptr comp, QtProperty* group, const std::string & pref, bool tie)
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
				if ( propValue )
				{
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

    if( m_cfBlnMng->value(m_cfProp["UseDeltaFunc"]) )
      fitType += "Delta";

    switch ( uiForm().confit_cbFitType->currentIndex() )
    {
    case 0:
      break;
    case 1:
      fitType += "1L"; break;
    case 2:
      fitType += "2L"; break;
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
    switch ( uiForm().confit_cbBackground->currentIndex() )
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
    m_cfTree->removeProperty(m_cfProp["Lorentzian1"]);
    m_cfTree->removeProperty(m_cfProp["Lorentzian2"]);
  
    switch ( index )
    {
    case 0:
      m_cfHwhmRange->setVisible(false);
      break;
    case 1:
      m_cfTree->addProperty(m_cfProp["Lorentzian1"]);
      m_cfHwhmRange->setVisible(true);
      break;
    case 2:
      m_cfTree->addProperty(m_cfProp["Lorentzian1"]);
      m_cfTree->addProperty(m_cfProp["Lorentzian2"]);
      m_cfHwhmRange->setVisible(true);
      break;
    }    
  }

  void ConvFit::bgTypeSelection(int index)
  {
    if ( index == 2 )
    {
      m_cfProp["LinearBackground"]->addSubProperty(m_cfProp["BGA1"]);
    }
    else
    {
      m_cfProp["LinearBackground"]->removeSubProperty(m_cfProp["BGA1"]);
    }
  }

  void ConvFit::plotInput()
  {
    using Mantid::API::MatrixWorkspace;
    using Mantid::API::AnalysisDataService;
    using Mantid::Kernel::Exception::NotFoundError;

    const bool plotGuess = uiForm().confit_ckPlotGuess->isChecked();
    uiForm().confit_ckPlotGuess->setChecked(false);

    if(uiForm().confit_dsSampleInput->getCurrentDataName() != m_cfInputWSName)
    {      
      m_cfInputWSName = uiForm().confit_dsSampleInput->getCurrentDataName();
      m_cfInputWS = AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>(m_cfInputWSName.toStdString());
      
      if(!m_cfInputWS)
      {
        showInformationBox("Could not find the workspace in ADS. See log for details.");
      }
    }

    int specNo = uiForm().confit_leSpecNo->text().toInt();
    // Set spectra max value
    size_t specMax = m_cfInputWS->getNumberHistograms();
    if( specMax > 0 ) specMax -= 1;
    if ( specNo < 0 || static_cast<size_t>(specNo) > specMax ) //cast is okay as the first check is for less-than-zero
    {
      uiForm().confit_leSpecNo->setText("0");
      specNo = 0;
    }
    int smCurrent = uiForm().confit_leSpecMax->text().toInt();
    if ( smCurrent < 0 || static_cast<size_t>(smCurrent) > specMax )
    {
      uiForm().confit_leSpecMax->setText(QString::number(specMax));
    }

    m_cfDataCurve = plotMiniplot(m_cfPlot, m_cfDataCurve, m_cfInputWS, specNo);
    try
    {
      const std::pair<double, double> range = getCurveRange(m_cfDataCurve);
      m_cfRangeS->setRange(range.first, range.second);
      uiForm().confit_ckPlotGuess->setChecked(plotGuess);
    }
    catch(std::invalid_argument & exc)
    {
      showInformationBox(exc.what());
    }
  }

  void ConvFit::plotGuess(QtProperty*)
  {
    if ( ! uiForm().confit_ckPlotGuess->isChecked() || m_cfDataCurve == NULL )
    {
      return;
    }

    bool tieCentres = (uiForm().confit_cbFitType->currentIndex() > 1);
    Mantid::API::CompositeFunction_sptr function = createFunction(tieCentres);

    if ( m_cfInputWS == NULL )
    {
      plotInput();
    }

    const size_t binIndexLow = m_cfInputWS->binIndexOf(m_cfDblMng->value(m_cfProp["StartX"]));
    const size_t binIndexHigh = m_cfInputWS->binIndexOf(m_cfDblMng->value(m_cfProp["EndX"]));
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

    Mantid::API::FunctionDomain1DVector domain(inputXData);
    Mantid::API::FunctionValues outputData(domain);
    function->function(domain, outputData);

    QVector<double> dataX, dataY;

    for ( size_t i = 0; i < nData; i++ )
    {
      dataX.append(inputXData[i]);
      dataY.append(outputData.getCalculated(i));
    }

    if ( m_cfCalcCurve != NULL )
    {
      m_cfCalcCurve->attach(0);
      delete m_cfCalcCurve;
      m_cfCalcCurve = 0;
    }

    m_cfCalcCurve = new QwtPlotCurve();
    m_cfCalcCurve->setData(dataX, dataY);
    QPen fitPen(Qt::red, Qt::SolidLine);
    m_cfCalcCurve->setPen(fitPen);
    m_cfCalcCurve->attach(m_cfPlot);
    m_cfPlot->replot();
  }

  void ConvFit::singleFit()
  {
    const QString error = validate();
    if( ! error.isEmpty() )
    {
      showInformationBox(error);
      return;
    }

    plotInput();

    if ( m_cfDataCurve == NULL )
    {
      showInformationBox("There was an error reading the data file.");
      return;
    }

    uiForm().confit_ckPlotGuess->setChecked(false);

    Mantid::API::CompositeFunction_sptr function = createFunction(uiForm().confit_ckTieCentres->isChecked());

    // get output name
    QString ftype = fitTypeString();
    QString bg = backgroundString();

    if(ftype == "")
    {
      g_log.error("No fit type defined!");
    }

    QString outputNm = runPythonCode(QString("from IndirectCommon import getWSprefix\nprint getWSprefix('") + m_cfInputWSName + QString("')\n")).trimmed();
    outputNm += QString("conv_") + ftype + bg + uiForm().confit_leSpecNo->text();  
    std::string output = outputNm.toStdString();

    Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("Fit");
    alg->initialize();
    alg->setPropertyValue("Function", function->asString());
    alg->setPropertyValue("InputWorkspace", m_cfInputWSName.toStdString());
    alg->setProperty<int>("WorkspaceIndex", uiForm().confit_leSpecNo->text().toInt());
    alg->setProperty<double>("StartX", m_cfDblMng->value(m_cfProp["StartX"]));
    alg->setProperty<double>("EndX", m_cfDblMng->value(m_cfProp["EndX"]));
    alg->setPropertyValue("Output", output);
    alg->execute();
   
    if ( ! alg->isExecuted() )
    {
      showInformationBox("Fit algorithm failed.");
      return;
    }

    // Plot the line on the mini plot
    m_cfCalcCurve = plotMiniplot(m_cfPlot, m_cfCalcCurve, outputNm+"_Workspace", 1);
    QPen fitPen(Qt::red, Qt::SolidLine);
    m_cfCalcCurve->setPen(fitPen);
    m_cfPlot->replot();

    Mantid::API::IFunction_sptr outputFunc = alg->getProperty("Function");

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
    m_cfDblMng->setValue(m_cfProp["BGA0"], parameters["f0.A0"]);
    m_cfDblMng->setValue(m_cfProp["BGA1"], parameters["f0.A1"]);

    int noLorentz = uiForm().confit_cbFitType->currentIndex();

    int funcIndex = 0;
		int subIndex = 0;

		//check if we're using a temperature correction
		if (uiForm().confit_ckTempCorrection->isChecked() && 
				!uiForm().confit_leTempCorrection->text().isEmpty())
		{
				subIndex++;
		}

		bool usingDeltaFunc = m_cfBlnMng->value(m_cfProp["UseDeltaFunc"]);
		bool usingCompositeFunc = ((usingDeltaFunc && noLorentz > 0) || noLorentz > 1);
    QString prefBase = "f1.f1.";

		if ( usingDeltaFunc )
    {
      QString key = prefBase;
			if (usingCompositeFunc)
			{
				key += "f0.";
			}
			
			key += "Height";

      m_cfDblMng->setValue(m_cfProp["DeltaHeight"], parameters[key]);
      funcIndex++;
    }

    if ( noLorentz > 0 )
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

      m_cfDblMng->setValue(m_cfProp["Lorentzian 1.Amplitude"], parameters[pref+"Amplitude"]);
      m_cfDblMng->setValue(m_cfProp["Lorentzian 1.PeakCentre"], parameters[pref+"PeakCentre"]);
      m_cfDblMng->setValue(m_cfProp["Lorentzian 1.FWHM"], parameters[pref+"FWHM"]);
      funcIndex++;
    }

    if ( noLorentz > 1 )
    {
      // Two Lorentz
			QString pref = prefBase;

			if ( usingCompositeFunc )
			{
				pref += "f" + QString::number(funcIndex) + ".f" + QString::number(subIndex) + ".";
			}
			else
			{
				pref += "f" + QString::number(subIndex) + ".";
			}

      m_cfDblMng->setValue(m_cfProp["Lorentzian 2.Amplitude"], parameters[pref+"Amplitude"]);
      m_cfDblMng->setValue(m_cfProp["Lorentzian 2.PeakCentre"], parameters[pref+"PeakCentre"]);
      m_cfDblMng->setValue(m_cfProp["Lorentzian 2.FWHM"], parameters[pref+"FWHM"]);
    }

    // Plot Output
    if ( uiForm().confit_ckPlotOutput->isChecked() )
    {
      QString pyInput =
        "plotSpectrum('" + QString::fromStdString(output) + "_Workspace', [0,1,2])\n";
      QString pyOutput = runPythonCode(pyInput);
    }
  }

  void ConvFit::minChanged(double val)
  {
    m_cfDblMng->setValue(m_cfProp["StartX"], val);
  }

  void ConvFit::maxChanged(double val)
  {
    m_cfDblMng->setValue(m_cfProp["EndX"], val);
  }

  void ConvFit::hwhmChanged(double val)
  {
    const double peakCentre = m_cfDblMng->value(m_cfProp["Lorentzian 1.PeakCentre"]);
    // Always want FWHM to display as positive.
    const double hwhm = std::fabs(val-peakCentre);
    // Update the property
    m_cfHwhmRange->blockSignals(true);
    m_cfDblMng->setValue(m_cfProp["Lorentzian 1.FWHM"], hwhm*2);
    m_cfHwhmRange->blockSignals(false);
  }

  void ConvFit::backgLevel(double val)
  {
    m_cfDblMng->setValue(m_cfProp["BGA0"], val);
  }

  void ConvFit::updateRS(QtProperty* prop, double val)
  {
    if ( prop == m_cfProp["StartX"] ) { m_cfRangeS->setMinimum(val); }
    else if ( prop == m_cfProp["EndX"] ) { m_cfRangeS->setMaximum(val); }
    else if ( prop == m_cfProp["BGA0"] ) { m_cfBackgS->setMinimum(val); }
    else if ( prop == m_cfProp["Lorentzian 1.FWHM"] ) { hwhmUpdateRS(val); }
    else if ( prop == m_cfProp["Lorentzian 1.PeakCentre"] )
    {
        hwhmUpdateRS(m_cfDblMng->value(m_cfProp["Lorentzian 1.FWHM"]));
    }
  }

  void ConvFit::hwhmUpdateRS(double val)
  {
    const double peakCentre = m_cfDblMng->value(m_cfProp["Lorentzian 1.PeakCentre"]);
    m_cfHwhmRange->setMinimum(peakCentre-val/2);
    m_cfHwhmRange->setMaximum(peakCentre+val/2);
  }

  void ConvFit::checkBoxUpdate(QtProperty* prop, bool checked)
  {
    // Add/remove some properties to display only relevant options
    if ( prop == m_cfProp["UseDeltaFunc"] )
    {
      if ( checked ) 
      { 
        m_cfProp["DeltaFunction"]->addSubProperty(m_cfProp["DeltaHeight"]);
        uiForm().confit_cbPlotOutput->addItem("Height");
        uiForm().confit_cbPlotOutput->addItem("EISF");
      }
      else 
      { 
        m_cfProp["DeltaFunction"]->removeSubProperty(m_cfProp["DeltaHeight"]);
        uiForm().confit_cbPlotOutput->removeItem(uiForm().confit_cbPlotOutput->count()-1);
        uiForm().confit_cbPlotOutput->removeItem(uiForm().confit_cbPlotOutput->count()-1);
      }
    }
  }

  void ConvFit::fitContextMenu(const QPoint &)
  {
    QtBrowserItem* item(NULL);

    item = m_cfTree->currentItem();

    if ( ! item )
      return;

    // is it a fit property ?
    QtProperty* prop = item->property();

    if ( prop == m_cfProp["StartX"] || prop == m_cfProp["EndX"] )
      return;

    // is it already fixed?
    bool fixed = prop->propertyManager() != m_cfDblMng;

    if ( fixed && prop->propertyManager() != m_stringManager ) 
      return;

    // Create the menu
    QMenu* menu = new QMenu("ConvFit", m_cfTree);
    QAction* action;

    if ( ! fixed )
    {
      action = new QAction("Fix", this);
      connect(action, SIGNAL(triggered()), this, SLOT(fixItem()));
    }
    else
    {
      action = new QAction("Remove Fix", this);
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
      uiForm().confit_ckTieCentres->setVisible( fitType == "Two Lorentzians" );
  }

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
