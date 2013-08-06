#include "MantidQtCustomInterfaces/Elwin.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"

#include "MantidQtMantidWidgets/RangeSelector.h"

#include <QFileInfo>

#include <qwt_plot.h>

namespace MantidQt
{
namespace CustomInterfaces
{
namespace IDA
{
  Elwin::Elwin(QWidget * parent) : IDATab(parent), m_elwPlot(NULL), m_elwR1(NULL), 
    m_elwR2(NULL), m_elwDataCurve(NULL), m_elwTree(NULL), m_elwProp(), 
    m_elwDblMng(NULL), m_elwBlnMng(NULL), m_elwGrpMng(NULL)
  {}
  
  void Elwin::setup()
  {
    // Create QtTreePropertyBrowser object
    m_elwTree = new QtTreePropertyBrowser();
    uiForm().elwin_properties->addWidget(m_elwTree);

    // Create Manager Objects
    m_elwDblMng = new QtDoublePropertyManager();
    m_elwBlnMng = new QtBoolPropertyManager();
    m_elwGrpMng = new QtGroupPropertyManager();

    // Editor Factories
    m_elwTree->setFactoryForManager(m_elwDblMng, doubleEditorFactory());
    m_elwTree->setFactoryForManager(m_elwBlnMng, qtCheckBoxFactory());

    // Create Properties
    m_elwProp["R1S"] = m_elwDblMng->addProperty("Start");
    m_elwDblMng->setDecimals(m_elwProp["R1S"], NUM_DECIMALS);
    m_elwProp["R1E"] = m_elwDblMng->addProperty("End");
    m_elwDblMng->setDecimals(m_elwProp["R1E"], NUM_DECIMALS);  
    m_elwProp["R2S"] = m_elwDblMng->addProperty("Start");
    m_elwDblMng->setDecimals(m_elwProp["R2S"], NUM_DECIMALS);
    m_elwProp["R2E"] = m_elwDblMng->addProperty("End");
    m_elwDblMng->setDecimals(m_elwProp["R2E"], NUM_DECIMALS);

    m_elwProp["UseTwoRanges"] = m_elwBlnMng->addProperty("Use Two Ranges");

    m_elwProp["Range1"] = m_elwGrpMng->addProperty("Range One");
    m_elwProp["Range1"]->addSubProperty(m_elwProp["R1S"]);
    m_elwProp["Range1"]->addSubProperty(m_elwProp["R1E"]);
    m_elwProp["Range2"] = m_elwGrpMng->addProperty("Range Two");
    m_elwProp["Range2"]->addSubProperty(m_elwProp["R2S"]);
    m_elwProp["Range2"]->addSubProperty(m_elwProp["R2E"]);

    m_elwTree->addProperty(m_elwProp["Range1"]);
    m_elwTree->addProperty(m_elwProp["UseTwoRanges"]);
    m_elwTree->addProperty(m_elwProp["Range2"]);

    // Create Slice Plot Widget for Range Selection
    m_elwPlot = new QwtPlot(this);
    m_elwPlot->setAxisFont(QwtPlot::xBottom, this->font());
    m_elwPlot->setAxisFont(QwtPlot::yLeft, this->font());
    uiForm().elwin_plot->addWidget(m_elwPlot);
    m_elwPlot->setCanvasBackground(Qt::white);
    // We always want one range selector... the second one can be controlled from
    // within the elwinTwoRanges(bool state) function
    m_elwR1 = new MantidWidgets::RangeSelector(m_elwPlot);
    connect(m_elwR1, SIGNAL(minValueChanged(double)), this, SLOT(minChanged(double)));
    connect(m_elwR1, SIGNAL(maxValueChanged(double)), this, SLOT(maxChanged(double)));
    // create the second range
    m_elwR2 = new MantidWidgets::RangeSelector(m_elwPlot);
    m_elwR2->setColour(Qt::darkGreen); // dark green for background
    connect(m_elwR1, SIGNAL(rangeChanged(double, double)), m_elwR2, SLOT(setRange(double, double)));
    connect(m_elwR2, SIGNAL(minValueChanged(double)), this, SLOT(minChanged(double)));
    connect(m_elwR2, SIGNAL(maxValueChanged(double)), this, SLOT(maxChanged(double)));
    m_elwR2->setRange(m_elwR1->getRange());
    // Refresh the plot window
    m_elwPlot->replot();
  
    connect(m_elwDblMng, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(updateRS(QtProperty*, double)));
    connect(m_elwBlnMng, SIGNAL(valueChanged(QtProperty*, bool)), this, SLOT(twoRanges(QtProperty*, bool)));
    twoRanges(0, false);

    // m_uiForm element signals and slots
    connect(uiForm().elwin_pbPlotInput, SIGNAL(clicked()), this, SLOT(plotInput()));

    // Set any default values
    m_elwDblMng->setValue(m_elwProp["R1S"], -0.02);
    m_elwDblMng->setValue(m_elwProp["R1E"], 0.02);
  }

  void Elwin::run()
  {
    QString pyInput =
      "from IndirectDataAnalysis import elwin\n"
      "input = [r'" + uiForm().elwin_inputFile->getFilenames().join("', r'") + "']\n"
      "eRange = [ " + QString::number(m_elwDblMng->value(m_elwProp["R1S"])) +","+ QString::number(m_elwDblMng->value(m_elwProp["R1E"]));

    if ( m_elwBlnMng->value(m_elwProp["UseTwoRanges"]) )
    {
      pyInput += ", " + QString::number(m_elwDblMng->value(m_elwProp["R2S"])) + ", " + QString::number(m_elwDblMng->value(m_elwProp["R2E"]));
    }

    pyInput+= "]\n";

    pyInput+= "logType = '"+ uiForm().leLogName->text() +"'\n";
    
    if ( uiForm().elwin_ckNormalise->isChecked() ) pyInput += "normalise = True\n";
    else pyInput += "normalise = False\n";

    if ( uiForm().elwin_ckSave->isChecked() ) pyInput += "save = True\n";
    else pyInput += "save = False\n";

    if ( uiForm().elwin_ckVerbose->isChecked() ) pyInput += "verbose = True\n";
    else pyInput += "verbose = False\n";

    if ( uiForm().elwin_ckPlot->isChecked() ) pyInput += "plot = True\n";
    else pyInput += "plot = False\n";


    pyInput +=
      "eq1_ws, eq2_ws = elwin(input, eRange, log_type=logType, Normalise=normalise, Save=save, Verbose=verbose, Plot=plot)\n";

    QString pyOutput = runPythonCode(pyInput).trimmed();

  }

  QString Elwin::validate()
  {
    UserInputValidator uiv;
    
    uiv.checkMWRunFilesIsValid("Input", uiForm().elwin_inputFile);

    auto rangeOne = std::make_pair(m_elwDblMng->value(m_elwProp["R1S"]), m_elwDblMng->value(m_elwProp["R1E"]));
    uiv.checkValidRange("Range One", rangeOne);

    bool useTwoRanges = m_elwBlnMng->value(m_elwProp["UseTwoRanges"]);
    if( useTwoRanges )
    {
      auto rangeTwo = std::make_pair(m_elwDblMng->value(m_elwProp["R2S"]), m_elwDblMng->value(m_elwProp["R2E"]));
      uiv.checkValidRange("Range Two", rangeTwo);
      uiv.checkRangesDontOverlap(rangeOne, rangeTwo);
    }

    return uiv.generateErrorMessage();
  }

  void Elwin::loadSettings(const QSettings & settings)
  {
    uiForm().elwin_inputFile->readSettings(settings.group());
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
        m_elwDblMng->setValue(m_elwProp["R1S"], -res);
        m_elwDblMng->setValue(m_elwProp["R1E"], res);
      }

    }
  }

  void Elwin::plotInput()
  {
    if ( uiForm().elwin_inputFile->isValid() )
    {
      QString filename = uiForm().elwin_inputFile->getFirstFilename();
      QFileInfo fi(filename);
      QString wsname = fi.baseName();

      auto ws = runLoadNexus(filename, wsname);

      if(!ws)
      {
        showInformationBox(QString("Unable to load file: ") + filename);
        return;
      }

      setDefaultResolution(ws);

      m_elwDataCurve = plotMiniplot(m_elwPlot, m_elwDataCurve, ws, 0);
      try
      {
        const std::pair<double, double> range = getCurveRange(m_elwDataCurve);
        m_elwR1->setRange(range.first, range.second);
        // Replot
        m_elwPlot->replot();
      }
      catch(std::invalid_argument & exc)
      {
        showInformationBox(exc.what());
      }

    }
    else
    {
      showInformationBox("Selected input files are invalid.");
    }
  }

  void Elwin::twoRanges(QtProperty*, bool val)
  {
    m_elwR2->setVisible(val);
  }

  void Elwin::minChanged(double val)
  {
    MantidWidgets::RangeSelector* from = qobject_cast<MantidWidgets::RangeSelector*>(sender());
    if ( from == m_elwR1 )
    {
      m_elwDblMng->setValue(m_elwProp["R1S"], val);
    }
    else if ( from == m_elwR2 )
    {
      m_elwDblMng->setValue(m_elwProp["R2S"], val);
    }
  }

  void Elwin::maxChanged(double val)
  {
    MantidWidgets::RangeSelector* from = qobject_cast<MantidWidgets::RangeSelector*>(sender());
    if ( from == m_elwR1 )
    {
      m_elwDblMng->setValue(m_elwProp["R1E"], val);
    }
    else if ( from == m_elwR2 )
    {
      m_elwDblMng->setValue(m_elwProp["R2E"], val);
    }
  }

  void Elwin::updateRS(QtProperty* prop, double val)
  {
    if ( prop == m_elwProp["R1S"] ) m_elwR1->setMinimum(val);
    else if ( prop == m_elwProp["R1E"] ) m_elwR1->setMaximum(val);
    else if ( prop == m_elwProp["R2S"] ) m_elwR2->setMinimum(val);
    else if ( prop == m_elwProp["R2E"] ) m_elwR2->setMaximum(val);
  }
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
