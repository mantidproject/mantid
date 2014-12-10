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
    m_furTree(NULL),
    m_furyResFileType()
  {
  }

  void Fury::setup()
  {
    m_furTree = new QtTreePropertyBrowser();
    uiForm().fury_TreeSpace->addWidget(m_furTree);

    m_plots["FuryPlot"] = new QwtPlot(m_parentWidget);
    uiForm().fury_PlotSpace->addWidget(m_plots["FuryPlot"]);
    m_plots["FuryPlot"]->setCanvasBackground(Qt::white);
    m_plots["FuryPlot"]->setAxisFont(QwtPlot::xBottom, m_parentWidget->font());
    m_plots["FuryPlot"]->setAxisFont(QwtPlot::yLeft, m_parentWidget->font());

    // Create and configure properties
    m_properties["ELow"] = m_dblManager->addProperty("ELow");
    m_dblManager->setDecimals(m_properties["ELow"], NUM_DECIMALS);

    m_properties["EWidth"] = m_dblManager->addProperty("EWidth");
    m_dblManager->setDecimals(m_properties["EWidth"], NUM_DECIMALS);
    m_properties["EWidth"]->setEnabled(false);

    m_properties["EHigh"] = m_dblManager->addProperty("EHigh");
    m_dblManager->setDecimals(m_properties["EHigh"], NUM_DECIMALS);

    m_properties["SampleBinning"] = m_dblManager->addProperty("SampleBinning");
    m_dblManager->setDecimals(m_properties["SampleBinning"], 0);

    m_properties["SampleBins"] = m_dblManager->addProperty("SampleBins");
    m_dblManager->setDecimals(m_properties["SampleBins"], 0);
    m_properties["SampleBins"]->setEnabled(false);

    m_properties["ResolutionBins"] = m_dblManager->addProperty("ResolutionBins");
    m_dblManager->setDecimals(m_properties["ResolutionBins"], 0);
    m_properties["ResolutionBins"]->setEnabled(false);

    m_furTree->addProperty(m_properties["ELow"]);
    m_furTree->addProperty(m_properties["EWidth"]);
    m_furTree->addProperty(m_properties["EHigh"]);
    m_furTree->addProperty(m_properties["SampleBinning"]);
    m_furTree->addProperty(m_properties["SampleBins"]);
    m_furTree->addProperty(m_properties["ResolutionBins"]);

    m_dblManager->setValue(m_properties["SampleBinning"], 1);

    m_furTree->setFactoryForManager(m_dblManager, doubleEditorFactory());

    m_rangeSelectors["FuryRange"] = new MantidQt::MantidWidgets::RangeSelector(m_plots["FuryPlot"]);

    // signals / slots & validators
    connect(m_rangeSelectors["FuryRange"], SIGNAL(selectionChangedLazy(double, double)), this, SLOT(rsRangeChangedLazy(double, double)));
    connect(m_dblManager, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(updateRS(QtProperty*, double)));
    connect(m_dblManager, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(updatePropertyValues(QtProperty*, double)));
    connect(uiForm().fury_dsInput, SIGNAL(dataReady(const QString&)), this, SLOT(plotInput(const QString&)));
    connect(uiForm().fury_dsResInput, SIGNAL(dataReady(const QString&)), this, SLOT(calculateBinning()));
  }

  void Fury::run()
  {
    using namespace Mantid::API;

    calculateBinning();

    QString wsName = uiForm().fury_dsInput->getCurrentDataName();
    QString resName = uiForm().fury_dsResInput->getCurrentDataName();

    double energyMin = m_dblManager->value(m_properties["ELow"]);
    double energyMax = m_dblManager->value(m_properties["EHigh"]);
    long numBins = static_cast<long>(m_dblManager->value(m_properties["SampleBinning"]));

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

    // Set the result workspace for Python script export
    QString sampleName = uiForm().fury_dsInput->getCurrentDataName();
    m_pythonExportWsName = sampleName.left(sampleName.lastIndexOf("_")).toStdString() + "_iqt";
  }

  /**
   * Ensure we have present and valid file/ws inputs.
   *
   * The underlying Fourier transform of Fury
   * also means we must enforce several rules on the parameters.
   */
  bool Fury::validate()
  {
    UserInputValidator uiv;

    uiv.checkDataSelectorIsValid("Sample", uiForm().fury_dsInput);
    uiv.checkDataSelectorIsValid("Resolution", uiForm().fury_dsResInput);

    QString message = uiv.generateErrorMessage();
    showMessageBox(message);

    return message.isEmpty();
  }

  /**
   * Ensures that absolute min and max energy are equal.
   *
   * @param prop Qt property that was changed
   * @param val New value of that property
   */
  void Fury::updatePropertyValues(QtProperty *prop, double val)
  {
    disconnect(m_dblManager, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(updatePropertyValues(QtProperty*, double)));

    if(prop == m_properties["EHigh"])
    {
      // If the user enters a negative value for EHigh assume they did not mean to add a -
      if(val < 0)
      {
        val = -val;
        m_dblManager->setValue(m_properties["EHigh"], val);
      }

      m_dblManager->setValue(m_properties["ELow"], -val);
    }
    else if(prop == m_properties["ELow"])
    {
      // If the user enters a positive value for ELow, assume they ment to add a
      if(val > 0)
      {
        val = -val;
        m_dblManager->setValue(m_properties["ELow"], val);
      }

      m_dblManager->setValue(m_properties["EHigh"], -val);
    }

    connect(m_dblManager, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(updatePropertyValues(QtProperty*, double)));

    calculateBinning();
  }

  /**
   * Calculates binning parameters.
   */
  void Fury::calculateBinning()
  {
    using namespace Mantid::API;

    disconnect(m_dblManager, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(updatePropertyValues(QtProperty*, double)));

    QString wsName = uiForm().fury_dsInput->getCurrentDataName();
    QString resName = uiForm().fury_dsResInput->getCurrentDataName();
    if(wsName.isEmpty() || resName.isEmpty())
      return;

    double energyMin = m_dblManager->value(m_properties["ELow"]);
    double energyMax = m_dblManager->value(m_properties["EHigh"]);

    // Estimate number of SampleBinning according to whether the workspace has angles or momentum transfer
    MatrixWorkspace_const_sptr workspace;
    try
    {
      workspace = Mantid::API::AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>(wsName.toStdString());
    }
    catch(Mantid::Kernel::Exception::NotFoundError&)
    {
      showMessageBox(QString("Unable to retrieve workspace: " + wsName));
      return;
    }
    long numBins = static_cast<long>(m_dblManager->value(m_properties["SampleBinning"])); // Default value
    const std::string unitID( workspace->getAxis(0)->unit()->unitID() );
    if( unitID == "MomentumTransfer")
    {
      numBins = 1;
    }

    if(numBins == 0)
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
    m_dblManager->setValue(m_properties["EWidth"], energyWidth);
    m_dblManager->setValue(m_properties["ResolutionBins"], resolutionBins);
    m_dblManager->setValue(m_properties["SampleBins"], sampleBins);

    connect(m_dblManager, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(updatePropertyValues(QtProperty*, double)));

    // Warn for low number of resolution bins
    int numResolutionBins = static_cast<int>(m_dblManager->value(m_properties["ResolutionBins"]));
    if(numResolutionBins < 5)
      showMessageBox("Number of resolution bins is less than 5.\nResults may be inaccurate.");
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
      showMessageBox(QString("Unable to retrieve workspace: " + wsname));
      return;
    }

    plotMiniPlot(workspace, 0, "FuryPlot", "FuryCurve");
    try
    {
      const std::pair<double, double> range = getCurveRange("FuryCurve");
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
        m_rangeSelectors["FuryRange"]->setRange(rounded_min, rounded_max);
        m_dblManager->setValue(m_properties["ELow"], rounded_min);
        m_dblManager->setValue(m_properties["EHigh"], rounded_max);
      }
      else
      {
        m_rangeSelectors["FuryRange"]->setRange(range.first, range.second);
        m_dblManager->setValue(m_properties["ELow"], range.first);
        m_dblManager->setValue(m_properties["EHigh"], range.second);
      }
      //set default value for width
      m_dblManager->setValue(m_properties["EWidth"], 0.005);
      replot("FuryPlot");
    }
    catch(std::invalid_argument & exc)
    {
      showMessageBox(exc.what());
    }

    calculateBinning();
  }

  /**
   * Updates the range selectors and properties when range selector is moved.
   *
   * @param min Range selector min value
   * @param max Range selector amx value
   */
  void Fury::rsRangeChangedLazy(double min, double max)
  {
    double oldMin = m_dblManager->value(m_properties["ELow"]);
    double oldMax = m_dblManager->value(m_properties["EHigh"]);

    if(fabs(oldMin - min) > 0.0000001)
      m_dblManager->setValue(m_properties["ELow"], min);

    if(fabs(oldMax - max) > 0.0000001)
      m_dblManager->setValue(m_properties["EHigh"], max);
  }

  void Fury::updateRS(QtProperty* prop, double val)
  {
    if(prop == m_properties["ELow"])
      m_rangeSelectors["FuryRange"]->setMinimum(val);
    else if(prop == m_properties["EHigh"])
      m_rangeSelectors["FuryRange"]->setMaximum(val);
  }

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
