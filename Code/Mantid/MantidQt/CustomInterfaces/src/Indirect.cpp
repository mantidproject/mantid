#include "MantidQtCustomInterfaces/Indirect.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/ConfigService.h"
#include "MantidQtCustomInterfaces/Background.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"

#include <cmath>

#include <Poco/NObserver.h>

#include <QtCheckBoxFactory>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QInputDialog>
#include <QLineEdit>
#include <QUrl>

// Suppress a warning coming out of code that isn't ours
#if defined(__INTEL_COMPILER)
  #pragma warning disable 1125
#elif defined(__GNUC__)
  #if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6 )
    #pragma GCC diagnostic push
  #endif
  #pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
#include "qttreepropertybrowser.h"
#include "qtpropertymanager.h"
#include "qteditorfactory.h"
#include "DoubleEditorFactory.h"
#if defined(__INTEL_COMPILER)
  #pragma warning enable 1125
#elif defined(__GNUC__)
  #if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6 )
    #pragma GCC diagnostic pop
  #endif
#endif

using namespace MantidQt::CustomInterfaces;
using namespace MantidQt;

/**
* This is the constructor for the Indirect Instruments Interface.
* It is used primarily to ensure sane values for member variables.
*/
Indirect::Indirect(QWidget *parent, Ui::IndirectDataReduction & uiForm) : 
  UserSubWindow(parent), m_uiForm(uiForm),
  m_changeObserver(*this, &Indirect::handleDirectoryChange),

  // Additional tab interfaces
  m_tab_convert_to_energy(new IndirectConvertToEnergy(m_uiForm, this)),
  m_tab_sqw(new IndirectSqw(m_uiForm, this)),
  m_tab_diagnostics(new IndirectDiagnostics(m_uiForm, this)),
  m_tab_calibration(new IndirectCalibration(m_uiForm, this)),
  m_tab_trans(new IndirectTransmission(m_uiForm, this)),
  m_tab_moments(new IndirectMoments(m_uiForm, this))
{
}

/**
* This function performs any one-time actions needed when the Inelastic interface
* is first selected, such as connecting signals to slots.
*/
void Indirect::initLayout()
{
  Mantid::Kernel::ConfigService::Instance().addObserver(m_changeObserver);

  m_settingsGroup = "CustomInterfaces/ConvertToEnergy/Indirect/";

  //All tab signals
  connect(m_tab_convert_to_energy, SIGNAL(runAsPythonScript(const QString&, bool)), this, SIGNAL(runAsPythonScript(const QString&, bool)));
  connect(m_tab_sqw, SIGNAL(runAsPythonScript(const QString&, bool)), this, SIGNAL(runAsPythonScript(const QString&, bool)));
  connect(m_tab_calibration, SIGNAL(runAsPythonScript(const QString&, bool)), this, SIGNAL(runAsPythonScript(const QString&, bool)));
  connect(m_tab_diagnostics, SIGNAL(runAsPythonScript(const QString&, bool)), this, SIGNAL(runAsPythonScript(const QString&, bool)));
  connect(m_tab_trans, SIGNAL(runAsPythonScript(const QString&, bool)), this, SIGNAL(runAsPythonScript(const QString&, bool)));
  connect(m_tab_moments, SIGNAL(runAsPythonScript(const QString&, bool)), this, SIGNAL(runAsPythonScript(const QString&, bool)));

  connect(m_tab_convert_to_energy, SIGNAL(showMessageBox(const QString&)), this, SLOT(showMessageBox(const QString&)));
  connect(m_tab_sqw, SIGNAL(showMessageBox(const QString&)), this, SLOT(showMessageBox(const QString&)));
  connect(m_tab_calibration, SIGNAL(showMessageBox(const QString&)), this, SLOT(showMessageBox(const QString&)));
  connect(m_tab_diagnostics, SIGNAL(showMessageBox(const QString&)), this, SLOT(showMessageBox(const QString&)));
  connect(m_tab_trans, SIGNAL(showMessageBox(const QString&)), this, SLOT(showMessageBox(const QString&)));
  connect(m_tab_moments, SIGNAL(showMessageBox(const QString&)), this, SLOT(showMessageBox(const QString&)));

  // set default values for save formats
  m_uiForm.save_ckSPE->setChecked(false);
  m_uiForm.save_ckNexus->setChecked(true);

  loadSettings();
}

/**
* This function will hold any Python-dependent setup actions for the interface.
*/
void Indirect::initLocalPython()
{
}

/**
* This function opens a web browser window to the Mantid Project wiki page for this
* interface ("Inelastic" subsection of ConvertToEnergy).
*/
void Indirect::helpClicked()
{
}

/**
* This function will control the actions needed for the Indirect interface when the
* "Run" button is clicked by the user.
*/
void Indirect::runClicked()
{
}

