//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/IndirectDiffractionReduction.h"

#include "MantidQtAPI/ManageUserDirectories.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/MultiFileNameParser.h"

#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>

//Add this class to the list of specialised dialogs in this namespace
namespace MantidQt
{
namespace CustomInterfaces
{

namespace // anon
{
  /// static logger
  Mantid::Kernel::Logger g_log("IndirectDiffractionReduction");

  // Helper function for use with std::transform.
  std::string toStdString(const QString & qString)
  {
    return qString.toStdString();
  }
} // anon namespace

DECLARE_SUBWINDOW(IndirectDiffractionReduction);

using namespace MantidQt::CustomInterfaces;

//----------------------
// Public member functions
//----------------------
///Constructor
IndirectDiffractionReduction::IndirectDiffractionReduction(QWidget *parent) :
  UserSubWindow(parent), m_valInt(NULL), m_valDbl(NULL), m_settingsGroup("CustomInterfaces/DEMON")
{
}

///Destructor
IndirectDiffractionReduction::~IndirectDiffractionReduction()
{
  saveSettings();
}

void IndirectDiffractionReduction::demonRun()
{
  if ( !validateDemon() )
  {
    showInformationBox("Invalid invalid. Incorrect entries marked with red star.");
    return;
  }

  QString instName=m_uiForm.cbInst->currentText();
  QString mode = m_uiForm.cbReflection->currentText();
  if ( instName != "OSIRIS" || mode == "diffspec" )
  {
    // MSGDiffractionReduction
    QString pfile = instName + "_diffraction_" + mode + "_Parameters.xml";
    QString pyInput =
      "from IndirectDiffractionReduction import MSGDiffractionReducer\n"
      "reducer = MSGDiffractionReducer()\n"
      "reducer.set_instrument_name('" + instName + "')\n"
      "reducer.set_detector_range("+m_uiForm.set_leSpecMin->text()+"-1, " +m_uiForm.set_leSpecMax->text()+"-1)\n"
      "reducer.set_parameter_file('" + pfile + "')\n"
      "files = [r'" + m_uiForm.dem_rawFiles->getFilenames().join("',r'") + "']\n"
      "for file in files:\n"
      "    reducer.append_data_file(file)\n";
    // Fix Vesuvio to FoilOut for now
    if(instName == "VESUVIO")
      pyInput += "reducer.append_load_option('Mode','FoilOut')\n";
        
    if ( m_uiForm.dem_ckSumFiles->isChecked() )
    {
      pyInput += "reducer.set_sum_files(True)\n";
    }

    //if we're using a can, add it to the reduction and set the scaling factor
    if(m_uiForm.ckUseCan->isChecked())
    {
      QFileInfo finfo(m_uiForm.dem_canFile->getFirstFilename());
      pyInput += "reducer.set_container('" + finfo.baseName() + "')\n";
      //can is reduced as well, then dedcuted in post processing
      pyInput += "reducer.append_data_file('" + m_uiForm.dem_canFile->getFirstFilename()+ "')\n";

      if(m_uiForm.ckScaleCan->isChecked())
      {
        pyInput += "reducer.set_container_scale_factor(" + m_uiForm.le_scaleFactor->text() + ")\n";
      }
    }

    pyInput += "formats = []\n";
    if ( m_uiForm.ckGSS->isChecked() ) pyInput += "formats.append('gss')\n";
    if ( m_uiForm.ckNexus->isChecked() ) pyInput += "formats.append('nxs')\n";
    if ( m_uiForm.ckAscii->isChecked() ) pyInput += "formats.append('ascii')\n";

    QString rebin = m_uiForm.leRebinStart->text() + "," + m_uiForm.leRebinWidth->text() 
      + "," + m_uiForm.leRebinEnd->text();
    if ( rebin != ",," )
    {
      pyInput += "reducer.set_rebin_string('" + rebin +"')\n";
    }

    pyInput += "reducer.set_save_formats(formats)\n";
    pyInput +=
      "reducer.reduce()\n";

    if ( m_uiForm.cbPlotType->currentText() == "Spectra" )
    {
      pyInput += "wslist = reducer.get_result_workspaces()\n"
        "from mantidplot import *\n"
        "plotSpectrum(wslist, 0)\n";
    }

    QString pyOutput = runPythonCode(pyInput).trimmed();
  }
  else
  {
    // Get the files names from MWRunFiles widget, and convert them from Qt forms into stl equivalents.
    QStringList fileNames = m_uiForm.dem_rawFiles->getFilenames();
    std::vector<std::string> stlFileNames;
    stlFileNames.reserve(fileNames.size());
    std::transform(fileNames.begin(),fileNames.end(),std::back_inserter(stlFileNames), toStdString);

    // Use the file names to suggest a workspace name to use.  Report to logger and stop if unable to parse correctly.
    QString drangeWsName;
    QString tofWsName;
    try
    {
      QString nameBase = QString::fromStdString(Mantid::Kernel::MultiFileNameParsing::suggestWorkspaceName(stlFileNames));
      tofWsName = "'" + nameBase + "_tof'";
      drangeWsName = "'" + nameBase + "_dRange'";
    }
    catch(std::runtime_error & re)
    {
      g_log.error(re.what());
      return;
    }

    QString pyInput = 
      "from mantid.simpleapi import *\n"
      "OSIRISDiffractionReduction("
      "Sample=r'" + m_uiForm.dem_rawFiles->getFilenames().join(", ") + "', "
      "Vanadium=r'" + m_uiForm.dem_vanadiumFile->getFilenames().join(", ") + "', "
      "CalFile=r'" + m_uiForm.dem_calFile->getFirstFilename() + "', "
      "OutputWorkspace=" + drangeWsName + ")\n";

    pyInput += "ConvertUnits(InputWorkspace=" + drangeWsName + ", OutputWorkspace=" + tofWsName + ", Target='TOF')\n";
    
    if ( m_uiForm.ckGSS->isChecked() )
    {
      pyInput += "SaveGSS(InputWorkspace=" + tofWsName + ", Filename=" + tofWsName + " + '.gss')\n";
    }

    if ( m_uiForm.ckNexus->isChecked() ) 
      pyInput += "SaveNexusProcessed(InputWorkspace=" + drangeWsName + ", Filename=" + drangeWsName + "+'.nxs')\n";

    if ( m_uiForm.ckAscii->isChecked() ) 
      pyInput += "SaveAscii(InputWorkspace=" + drangeWsName + ", Filename=" + drangeWsName + " +'.dat')\n";

    if ( m_uiForm.cbPlotType->currentText() == "Spectra" )
    {
      pyInput += "from mantidplot import *\n"
        "plotSpectrum(" + drangeWsName + ", 0)\n"
        "plotSpectrum(" + tofWsName + ", 0)\n";
    }

    QString pyOutput = runPythonCode(pyInput).trimmed();
  }
}

void IndirectDiffractionReduction::instrumentSelected(int)
{
  if ( ! m_uiForm.cbInst->isVisible() )
  {
    // If the interface is not shown, do not go looking for parameter files, etc.
    return;
  }

  m_uiForm.cbReflection->blockSignals(true);
  m_uiForm.cbReflection->clear();

  QString pyInput = 
    "from IndirectEnergyConversion import getInstrumentDetails\n"
    "print getInstrumentDetails('" + m_uiForm.cbInst->currentText() + "')\n";

  QString pyOutput = runPythonCode(pyInput).trimmed();

  if(pyOutput.length() > 0)
  {
    QStringList analysers = pyOutput.split("\n", QString::SkipEmptyParts);

    for (int i = 0; i< analysers.count(); i++ )
    {
      QStringList analyser = analysers[i].split("-", QString::SkipEmptyParts);
      if ( analyser[0] == "diffraction" && analyser.count() > 1)
      {
        QStringList reflections = analyser[1].split(",", QString::SkipEmptyParts);
        for ( int j = 0; j < reflections.count(); j++ )
        {
          m_uiForm.cbReflection->addItem(reflections[j]);
        }
      }
    }
  }

  reflectionSelected(m_uiForm.cbReflection->currentIndex());
  m_uiForm.cbReflection->blockSignals(false);

  // Disable summing file options for OSIRIS.
  if ( m_uiForm.cbInst->currentText() != "OSIRIS" )
    m_uiForm.dem_ckSumFiles->setEnabled(true);
  else
  {
    m_uiForm.dem_ckSumFiles->setChecked(true);
    m_uiForm.dem_ckSumFiles->setEnabled(false);
  }
}

void IndirectDiffractionReduction::reflectionSelected(int)
{
  QString pyInput =
    "from IndirectEnergyConversion import getReflectionDetails\n"
    "instrument = '" + m_uiForm.cbInst->currentText() + "'\n"
    "reflection = '" + m_uiForm.cbReflection->currentText() + "'\n"
    "print getReflectionDetails(instrument, 'diffraction', reflection)\n";

  QString pyOutput = runPythonCode(pyInput).trimmed();
  QStringList values = pyOutput.split("\n", QString::SkipEmptyParts);

  if ( values.count() < 3 )
  {
    showInformationBox("Could not gather necessary data from parameter file.");
    return;
  }
  else
  {
    QString analysisType = values[0];
    m_uiForm.set_leSpecMin->setText(values[1]);
    m_uiForm.set_leSpecMax->setText(values[2]);
  }

  // Determine whether we need vanadium input
  pyInput = "from IndirectDiffractionReduction import getStringProperty\n"
      "print getStringProperty('__empty_" + m_uiForm.cbInst->currentText() + "', 'Workflow.Diffraction.Correction')\n";

  pyOutput = runPythonCode(pyInput).trimmed();

  bool showVanadiumInput = (pyOutput == "Vanadium");
  m_uiForm.gbCanInput->setVisible(!showVanadiumInput);

  if ( showVanadiumInput )
  {
    m_uiForm.swVanadium->setCurrentIndex(0);
  }
  else
  {
    m_uiForm.swVanadium->setCurrentIndex(1);
  }
}

void IndirectDiffractionReduction::openDirectoryDialog()
{
  MantidQt::API::ManageUserDirectories *ad = new MantidQt::API::ManageUserDirectories(this);
  ad->show();
  ad->setFocus();
}

void IndirectDiffractionReduction::help()
{
  QString url = "http://www.mantidproject.org/Indirect_Diffraction_Reduction";
  QDesktopServices::openUrl(QUrl(url));
}

void IndirectDiffractionReduction::initLayout()
{
  m_uiForm.setupUi(this);

  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(help()));
  connect(m_uiForm.pbManageDirs, SIGNAL(clicked()), this, SLOT(openDirectoryDialog()));
  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(demonRun()));

  connect(m_uiForm.cbInst, SIGNAL(currentIndexChanged(int)), this, SLOT(instrumentSelected(int)));
  connect(m_uiForm.cbReflection, SIGNAL(currentIndexChanged(int)), this, SLOT(reflectionSelected(int)));
  connect(m_uiForm.ckUseCan, SIGNAL(toggled(bool)), m_uiForm.dem_canFile, SLOT(setEnabled(bool)));
  connect(m_uiForm.ckUseCan, SIGNAL(toggled(bool)), m_uiForm.ckScaleCan, SLOT(setEnabled(bool)));
  connect(m_uiForm.ckUseCan, SIGNAL(toggled(bool)), this, SLOT(scaleMultiplierCheck(bool)));
  connect(m_uiForm.ckScaleCan, SIGNAL(toggled(bool)), m_uiForm.le_scaleFactor, SLOT(setEnabled(bool)));

  m_valInt = new QIntValidator(this);
  m_valDbl = new QDoubleValidator(this);

  m_uiForm.le_scaleFactor->setValidator(m_valDbl);
  m_uiForm.set_leSpecMin->setValidator(m_valInt);
  m_uiForm.set_leSpecMax->setValidator(m_valInt);

  m_uiForm.leRebinStart->setValidator(m_valDbl);
  m_uiForm.leRebinWidth->setValidator(m_valDbl);
  m_uiForm.leRebinEnd->setValidator(m_valDbl);

  loadSettings();

  validateDemon();
}

void IndirectDiffractionReduction::initLocalPython()
{
  instrumentSelected(0);
}

void IndirectDiffractionReduction::loadSettings()
{
  QSettings settings;
  QString dataDir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("datasearch.directories")).split(";")[0];

  settings.beginGroup(m_settingsGroup);
  settings.setValue("last_directory", dataDir);
  m_uiForm.dem_rawFiles->readSettings(settings.group());
  m_uiForm.dem_calFile->readSettings(settings.group());
  m_uiForm.dem_calFile->setUserInput(settings.value("last_cal_file").toString());
  m_uiForm.dem_vanadiumFile->setUserInput(settings.value("last_van_files").toString());
  settings.endGroup();
  
}

void IndirectDiffractionReduction::saveSettings()
{
  QSettings settings;

  settings.beginGroup(m_settingsGroup);
  settings.setValue("last_cal_file", m_uiForm.dem_calFile->getText());
  settings.setValue("last_van_files", m_uiForm.dem_vanadiumFile->getText());
  settings.endGroup();
}

/**
* Disables/enables the relevant parts of the UI when user checks/unchecks the 'Scale' option
* m_uiForm.ckScaleCan checkbox.
* @param state :: state of the checkbox
*/
void IndirectDiffractionReduction::scaleMultiplierCheck(bool state)
{
  //scale input should be disabled if we're not using a can
  if(!m_uiForm.ckUseCan->isChecked())
  {
    m_uiForm.le_scaleFactor->setEnabled(false);
  }
  else
  {
    //else it should be whatever the scale checkbox is
    state = m_uiForm.ckScaleCan->isChecked();
    m_uiForm.le_scaleFactor->setEnabled(state);
  }
}

bool IndirectDiffractionReduction::validateDemon()
{
  UserInputValidator uiv;
  bool rawValid = true;
  if ( ! m_uiForm.dem_rawFiles->isValid() ) { rawValid = false; }

  if( m_uiForm.ckUseCan->isChecked() && m_uiForm.ckUseCan->isVisible())
  {
    uiv.checkMWRunFilesIsValid("Can File", m_uiForm.dem_canFile);

    if(m_uiForm.ckScaleCan->isChecked() && m_uiForm.le_scaleFactor->text().isEmpty())
    {
      uiv.addErrorMessage("Invalid value for can scale factor.");
    }
  }

  QString rebStartTxt = m_uiForm.leRebinStart->text();
  QString rebStepTxt = m_uiForm.leRebinWidth->text();
  QString rebEndTxt = m_uiForm.leRebinEnd->text();

  bool rebinValid = true;
  // Need all or none
  if(rebStartTxt.isEmpty() && rebStepTxt.isEmpty() && rebEndTxt.isEmpty() )
  {
    rebinValid = true;
    m_uiForm.valRebinStart->setText("");
    m_uiForm.valRebinWidth->setText("");
    m_uiForm.valRebinEnd->setText("");
  }
  else
  {
#define CHECK_VALID(text,validator)\
    if(text.isEmpty())\
    {\
      rebinValid = false;\
      validator->setText("*");\
    }\
    else\
    {\
      rebinValid = true;\
      validator->setText("");\
    }

    CHECK_VALID(rebStartTxt,m_uiForm.valRebinStart);
    CHECK_VALID(rebStepTxt,m_uiForm.valRebinWidth);
    CHECK_VALID(rebEndTxt,m_uiForm.valRebinEnd);

    if(rebinValid && rebStartTxt.toDouble() > rebEndTxt.toDouble())
    {
      rebinValid = false;
      m_uiForm.valRebinStart->setText("*");
      m_uiForm.valRebinEnd->setText("*");
    }
  }

  return rawValid && rebinValid && uiv.generateErrorMessage().isEmpty();
}

}
}
