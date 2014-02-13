//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidQtCustomInterfaces/MuonAnalysisOptionTab.h"
#include "MantidQtCustomInterfaces/MuonAnalysisHelper.h"

#include <QLineEdit>
#include <QSettings>
#include <QDesktopServices>
#include <QUrl>

//-----------------------------------------------------------------------------
using namespace Mantid::Kernel;

namespace MantidQt
{
  using namespace API;
  using namespace MantidWidgets;

namespace CustomInterfaces
{
  using namespace MuonAnalysisHelper;

namespace Muon
{

MuonAnalysisOptionTab::MuonAnalysisOptionTab(Ui::MuonAnalysis &uiForm, const QString &settingsGroup)
  : m_uiForm(uiForm), m_autoSaver(settingsGroup)
{}

/**
 * Initialise the layout of the tab
 */
void MuonAnalysisOptionTab::initLayout()
{
  // Register all the widgets for auto-saving
  m_autoSaver.beginGroup("PlotStyleOptions");
  m_autoSaver.registerWidget(m_uiForm.connectPlotType, "connectPlotStyle", 0);
  m_autoSaver.registerWidget(m_uiForm.timeAxisStartAtInput, "timeAxisStart", "0.3");
  m_autoSaver.registerWidget(m_uiForm.timeAxisFinishAtInput, "timeAxisFinish", "16.0");
  m_autoSaver.registerWidget(m_uiForm.timeComboBox, "timeComboBoxIndex", 0);
  m_autoSaver.registerWidget(m_uiForm.yAxisMinimumInput, "yAxisStart", "");
  m_autoSaver.registerWidget(m_uiForm.yAxisMaximumInput, "yAxisFinish", "");
  m_autoSaver.registerWidget(m_uiForm.yAxisAutoscale, "axisAutoScaleOnOff", true);
  m_autoSaver.registerWidget(m_uiForm.showErrorBars, "errorBars", 0);
  m_autoSaver.endGroup();

  m_autoSaver.beginGroup("BinningOptions");
  m_autoSaver.registerWidget(m_uiForm.optionStepSizeText, "rebinFixed", "1");
  m_autoSaver.registerWidget(m_uiForm.binBoundaries, "rebinVariable", "1");
  m_autoSaver.registerWidget(m_uiForm.rebinComboBox, "rebinComboBoxIndex", 0);
  m_autoSaver.endGroup();

  m_autoSaver.beginGroup("GeneralOptions");
  m_autoSaver.registerWidget(m_uiForm.plotCreation, "plotCreation", 0);
  m_autoSaver.registerWidget(m_uiForm.hideToolbars, "toolbars", true);
  m_autoSaver.registerWidget(m_uiForm.hideGraphs, "hiddenGraphs", true);
  m_autoSaver.endGroup();

  // Set validators for double fields
  setDoubleValidator(m_uiForm.timeAxisStartAtInput);
  setDoubleValidator(m_uiForm.timeAxisFinishAtInput);
  setDoubleValidator(m_uiForm.yAxisMinimumInput);
  setDoubleValidator(m_uiForm.yAxisMaximumInput);
  setDoubleValidator(m_uiForm.optionStepSizeText);

  // Load saved values
  m_autoSaver.loadWidgetValues();

  // Run slots manually, because default values might not have been changed
  onTimeAxisChanged(m_uiForm.timeComboBox->currentIndex());
  onAutoscaleToggled(m_uiForm.yAxisAutoscale->isChecked());
  m_uiForm.rebinEntryState->setCurrentIndex(m_uiForm.rebinComboBox->currentIndex());

  // Enable auto-saving
  m_autoSaver.setAutoSaveEnabled(true);

  // Connect various sync stuff
  connect(m_uiForm.timeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onTimeAxisChanged(int)));
  connect(m_uiForm.yAxisAutoscale, SIGNAL(toggled(bool)), this, SLOT(onAutoscaleToggled(bool)));
  connect(m_uiForm.rebinComboBox, SIGNAL(currentIndexChanged(int)), m_uiForm.rebinEntryState,
          SLOT(setCurrentIndex(int)));

  // Connect help clicked
  connect(m_uiForm.muonAnalysisHelpPlotting, SIGNAL(clicked()), this, SLOT(muonAnalysisHelpSettingsClicked()));
  connect(m_uiForm.binBoundariesHelp, SIGNAL(clicked()), this, SLOT(rebinHelpClicked()));

  // Connect auto-updates for plot style
  connect(m_uiForm.connectPlotType, SIGNAL(currentIndexChanged(int)), this, SIGNAL(plotStyleChanged()));
  connect(m_uiForm.showErrorBars, SIGNAL(clicked()), this, SIGNAL(plotStyleChanged()));
  connect(m_uiForm.yAxisAutoscale, SIGNAL(clicked()), this, SIGNAL(plotStyleChanged()));
  connect(m_uiForm.yAxisMinimumInput, SIGNAL(returnPressed ()), this, SIGNAL(plotStyleChanged()));
  connect(m_uiForm.yAxisMaximumInput, SIGNAL(returnPressed ()), this, SIGNAL(plotStyleChanged()));
  
  // Connect auto updates of plot data
  connect(m_uiForm.timeComboBox, SIGNAL(currentIndexChanged(int)), this, SIGNAL(settingsTabUpdatePlot()));
  connect(m_uiForm.timeAxisStartAtInput, SIGNAL(returnPressed ()), this, SIGNAL(settingsTabUpdatePlot()));
  connect(m_uiForm.timeAxisFinishAtInput, SIGNAL(returnPressed ()), this, SIGNAL(settingsTabUpdatePlot()));
  connect(m_uiForm.rebinComboBox, SIGNAL(currentIndexChanged(int)), this, SIGNAL(settingsTabUpdatePlot()));
  connect(m_uiForm.optionStepSizeText, SIGNAL(returnPressed()), this, SIGNAL(settingsTabUpdatePlot()));
  connect(m_uiForm.binBoundaries, SIGNAL(returnPressed()), this, SIGNAL(settingsTabUpdatePlot()));
}

/**
* Muon Analysis Settings help.
*/
void MuonAnalysisOptionTab::muonAnalysisHelpSettingsClicked()
{
  QDesktopServices::openUrl(QUrl(QString("http://www.mantidproject.org/") +
            "MuonAnalysisSettings"));
}


/*
* Muon Analysis Rebin help (located on settings wiki).
*/
void MuonAnalysisOptionTab::rebinHelpClicked()
{
  QDesktopServices::openUrl(QUrl(QString("http://www.mantidproject.org/") +
            "MuonAnalysisSettings#Variable_Rebin"));
}

/**
 * Run when autoscale check-box state is changed
 * @param state :: New state of the check-box
 */
void MuonAnalysisOptionTab::onAutoscaleToggled(bool state)
{
  // Max and min input widgets
  auto maxInput = m_uiForm.yAxisMaximumInput;
  auto minInput = m_uiForm.yAxisMinimumInput;

  // Disable if autoscale
  maxInput->setEnabled(!state);
  minInput->setEnabled(!state);

  // Disable auto-save if autoscale
  m_autoSaver.setAutoSaveEnabled(maxInput, !state);
  m_autoSaver.setAutoSaveEnabled(minInput, !state);

  if(state)
  {
    maxInput->setText("N/A");
    minInput->setText("N/A");
  }
  else
  {
    m_autoSaver.loadWidgetValue(maxInput);
    m_autoSaver.loadWidgetValue(minInput);
  }
}

/**
 * Run when time axis combo-box is changed
 * @param index :: New index selected in the combo box
 */
void MuonAnalysisOptionTab::onTimeAxisChanged(int index)
{
  // Start input widget
  auto startInput = m_uiForm.timeAxisStartAtInput;

  // Start input enabled only if Custom value selected
  startInput->setEnabled(index == 2);

  // Auto-save enabled only for Custom value
  m_autoSaver.setAutoSaveEnabled(startInput, index == 2);

  // Get new value of the Start input
  switch(index)
  {
  case(0): // Start at First Good Data
    startInput->setText(m_uiForm.firstGoodBinFront->text());
    break;
  case(1): // Start at Time Zero
    startInput->setText("0.0");
    break;
  case(2): // Custom Value
    m_autoSaver.loadWidgetValue(startInput);
    break;
  }

  if(index == 0)
  {
    // Synchronize First Good Data box on Home tab with the one on this tab, if Start at First Good
    // Data is enabled.
    connect(m_uiForm.firstGoodBinFront, SIGNAL(textChanged(const QString&)),
            startInput, SLOT(setText(const QString&)));
  }
  else
  {
    // Disable synchronization otherwise
    disconnect(m_uiForm.firstGoodBinFront, SIGNAL(textChanged(const QString&)),
               startInput, SLOT(setText(const QString&)));
  }
}

/**
 * Get plot style parameters from widgets. Parameters are as follows:
 *   - ConnectType: 0 for Line, 1 for Scatter, 3 for Line + Symbol
 *   - ShowErrors: True of False
 *   - YAxisAuto: True or False
 *   - YAxisMin/YAxisMax: Double values
 */
QMap<QString, QString> MuonAnalysisOptionTab::parsePlotStyleParams() const
{
  QMap<QString, QString> params;

  params["ConnectType"] = QString::number(m_uiForm.connectPlotType->currentIndex());

  params["ShowErrors"] = m_uiForm.showErrorBars->isChecked() ? "True" : "False";

  params["YAxisAuto"] = m_uiForm.yAxisAutoscale->isChecked() ? "True" : "False";
  params["YAxisMin"] = m_uiForm.yAxisMinimumInput->text();
  params["YAxisMax"] = m_uiForm.yAxisMaximumInput->text();

  return(params);
}

}
}
}
