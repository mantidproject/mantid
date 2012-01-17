//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidQtCustomInterfaces/MuonAnalysisOptionTab.h"
#include "MantidKernel/ConfigService.h"

#include "MantidQtAPI/UserSubWindow.h"

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


namespace MantidQt
{
namespace CustomInterfaces
{
namespace Muon
{
  using namespace MantidQt::API;
  using namespace Mantid::Kernel;
  using namespace MantidQt::MantidWidgets;

void MuonAnalysisOptionTab::initLayout()
{
  ////////////// Default Plot Style slots ///////////////
  connect(m_uiForm.timeComboBox, SIGNAL(currentIndexChanged(int)), this, 
           SLOT(runTimeComboBox(int)));
  connect(m_uiForm.timeAxisStartAtInput, SIGNAL(lostFocus()), this, 
           SLOT(runTimeAxisStartAtInput()));
  connect(m_uiForm.timeAxisFinishAtInput, SIGNAL(lostFocus()), this, 
           SLOT(runTimeAxisFinishAtInput()));
  connect(m_uiForm.yAxisMinimumInput, SIGNAL(lostFocus()), this, 
           SLOT(runyAxisMinimumInput()));
  connect(m_uiForm.yAxisMaximumInput, SIGNAL(lostFocus()), this, 
           SLOT(runyAxisMaximumInput()));
  connect(m_uiForm.yAxisAutoscale, SIGNAL(toggled(bool)), this,  
           SLOT(runyAxisAutoscale(bool)));
  connect(m_uiForm.plotCreation, SIGNAL(currentIndexChanged(int)), this, SLOT(plotCreationChanged(int)));
  connect(m_uiForm.connectPlotType, SIGNAL(currentIndexChanged(int)), this, SLOT(plotTypeChanged(int)));
  connect(m_uiForm.showErrorBars, SIGNAL(toggled(bool)), this, SLOT(errorBarsChanged(bool)));

  ////////////// Data Binning slots ///////////////
  connect(m_uiForm.rebinComboBox, SIGNAL(currentIndexChanged(int)), this, 
           SLOT(runRebinComboBox(int)));
  connect(m_uiForm.optionStepSizeText, SIGNAL(lostFocus()), this, 
           SLOT(runOptionStepSizeText()));

  connect(m_uiForm.muonAnalysisHelpPlotting, SIGNAL(clicked()), this, SLOT(muonAnalysisHelpSettingsClicked()));

  ////////////// Auto Update  /////////////////
  connect(m_uiForm.connectPlotType, SIGNAL(currentIndexChanged(int)), this, SIGNAL(settingsTabUpdatePlot()));
  connect(m_uiForm.timeComboBox, SIGNAL(currentIndexChanged(int)), this, SIGNAL(settingsTabUpdatePlot()));
  connect(m_uiForm.timeAxisStartAtInput, SIGNAL(textChanged(const QString&)), this, SIGNAL(settingsTabUpdatePlot()));
  connect(m_uiForm.timeAxisFinishAtInput, SIGNAL(textChanged(const QString&)), this, SIGNAL(settingsTabUpdatePlot()));
  connect(m_uiForm.yAxisMinimumInput, SIGNAL(textChanged(const QString&)), this, SIGNAL(settingsTabUpdatePlot()));
  connect(m_uiForm.yAxisMaximumInput, SIGNAL(textChanged(const QString&)), this, SIGNAL(settingsTabUpdatePlot()));
  connect(m_uiForm.optionStepSizeText, SIGNAL(textChanged(const QString&)), this, SIGNAL(settingsTabUpdatePlot()));
  connect(m_uiForm.rebinComboBox, SIGNAL(currentIndexChanged(int)), this, SIGNAL(settingsTabUpdatePlot()));
  connect(m_uiForm.showErrorBars, SIGNAL(clicked()), this, SIGNAL(settingsTabUpdatePlot()));
}


/**
* Muon Analysis Plotting help (slot)
*/
void MuonAnalysisOptionTab::muonAnalysisHelpSettingsClicked()
{
  QDesktopServices::openUrl(QUrl(QString("http://www.mantidproject.org/") +
            "MuonAnalysisPlotting"));
}


////////////// Data Binning slots ///////////////

/**
* When clicking Rebin combobox (slot)
*/
void MuonAnalysisOptionTab::runRebinComboBox(int index)
{
  if (index == 0)
  {
    m_uiForm.optionBinStep->setVisible(false);
    m_uiForm.optionStepSizeText->setVisible(false);
  }
  else
  {
    m_uiForm.optionBinStep->setVisible(true);
    m_uiForm.optionStepSizeText->setVisible(true);
  }

  QSettings group;
  group.beginGroup(m_settingsGroup + "BinningOptions");
  group.setValue("rebinComboBoxIndex", index); 
}


/**
* When clicking Rebin step size text box (slot)
*/  
void MuonAnalysisOptionTab::runOptionStepSizeText()
{
  try 
  {
    int boevs = boost::lexical_cast<int>(m_uiForm.optionStepSizeText->text().toStdString());

    QSettings group;
    group.beginGroup(m_settingsGroup + "BinningOptions");
    group.setValue("constStepSize", boevs); 
  }
  catch (...)
  {
    QMessageBox::warning(this,"Mantid - MuonAnalysis", "Integer not recognised in Binning Option 'Step' input box. Reset to 1.");
    m_uiForm.optionStepSizeText->setText("1");
  }
}


////////////// Default Plot Style slots ///////////////

/**
* When clicking autoscale (slot)
*/
void MuonAnalysisOptionTab::runyAxisAutoscale(bool state)
{
  m_uiForm.yAxisMinimumInput->setEnabled(!state);
  m_uiForm.yAxisMaximumInput->setEnabled(!state);

  QSettings group;
  group.beginGroup(m_settingsGroup + "plotStyleOptions");
  group.setValue("axisAutoScaleOnOff", state);   
}

/**
* Plot option time combo box (slot)
*/
void MuonAnalysisOptionTab::runTimeComboBox(int index)
{
  if ( index == 1 ) // Start at Time Zero
  {
    m_uiForm.timeAxisStartAtInput->setEnabled(false);
    m_uiForm.timeAxisStartAtInput->setText("0");
  }

  if ( index == 0 ) // Start at First Good Data
  {
    m_uiForm.timeAxisStartAtInput->setEnabled(true);
    m_uiForm.timeAxisStartAtInput->setText(m_uiForm.firstGoodBinFront->text());
  }

  // save this new choice
  QSettings group;
  group.beginGroup(m_settingsGroup + "plotStyleOptions");
  group.setValue("timeComboBoxIndex", index); 
}

/**
* Check input is valid in input box (slot)
*/
void MuonAnalysisOptionTab::runTimeAxisStartAtInput()
{
  try 
  {
    double boevs = boost::lexical_cast<double>(m_uiForm.timeAxisStartAtInput->text().toStdString());

    QSettings group;
    group.beginGroup(m_settingsGroup + "plotStyleOptions");
    group.setValue("timeAxisStart", boevs); 
  }
  catch (...)
  {
    QMessageBox::warning(this,"Mantid - MuonAnalysis", "Number not recognised in Plot Option 'Start (ms):' input box. Reset to zero.");
    m_uiForm.timeAxisStartAtInput->setText("0");
  }
}


/**
* Check input is valid in input box (slot)
*/
void MuonAnalysisOptionTab::runTimeAxisFinishAtInput()
{
  if (m_uiForm.timeAxisFinishAtInput->text().isEmpty())
    return;

  try 
  {
    double boevs = boost::lexical_cast<double>(m_uiForm.timeAxisFinishAtInput->text().toStdString());

    QSettings group;
    group.beginGroup(m_settingsGroup + "plotStyleOptions");
    group.setValue("timeAxisFinish", boevs); 
  }
  catch (...)
  {
    QMessageBox::warning(this,"Mantid - MuonAnalysis", "Number not recognised in Plot Option 'Finish (ms):' input box.");
    m_uiForm.timeAxisFinishAtInput->setText("");
  }
}


/**
* Check input is valid in input box (slot)
*/
void MuonAnalysisOptionTab::runyAxisMinimumInput()
{
  if (m_uiForm.yAxisMinimumInput->text().isEmpty())
    return;

  try 
  {
    double boevs = boost::lexical_cast<double>(m_uiForm.yAxisMinimumInput->text().toStdString());

    QSettings group;
    group.beginGroup(m_settingsGroup + "plotStyleOptions");
    group.setValue("yAxisStart", boevs); 
  }
  catch (...)
  {
    QMessageBox::warning(this,"Mantid - MuonAnalysis", "Number not recognised in Plot Option 'Minimum:' input box.");
    m_uiForm.yAxisMinimumInput->setText("");
  }
}


/**
* Check input is valid in input box (slot)
*/
void MuonAnalysisOptionTab::runyAxisMaximumInput()
{
  if (m_uiForm.yAxisMaximumInput->text().isEmpty())
    return;

  try 
  {
    double boevs = boost::lexical_cast<double>(m_uiForm.yAxisMaximumInput->text().toStdString());

    QSettings group;
    group.beginGroup(m_settingsGroup + "plotStyleOptions");
    group.setValue("yAxisFinish", boevs); 
  }
  catch (...)
  {
    QMessageBox::warning(this,"Mantid - MuonAnalysis", "Number not recognised in Plot Option 'Maximum:' input box.");
    m_uiForm.yAxisMaximumInput->setText("");
  }
}

void MuonAnalysisOptionTab::plotCreationChanged(int index)
{
  // save this new choice
  QSettings group;
  group.beginGroup(m_settingsGroup + "SettingOptions");
  group.setValue("plotCreation", index);
}

void MuonAnalysisOptionTab::plotTypeChanged(int index)
{
  QSettings group;
  group.beginGroup(m_settingsGroup + "SettingOptions");
  group.setValue("connectPlotStyle", index);
}

void MuonAnalysisOptionTab::errorBarsChanged(bool state)
{
  QSettings group;
  group.beginGroup(m_settingsGroup + "SettingOptions");
  group.setValue("errorBars", state);
}

}
}
}
