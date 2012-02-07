//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/IndirectDiffractionReduction.h"

#include "MantidQtAPI/ManageUserDirectories.h"

#include <QDesktopServices>
#include <QUrl>


//Add this class to the list of specialised dialogs in this namespace
namespace MantidQt
{
namespace CustomInterfaces
{

DECLARE_SUBWINDOW(IndirectDiffractionReduction);

using namespace MantidQt::CustomInterfaces;

//----------------------
// Public member functions
//----------------------
///Constructor
IndirectDiffractionReduction::IndirectDiffractionReduction(QWidget *parent) :
  UserSubWindow(parent), m_valInt(NULL), m_valDbl(NULL)
{
}

void IndirectDiffractionReduction::demonRun()
{
  if ( validateDemon() )
  {
    if ( m_uiForm.cbInst->currentText() != "OSIRIS" )
    {
      // MSGDiffractionReduction
      QString pfile = m_uiForm.cbInst->currentText() + "_diffraction_" + m_uiForm.cbReflection->currentText() + "_Parameters.xml";
      QString pyInput =
        "from IndirectDiffractionReduction import MSGDiffractionReducer\n"
        "reducer = MSGDiffractionReducer()\n"
        "reducer.set_instrument_name('" + m_uiForm.cbInst->currentText() + "')\n"
        "reducer.set_detector_range("+m_uiForm.set_leSpecMin->text()+"-1, " +m_uiForm.set_leSpecMax->text()+"-1)\n"
        "reducer.set_parameter_file('" + pfile + "')\n"
        "files = [r'" + m_uiForm.dem_rawFiles->getFilenames().join("',r'") + "']\n"
        "for file in files:\n"
        "    reducer.append_data_file(file)\n";
        
      if ( m_uiForm.dem_ckSumFiles->isChecked() )
      {
        pyInput += "reducer.set_sum_files(True)\n";
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
      // OSIRISDiffractionReduction
    }
  }
  else
  {
    showInformationBox("Input invalid.");
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

  if ( pyOutput == "" )
  {
    showInformationBox("Could not get list of analysers from Instrument Parameter file.");
  }
  else
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

    if ( m_uiForm.cbReflection->count() > 1 )
    {
      m_uiForm.swReflections->setCurrentIndex(0);
    }
    else
    {
      m_uiForm.swReflections->setCurrentIndex(1);
    }
    
    reflectionSelected(m_uiForm.cbReflection->currentIndex());

    m_uiForm.cbReflection->blockSignals(false);

    pyInput = "from IndirectDiffractionReduction import getStringProperty\n"
      "print getStringProperty('__empty_" + m_uiForm.cbInst->currentText() + "', 'Workflow.Diffraction.Correction')\n";

    pyOutput = runPythonCode(pyInput).trimmed();

    if ( pyOutput == "Vanadium" )
    {
      m_uiForm.swVanadium->setCurrentIndex(0);
    }
    else
    {
      m_uiForm.swVanadium->setCurrentIndex(1);
    }
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
  
  m_valInt = new QIntValidator(this);
  m_valDbl = new QDoubleValidator(this);

  m_uiForm.set_leSpecMin->setValidator(m_valInt);
  m_uiForm.set_leSpecMax->setValidator(m_valInt);

  m_uiForm.leRebinStart->setValidator(m_valDbl);
  m_uiForm.leRebinWidth->setValidator(m_valDbl);
  m_uiForm.leRebinEnd->setValidator(m_valDbl);

  loadSettings();
}

void IndirectDiffractionReduction::initLocalPython()
{
  instrumentSelected(0);
}

void IndirectDiffractionReduction::loadSettings()
{
  QSettings settings;
  QString settingsGroup = "CustomInterfaces/DEMON/DataDir";
  QString dataDir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("datasearch.directories")).split(";")[0];

  settings.beginGroup(settingsGroup);
  settings.setValue("last_directory", dataDir);
  m_uiForm.dem_rawFiles->readSettings(settings.group());
  m_uiForm.dem_calFile->readSettings(settings.group());
  settings.endGroup();
}

bool IndirectDiffractionReduction::validateDemon()
{
  bool valid = true;

  if ( ! m_uiForm.dem_rawFiles->isValid() ) { valid = false; }

  QString rebin = m_uiForm.leRebinStart->text() + "," + m_uiForm.leRebinEnd->text() +
    "," + m_uiForm.leRebinEnd->text();
  if ( rebin != ",," )
  {
    // validate rebin parameters
    if ( m_uiForm.leRebinStart->text() == "" )
    {
      valid = false;
      m_uiForm.valRebinStart->setText("*");
    } else { m_uiForm.valRebinStart->setText(""); }

    if ( m_uiForm.leRebinWidth->text() == "" )
    {
      valid = false;
      m_uiForm.valRebinWidth->setText("*");
    } else { m_uiForm.valRebinWidth->setText(""); }

    if ( m_uiForm.leRebinEnd->text() == "" )
    {
      valid = false;
      m_uiForm.valRebinEnd->setText("*");
    } else { m_uiForm.valRebinEnd->setText(""); }

    if ( m_uiForm.leRebinStart->text().toDouble() > m_uiForm.leRebinEnd->text().toDouble() )
    {
      valid = false;
      m_uiForm.valRebinStart->setText("*");
      m_uiForm.valRebinEnd->setText("*");
    }
  }

  return valid;
}

}
}