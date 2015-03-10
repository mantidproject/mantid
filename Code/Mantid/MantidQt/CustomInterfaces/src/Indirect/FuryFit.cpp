#include "MantidQtCustomInterfaces/Indirect/FuryFit.h"

#include "MantidQtCustomInterfaces/UserInputValidator.h"
#include "MantidQtMantidWidgets/RangeSelector.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionDomain1D.h"

#include <math.h>
#include <QFileInfo>
#include <QMenu>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>

using namespace Mantid::API;

namespace
{
  Mantid::Kernel::Logger g_log("FuryFit");
}

namespace MantidQt
{
namespace CustomInterfaces
{
namespace IDA
{
  FuryFit::FuryFit(QWidget * parent) :
    IDATab(parent),
    m_stringManager(NULL), m_ffTree(NULL),
    m_ffRangeManager(NULL),
    m_fixedProps(),
    m_ffInputWS(), m_ffOutputWS(),
    m_ffInputWSName(),
    m_ties()
  {
    m_uiForm.setupUi(parent);
  }

  void FuryFit::setup()
  {
    m_stringManager = new QtStringPropertyManager(m_parentWidget);

    m_ffTree = new QtTreePropertyBrowser(m_parentWidget);
    m_uiForm.properties->addWidget(m_ffTree);

    m_rangeSelectors["FuryFitRange"] = new MantidQt::MantidWidgets::RangeSelector(m_uiForm.ppPlot);
    connect(m_rangeSelectors["FuryFitRange"], SIGNAL(minValueChanged(double)), this, SLOT(xMinSelected(double)));
    connect(m_rangeSelectors["FuryFitRange"], SIGNAL(maxValueChanged(double)), this, SLOT(xMaxSelected(double)));

    m_rangeSelectors["FuryFitBackground"] = new MantidQt::MantidWidgets::RangeSelector(m_uiForm.ppPlot,
      MantidQt::MantidWidgets::RangeSelector::YSINGLE);
    m_rangeSelectors["FuryFitBackground"]->setRange(0.0,1.0);
    m_rangeSelectors["FuryFitBackground"]->setColour(Qt::darkGreen);
    connect(m_rangeSelectors["FuryFitBackground"], SIGNAL(minValueChanged(double)), this, SLOT(backgroundSelected(double)));

    // setupTreePropertyBrowser
    m_ffRangeManager = new QtDoublePropertyManager(m_parentWidget);

    m_ffTree->setFactoryForManager(m_dblManager, m_dblEdFac);
    m_ffTree->setFactoryForManager(m_ffRangeManager, m_dblEdFac);

    m_properties["StartX"] = m_ffRangeManager->addProperty("StartX");
    m_ffRangeManager->setDecimals(m_properties["StartX"], NUM_DECIMALS);
    m_properties["EndX"] = m_ffRangeManager->addProperty("EndX");
    m_ffRangeManager->setDecimals(m_properties["EndX"], NUM_DECIMALS);

    connect(m_ffRangeManager, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(propertyChanged(QtProperty*, double)));
    connect(m_dblManager, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(propertyChanged(QtProperty*, double)));

    m_properties["LinearBackground"] = m_grpManager->addProperty("LinearBackground");
    m_properties["BackgroundA0"] = m_ffRangeManager->addProperty("A0");
    m_ffRangeManager->setDecimals(m_properties["BackgroundA0"], NUM_DECIMALS);
    m_properties["LinearBackground"]->addSubProperty(m_properties["BackgroundA0"]);

    m_properties["Exponential1"] = createExponential("Exponential1");
    m_properties["Exponential2"] = createExponential("Exponential2");

    m_properties["StretchedExp"] = createStretchedExp("StretchedExp");

    m_ffRangeManager->setMinimum(m_properties["BackgroundA0"], 0);
    m_ffRangeManager->setMaximum(m_properties["BackgroundA0"], 1);

    m_dblManager->setMinimum(m_properties["Exponential1.Intensity"], 0);
    m_dblManager->setMaximum(m_properties["Exponential1.Intensity"], 1);

    m_dblManager->setMinimum(m_properties["Exponential2.Intensity"], 0);
    m_dblManager->setMaximum(m_properties["Exponential2.Intensity"], 1);

    m_dblManager->setMinimum(m_properties["StretchedExp.Intensity"], 0);
    m_dblManager->setMaximum(m_properties["StretchedExp.Intensity"], 1);

    typeSelection(m_uiForm.cbFitType->currentIndex());

    // Update guess curve on property change
    connect(m_dblManager, SIGNAL(propertyChanged(QtProperty*)), this, SLOT(plotGuess(QtProperty*)));

    // Signal/slot ui connections
    connect(m_uiForm.dsSampleInput, SIGNAL(dataReady(const QString&)), this, SLOT(newDataLoaded(const QString&)));
    connect(m_uiForm.cbFitType, SIGNAL(currentIndexChanged(int)), this, SLOT(typeSelection(int)));
    connect(m_uiForm.pbSingle, SIGNAL(clicked()), this, SLOT(singleFit()));

    connect(m_uiForm.dsSampleInput, SIGNAL(filesFound()), this, SLOT(updatePlot()));

    connect(m_uiForm.spPlotSpectrum, SIGNAL(valueChanged(int)), this, SLOT(updatePlot()));

    connect(m_uiForm.spSpectraMin, SIGNAL(valueChanged(int)), this, SLOT(specMinChanged(int)));
    connect(m_uiForm.spSpectraMax, SIGNAL(valueChanged(int)), this, SLOT(specMaxChanged(int)));

    // Set a custom handler for the QTreePropertyBrowser's ContextMenu event
    m_ffTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_ffTree, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(fitContextMenu(const QPoint &)));
  }

  void FuryFit::run()
  {
    if ( m_ffInputWS == NULL )
    {
      return;
    }

    const bool constrainBeta = m_uiForm.ckConstrainBeta->isChecked();
    const bool constrainIntens = m_uiForm.ckConstrainIntensities->isChecked();
    CompositeFunction_sptr func = createFunction();
    func->tie("f0.A1", "0");

    if ( constrainIntens )
    {
      constrainIntensities(func);
    }

    func->applyTies();

    std::string function = std::string(func->asString());
    QString fitType = fitTypeString();
    QString specMin = m_uiForm.spSpectraMin->text();
    QString specMax = m_uiForm.spSpectraMax->text();

    QString pyInput = "from IndirectDataAnalysis import furyfitSeq, furyfitMult\n"
      "input = '" + m_ffInputWSName + "'\n"
      "func = r'" + QString::fromStdString(function) + "'\n"
      "ftype = '"   + fitTypeString() + "'\n"
      "startx = " + m_properties["StartX"]->valueText() + "\n"
      "endx = " + m_properties["EndX"]->valueText() + "\n"
      "plot = '" + m_uiForm.cbPlotType->currentText() + "'\n"
      "spec_min = " + specMin + "\n"
      "spec_max = " + specMax + "\n"
      "spec_max = None\n";

    if (constrainIntens) pyInput += "constrain_intens = True \n";
    else pyInput += "constrain_intens = False \n";

    if ( m_uiForm.ckSave->isChecked() ) pyInput += "save = True\n";
    else pyInput += "save = False\n";

    if( !constrainBeta )
    {
      pyInput += "furyfitSeq(input, func, ftype, startx, endx, spec_min=spec_min, spec_max=spec_max, intensities_constrained=constrain_intens, Save=save, Plot=plot)\n";
    }
    else
    {
      pyInput += "furyfitMult(input, func, ftype, startx, endx, spec_min=spec_min, spec_max=spec_max, intensities_constrained=constrain_intens, Save=save, Plot=plot)\n";
    }

    QString pyOutput = runPythonCode(pyInput);

    // Set the result workspace for Python script export
    QString inputWsName = QString::fromStdString(m_ffInputWS->getName());
    QString resultWsName = inputWsName.left(inputWsName.lastIndexOf("_")) + "_fury_" + fitType + specMin + "_to_" + specMax + "_Workspaces";
    m_pythonExportWsName = resultWsName.toStdString();

    updatePlot();
  }

  bool FuryFit::validate()
  {
    UserInputValidator uiv;

    uiv.checkDataSelectorIsValid("Sample", m_uiForm.dsSampleInput);

    auto range = std::make_pair(m_ffRangeManager->value(m_properties["StartX"]), m_ffRangeManager->value(m_properties["EndX"]));
    uiv.checkValidRange("Ranges", range);

    QString error = uiv.generateErrorMessage();
    showMessageBox(error);

    return error.isEmpty();
  }

  void FuryFit::loadSettings(const QSettings & settings)
  {
    m_uiForm.dsSampleInput->readSettings(settings.group());
  }

  /**
   * Called when new data has been loaded by the data selector.
   *
   * Configures ranges for spin boxes before raw plot is done.
   *
   * @param wsName Name of new workspace loaded
   */
  void FuryFit::newDataLoaded(const QString wsName)
  {
    m_ffInputWSName = wsName;
    m_ffInputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_ffInputWSName.toStdString());

    int maxSpecIndex = static_cast<int>(m_ffInputWS->getNumberHistograms()) - 1;

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

  CompositeFunction_sptr FuryFit::createFunction(bool tie)
  {
    CompositeFunction_sptr result( new CompositeFunction );
    QString fname;
    const int fitType = m_uiForm.cbFitType->currentIndex();

    IFunction_sptr func = FunctionFactory::Instance().createFunction("LinearBackground");
    func->setParameter("A0", m_ffRangeManager->value(m_properties["BackgroundA0"]));
    result->addFunction(func);
    result->tie("f0.A1", "0");
    if ( tie ) { result->tie("f0.A0", m_properties["BackgroundA0"]->valueText().toStdString()); }

    if ( fitType == 2 ) { fname = "StretchedExp"; }
    else { fname = "Exponential1"; }

    result->addFunction(createUserFunction(fname, tie));

    if ( fitType == 1 || fitType == 3 )
    {
      if ( fitType == 1 ) { fname = "Exponential2"; }
      else { fname = "StretchedExp"; }
      result->addFunction(createUserFunction(fname, tie));
    }

    // Return CompositeFunction object to caller.
    result->applyTies();
    return result;
  }

  IFunction_sptr FuryFit::createUserFunction(const QString & name, bool tie)
  {
    IFunction_sptr result = FunctionFactory::Instance().createFunction("UserFunction");
    std::string formula;

    if ( name.startsWith("Exp") ) { formula = "Intensity*exp(-(x/Tau))"; }
    else { formula = "Intensity*exp(-(x/Tau)^Beta)"; }

    IFunction::Attribute att(formula);
    result->setAttribute("Formula", att);

    QList<QtProperty*> props = m_properties[name]->subProperties();
    for ( int i = 0; i < props.size(); i++ )
    {
      std::string name = props[i]->propertyName().toStdString();
      result->setParameter(name, m_dblManager->value(props[i]));

      //add tie if parameter is fixed
      if ( tie || ! props[i]->subProperties().isEmpty() )
      {
        std::string value = props[i]->valueText().toStdString();
        result->tie(name, value);
      }
    }

    result->applyTies();
    return result;
  }

  QtProperty* FuryFit::createExponential(const QString & name)
  {
    QtProperty* expGroup = m_grpManager->addProperty(name);
    m_properties[name+".Intensity"] = m_dblManager->addProperty("Intensity");
    m_dblManager->setDecimals(m_properties[name+".Intensity"], NUM_DECIMALS);
    m_properties[name+".Tau"] = m_dblManager->addProperty("Tau");
    m_dblManager->setDecimals(m_properties[name+".Tau"], NUM_DECIMALS);
    expGroup->addSubProperty(m_properties[name+".Intensity"]);
    expGroup->addSubProperty(m_properties[name+".Tau"]);
    return expGroup;
  }

  QtProperty* FuryFit::createStretchedExp(const QString & name)
  {
    QtProperty* prop = m_grpManager->addProperty(name);
    m_properties[name+".Intensity"] = m_dblManager->addProperty("Intensity");
    m_properties[name+".Tau"] = m_dblManager->addProperty("Tau");
    m_properties[name+".Beta"] = m_dblManager->addProperty("Beta");
    m_dblManager->setRange(m_properties[name+".Beta"], 0, 1);
    m_dblManager->setDecimals(m_properties[name+".Intensity"], NUM_DECIMALS);
    m_dblManager->setDecimals(m_properties[name+".Tau"], NUM_DECIMALS);
    m_dblManager->setDecimals(m_properties[name+".Beta"], NUM_DECIMALS);
    prop->addSubProperty(m_properties[name+".Intensity"]);
    prop->addSubProperty(m_properties[name+".Tau"]);
    prop->addSubProperty(m_properties[name+".Beta"]);
    return prop;
  }

  QString FuryFit::fitTypeString() const
  {
    switch ( m_uiForm.cbFitType->currentIndex() )
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

    m_ffTree->addProperty(m_properties["StartX"]);
    m_ffTree->addProperty(m_properties["EndX"]);
    m_ffTree->addProperty(m_properties["LinearBackground"]);

    //option should only be available with a single stretched exponential
    m_uiForm.ckConstrainBeta->setEnabled((index == 2));
    if (!m_uiForm.ckConstrainBeta->isEnabled())
    {
      m_uiForm.ckConstrainBeta->setChecked(false);
    }

    switch ( index )
    {
    case 0:
      m_ffTree->addProperty(m_properties["Exponential1"]);

      //remove option to plot beta
      m_uiForm.cbPlotType->removeItem(4);
      break;
    case 1:
      m_ffTree->addProperty(m_properties["Exponential1"]);
      m_ffTree->addProperty(m_properties["Exponential2"]);

      //remove option to plot beta
      m_uiForm.cbPlotType->removeItem(4);
      break;
    case 2:
      m_ffTree->addProperty(m_properties["StretchedExp"]);

      //add option to plot beta
      if(m_uiForm.cbPlotType->count() == 4)
      {
        m_uiForm.cbPlotType->addItem("Beta");
      }

      break;
    case 3:
      m_ffTree->addProperty(m_properties["Exponential1"]);
      m_ffTree->addProperty(m_properties["StretchedExp"]);

      //add option to plot beta
      if(m_uiForm.cbPlotType->count() == 4)
      {
        m_uiForm.cbPlotType->addItem("Beta");
      }

      break;
    }

    plotGuess(NULL);
  }

  void FuryFit::updatePlot()
  {
    if(!m_ffInputWS)
    {
      g_log.error("No workspace loaded, cannot create preview plot.");
      return;
    }

    int specNo = m_uiForm.spPlotSpectrum->value();

    m_uiForm.ppPlot->clear();
    m_uiForm.ppPlot->addSpectrum("Sample", m_ffInputWS, specNo);

    try
    {
      const QPair<double, double> curveRange = m_uiForm.ppPlot->getCurveRange("Sample");
      const std::pair<double, double> range(curveRange.first, curveRange.second);
      m_rangeSelectors["FuryFitRange"]->setRange(range.first, range.second);
      m_ffRangeManager->setRange(m_properties["StartX"], range.first, range.second);
      m_ffRangeManager->setRange(m_properties["EndX"], range.first, range.second);

      setDefaultParameters("Exponential1");
      setDefaultParameters("Exponential2");
      setDefaultParameters("StretchedExp");

      m_uiForm.ppPlot->resizeX();
      m_uiForm.ppPlot->setAxisRange(qMakePair(0.0, 1.0), QwtPlot::yLeft);
    }
    catch(std::invalid_argument & exc)
    {
      showMessageBox(exc.what());
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

  void FuryFit::setDefaultParameters(const QString& name)
  {
    double background = m_dblManager->value(m_properties["BackgroundA0"]);
    //intensity is always 1-background
    m_dblManager->setValue(m_properties[name+".Intensity"], 1.0-background);
    auto x = m_ffInputWS->readX(0);
    auto y = m_ffInputWS->readY(0);
    double tau = 0;

    if (x.size() > 4)
    {
      tau = -x[4] / log(y[4]);
    }

    m_dblManager->setValue(m_properties[name+".Tau"], tau);
    m_dblManager->setValue(m_properties[name+".Beta"], 1.0);
  }

  /**
   * Handles the user entering a new minimum spectrum index.
   *
   * Prevents the user entering an overlapping spectra range.
   *
   * @param value Minimum spectrum index
   */
  void FuryFit::specMinChanged(int value)
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
  void FuryFit::specMaxChanged(int value)
  {
    m_uiForm.spSpectraMin->setMaximum(value);
  }

  void FuryFit::xMinSelected(double val)
  {
    m_ffRangeManager->setValue(m_properties["StartX"], val);
  }

  void FuryFit::xMaxSelected(double val)
  {
    m_ffRangeManager->setValue(m_properties["EndX"], val);
  }

  void FuryFit::backgroundSelected(double val)
  {
    m_ffRangeManager->setValue(m_properties["BackgroundA0"], val);
    m_dblManager->setValue(m_properties["Exponential1.Intensity"], 1.0-val);
    m_dblManager->setValue(m_properties["Exponential2.Intensity"], 1.0-val);
    m_dblManager->setValue(m_properties["StretchedExp.Intensity"], 1.0-val);
  }

  void FuryFit::propertyChanged(QtProperty* prop, double val)
  {
    if ( prop == m_properties["StartX"] )
    {
      m_rangeSelectors["FuryFitRange"]->setMinimum(val);
    }
    else if ( prop == m_properties["EndX"] )
    {
      m_rangeSelectors["FuryFitRange"]->setMaximum(val);
    }
    else if ( prop == m_properties["BackgroundA0"])
    {
      m_rangeSelectors["FuryFitBackground"]->setMinimum(val);
      m_dblManager->setValue(m_properties["Exponential1.Intensity"], 1.0-val);
      m_dblManager->setValue(m_properties["Exponential2.Intensity"], 1.0-val);
      m_dblManager->setValue(m_properties["StretchedExp.Intensity"], 1.0-val);
    }
    else if( prop == m_properties["Exponential1.Intensity"]
      || prop == m_properties["Exponential2.Intensity"]
      || prop == m_properties["StretchedExp.Intensity"])
    {
      m_rangeSelectors["FuryFitBackground"]->setMinimum(1.0-val);
      m_dblManager->setValue(m_properties["Exponential1.Intensity"], val);
      m_dblManager->setValue(m_properties["Exponential2.Intensity"], val);
      m_dblManager->setValue(m_properties["StretchedExp.Intensity"], val);
    }
  }

  void FuryFit::constrainIntensities(CompositeFunction_sptr func)
  {
    std::string paramName = "f1.Intensity";
    size_t index = func->parameterIndex(paramName);

    switch ( m_uiForm.cbFitType->currentIndex() )
    {
    case 0: // 1 Exp
    case 2: // 1 Str
      if(!func->isFixed(index))
      {
        func->tie(paramName, "1-f0.A0");
      }
      else
      {
        std::string paramValue = boost::lexical_cast<std::string>(func->getParameter(paramName));
        func->tie(paramName, paramValue);
        func->tie("f0.A0", "1-"+paramName);
      }
      break;
    case 1: // 2 Exp
    case 3: // 1 Exp & 1 Str
      if(!func->isFixed(index))
      {
        func->tie(paramName,"1-f2.Intensity-f0.A0");
      }
      else
      {
        std::string paramValue = boost::lexical_cast<std::string>(func->getParameter(paramName));
        func->tie(paramName,"1-f2.Intensity-f0.A0");
        func->tie(paramName, paramValue);
      }
      break;
    }
  }

  void FuryFit::singleFit()
  {
    if(!validate())
      return;

    // Don't plot a new guess curve until there is a fit
    disconnect(m_dblManager, SIGNAL(propertyChanged(QtProperty*)), this, SLOT(plotGuess(QtProperty*)));

    // First create the function
    auto function = createFunction();

    const int fitType = m_uiForm.cbFitType->currentIndex();
    if ( m_uiForm.ckConstrainIntensities->isChecked() )
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

    updatePlot();
    if ( m_ffInputWS == NULL )
    {
      return;
    }

    QString pyInput = "from IndirectCommon import getWSprefix\nprint getWSprefix('%1')\n";
    pyInput = pyInput.arg(m_ffInputWSName);
    QString outputNm = runPythonCode(pyInput).trimmed();
    outputNm += QString("fury_") + ftype + m_uiForm.spPlotSpectrum->text();
    std::string output = outputNm.toStdString();

    // Create the Fit Algorithm
    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("Fit");
    alg->initialize();
    alg->setPropertyValue("Function", function->asString());
    alg->setPropertyValue("InputWorkspace", m_ffInputWSName.toStdString());
    alg->setProperty("WorkspaceIndex", m_uiForm.spPlotSpectrum->text().toInt());
    alg->setProperty("StartX", m_ffRangeManager->value(m_properties["StartX"]));
    alg->setProperty("EndX", m_ffRangeManager->value(m_properties["EndX"]));
    alg->setProperty("Ties", m_ties.toStdString());
    alg->setPropertyValue("Output", output);
    alg->execute();

    if ( ! alg->isExecuted() )
    {
      QString msg = "There was an error executing the fitting algorithm. Please see the "
        "Results Log pane for more details.";
      showMessageBox(msg);
      return;
    }

    IFunction_sptr outputFunc = alg->getProperty("Function");

    // Get params.
    QMap<QString,double> parameters;
    std::vector<std::string> parNames = outputFunc->getParameterNames();
    std::vector<double> parVals;

    for( size_t i = 0; i < parNames.size(); ++i )
      parVals.push_back(outputFunc->getParameter(parNames[i]));

    for ( size_t i = 0; i < parNames.size(); ++i )
      parameters[QString(parNames[i].c_str())] = parVals[i];

    m_ffRangeManager->setValue(m_properties["BackgroundA0"], parameters["f0.A0"]);

    if ( fitType != 2 )
    {
      // Exp 1
      m_dblManager->setValue(m_properties["Exponential1.Intensity"], parameters["f1.Intensity"]);
      m_dblManager->setValue(m_properties["Exponential1.Tau"], parameters["f1.Tau"]);

      if ( fitType == 1 )
      {
        // Exp 2
        m_dblManager->setValue(m_properties["Exponential2.Intensity"], parameters["f2.Intensity"]);
        m_dblManager->setValue(m_properties["Exponential2.Tau"], parameters["f2.Tau"]);
      }
    }

    if ( fitType > 1 )
    {
      // Str
      QString fval;
      if ( fitType == 2 ) { fval = "f1."; }
      else { fval = "f2."; }

      m_dblManager->setValue(m_properties["StretchedExp.Intensity"], parameters[fval+"Intensity"]);
      m_dblManager->setValue(m_properties["StretchedExp.Tau"], parameters[fval+"Tau"]);
      m_dblManager->setValue(m_properties["StretchedExp.Beta"], parameters[fval+"Beta"]);
    }

    // Can start upddating the guess curve again
    connect(m_dblManager, SIGNAL(propertyChanged(QtProperty*)), this, SLOT(plotGuess(QtProperty*)));

    // Plot the guess first so that it is under the fit
    plotGuess(NULL);
    // Now show the fitted curve of the mini plot
    m_uiForm.ppPlot->addSpectrum("Fit", outputNm+"_Workspace", 1, Qt::red);

    m_pythonExportWsName = "";
  }

  void FuryFit::plotGuess(QtProperty*)
  {
    // Do nothing if there is no sample data curve
    if(!m_uiForm.ppPlot->hasCurve("Sample"))
      return;

    CompositeFunction_sptr function = createFunction(true);

    // Create the double* array from the input workspace
    const size_t binIndxLow = m_ffInputWS->binIndexOf(m_ffRangeManager->value(m_properties["StartX"]));
    const size_t binIndxHigh = m_ffInputWS->binIndexOf(m_ffRangeManager->value(m_properties["EndX"]));
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

    FunctionDomain1DVector domain(inputXData);
    FunctionValues outputData(domain);
    function->function(domain, outputData);

    QVector<double> dataX;
    QVector<double> dataY;

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

  void FuryFit::fitContextMenu(const QPoint &)
  {
    QtBrowserItem* item(NULL);

    item = m_ffTree->currentItem();

    if ( ! item )
      return;

    // is it a fit property ?
    QtProperty* prop = item->property();

    // is it already fixed?
    bool fixed = prop->propertyManager() != m_dblManager;

    if ( fixed && prop->propertyManager() != m_stringManager )
      return;

    // Create the menu
    QMenu* menu = new QMenu("FuryFit", m_ffTree);
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
