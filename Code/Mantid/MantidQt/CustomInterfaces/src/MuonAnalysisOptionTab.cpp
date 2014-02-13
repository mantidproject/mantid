//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/ConfigService.h"

#include "MantidQtCustomInterfaces/MuonAnalysisOptionTab.h"
#include "MantidQtCustomInterfaces/MuonAnalysisHelper.h"

#include "MantidQtAPI/UserSubWindow.h"
#include "MantidQtAPI/ManageUserDirectories.h"

#include <boost/shared_ptr.hpp>
#include <fstream>  

#include <QLineEdit>
#include <QFileDialog>
#include <QHash>
#include <QTextStream>
#include <QTreeWidgetItem>
#include <QSettings>
#include <QMessageBox>
#include <QInputDialog>
#include <QSignalMapper>
#include <QHeaderView>
#include <QApplication>
#include <QClipboard>
#include <QTemporaryFile>
#include <QDateTime>
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
  : m_uiForm(uiForm), m_settingsGroup(settingsGroup), m_yAxisMinimum(), m_yAxisMaximum(),
    m_customTimeValue(), m_autoSaver(settingsGroup)
{
  m_autoSaver.beginGroup("PlotStyleOptions");
  m_autoSaver.registerWidget(m_uiForm.connectPlotType, "connectPlotStyle", 0);
  m_autoSaver.registerWidget(m_uiForm.timeAxisStartAtInput, "timeAxisStart", "0.3");
  m_autoSaver.registerWidget(m_uiForm.timeAxisFinishAtInput, "timeAxisFinish", "16.0");
  m_autoSaver.registerWidget(m_uiForm.yAxisMinimumInput, "yAxisStart", "");
  m_autoSaver.registerWidget(m_uiForm.yAxisMaximumInput, "yAxisFinish", "");
  m_autoSaver.registerWidget(m_uiForm.showErrorBars, "errorBars", 0);
  m_autoSaver.endGroup();
  m_autoSaver.beginGroup("BinningOptions");
  m_autoSaver.registerWidget(m_uiForm.optionStepSizeText, "rebinFixed", "1");
  m_autoSaver.registerWidget(m_uiForm.binBoundaries, "rebinVariable", "1");
  m_autoSaver.endGroup();

  m_autoSaver.beginGroup("SettingOptions");
  m_autoSaver.registerWidget(m_uiForm.plotCreation, "plotCreation", 0);
  m_autoSaver.registerWidget(m_uiForm.hideToolbars, "toolbars", 1);
  m_autoSaver.registerWidget(m_uiForm.hideGraphs, "hiddenGraphs", 1);
  m_autoSaver.endGroup();
}


void MuonAnalysisOptionTab::initLayout()
{
  // Set validators for double fields
  setDoubleValidator(m_uiForm.timeAxisStartAtInput);
  setDoubleValidator(m_uiForm.timeAxisFinishAtInput);
  setDoubleValidator(m_uiForm.yAxisMinimumInput);
  setDoubleValidator(m_uiForm.yAxisMaximumInput);
  setDoubleValidator(m_uiForm.optionStepSizeText);

  // Help
  connect(m_uiForm.muonAnalysisHelpPlotting, SIGNAL(clicked()), this, SLOT(muonAnalysisHelpSettingsClicked()));
  connect(m_uiForm.binBoundariesHelp, SIGNAL(clicked()), this, SLOT(rebinHelpClicked()));

  ////////////// Default Plot Style slots ///////////////
  connect(m_uiForm.timeComboBox, SIGNAL(currentIndexChanged(int)), this, 
           SLOT(runTimeComboBox(int)));
  connect(m_uiForm.yAxisAutoscale, SIGNAL(toggled(bool)), this,  
           SLOT(runyAxisAutoscale(bool)));

  ////////////// Auto-update plot style //////////////
  connect(m_uiForm.connectPlotType, SIGNAL(currentIndexChanged(int)), this, SIGNAL(plotStyleChanged()));
  connect(m_uiForm.showErrorBars, SIGNAL(clicked()), this, SIGNAL(plotStyleChanged()));
  connect(m_uiForm.yAxisAutoscale, SIGNAL(clicked()), this, SIGNAL(plotStyleChanged()));
  connect(m_uiForm.yAxisMinimumInput, SIGNAL(returnPressed ()), this, SIGNAL(plotStyleChanged()));
  connect(m_uiForm.yAxisMaximumInput, SIGNAL(returnPressed ()), this, SIGNAL(plotStyleChanged()));
  
  ////////////// Auto Update  /////////////////
  connect(m_uiForm.timeComboBox, SIGNAL(currentIndexChanged(int)), this, SIGNAL(settingsTabUpdatePlot()));
  connect(m_uiForm.timeAxisStartAtInput, SIGNAL(returnPressed ()), this, SIGNAL(settingsTabUpdatePlot()));
  connect(m_uiForm.timeAxisFinishAtInput, SIGNAL(returnPressed ()), this, SIGNAL(settingsTabUpdatePlot()));
  connect(m_uiForm.rebinComboBox, SIGNAL(currentIndexChanged(int)), this, SIGNAL(settingsTabUpdatePlot()));
  connect(m_uiForm.optionStepSizeText, SIGNAL(returnPressed()), this, SIGNAL(settingsTabUpdatePlot()));
  connect(m_uiForm.binBoundaries, SIGNAL(returnPressed()), this, SIGNAL(settingsTabUpdatePlot()));

  // Manage User Directories
  connect(m_uiForm.manageDirectoriesBtn, SIGNAL(clicked()), this, SLOT(openDirectoryDialog() ) );
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


////////////// Data Binning slots ///////////////

/**
* When clicking Rebin combobox (slot)
*/
void MuonAnalysisOptionTab::runRebinComboBox(int index)
{
  // Set which rebin entry to display.
  m_uiForm.rebinEntryState->setCurrentIndex(index);

  QSettings group;
  group.beginGroup(m_settingsGroup + "BinningOptions");
  group.setValue("rebinComboBoxIndex", index); 

  emit settingsTabUpdatePlot();
}

////////////// Default Plot Style slots ///////////////

/**
* When clicking autoscale (slot)
*/
void MuonAnalysisOptionTab::runyAxisAutoscale(bool state)
{
  m_uiForm.yAxisMinimumInput->setEnabled(!state);
  m_uiForm.yAxisMaximumInput->setEnabled(!state);

  if(state)
  {
    m_yAxisMinimum = m_uiForm.yAxisMinimumInput->text();
    m_yAxisMaximum = m_uiForm.yAxisMaximumInput->text();

    m_uiForm.yAxisMinimumInput->setText("N/A");
    m_uiForm.yAxisMaximumInput->setText("N/A");
  }
  else
  {
    m_uiForm.yAxisMinimumInput->setText(m_yAxisMinimum);
    m_uiForm.yAxisMaximumInput->setText(m_yAxisMaximum);
  }

  QSettings group;
  group.beginGroup(m_settingsGroup + "plotStyleOptions");
  group.setValue("axisAutoScaleOnOff", state);   
}

/**
* Plot option time combo box (slot)
*/
void MuonAnalysisOptionTab::runTimeComboBox(int index)
{
  switch(index)
  {
  case(0): // Start at First Good Data
    m_uiForm.timeAxisStartAtInput->setEnabled(false);
    m_uiForm.timeAxisStartAtInput->setText(m_uiForm.firstGoodBinFront->text());
    break;
  case(1): // Start at Time Zero
    m_uiForm.timeAxisStartAtInput->setEnabled(false);
    m_uiForm.timeAxisStartAtInput->setText("0");
    break;
  case(2): // Custom Value
    m_uiForm.timeAxisStartAtInput->setEnabled(true);
    if(m_customTimeValue.isEmpty())
      m_uiForm.timeAxisStartAtInput->setText("0.0");
    else
      m_uiForm.timeAxisStartAtInput->setText(m_customTimeValue);
  }

  if(index == 0)
  {
    // Synchronize First Good Data box on Home tab with the one on this tab, if Start at First Good
    // Data is enabled.
    connect(m_uiForm.firstGoodBinFront, SIGNAL(textChanged(const QString&)),
      m_uiForm.timeAxisStartAtInput, SLOT(setText(const QString&)));
  }
  else
  {
    // Disable synchronization otherwise
    disconnect(m_uiForm.firstGoodBinFront, SIGNAL(textChanged(const QString&)),
      m_uiForm.timeAxisStartAtInput, SLOT(setText(const QString&)));
  }

  // save this new choice
  QSettings group;
  group.beginGroup(m_settingsGroup + "plotStyleOptions");
  group.setValue("timeComboBoxIndex", index); 
}

/**
 * When no data loaded set various buttons etc to inactive
 */
void MuonAnalysisOptionTab::noDataAvailable()
{
  m_uiForm.frontPlotButton->setEnabled(false);
  m_uiForm.groupTablePlotButton->setEnabled(false);
  m_uiForm.pairTablePlotButton->setEnabled(false);
  m_uiForm.guessAlphaButton->setEnabled(false);
}


/**
 * When data loaded set various buttons etc to active
 */
void MuonAnalysisOptionTab::nowDataAvailable()
{
  m_uiForm.frontPlotButton->setEnabled(true);
  m_uiForm.groupTablePlotButton->setEnabled(true);
  m_uiForm.pairTablePlotButton->setEnabled(true);
  m_uiForm.guessAlphaButton->setEnabled(true);
}


void MuonAnalysisOptionTab::openDirectoryDialog()
{
  MantidQt::API::ManageUserDirectories *ad = new MantidQt::API::ManageUserDirectories(this);
  ad->show();
  ad->setFocus();
}

/// Set the *stored" yAxisMinimum value.
void MuonAnalysisOptionTab::setStoredYAxisMinimum(const QString & yAxisMinimum)
{
  m_yAxisMinimum = yAxisMinimum;
}

/// Set the *stored" yAxisMaximum value.
void MuonAnalysisOptionTab::setStoredYAxisMaximum(const QString & yAxisMaximum)
{
  m_yAxisMaximum = yAxisMaximum;
}

/// Set the stored custom time value.
void MuonAnalysisOptionTab::setStoredCustomTimeValue(const QString & storedCustomTimeValue)
{
  m_customTimeValue = storedCustomTimeValue;
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
