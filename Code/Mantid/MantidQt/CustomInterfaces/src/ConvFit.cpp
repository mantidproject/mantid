#include "MantidQtCustomInterfaces/ConvFit.h"

#include "MantidQtCustomInterfaces/UserInputValidator.h"
#include "MantidQtMantidWidgets/RangeSelector.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionDomain1D.h"

#include <QFileInfo>
#include <QMenu>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>

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

    // Connections
    connect(m_cfRangeS, SIGNAL(minValueChanged(double)), this, SLOT(minChanged(double)));
    connect(m_cfRangeS, SIGNAL(maxValueChanged(double)), this, SLOT(maxChanged(double)));
    connect(m_cfBackgS, SIGNAL(minValueChanged(double)), this, SLOT(backgLevel(double)));
    connect(m_cfHwhmRange, SIGNAL(minValueChanged(double)), this, SLOT(hwhmChanged(double)));
    connect(m_cfHwhmRange, SIGNAL(maxValueChanged(double)), this, SLOT(hwhmChanged(double)));
    connect(m_cfDblMng, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(updateRS(QtProperty*, double)));
    connect(m_cfBlnMng, SIGNAL(valueChanged(QtProperty*, bool)), this, SLOT(checkBoxUpdate(QtProperty*, bool)));

    connect(m_cfDblMng, SIGNAL(propertyChanged(QtProperty*)), this, SLOT(plotGuess(QtProperty*)));

    // Have HWHM Range linked to Fit Start/End Range
    connect(m_cfRangeS, SIGNAL(rangeChanged(double, double)), m_cfHwhmRange, SLOT(setRange(double, double)));
    m_cfHwhmRange->setRange(-1.0,1.0);
    hwhmUpdateRS(0.02);

    typeSelection(uiForm().confit_cbFitType->currentIndex());
    bgTypeSelection(uiForm().confit_cbBackground->currentIndex());

    // Replot input automatically when file / spec no changes
    connect(uiForm().confit_leSpecNo, SIGNAL(editingFinished()), this, SLOT(plotInput()));
    connect(uiForm().confit_inputFile, SIGNAL(fileEditingFinished()), this, SLOT(plotInput()));
    connect(uiForm().confit_cbInputType, SIGNAL(currentIndexChanged(int)), uiForm().confit_swInput, SLOT(setCurrentIndex(int)));
    connect(uiForm().confit_cbResType, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(resType(const QString&)));
    connect(uiForm().confit_cbFitType, SIGNAL(currentIndexChanged(int)), this, SLOT(typeSelection(int)));
    connect(uiForm().confit_cbBackground, SIGNAL(currentIndexChanged(int)), this, SLOT(bgTypeSelection(int)));
    connect(uiForm().confit_pbSequential, SIGNAL(clicked()), this, SLOT(sequential()));

    //signals for plotting input
    connect(uiForm().confit_pbPlotInput, SIGNAL(clicked()), this, SLOT(plotInput()));
    connect(uiForm().confit_cbInputType, SIGNAL(currentIndexChanged(int)), this, SLOT(plotInput()));
    connect(uiForm().confit_inputFile, SIGNAL(filesFound()), this, SLOT(plotInput()));
    connect(uiForm().confit_wsSample, SIGNAL(currentIndexChanged(int)), this, SLOT(plotInput()));

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

    int funcIndex = 1;
    QString prefBase = "f1.f";
    if ( noLorentz > 1 || ( noLorentz > 0 && m_cfBlnMng->value(m_cfProp["UseDeltaFunc"]) ) )
    {
      prefBase += "1.f";
      funcIndex--;
    }

    if ( m_cfBlnMng->value(m_cfProp["UseDeltaFunc"]) )
    {
      QString key = prefBase+QString::number(funcIndex)+".Height";
      m_cfDblMng->setValue(m_cfProp["DeltaHeight"], parameters[key]);
      funcIndex++;
    }

    if ( noLorentz > 0 )
    {
      // One Lorentz
      QString pref = prefBase + QString::number(funcIndex) + ".";
      m_cfDblMng->setValue(m_cfProp["Lorentzian 1.Amplitude"], parameters[pref+"Amplitude"]);
      m_cfDblMng->setValue(m_cfProp["Lorentzian 1.PeakCentre"], parameters[pref+"PeakCentre"]);
      m_cfDblMng->setValue(m_cfProp["Lorentzian 1.HWHM"], parameters[pref+"HWHM"]);
      funcIndex++;
    }

    if ( noLorentz > 1 )
    {
      // Two Lorentz
      QString pref = prefBase + QString::number(funcIndex) + ".";
      m_cfDblMng->setValue(m_cfProp["Lorentzian 2.Amplitude"], parameters[pref+"Amplitude"]);
      m_cfDblMng->setValue(m_cfProp["Lorentzian 2.PeakCentre"], parameters[pref+"PeakCentre"]);
      m_cfDblMng->setValue(m_cfProp["Lorentzian 2.HWHM"], parameters[pref+"HWHM"]);
    }

    // Plot Output
    if ( uiForm().confit_ckPlotOutput->isChecked() )
    {
      QString pyInput =
        "plotSpectrum('" + QString::fromStdString(output) + "_Workspace', [0,1,2])\n";
      QString pyOutput = runPythonCode(pyInput);
    }

  }

  /**
   * Validates the user's inputs in the ConvFit tab.
   *
   * @returns an string containing an error message if invalid input detected, else an empty string.
   */
  QString ConvFit::validate()
  {
    UserInputValidator uiv;

    switch( uiForm().confit_cbInputType->currentIndex() )
    {
    case 0:
      uiv.checkMWRunFilesIsValid("Reduction", uiForm().confit_inputFile); break;
    case 1:
      uiv.checkWorkspaceSelectorIsNotEmpty("Reduction", uiForm().confit_wsSample); break;
    }

    uiv.checkMWRunFilesIsValid("Resolution", uiForm().confit_resInput);

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
    uiForm().confit_inputFile->readSettings(settings.group());
    uiForm().confit_resInput->readSettings(settings.group());
  }

  void ConvFit::resType(const QString& type)
  {
    QStringList exts;
    if ( type == "RES File" )
    {
      exts.append("_res.nxs");
      m_confitResFileType = true;
    }
    else
    {
      exts.append("_red.nxs");
      m_confitResFileType = false;
    }
    uiForm().confit_resInput->setFileExtensions(exts);
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
   *      +-- Model (AT LEAST one of the following. Composite if more than one.)
   *          |
   *          +-- DeltaFunction (yes/no)
   *          +-- Lorentzian 1 (yes/no)
   *          +-- Lorentzian 2 (yes/no)
   *
   * @param tie :: whether to tie centres of the two lorentzians.
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
    index = comp->addFunction(func); 

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
    index = conv->addFunction(func);
    std::string resfilename = uiForm().confit_resInput->getFirstFilename().toStdString();
    Mantid::API::IFunction::Attribute attr(resfilename);
    func->setAttribute("FileName", attr);

    // --------------------------------------------------------
    // --- Composite / Convolution / Model / Delta Function ---
    // --------------------------------------------------------
    size_t subIndex = 0;

    if ( m_cfBlnMng->value(m_cfProp["UseDeltaFunc"]) )
    {
      func = Mantid::API::FunctionFactory::Instance().createFunction("DeltaFunction");
      index = conv->addFunction(func);

      if ( /*tie  ||*/ ! m_cfProp["DeltaHeight"]->subProperties().isEmpty() )
      {
        std::string parName = createParName(index, "Height");
        conv->tie(parName, m_cfProp["DeltaHeight"]->valueText().toStdString() );
      }

      else { func->setParameter("Height", m_cfProp["DeltaHeight"]->valueText().toDouble()); }
      subIndex++;
    }
  
    // -----------------------------------------------------
    // --- Composite / Convolution / Model / Lorentzians ---
    // -----------------------------------------------------
    std::string prefix1;
    std::string prefix2;
    switch ( uiForm().confit_cbFitType->currentIndex() )
    {
    case 0: // No Lorentzians

      break;

    case 1: // 1 Lorentzian

      func = Mantid::API::FunctionFactory::Instance().createFunction("Lorentzian");
      index = conv->addFunction(func);

      // If it's the first "sub" function of model, then it wont be nested inside Convolution ...
      if( subIndex == 0 ) { prefix1 = createParName(index); }
      // ... else it's part of a composite function inside Convolution.
      else { prefix1 = createParName(index, subIndex); }

      populateFunction(func, conv, m_cfProp["Lorentzian1"], prefix1, false);
      subIndex++;
      break;

    case 2: // 2 Lorentzians

      func = Mantid::API::FunctionFactory::Instance().createFunction("Lorentzian");
      index = conv->addFunction(func);

      // If it's the first "sub" function of model, then it wont be nested inside Convolution ...
      if( subIndex == 0 ) { prefix1 = createParName(index); }
      // ... else it's part of a composite function inside Convolution.
      else { prefix1 = createParName(index, subIndex); }

      populateFunction(func, conv, m_cfProp["Lorentzian1"], prefix1, false);
      subIndex++;

      func = Mantid::API::FunctionFactory::Instance().createFunction("Lorentzian");
      index = conv->addFunction(func);

      prefix2 = createParName(index, subIndex); // (Part of a composite.)
      populateFunction(func, conv, m_cfProp["Lorentzian2"], prefix2, false);

      // Now prefix1 should be changed to reflect the fact that it is now part of a composite function inside Convolution.
      prefix1 = createParName(index, subIndex-1);

      // Tie PeakCentres together
      if ( tieCentres )
      {
        QString tieL = QString::fromStdString(prefix1 + "PeakCentre");
        QString tieR = QString::fromStdString(prefix2 + "PeakCentre");
        conv->tie(tieL.toStdString(), tieR.toStdString());
      }
      break;
    }

    comp->addFunction(conv);

    comp->applyTies();

    return comp;
  }

  QtProperty* ConvFit::createLorentzian(const QString & name)
  {
    QtProperty* lorentzGroup = m_cfGrpMng->addProperty(name);
    m_cfProp[name+".Amplitude"] = m_cfDblMng->addProperty("Amplitude");
    // m_cfDblMng->setRange(m_cfProp[name+".Amplitude"], 0.0, 1.0); // 0 < Amplitude < 1
    m_cfProp[name+".PeakCentre"] = m_cfDblMng->addProperty("PeakCentre");
    m_cfProp[name+".HWHM"] = m_cfDblMng->addProperty("HWHM");
    m_cfDblMng->setDecimals(m_cfProp[name+".Amplitude"], NUM_DECIMALS);
    m_cfDblMng->setDecimals(m_cfProp[name+".PeakCentre"], NUM_DECIMALS);
    m_cfDblMng->setDecimals(m_cfProp[name+".HWHM"], NUM_DECIMALS);
    m_cfDblMng->setValue(m_cfProp[name+".HWHM"], 0.02);
    lorentzGroup->addSubProperty(m_cfProp[name+".Amplitude"]);
    lorentzGroup->addSubProperty(m_cfProp[name+".PeakCentre"]);
    lorentzGroup->addSubProperty(m_cfProp[name+".HWHM"]);
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
        comp->tie(name, value );
      }
      else
      {
        std::string propName = props[i]->propertyName().toStdString();
        double propValue = props[i]->valueText().toDouble();
        func->setParameter(propName, propValue);
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
    default:
      assert( false ); // Should never happen.
    }

    // We should never get to a stage where the user is allowed to
    // continue having not selected at least one fit - be it 
    // Lorentzian, delta, or both.
    assert( ! fitType.isEmpty() );

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
      assert( false ); // Should never happen.
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

    // Find wsname and set m_cfInputWS to point to that workspace.
    switch ( uiForm().confit_cbInputType->currentIndex() )
    {
    case 0: // "File"
      {
        if(uiForm().confit_inputFile->isEmpty())
        {
          return;
        }
        if ( ! uiForm().confit_inputFile->isValid() )
        {
          return;
        }
        else
        {
          QString filename = uiForm().confit_inputFile->getFirstFilename();
          QFileInfo fi(filename);
          QString wsname = fi.baseName();

          // Load the file if it has not already been loaded.
          if ( (m_cfInputWS == NULL) || ( wsname != m_cfInputWSName ))
          {
            m_cfInputWSName = wsname;
            m_cfInputWS = runLoadNexus(filename, wsname);
            if(!m_cfInputWS)
            {
              return;
            }
          }
        }
      }
      break;
    case 1: // Workspace
      {
        m_cfInputWSName = uiForm().confit_wsSample->currentText();
        if(m_cfInputWSName.isEmpty())
        {
         return;
        }
        try
        {
          m_cfInputWS = AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>(m_cfInputWSName.toStdString());
        }
        catch ( NotFoundError & )
        {
          QString msg = "Workspace: '" + m_cfInputWSName + "' could not be "
            "found in the Analysis Data Service.";
          showInformationBox(msg);
          return;
        }
      }
      break;
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

    Mantid::API::CompositeFunction_sptr function = createFunction(true);

    if ( m_cfInputWS == NULL )
    {
      plotInput();
    }

    // std::string inputName = m_cfInputWS->getName();  // Unused

    const size_t binIndexLow = m_cfInputWS->binIndexOf(m_cfDblMng->value(m_cfProp["StartX"]));
    const size_t binIndexHigh = m_cfInputWS->binIndexOf(m_cfDblMng->value(m_cfProp["EndX"]));
    const size_t nData = binIndexHigh - binIndexLow;

    std::vector<double> inputXData(nData);
    //double* outputData = new double[nData];

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

  void ConvFit::sequential()
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

    Mantid::API::CompositeFunction_sptr func = createFunction(uiForm().confit_ckTieCentres->isChecked());
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
      "save = ";
  
    pyInput += uiForm().confit_ckSaveSeq->isChecked() ? "True\n" : "False\n";

    if ( uiForm().confit_ckVerbose->isChecked() ) pyInput += "verbose = True\n";
    else pyInput += "verbose = False\n";
  
    pyInput +=    
      "bg = '" + bg + "'\n"
      "ftype = '" + ftype + "'\n"
      "confitSeq(input, func, startx, endx, save, plot, ftype, bg, specMin, specMax, Verbose=verbose)\n";

    QString pyOutput = runPythonCode(pyInput);
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
    // Always want HWHM to display as positive.
    if ( val > peakCentre )
    {
      m_cfDblMng->setValue(m_cfProp["Lorentzian 1.HWHM"], val-peakCentre);
    }
    else
    {
      m_cfDblMng->setValue(m_cfProp["Lorentzian 1.HWHM"], peakCentre-val);
    }
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
    else if ( prop == m_cfProp["Lorentzian 1.HWHM"] ) { hwhmUpdateRS(val); }
  }

  void ConvFit::hwhmUpdateRS(double val)
  {
    const double peakCentre = m_cfDblMng->value(m_cfProp["Lorentzian 1.PeakCentre"]);
    m_cfHwhmRange->setMinimum(peakCentre-val);
    m_cfHwhmRange->setMaximum(peakCentre+val);
  }

  void ConvFit::checkBoxUpdate(QtProperty* prop, bool checked)
  {
    // Add/remove some properties to display only relevant options
    if ( prop == m_cfProp["UseDeltaFunc"] )
    {
      if ( checked ) { m_cfProp["DeltaFunction"]->addSubProperty(m_cfProp["DeltaHeight"]); }
      else { m_cfProp["DeltaFunction"]->removeSubProperty(m_cfProp["DeltaHeight"]); }
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
    QMenu* menu = new QMenu("FuryFit", m_cfTree);
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
