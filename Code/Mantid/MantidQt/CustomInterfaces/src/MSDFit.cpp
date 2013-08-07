#include "MantidQtCustomInterfaces/MSDFit.h"
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
  MSDFit::MSDFit(QWidget * parent) : IDATab(parent), m_msdPlot(NULL), m_msdRange(NULL), m_msdDataCurve(NULL), 
    m_msdTree(NULL), m_msdProp(), m_msdDblMng(NULL)
  {}
  
  void MSDFit::setup()
  {
    // Tree Browser
    m_msdTree = new QtTreePropertyBrowser();
    uiForm().msd_properties->addWidget(m_msdTree);

    m_msdDblMng = new QtDoublePropertyManager();

    m_msdTree->setFactoryForManager(m_msdDblMng, doubleEditorFactory());

    m_msdProp["Start"] = m_msdDblMng->addProperty("StartX");
    m_msdDblMng->setDecimals(m_msdProp["Start"], NUM_DECIMALS);
    m_msdProp["End"] = m_msdDblMng->addProperty("EndX");
    m_msdDblMng->setDecimals(m_msdProp["End"], NUM_DECIMALS);

    m_msdTree->addProperty(m_msdProp["Start"]);
    m_msdTree->addProperty(m_msdProp["End"]);

    m_msdPlot = new QwtPlot(this);
    uiForm().msd_plot->addWidget(m_msdPlot);

    // Cosmetics
    m_msdPlot->setAxisFont(QwtPlot::xBottom, this->font());
    m_msdPlot->setAxisFont(QwtPlot::yLeft, this->font());
    m_msdPlot->setCanvasBackground(Qt::white);

    m_msdRange = new MantidWidgets::RangeSelector(m_msdPlot);

    connect(m_msdRange, SIGNAL(minValueChanged(double)), this, SLOT(minChanged(double)));
    connect(m_msdRange, SIGNAL(maxValueChanged(double)), this, SLOT(maxChanged(double)));
    connect(m_msdDblMng, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(updateRS(QtProperty*, double)));

    connect(uiForm().msd_pbPlotInput, SIGNAL(clicked()), this, SLOT(plotInput()));
    connect(uiForm().msd_inputFile, SIGNAL(filesFound()), this, SLOT(plotInput()));
  }

  void MSDFit::run()
  {
    QString pyInput =
      "from IndirectDataAnalysis import msdfit\n"
      "startX = " + QString::number(m_msdDblMng->value(m_msdProp["Start"])) +"\n"
      "endX = " + QString::number(m_msdDblMng->value(m_msdProp["End"])) +"\n"
      "inputs = [r'" + uiForm().msd_inputFile->getFilenames().join("', r'") + "']\n";

    if ( uiForm().msd_ckVerbose->isChecked() ) pyInput += "verbose = True\n";
    else pyInput += "verbose = False\n";

    if ( uiForm().msd_ckPlot->isChecked() ) pyInput += "plot = True\n";
    else pyInput += "plot = False\n";

    if ( uiForm().msd_ckSave->isChecked() ) pyInput += "save = True\n";
    else pyInput += "save = False\n";

    pyInput +=
      "msdfit(inputs, startX, endX, Save=save, Verbose=verbose, Plot=plot)\n";

    QString pyOutput = runPythonCode(pyInput).trimmed();
  }

  QString MSDFit::validate()
  {
    UserInputValidator uiv;
    
    uiv.checkMWRunFilesIsValid("Input", uiForm().msd_inputFile);

    auto range = std::make_pair(m_msdDblMng->value(m_msdProp["Start"]), m_msdDblMng->value(m_msdProp["End"]));
    uiv.checkValidRange("a range", range);

    return uiv.generateErrorMessage();
  }

  void MSDFit::loadSettings(const QSettings & settings)
  {
    uiForm().msd_inputFile->readSettings(settings.group());
  }

  void MSDFit::plotInput()
  {
    if ( uiForm().msd_inputFile->isValid() )
    {
      QString filename = uiForm().msd_inputFile->getFirstFilename();
      QFileInfo fi(filename);
      QString wsname = fi.baseName();
      auto ws = runLoadNexus(filename, wsname);
      if(!ws)
      {
        showInformationBox(QString("Unable to load file: ") + filename);
        return;
      }

      m_msdDataCurve = plotMiniplot(m_msdPlot, m_msdDataCurve, ws, 0);
      try
      {
        const std::pair<double, double> range = getCurveRange(m_msdDataCurve);
        m_msdRange->setRange(range.first, range.second);
        // Replot
        m_msdPlot->replot();
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

  void MSDFit::minChanged(double val)
  {
    m_msdDblMng->setValue(m_msdProp["Start"], val);
  }

  void MSDFit::maxChanged(double val)
  {
    m_msdDblMng->setValue(m_msdProp["End"], val);
  }

  void MSDFit::updateRS(QtProperty* prop, double val)
  {
    if ( prop == m_msdProp["Start"] ) m_msdRange->setMinimum(val);
    else if ( prop == m_msdProp["End"] ) m_msdRange->setMaximum(val);
  }
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
