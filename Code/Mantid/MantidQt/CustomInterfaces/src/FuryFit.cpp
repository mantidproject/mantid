#include "MantidQtCustomInterfaces/FuryFit.h"

#include "MantidQtCustomInterfaces/UserInputValidator.h"
#include "MantidQtMantidWidgets/RangeSelector.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/CompositeFunction.h"
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
  FuryFit::FuryFit(QWidget * parent) : 
    IDATab(parent), m_intVal(NULL), m_stringManager(NULL), m_ffTree(NULL), m_groupManager(NULL), m_ffDblMng(NULL),
    m_ffRangeManager(NULL), m_ffProp(), m_fixedProps(), m_ffPlot(NULL), m_ffDataCurve(NULL), m_ffFitCurve(NULL),
    m_ffRangeS(NULL), m_ffBackRangeS(NULL), m_ffInputWS(), m_ffOutputWS(), m_ffInputWSName(), m_ties()
  {}
      
  void FuryFit::setup()
  {
    m_intVal = new QIntValidator(this);
    
    m_stringManager = new QtStringPropertyManager(this);

    m_ffTree = new QtTreePropertyBrowser(this);
    uiForm().furyfit_properties->addWidget(m_ffTree);
  
    // Setup FuryFit Plot Window
    m_ffPlot = new QwtPlot(this);
    m_ffPlot->setAxisFont(QwtPlot::xBottom, this->font());
    m_ffPlot->setAxisFont(QwtPlot::yLeft, this->font());
    uiForm().furyfit_vlPlot->addWidget(m_ffPlot);
    m_ffPlot->setCanvasBackground(QColor(255,255,255));
  
    m_ffRangeS = new MantidQt::MantidWidgets::RangeSelector(m_ffPlot);
    connect(m_ffRangeS, SIGNAL(minValueChanged(double)), this, SLOT(xMinSelected(double)));
    connect(m_ffRangeS, SIGNAL(maxValueChanged(double)), this, SLOT(xMaxSelected(double)));

    m_ffBackRangeS = new MantidQt::MantidWidgets::RangeSelector(m_ffPlot,
      MantidQt::MantidWidgets::RangeSelector::YSINGLE);
    m_ffBackRangeS->setRange(0.0,1.0);
    m_ffBackRangeS->setColour(Qt::darkGreen);
    connect(m_ffBackRangeS, SIGNAL(minValueChanged(double)), this, SLOT(backgroundSelected(double)));

    // setupTreePropertyBrowser
    m_groupManager = new QtGroupPropertyManager(this);
    m_ffDblMng = new QtDoublePropertyManager(this);
    m_ffRangeManager = new QtDoublePropertyManager(this);
  
    m_ffTree->setFactoryForManager(m_ffDblMng, doubleEditorFactory());
    m_ffTree->setFactoryForManager(m_ffRangeManager, doubleEditorFactory());

    m_ffProp["StartX"] = m_ffRangeManager->addProperty("StartX");
    m_ffRangeManager->setDecimals(m_ffProp["StartX"], NUM_DECIMALS);
    m_ffProp["EndX"] = m_ffRangeManager->addProperty("EndX");
    m_ffRangeManager->setDecimals(m_ffProp["EndX"], NUM_DECIMALS);

    connect(m_ffRangeManager, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(rangePropChanged(QtProperty*, double)));

    m_ffProp["LinearBackground"] = m_groupManager->addProperty("LinearBackground");
    m_ffProp["BackgroundA0"] = m_ffRangeManager->addProperty("A0");
    m_ffRangeManager->setDecimals(m_ffProp["BackgroundA0"], NUM_DECIMALS);
    m_ffProp["LinearBackground"]->addSubProperty(m_ffProp["BackgroundA0"]);

    m_ffProp["Exponential1"] = createExponential("Exponential 1");
    m_ffProp["Exponential2"] = createExponential("Exponential 2");
  
    m_ffProp["StretchedExp"] = createStretchedExp("Stretched Exponential");

    typeSelection(uiForm().furyfit_cbFitType->currentIndex());


    // Connect to PlotGuess checkbox
    connect(m_ffDblMng, SIGNAL(propertyChanged(QtProperty*)), this, SLOT(plotGuess(QtProperty*)));

    // Signal/slot ui connections
    connect(uiForm().furyfit_inputFile, SIGNAL(fileEditingFinished()), this, SLOT(plotInput()));
    connect(uiForm().furyfit_cbFitType, SIGNAL(currentIndexChanged(int)), this, SLOT(typeSelection(int)));
    connect(uiForm().furyfit_pbPlotInput, SIGNAL(clicked()), this, SLOT(plotInput()));
    connect(uiForm().furyfit_leSpecNo, SIGNAL(editingFinished()), this, SLOT(plotInput()));
    connect(uiForm().furyfit_cbInputType, SIGNAL(currentIndexChanged(int)), uiForm().furyfit_swInput, SLOT(setCurrentIndex(int)));  
    connect(uiForm().furyfit_pbSeqFit, SIGNAL(clicked()), this, SLOT(sequential()));
    // apply validators - furyfit
    uiForm().furyfit_leSpecNo->setValidator(m_intVal);

    // Set a custom handler for the QTreePropertyBrowser's ContextMenu event
    m_ffTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_ffTree, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(fitContextMenu(const QPoint &)));
  }

  void FuryFit::run()
  {
    // First create the function
    auto function = createFunction();

    uiForm().furyfit_ckPlotGuess->setChecked(false);
    
    const int fitType = uiForm().furyfit_cbFitType->currentIndex();

    if ( uiForm().furyfit_ckConstrainIntensities->isChecked() )
    {
      switch ( fitType )
      {
      case 0: // 1 Exp
      case 2: // 1 Str
        m_ties = "f1.Intensity = 1-f0.A0";
        break;
      case 1: // 2 Exp
      case 3: // 1 Exp & 1 Str
        m_ties = "f1.Intensity=1-f2.Intensity-f0.A0";
        break;
      default:
        break;
      }
    }
    QString ftype = fitTypeString();

    plotInput();
    if ( m_ffInputWS == NULL )
    {
      return;
    }

    QString pyInput = "from IndirectCommon import getWSprefix\nprint getWSprefix('%1')\n";
    pyInput = pyInput.arg(m_ffInputWSName);
    QString outputNm = runPythonCode(pyInput).trimmed();
    outputNm += QString("fury_") + ftype + uiForm().furyfit_leSpecNo->text();
    std::string output = outputNm.toStdString();

    // Create the Fit Algorithm
    Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("Fit");
    alg->initialize();
    alg->setPropertyValue("Function", function->asString());
    alg->setPropertyValue("InputWorkspace", m_ffInputWSName.toStdString());
    alg->setProperty("WorkspaceIndex", uiForm().furyfit_leSpecNo->text().toInt());
    alg->setProperty("StartX", m_ffRangeManager->value(m_ffProp["StartX"]));
    alg->setProperty("EndX", m_ffRangeManager->value(m_ffProp["EndX"]));
    alg->setProperty("Ties", m_ties.toStdString());
    alg->setPropertyValue("Output", output);
    alg->execute();

    if ( ! alg->isExecuted() )
    {
      QString msg = "There was an error executing the fitting algorithm. Please see the "
        "Results Log pane for more details.";
      showInformationBox(msg);
      return;
    }

    // Now show the fitted curve of the mini plot
    m_ffFitCurve = plotMiniplot(m_ffPlot, m_ffFitCurve, outputNm+"_Workspace", 1);
    QPen fitPen(Qt::red, Qt::SolidLine);
    m_ffFitCurve->setPen(fitPen);
    m_ffPlot->replot();

    Mantid::API::IFunction_sptr outputFunc = alg->getProperty("Function");

    // Get params.
    QMap<QString,double> parameters;
    std::vector<std::string> parNames = outputFunc->getParameterNames();
    std::vector<double> parVals;

    for( size_t i = 0; i < parNames.size(); ++i )
      parVals.push_back(outputFunc->getParameter(parNames[i]));

    for ( size_t i = 0; i < parNames.size(); ++i )
      parameters[QString(parNames[i].c_str())] = parVals[i];

    m_ffRangeManager->setValue(m_ffProp["BackgroundA0"], parameters["f0.A0"]);
  
    if ( fitType != 2 )
    {
      // Exp 1
      m_ffDblMng->setValue(m_ffProp["Exponential 1.Intensity"], parameters["f1.Intensity"]);
      m_ffDblMng->setValue(m_ffProp["Exponential 1.Tau"], parameters["f1.Tau"]);
    
      if ( fitType == 1 )
      {
        // Exp 2
        m_ffDblMng->setValue(m_ffProp["Exponential 2.Intensity"], parameters["f2.Intensity"]);
        m_ffDblMng->setValue(m_ffProp["Exponential 2.Tau"], parameters["f2.Tau"]);
      }
    }
  
    if ( fitType > 1 )
    {
      // Str
      QString fval;
      if ( fitType == 2 ) { fval = "f1."; }
      else { fval = "f2."; }
    
      m_ffDblMng->setValue(m_ffProp["Stretched Exponential.Intensity"], parameters[fval+"Intensity"]);
      m_ffDblMng->setValue(m_ffProp["Stretched Exponential.Tau"], parameters[fval+"Tau"]);
      m_ffDblMng->setValue(m_ffProp["Stretched Exponential.Beta"], parameters[fval+"Beta"]);
    }

    if ( uiForm().furyfit_ckPlotOutput->isChecked() )
    {
      QString pyInput = "from mantidplot import *\n"
        "plotSpectrum('" + QString::fromStdString(output) + "_Workspace', [0,1,2])\n";
      QString pyOutput = runPythonCode(pyInput);
    }
  }

  QString FuryFit::validate()
  {
    UserInputValidator uiv;

    switch( uiForm().furyfit_cbInputType->currentIndex() )
    {
    case 0:
      uiv.checkMWRunFilesIsValid("Input", uiForm().furyfit_inputFile); break;
    case 1:
      uiv.checkWorkspaceSelectorIsNotEmpty("Input", uiForm().furyfit_wsIqt); break;
    }

    auto range = std::make_pair(m_ffRangeManager->value(m_ffProp["StartX"]), m_ffRangeManager->value(m_ffProp["EndX"]));
    uiv.checkValidRange("Ranges", range);

    return uiv.generateErrorMessage();
  }

  void FuryFit::loadSettings(const QSettings & settings)
  {
    uiForm().furyfit_inputFile->readSettings(settings.group());
  }

  Mantid::API::CompositeFunction_sptr FuryFit::createFunction(bool tie)
  {
    Mantid::API::CompositeFunction_sptr result( new Mantid::API::CompositeFunction );
    QString fname;
    const int fitType = uiForm().furyfit_cbFitType->currentIndex();

    Mantid::API::IFunction_sptr func = Mantid::API::FunctionFactory::Instance().createFunction("LinearBackground");
    func->setParameter("A0", m_ffRangeManager->value(m_ffProp["BackgroundA0"]));
    result->addFunction(func);
    result->tie("f0.A1", "0");
    if ( tie ) { result->tie("f0.A0", m_ffProp["BackgroundA0"]->valueText().toStdString()); }
  
    if ( fitType == 2 ) { fname = "Stretched Exponential"; }
    else { fname = "Exponential 1"; }

    result->addFunction(createUserFunction(fname, tie));

    if ( fitType == 1 || fitType == 3 )
    {
      if ( fitType == 1 ) { fname = "Exponential 2"; }
      else { fname = "Stretched Exponential"; }
      result->addFunction(createUserFunction(fname, tie));
    }

    // Return CompositeFunction object to caller.
    result->applyTies();
    return result;
  }

  Mantid::API::IFunction_sptr FuryFit::createUserFunction(const QString & name, bool tie)
  {
    Mantid::API::IFunction_sptr result = Mantid::API::FunctionFactory::Instance().createFunction("UserFunction");  
    std::string formula;

    if ( name.startsWith("Exp") ) { formula = "Intensity*exp(-(x/Tau))"; }
    else { formula = "Intensity*exp(-(x/Tau)^Beta)"; }

    Mantid::API::IFunction::Attribute att(formula);  
    result->setAttribute("Formula", att);

    result->setParameter("Intensity", m_ffDblMng->value(m_ffProp[name+".Intensity"]));

    if ( tie || ! m_ffProp[name+".Intensity"]->subProperties().isEmpty() )
    {
      result->tie("Intensity", m_ffProp[name+".Intensity"]->valueText().toStdString());
    }
    result->setParameter("Tau", m_ffDblMng->value(m_ffProp[name+".Tau"]));
    if ( tie || ! m_ffProp[name+".Tau"]->subProperties().isEmpty() )
    {
      result->tie("Tau", m_ffProp[name+".Tau"]->valueText().toStdString());
    }
    if ( name.startsWith("Str") )
    {
      result->setParameter("Beta", m_ffDblMng->value(m_ffProp[name+".Beta"]));
      if ( tie || ! m_ffProp[name+".Beta"]->subProperties().isEmpty() )
      {
        result->tie("Beta", m_ffProp[name+".Beta"]->valueText().toStdString());
      }
    }

    return result;
  }

  QtProperty* FuryFit::createExponential(const QString & name)
  {
    QtProperty* expGroup = m_groupManager->addProperty(name);
    m_ffProp[name+".Intensity"] = m_ffDblMng->addProperty("Intensity");
    m_ffDblMng->setDecimals(m_ffProp[name+".Intensity"], NUM_DECIMALS);
    m_ffProp[name+".Tau"] = m_ffDblMng->addProperty("Tau");
    m_ffDblMng->setDecimals(m_ffProp[name+".Tau"], NUM_DECIMALS);
    expGroup->addSubProperty(m_ffProp[name+".Intensity"]);
    expGroup->addSubProperty(m_ffProp[name+".Tau"]);
    return expGroup;
  }

  QtProperty* FuryFit::createStretchedExp(const QString & name)
  {
    QtProperty* prop = m_groupManager->addProperty(name);
    m_ffProp[name+".Intensity"] = m_ffDblMng->addProperty("Intensity");
    m_ffProp[name+".Tau"] = m_ffDblMng->addProperty("Tau");
    m_ffProp[name+".Beta"] = m_ffDblMng->addProperty("Beta");
    m_ffDblMng->setRange(m_ffProp[name+".Beta"], 0, 1);
    m_ffDblMng->setDecimals(m_ffProp[name+".Intensity"], NUM_DECIMALS);
    m_ffDblMng->setDecimals(m_ffProp[name+".Tau"], NUM_DECIMALS);
    m_ffDblMng->setDecimals(m_ffProp[name+".Beta"], NUM_DECIMALS);
    prop->addSubProperty(m_ffProp[name+".Intensity"]);
    prop->addSubProperty(m_ffProp[name+".Tau"]);
    prop->addSubProperty(m_ffProp[name+".Beta"]);
    return prop;
  }

  QString FuryFit::fitTypeString() const
  {
    switch ( uiForm().furyfit_cbFitType->currentIndex() )
    {
    case 0:
      return "1E_s";
    case 1:
      return "2E_s";
    case 2:
      return "1S_s";
    case 3:
      return "1E1S_s";
    default:
      return "s";
    };
  }

  void FuryFit::typeSelection(int index)
  {
    m_ffTree->clear();

    m_ffTree->addProperty(m_ffProp["StartX"]);
    m_ffTree->addProperty(m_ffProp["EndX"]);

    m_ffTree->addProperty(m_ffProp["LinearBackground"]);

    switch ( index )
    {
    case 0:
      m_ffTree->addProperty(m_ffProp["Exponential1"]);
      break;
    case 1:
      m_ffTree->addProperty(m_ffProp["Exponential1"]);
      m_ffTree->addProperty(m_ffProp["Exponential2"]);
      break;
    case 2:
      m_ffTree->addProperty(m_ffProp["StretchedExp"]);
      break;
    case 3:
      m_ffTree->addProperty(m_ffProp["Exponential1"]);
      m_ffTree->addProperty(m_ffProp["StretchedExp"]);
      break;
    }
  }

  void FuryFit::plotInput()
  {
    using namespace Mantid::API;
    switch ( uiForm().furyfit_cbInputType->currentIndex() )
    {
    case 0: // "File"
      {
        if ( ! uiForm().furyfit_inputFile->isValid() )
        {
          return;
        }
        else
        {
        QFileInfo fi(uiForm().furyfit_inputFile->getFirstFilename());
        QString wsname = fi.baseName();
        if ( (m_ffInputWS == NULL) || ( wsname != m_ffInputWSName ) )
        {
          m_ffInputWSName = wsname;
          QString filename = uiForm().furyfit_inputFile->getFirstFilename();
          // get the output workspace
          m_ffInputWS = runLoadNexus(filename, m_ffInputWSName);
        }
        }
      }
      break;
    case 1: // Workspace
      {
        m_ffInputWSName = uiForm().furyfit_wsIqt->currentText();
        try
        {
          m_ffInputWS = AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>(m_ffInputWSName.toStdString());
        }
        catch ( Mantid::Kernel::Exception::NotFoundError & )
        {
          QString msg = "Workspace: '" + m_ffInputWSName + "' could not be "
            "found in the Analysis Data Service.";
          showInformationBox(msg);
          return;
        }
      }
      break;
    }

    int specNo = uiForm().furyfit_leSpecNo->text().toInt();

    m_ffDataCurve = plotMiniplot(m_ffPlot, m_ffDataCurve, m_ffInputWS, specNo);
    try
    {
      const std::pair<double, double> range = getCurveRange(m_ffDataCurve);
      m_ffRangeS->setRange(range.first, range.second);
      m_ffRangeManager->setRange(m_ffProp["StartX"], range.first, range.second);
      m_ffRangeManager->setRange(m_ffProp["EndX"], range.first, range.second);
    
      m_ffPlot->setAxisScale(QwtPlot::xBottom, range.first, range.second);
      m_ffPlot->setAxisScale(QwtPlot::yLeft, 0.0, 1.0);
      m_ffPlot->replot();
    }
    catch(std::invalid_argument & exc)
    {
      showInformationBox(exc.what());
    }
  }

  void FuryFit::xMinSelected(double val)
  {
    m_ffRangeManager->setValue(m_ffProp["StartX"], val);
  }

  void FuryFit::xMaxSelected(double val)
  {
    m_ffRangeManager->setValue(m_ffProp["EndX"], val);
  }

  void FuryFit::backgroundSelected(double val)
  {
    m_ffRangeManager->setValue(m_ffProp["BackgroundA0"], val);
  }

  void FuryFit::rangePropChanged(QtProperty* prop, double val)
  {
    if ( prop == m_ffProp["StartX"] )
    {
      m_ffRangeS->setMinimum(val);
    }
    else if ( prop == m_ffProp["EndX"] )
    {
      m_ffRangeS->setMaximum(val);
    }
    else if ( prop == m_ffProp["BackgroundA0"] )
    {
      m_ffBackRangeS->setMinimum(val);
    }
  }

  void FuryFit::sequential()
  {
    plotInput();
    if ( m_ffInputWS == NULL )
    {
      return;
    }

    Mantid::API::CompositeFunction_sptr func = createFunction();

    // Function Ties
    func->tie("f0.A1", "0");
    if ( uiForm().furyfit_ckConstrainIntensities->isChecked() )
    {
      switch ( uiForm().furyfit_cbFitType->currentIndex() )
      {
      case 0: // 1 Exp
      case 2: // 1 Str
        func->tie("f1.Intensity","1-f0.A0");
        break;
      case 1: // 2 Exp
      case 3: // 1 Exp & 1 Str
        func->tie("f1.Intensity","1-f2.Intensity-f0.A0");
        break;
      }
    }

    std::string function = std::string(func->asString());
  
    QString pyInput = "from IndirectDataAnalysis import furyfitSeq\n"
      "input = '" + m_ffInputWSName + "'\n"
      "func = r'" + QString::fromStdString(function) + "'\n"
      "ftype = '" + fitTypeString() + "'\n"
      "startx = " + m_ffProp["StartX"]->valueText() + "\n"
      "endx = " + m_ffProp["EndX"]->valueText() + "\n"
      "plot = '" + uiForm().furyfit_cbPlotOutput->currentText() + "'\n"
      "save = ";

    if ( uiForm().furyfit_ckVerbose->isChecked() ) pyInput += "verbose = True\n";
    else pyInput += "verbose = False\n";

    if ( uiForm().furyfit_ckSaveSeq->isChecked() ) pyInput += "save = True\n";
    else pyInput += "save = False\n";

    pyInput += "furyfitSeq(input, func, ftype, startx, endx, save, plot, verbose)\n";
  
    QString pyOutput = runPythonCode(pyInput);
  }

  void FuryFit::plotGuess(QtProperty*)
  {
    if ( ! uiForm().furyfit_ckPlotGuess->isChecked() || m_ffDataCurve == NULL )
    {
      return;
    }

    Mantid::API::CompositeFunction_sptr function = createFunction(true);

    // Create the double* array from the input workspace
    const size_t binIndxLow = m_ffInputWS->binIndexOf(m_ffRangeManager->value(m_ffProp["StartX"]));
    const size_t binIndxHigh = m_ffInputWS->binIndexOf(m_ffRangeManager->value(m_ffProp["EndX"]));
    const size_t nData = binIndxHigh - binIndxLow;

    std::vector<double> inputXData(nData);

    const Mantid::MantidVec& XValues = m_ffInputWS->readX(0);

    const bool isHistogram = m_ffInputWS->isHistogramData();

    for ( size_t i = 0; i < nData ; i++ )
    {
      if ( isHistogram )
        inputXData[i] = 0.5*(XValues[binIndxLow+i]+XValues[binIndxLow+i+1]);
      else
        inputXData[i] = XValues[binIndxLow+i];
    }

    Mantid::API::FunctionDomain1DVector domain(inputXData);
    Mantid::API::FunctionValues outputData(domain);
    function->function(domain, outputData);

    QVector<double> dataX;
    QVector<double> dataY;

    for ( size_t i = 0; i < nData; i++ )
    {
      dataX.append(inputXData[i]);
      dataY.append(outputData.getCalculated(i));
    }

    // Create the curve
    if ( m_ffFitCurve != NULL )
    {
      m_ffFitCurve->attach(0);
      delete m_ffFitCurve;
      m_ffFitCurve = 0;
    }

    m_ffFitCurve = new QwtPlotCurve();
    m_ffFitCurve->setData(dataX, dataY);
    m_ffFitCurve->attach(m_ffPlot);
    QPen fitPen(Qt::red, Qt::SolidLine);
    m_ffFitCurve->setPen(fitPen);
    m_ffPlot->replot();
  }

  void FuryFit::fitContextMenu(const QPoint &)
  {
    QtBrowserItem* item(NULL);

    item = m_ffTree->currentItem();

    if ( ! item )
      return;

    // is it a fit property ?
    QtProperty* prop = item->property();

    // is it already fixed?
    bool fixed = prop->propertyManager() != m_ffDblMng;

    if ( fixed && prop->propertyManager() != m_stringManager ) 
      return;

    // Create the menu
    QMenu* menu = new QMenu("FuryFit", m_ffTree);
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

  void FuryFit::fixItem()
  {
    QtBrowserItem* item = m_ffTree->currentItem();

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

  void FuryFit::unFixItem()
  {
    QtBrowserItem* item = m_ffTree->currentItem();

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
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
