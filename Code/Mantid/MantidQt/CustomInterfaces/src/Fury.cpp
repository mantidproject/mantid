#include "MantidQtCustomInterfaces/Fury.h"

#include "MantidQtCustomInterfaces/UserInputValidator.h"
#include "MantidQtMantidWidgets/RangeSelector.h"

#include <QFileInfo>

#include <qwt_plot.h>
#include <boost/lexical_cast.hpp>

#include <cmath>

namespace MantidQt
{
namespace CustomInterfaces
{
namespace IDA
{
  Fury::Fury(QWidget * parent) : IDATab(parent),
    m_furPlot(NULL), m_furRange(NULL), m_furCurve(NULL), m_furTree(NULL), 
    m_furProp(), m_furDblMng(NULL), m_furyResFileType()
  {}

  void Fury::setup()
  {
    m_furTree = new QtTreePropertyBrowser();
    uiForm().fury_TreeSpace->addWidget(m_furTree);

    m_furDblMng = new QtDoublePropertyManager();

    m_furPlot = new QwtPlot(this);
    uiForm().fury_PlotSpace->addWidget(m_furPlot);
    m_furPlot->setCanvasBackground(Qt::white);
    m_furPlot->setAxisFont(QwtPlot::xBottom, this->font());
    m_furPlot->setAxisFont(QwtPlot::yLeft, this->font());

    m_furProp["ELow"] = m_furDblMng->addProperty("ELow");
    m_furDblMng->setDecimals(m_furProp["ELow"], NUM_DECIMALS);
    m_furProp["EWidth"] = m_furDblMng->addProperty("EWidth");
    m_furDblMng->setDecimals(m_furProp["EWidth"], NUM_DECIMALS);
    m_furProp["EHigh"] = m_furDblMng->addProperty("EHigh");
    m_furDblMng->setDecimals(m_furProp["EHigh"], NUM_DECIMALS);

    m_furTree->addProperty(m_furProp["ELow"]);
    m_furTree->addProperty(m_furProp["EWidth"]);
    m_furTree->addProperty(m_furProp["EHigh"]);

    m_furTree->setFactoryForManager(m_furDblMng, doubleEditorFactory());

    m_furRange = new MantidQt::MantidWidgets::RangeSelector(m_furPlot);
    m_furRange->setInfoOnly(true);

    // signals / slots & validators
    connect(m_furRange, SIGNAL(minValueChanged(double)), this, SLOT(minChanged(double)));
    connect(m_furRange, SIGNAL(maxValueChanged(double)), this, SLOT(maxChanged(double)));
    connect(m_furDblMng, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(updateRS(QtProperty*, double)));
    connect(m_furDblMng, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(checkValidBinWidth(QtProperty*, double)));
    connect(uiForm().fury_dsInput, SIGNAL(dataReady(const QString&)), this, SLOT(plotInput(const QString&)));
  }

  void Fury::run()
  {
    QString pyInput =
      "from IndirectDataAnalysis import fury\n";

    QString wsName = uiForm().fury_dsInput->getCurrentDataName();
    QString resName = uiForm().fury_dsResInput->getCurrentDataName();

    if(uiForm().fury_dsResInput->isFileSelectorVisible())
    {
      runLoadNexus(uiForm().fury_dsResInput->getFullFilePath(), resName);
    }

    pyInput += "samples = [r'" + wsName + "']\n"
      "resolution = r'" + resName + "'\n"
      "rebin = '" + m_furProp["ELow"]->valueText() +","+ m_furProp["EWidth"]->valueText() +","+m_furProp["EHigh"]->valueText()+"'\n";

    if ( uiForm().fury_ckVerbose->isChecked() ) pyInput += "verbose = True\n";
    else pyInput += "verbose = False\n";

    if ( uiForm().fury_ckPlot->isChecked() ) pyInput += "plot = True\n";
    else pyInput += "plot = False\n";

    if ( uiForm().fury_ckSave->isChecked() ) pyInput += "save = True\n";
    else pyInput += "save = False\n";

    pyInput +=
      "fury_ws = fury(samples, resolution, rebin, Save=save, Verbose=verbose, Plot=plot)\n";
    QString pyOutput = runPythonCode(pyInput).trimmed();
  }

  /**
   * Ensure we have present and valid file/ws inputs.  The underlying Fourier transform of Fury
   * also means we must enforce several rules on the parameters.
   */
  QString Fury::validate()
  {
    UserInputValidator uiv;

    double eLow   = m_furDblMng->value(m_furProp["ELow"]);
    double eWidth = m_furDblMng->value(m_furProp["EWidth"]);
    double eHigh  = m_furDblMng->value(m_furProp["EHigh"]);

    uiv.checkBins(eLow, eWidth, eHigh);
    uiv.checkDataSelectorIsValid("Sample", uiForm().fury_dsInput);
    uiv.checkDataSelectorIsValid("Resolution", uiForm().fury_dsResInput);

    QString message = uiv.generateErrorMessage();

    return message;
  }

  /**
   * Runs validation when a new value has been entered for the bin width.
   *
   * @param prop QtProperty changed in the property tree
   * @param val new value of the property
   */
  void Fury::checkValidBinWidth(QtProperty *prop, double val)
  {
    double eLow   = m_furDblMng->value(m_furProp["ELow"]);
    double eWidth = m_furDblMng->value(m_furProp["EWidth"]);
    double eHigh  = m_furDblMng->value(m_furProp["EHigh"]);

    UserInputValidator uiv;
    uiv.checkBins(eLow, eWidth, eHigh);
    QString message = uiv.generateErrorMessage();

    if(prop == m_furProp["EWidth"])
    {
      if(message != "")
      {
        emit showInformationBox(message);
      }
    }
    else if(prop == m_furProp["ELow"] || prop == m_furProp["EHigh"])
    {
      if((eWidth != 0.0) && (message != ""))
      {
        double newWidth = (eHigh - eLow) / 10;
        m_furDblMng->setValue(m_furProp["EWidth"], newWidth);
      }
    }
  }

  void Fury::loadSettings(const QSettings & settings)
  {
    uiForm().fury_dsInput->readSettings(settings.group());
    uiForm().fury_dsResInput->readSettings(settings.group());
  }

  void Fury::plotInput(const QString& wsname)
  {
    using Mantid::API::MatrixWorkspace;
    using Mantid::API::MatrixWorkspace_const_sptr;

    MatrixWorkspace_const_sptr workspace;
    try
    {
      workspace = Mantid::API::AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>(wsname.toStdString());
    }
    catch(Mantid::Kernel::Exception::NotFoundError&)
    {
      showInformationBox(QString("Unable to retrieve workspace: " + wsname));
      return;
    }

    m_furCurve = plotMiniplot(m_furPlot, m_furCurve, workspace, 0);
    try
    {
      const std::pair<double, double> range = getCurveRange(m_furCurve);    
      double rounded_min = floor(range.first*10+0.5)/10.0;
      double rounded_max = floor(range.second*10+0.5)/10.0;

      //corrections for if nearest value is outside of range
      if (rounded_max > range.second)
      {
        rounded_max -= 0.1;
      }

      if(rounded_min < range.first)
      {
        rounded_min += 0.1;
      }

      //check incase we have a really small range
      if (fabs(rounded_min) > 0 && fabs(rounded_max) > 0)
      {
        m_furRange->setRange(rounded_min, rounded_max);
        m_furDblMng->setValue(m_furProp["ELow"], rounded_min);
        m_furDblMng->setValue(m_furProp["EHigh"], rounded_max);
      }
      else
      {
        m_furRange->setRange(range.first, range.second);
        m_furDblMng->setValue(m_furProp["ELow"], range.first);
        m_furDblMng->setValue(m_furProp["EHigh"], range.second);
      }
      //set default value for width
      m_furDblMng->setValue(m_furProp["EWidth"], 0.005);
      m_furPlot->replot();
    }
    catch(std::invalid_argument & exc)
    {
      showInformationBox(exc.what());
    }
  }

  void Fury::maxChanged(double val)
  {
    m_furDblMng->setValue(m_furProp["EHigh"], val);
  }

  void Fury::minChanged(double val)
  {
    m_furDblMng->setValue(m_furProp["ELow"], val);
  }

  void Fury::updateRS(QtProperty* prop, double val)
  {
    if ( prop == m_furProp["ELow"] )
      m_furRange->setMinimum(val);
    else if ( prop == m_furProp["EHigh"] )
      m_furRange->setMaximum(val);
  }
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
