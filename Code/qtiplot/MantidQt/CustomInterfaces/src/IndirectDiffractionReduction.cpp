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
  UserSubWindow(parent)
{
}

void IndirectDiffractionReduction::demonRun()
{
  if ( validateDemon() )
  {
    QString pyInput = "from IndirectDataAnalysis import demon\n"
      "files = [r'" + m_uiForm.dem_rawFiles->getFilenames().join("',r'") + "']\n"
      "first = " +m_uiForm.set_leSpecMin->text()+"\n"
      "last = " +m_uiForm.set_leSpecMax->text()+"\n";

    pyInput += "plot = '" + m_uiForm.cbPlotType->currentText() + "'\n";

    if ( m_uiForm.dem_ckVerbose->isChecked() ) pyInput += "verbose = True\n";
    else pyInput += "verbose = False\n";

    if ( m_uiForm.dem_ckSave->isChecked() ) pyInput += "save = True\n";
    else pyInput += "save = False\n";
    
    pyInput += "ws, rn = demon(files, first, last, Verbose=verbose, Plot=plot, Save=save)\n";

    QString pyOutput = runPythonCode(pyInput).trimmed();
  }
  else
  {
    showInformationBox("Input invalid.");
  }
}

void IndirectDiffractionReduction::instrumentSelected(int)
{
  m_uiForm.set_cbReflection->blockSignals(true);
  m_uiForm.set_cbReflection->clear();

  QString pyInput = 
    "from IndirectEnergyConversion import getInstrumentDetails\n"
    "result = getInstrumentDetails('" + m_uiForm.set_cbInst->currentText() + "')\n"
    "print result\n";

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
          m_uiForm.set_cbReflection->addItem(reflections[j]);
        }        
      }
    }

    reflectionSelected(m_uiForm.set_cbReflection->currentIndex());

    m_uiForm.set_cbReflection->blockSignals(false);
  }
}

void IndirectDiffractionReduction::reflectionSelected(int)
{
  QString pyInput =
    "from IndirectEnergyConversion import getReflectionDetails\n"
    "instrument = '" + m_uiForm.set_cbInst->currentText() + "'\n"
    "reflection = '" + m_uiForm.set_cbReflection->currentText() + "'\n"
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
  ad->setAttribute(Qt::WA_DeleteOnClose);
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

  connect(m_uiForm.set_cbInst, SIGNAL(currentIndexChanged(int)), this, SLOT(instrumentSelected(int)));
  connect(m_uiForm.set_cbReflection, SIGNAL(currentIndexChanged(int)), this, SLOT(reflectionSelected(int)));

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
  settings.endGroup();
}

bool IndirectDiffractionReduction::validateDemon()
{
  return m_uiForm.dem_rawFiles->isValid();
}

}
}
