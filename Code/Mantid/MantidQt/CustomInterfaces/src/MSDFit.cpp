#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtCustomInterfaces/MSDFit.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"
#include "MantidQtMantidWidgets/RangeSelector.h"


#include <QFileInfo>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>

namespace MantidQt
{
namespace CustomInterfaces
{
namespace IDA
{
  MSDFit::MSDFit(QWidget * parent) : IDATab(parent), m_msdPlot(NULL), m_msdRange(NULL), m_msdDataCurve(NULL), m_msdFitCurve(NULL), 
    m_msdTree(NULL), m_msdProp(), m_msdDblMng(NULL)
  {}
  
  void MSDFit::setup()
  {
    // Tree Browser
    m_msdTree = new QtTreePropertyBrowser();
    uiForm().msd_properties->addWidget(m_msdTree);
    m_intVal = new QIntValidator(this);
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
    connect(uiForm().msd_pbSequential, SIGNAL(clicked()), this, SLOT(sequential()));
    connect(uiForm().msd_lePlotSpectrum, SIGNAL(editingFinished()), this, SLOT(plotInput()));
    connect(uiForm().msd_leSpectraMin, SIGNAL(editingFinished()), this, SLOT(plotInput()));
    connect(uiForm().msd_leSpectraMax, SIGNAL(editingFinished()), this, SLOT(plotInput()));
    
    uiForm().msd_leSpectraMin->setValidator(m_intVal);
    uiForm().msd_leSpectraMax->setValidator(m_intVal);
  }

  void MSDFit::run()
  {
    QString pyInput =
      "from IndirectDataAnalysis import msdfit\n"
      "startX = " + QString::number(m_msdDblMng->value(m_msdProp["Start"])) +"\n"
      "endX = " + QString::number(m_msdDblMng->value(m_msdProp["End"])) +"\n"
      "specMin = " + uiForm().msd_lePlotSpectrum->text() + "\n"
      "specMax = " + uiForm().msd_lePlotSpectrum->text() + "\n"
      "inputs = [r'" + uiForm().msd_inputFile->getFilenames().join("', r'") + "']\n";

    if ( uiForm().msd_ckVerbose->isChecked() ) pyInput += "verbose = True\n";
    else pyInput += "verbose = False\n";

    pyInput +=
      "output = msdfit(inputs, startX, endX, spec_min=specMin, spec_max=specMax, Save=False, Verbose=verbose, Plot=False)\n"
      "print output \n";

    QString pyOutput = runPythonCode(pyInput).trimmed();
    plotFit(pyOutput);
  }

  void MSDFit::sequential()
  {
    QString errors = validate();

    if (errors.isEmpty())
    {
      QString pyInput =
      "from IndirectDataAnalysis import msdfit\n"
      "startX = " + QString::number(m_msdDblMng->value(m_msdProp["Start"])) +"\n"
      "endX = " + QString::number(m_msdDblMng->value(m_msdProp["End"])) +"\n"
      "specMin = " + uiForm().msd_leSpectraMin->text() + "\n"
      "specMax = " + uiForm().msd_leSpectraMax->text() + "\n"
      "inputs = [r'" + uiForm().msd_inputFile->getFilenames().join("', r'") + "']\n";

      if ( uiForm().msd_ckVerbose->isChecked() ) pyInput += "verbose = True\n";
      else pyInput += "verbose = False\n";

      if ( uiForm().msd_ckPlot->isChecked() ) pyInput += "plot = True\n";
      else pyInput += "plot = False\n";

      if ( uiForm().msd_ckSave->isChecked() ) pyInput += "save = True\n";
      else pyInput += "save = False\n";

      pyInput +=
        "msdfit(inputs, startX, endX, spec_min=specMin, spec_max=specMax, Save=save, Verbose=verbose, Plot=plot)\n";

      QString pyOutput = runPythonCode(pyInput).trimmed();
    }
    else
    {
      showInformationBox(errors);
    }

  }

  QString MSDFit::validate()
  {
    UserInputValidator uiv;
    
    uiv.checkMWRunFilesIsValid("Input", uiForm().msd_inputFile);

    auto range = std::make_pair(m_msdDblMng->value(m_msdProp["Start"]), m_msdDblMng->value(m_msdProp["End"]));
    uiv.checkValidRange("a range", range);

    QString specMin = uiForm().msd_leSpectraMin->text();
    QString specMax = uiForm().msd_leSpectraMax->text();
    auto specRange = std::make_pair(specMin.toDouble(), specMax.toDouble());
    uiv.checkValidRange("spectrum range", specRange);

    return uiv.generateErrorMessage();
  }

  void MSDFit::loadSettings(const QSettings & settings)
  {
    uiForm().msd_inputFile->readSettings(settings.group());
  }

  void MSDFit::plotFit(QString wsName)
  {
    if(Mantid::API::AnalysisDataService::Instance().doesExist(wsName.toStdString()))
    {
      //read the fit from the workspace
      auto groupWs = Mantid::API::AnalysisDataService::Instance().retrieveWS<const Mantid::API::WorkspaceGroup>(wsName.toStdString());
      auto ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(groupWs->getItem(0));
      m_msdFitCurve = plotMiniplot(m_msdPlot, m_msdFitCurve, ws, 1);
      QPen fitPen(Qt::red, Qt::SolidLine);
      m_msdFitCurve->setPen(fitPen);
      m_msdPlot->replot();
    }
  }

  void MSDFit::plotInput()
  {
    using namespace Mantid::API;
    if ( uiForm().msd_inputFile->isValid() )
    {
      QString filename = uiForm().msd_inputFile->getFirstFilename();
      QFileInfo fi(filename);
      QString wsname = fi.baseName();

      MatrixWorkspace_const_sptr ws;
      if(!AnalysisDataService::Instance().doesExist(wsname.toStdString()))
      {
        ws = runLoadNexus(filename, wsname);
      }
      else
      {
        ws = AnalysisDataService::Instance().retrieveWS<const Mantid::API::MatrixWorkspace>(wsname.toStdString());
      }

      if(!ws)
      {
        return;
      }
      
      int nHist = static_cast<int>(ws->getNumberHistograms());
      
      QString plotSpec =uiForm().msd_lePlotSpectrum->text();
      QString specMin = uiForm().msd_leSpectraMin->text();
      QString specMax = uiForm().msd_leSpectraMax->text();
      
      int wsIndex = 0;
      int minIndex = 0;
      int maxIndex = nHist-1;
      
      if (!plotSpec.isEmpty() && plotSpec.toInt() < nHist)
      {
        wsIndex = plotSpec.toInt();
      }

      if (!specMin.isEmpty())
      {
        minIndex = specMin.toInt();
      }

      if (!specMax.isEmpty())
      {
        maxIndex = specMax.toInt();
      }

      if (wsIndex < minIndex)
      {
        wsIndex = minIndex;
      }
      else if( wsIndex > maxIndex)
      {
        wsIndex = maxIndex;
      }

      m_msdDataCurve = plotMiniplot(m_msdPlot, m_msdDataCurve, ws, wsIndex);
      try
      {
        const std::pair<double, double> range = getCurveRange(m_msdDataCurve);
        m_msdRange->setRange(range.first, range.second);
        
        uiForm().msd_leSpectraMin->setText(QString::number(minIndex));
        uiForm().msd_leSpectraMax->setText(QString::number(maxIndex));
        uiForm().msd_lePlotSpectrum->setText(QString::number(wsIndex));

        m_intVal->setRange(0, nHist-1);

        //delete reference to fitting.
        if (m_msdFitCurve != NULL)
        {
          m_msdFitCurve->attach(0);
          delete m_msdFitCurve;
          m_msdFitCurve = 0;
        }

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
