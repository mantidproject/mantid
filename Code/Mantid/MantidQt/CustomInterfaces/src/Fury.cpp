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
  
    connect(uiForm().fury_cbResType, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(resType(const QString&)));
    connect(uiForm().fury_dsInput, SIGNAL(dataReady(const QString&)), this, SLOT(plotInput(const QString&)));
  }

  void Fury::run()
  {
    QString filenames = uiForm().fury_dsInput->getCurrentDataName();

    QString pyInput =
      "from IndirectDataAnalysis import fury\n"
      "samples = [r'" + filenames + "']\n"
      "resolution = r'" + uiForm().fury_resFile->getFirstFilename() + "'\n"
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
    uiv.checkMWRunFilesIsValid("Resolution", uiForm().fury_resFile);

    double eLow   = m_furDblMng->value(m_furProp["ELow"]);
    double eWidth = m_furDblMng->value(m_furProp["EWidth"]);
    double eHigh  = m_furDblMng->value(m_furProp["EHigh"]);

    uiv.checkBins(eLow, eWidth, eHigh);

    return uiv.generateErrorMessage();
  }

  void Fury::loadSettings(const QSettings & settings)
  {
    uiForm().fury_dsInput->readSettings(settings.group());
    uiForm().fury_resFile->readSettings(settings.group());
  }

  void Fury::resType(const QString& type)
  {
    QStringList exts;
    if ( type == "RES File" )
    {
      exts.append("_res.nxs");
      m_furyResFileType = true;
    }
    else
    {
      exts.append("_red.nxs");
      m_furyResFileType = false;
    }
    uiForm().fury_resFile->setFileExtensions(exts);
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
      m_furRange->setRange(range.first, range.second);
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
