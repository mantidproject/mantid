//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/Background.h"
#include "MantidAPI/FrameworkManager.h"
#include <QLabel>
#include <QHBoxLayout>
#include <QPushButton>

using namespace MantidQt::CustomInterfaces;

//----------------------
// Public member functions
//----------------------
/** Sets up all the controls in the dialog box using the settings found under
*  the passed group string, the settings must be setup before this called
*  @param parent used by QT
*  @param settingGrp the initial part of the string containing the user settings
*/
Background::Background(QWidget *parent, const QString &settingsGrp) :
  API::MantidQtDialog(parent), m_ckDoRemove(new QCheckBox),
  m_leStart(new QLineEdit), m_leEnd(new QLineEdit)
{
  setWindowTitle("Background Removal Settings");

  QGridLayout *grid = new QGridLayout;

  m_ckDoRemove->setText("Remove background as found between these points");

  QLabel *lbStart = new QLabel("TOF Start");
  QLabel *lbEnd = new QLabel("TOF End");
  
  QPushButton *pbOK = new QPushButton("OK");
  QPushButton *pbCancel = new QPushButton("Cancel");
  
  grid->addWidget(m_ckDoRemove, 0, 0, 1, 3);
  grid->addWidget(lbStart, 1, 0);
  grid->addWidget(m_leStart, 1, 1);
  grid->addWidget(lbEnd, 1, 2);
  grid->addWidget(m_leEnd, 1, 3);
  grid->addWidget(pbOK, 2, 1);
  grid->addWidget(pbCancel, 2, 2);
  
  // the settings differ depending on the instrument
  m_prevSets.beginGroup(settingsGrp);
  loadSettings();
  
  connect(pbOK, SIGNAL(clicked()), this, SLOT(saveSettings()));
  connect(pbCancel, SIGNAL(clicked()), this, SLOT(close()));

  setLayout(grid);

  setAttribute(Qt::WA_DeleteOnClose);
}

/// Set up the dialog layout
void Background::initLayout()
{
}
//----------------------
// Private member functions
//----------------------
/** these settings were either those that the user entered previously or
*  the default values
*/
void Background::loadSettings()
{
  bool doRemove = m_prevSets.value("bgremove").toString() !=
	"bg removal: none";
  m_ckDoRemove->setChecked(doRemove);
  double TOF = m_prevSets.value("TOFstart").toDouble();
  m_leStart->setText(QString::number(TOF));
  TOF = m_prevSets.value("TOFend").toDouble();
  m_leEnd->setText(QString::number(TOF));
}
/** this is run when the user clicks OK, it reads the user entered values,
*  saves them and sends them back to the form
*  @throw invalid_argument if the entry for one of the TOF values can't be converted to a number
*/
void Background::saveSettings()
{  
  bool noError;
  double start = m_leStart->text().toDouble(&noError);
  if ( ! noError )
  {
    throw std::invalid_argument("Can't convert " + m_leStart->text().toStdString() + " to a number");
  }  
  double end = m_leEnd->text().toDouble(&noError);
  if ( ! noError )
  {
    throw std::invalid_argument("Can't convert " + m_leEnd->text().toStdString() + " to a number");
  }

  m_prevSets.setValue("TOFstart", start);
  m_prevSets.setValue("TOFend", end);

  if (m_ckDoRemove->isChecked())
  {
    m_prevSets.setValue("bgremove", "bg removal: on");
  }
  else m_prevSets.setValue("bgremove", "bg removal: none");

  // this does a emit closeEvent() and closes the dialog
  close();
}
/** emits a signal with the user selected values
*  @param event a point passed from QT
*/
void Background::closeEvent(QCloseEvent *event)
{
  emit formClosed();

  event->accept();
}