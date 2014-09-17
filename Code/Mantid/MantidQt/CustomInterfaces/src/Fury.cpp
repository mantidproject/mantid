#include "MantidQtCustomInterfaces/Fury.h"

#include "MantidQtCustomInterfaces/UserInputValidator.h"
#include "MantidQtMantidWidgets/RangeSelector.h"

#include <QFileInfo>

#include <qwt_plot.h>
#include <boost/lexical_cast.hpp>

#include <cmath>

namespace
{
  Mantid::Kernel::Logger g_log("Fury");
}

using Mantid::API::MatrixWorkspace;
using Mantid::API::MatrixWorkspace_const_sptr;

namespace MantidQt
{
namespace CustomInterfaces
{
namespace IDA
{
  Fury::Fury(QWidget * parent) : IDATab(parent),
    m_furPlot(NULL), m_furRange(NULL), m_furCurve(NULL), m_furTree(NULL),
    m_furProp(), m_furDblMng(NULL), m_furyResFileType()
  {
  }

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

    // Create and configure properties
    m_furProp["ELow"] = m_furDblMng->addProperty("ELow");
    m_furDblMng->setDecimals(m_furProp["ELow"], NUM_DECIMALS);

    m_furProp["EWidth"] = m_furDblMng->addProperty("EWidth");
    m_furDblMng->setDecimals(m_furProp["EWidth"], NUM_DECIMALS);
    m_furProp["EWidth"]->setEnabled(false);

    m_furProp["EHigh"] = m_furDblMng->addProperty("EHigh");
    m_furDblMng->setDecimals(m_furProp["EHigh"], NUM_DECIMALS);

    m_furProp["SampleBinning"] = m_furDblMng->addProperty("SampleBinning");
    m_furDblMng->setDecimals(m_furProp["SampleBinning"], 0);

    m_furProp["SampleBins"] = m_furDblMng->addProperty("SampleBins");
    m_furDblMng->setDecimals(m_furProp["SampleBins"], 0);
    m_furProp["SampleBins"]->setEnabled(false);

    m_furProp["ResolutionBins"] = m_furDblMng->addProperty("ResolutionBins");
    m_furDblMng->setDecimals(m_furProp["ResolutionBins"], 0);
    m_furProp["ResolutionBins"]->setEnabled(false);

    m_furTree->addProperty(m_furProp["ELow"]);
    m_furTree->addProperty(m_furProp["EWidth"]);
    m_furTree->addProperty(m_furProp["EHigh"]);
    m_furTree->addProperty(m_furProp["SampleBinning"]);
    m_furTree->addProperty(m_furProp["SampleBins"]);
    m_furTree->addProperty(m_furProp["ResolutionBins"]);

    m_furDblMng->setValue(m_furProp["SampleBinning"], 10);

    m_furTree->setFactoryForManager(m_furDblMng, doubleEditorFactory());

    m_furRange = new MantidQt::MantidWidgets::RangeSelector(m_furPlot);

    // signals / slots & validators
    connect(m_furRange, SIGNAL(minValueChanged(double)), this, SLOT(minChanged(double)));
    connect(m_furRange, SIGNAL(maxValueChanged(double)), this, SLOT(maxChanged(double)));
    connect(m_furDblMng, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(updateRS(QtProperty*, double)));
    connect(m_furDblMng, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(updatePropertyValues(QtProperty*, double)));
    connect(uiForm().fury_dsInput, SIGNAL(dataReady(const QString&)), this, SLOT(plotInput(const QString&)));
    connect(uiForm().fury_dsResInput, SIGNAL(dataReady(const QString&)), this, SLOT(calculateBinning()));
  }

  void Fury::run()
  {
    using namespace Mantid::API;

    calculateBinning();

    QString wsName = uiForm().fury_dsInput->getCurrentDataName();
    QString resName = uiForm().fury_dsResInput->getCurrentDataName();

    double energyMin = m_furDblMng->value(m_furProp["ELow"]);
    double energyMax = m_furDblMng->value(m_furProp["EHigh"]);
    long numBins = static_cast<long>(m_furDblMng->value(m_furProp["SampleBinning"]));

    bool plot = uiForm().fury_ckPlot->isChecked();
    bool verbose = uiForm().fury_ckVerbose->isChecked();
    bool save = uiForm().fury_ckSave->isChecked();

    IAlgorithm_sptr furyAlg = AlgorithmManager::Instance().create("Fury", -1);
    furyAlg->initialize();

    furyAlg->setProperty("Sample", wsName.toStdString());
    furyAlg->setProperty("Resolution", resName.toStdString());

    furyAlg->setProperty("EnergyMin", energyMin);
    furyAlg->setProperty("EnergyMax", energyMax);
    furyAlg->setProperty("NumBins", numBins);

    furyAlg->setProperty("Plot", plot);
    furyAlg->setProperty("Verbose", verbose);
    furyAlg->setProperty("Save", save);
    furyAlg->setProperty("DryRun", false);

    runAlgorithm(furyAlg);
  }

  /**
   * Ensure we have present and valid file/ws inputs.
   *
   * The underlying Fourier transform of Fury
   * also means we must enforce several rules on the parameters.
   */
  QString Fury::validate()
  {
    UserInputValidator uiv;

    uiv.checkDataSelectorIsValid("Sample", uiForm().fury_dsInput);
    uiv.checkDataSelectorIsValid("Resolution", uiForm().fury_dsResInput);

    QString message = uiv.generateErrorMessage();

    return message;
  }

  /**
   * Ensures that absolute min and max energy are equal.
   *
   * @param prop Qt property that was changed
   * @param val New value of that property
   */
  void Fury::updatePropertyValues(QtProperty *prop, double val)
  {
    disconnect(m_furDblMng, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(updatePropertyValues(QtProperty*, double)));

    if(prop == m_furProp["EHigh"])
    {
      // If the user enters a negative value for EHigh assume they did not mean to add a -
      if(val < 0)
      {
        val = -val;
        m_furDblMng->setValue(m_furProp["EHigh"], val);
      }

      m_furDblMng->setValue(m_furProp["ELow"], -val);
    }
    else if(prop == m_furProp["ELow"])
    {
      // If the user enters a positive value for ELow, assume they ment to add a 
      if(val > 0)
      {
        val = -val;
        m_furDblMng->setValue(m_furProp["ELow"], val);
      }

      m_furDblMng->setValue(m_furProp["EHigh"], -val);
    }

    connect(m_furDblMng, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(updatePropertyValues(QtProperty*, double)));

    calculateBinning();
  }

  /**
   * Calculates binning parameters.
   */
  void Fury::calculateBinning()
  {
    using namespace Mantid::API;

    QString wsName = uiForm().fury_dsInput->getCurrentDataName();
    QString resName = uiForm().fury_dsResInput->getCurrentDataName();

    double energyMin = m_furDblMng->value(m_furProp["ELow"]);
    double energyMax = m_furDblMng->value(m_furProp["EHigh"]);
    long numBins = static_cast<long>(m_furDblMng->value(m_furProp["SampleBinning"]));

    if(wsName.isEmpty() || resName.isEmpty() || numBins == 0)
      return;

    bool verbose = uiForm().fury_ckVerbose->isChecked();

    IAlgorithm_sptr furyAlg = AlgorithmManager::Instance().create("Fury");
    furyAlg->initialize();

    furyAlg->setProperty("Sample", wsName.toStdString());
    furyAlg->setProperty("Resolution", resName.toStdString());
    furyAlg->setProperty("ParameterWorkspace", "__FuryProperties_temp");

    furyAlg->setProperty("EnergyMin", energyMin);
    furyAlg->setProperty("EnergyMax", energyMax);
    furyAlg->setProperty("NumBins", numBins);

    furyAlg->setProperty("Plot", false);
    furyAlg->setProperty("Verbose", verbose);
    furyAlg->setProperty("Save", false);
    furyAlg->setProperty("DryRun", true);

    furyAlg->execute();

    // Get property table from algorithm
    ITableWorkspace_sptr propsTable = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("__FuryProperties_temp");

    // Get data from property table
    double energyWidth = propsTable->getColumn("EnergyWidth")->cell<float>(0);
    int sampleBins = propsTable->getColumn("SampleOutputBins")->cell<int>(0);
    int resolutionBins = propsTable->getColumn("ResolutionBins")->cell<int>(0);

    // Update data in property editor
    m_furDblMng->setValue(m_furProp["EWidth"], energyWidth);
    m_furDblMng->setValue(m_furProp["ResolutionBins"], resolutionBins);
    m_furDblMng->setValue(m_furProp["SampleBins"], sampleBins);
  }

  void Fury::loadSettings(const QSettings & settings)
  {
    uiForm().fury_dsInput->readSettings(settings.group());
    uiForm().fury_dsResInput->readSettings(settings.group());
  }

  void Fury::plotInput(const QString& wsname)
  {
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
      double rounded_min = floor(range.first * 10 + 0.5) / 10.0;
      double rounded_max = floor(range.second * 10 + 0.5) / 10.0;

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

    calculateBinning();
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
