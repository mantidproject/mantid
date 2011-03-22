//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/Background.h"
#include "MantidAPI/FrameworkManager.h"
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QDoubleValidator>
#include <QCloseEvent>
#include <QShowEvent>

#include <QMessageBox>

using namespace MantidQt::CustomInterfaces;

//----------------------
// Public member functions
//----------------------
/** Sets up all the controls in the dialog box using the settings found under
 *  the passed group string, the settings must be setup before this called
 *  @param parent :: used by QT
 */
Background::Background(QWidget *parent) :
  API::MantidDialog(parent), m_ckDoRemove(new QCheckBox),
  m_leStart(new QLineEdit), m_leEnd(new QLineEdit), m_rangeMin(-1.0), 
  m_rangeMax(-1.0), m_doRemoval(false)
{
  setWindowTitle("Background Removal Settings");

  QGridLayout *grid = new QGridLayout;

  m_ckDoRemove->setText("Remove background as found between these points");
  QHBoxLayout *lineOne = new QHBoxLayout;
  lineOne->addWidget(m_ckDoRemove);
  lineOne->addStretch();

  QLabel *lbStart = new QLabel("TOF Start");
  QLabel *lbEnd = new QLabel("TOF End");
  m_leStart->setValidator(new QDoubleValidator(this));
  m_leEnd->setValidator(new QDoubleValidator(this));
  m_ckDoRemove->setChecked(false);

  QHBoxLayout *lineTwo = new QHBoxLayout;
  lineTwo->addStretch();
  lineTwo->addWidget(lbStart);
  lineTwo->addWidget(m_leStart);
  lineTwo->addWidget(lbEnd);
  lineTwo->addWidget(m_leEnd);
  lineTwo->addStretch();

  QPushButton *pbOK = new QPushButton("OK");
  QPushButton *pbCancel = new QPushButton("Cancel");
  connect(pbCancel, SIGNAL(clicked()), this, SLOT(reject()));
  connect(pbOK, SIGNAL(clicked()), this, SLOT(close()));
   QHBoxLayout *lineThree = new QHBoxLayout;
  lineThree->addStretch();
  lineThree->addWidget(pbOK);
  lineThree->addWidget(pbCancel);
 
  QVBoxLayout *dialogLayout = new QVBoxLayout;
  dialogLayout->addLayout(lineOne);
  dialogLayout->addLayout(lineTwo);
  dialogLayout->addLayout(lineThree);
   
  setLayout(dialogLayout);
}

//----------------------------------------------------
// Public member functions
//----------------------------------------------------

/**
 * Whether we are removing background or not
 * @returns A boolean indicating whether the background should be removed or not
 */
bool Background::removeBackground() const
{
  return m_doRemoval;
}

/**
 * Set whether to remove the background or not
 * @param remove :: If true, the background will be removed
 */
void Background::removeBackground(bool remove)
{
   m_doRemoval = remove;
}

/**
 * Retrieve the time-of-flight range from the dialog
 * @returns A pair containing the TOF range
 */
QPair<double, double> Background::getRange() const
{
  return QPair<double, double>(m_rangeMin, m_rangeMax);
}

/**
 * Set the background range
 * @param min :: Minimum value
 * @param max :: Maximum value
 */
void Background::setRange(double min, double max)
{
  m_rangeMin = min;
  m_rangeMax = max;
}

//----------------------------------------------------
// Public member functions
//----------------------------------------------------

/// Set up the dialog layout
void Background::initLayout()
{
}

/**
 * Called in response to a show() event
 * @param event :: The event details
 */
void Background::showEvent(QShowEvent* e)
{
  m_leStart->setText(QString::number(m_rangeMin));
  m_leEnd->setText(QString::number(m_rangeMax));
  m_ckDoRemove->setChecked(m_doRemoval);
  sanityCheck();
  e->accept();
}

/**
 * Called in response to a close event
 * @parma event The event details
 */
void Background::closeEvent(QCloseEvent* event)
{
  if( sanityCheck() )
  {
    m_doRemoval = m_ckDoRemove->isChecked();
    m_rangeMin = m_leStart->text().toDouble();;
    m_rangeMax = m_leEnd->text().toDouble();
    event->accept();
    emit accepted();
  }
  else
  {
    event->ignore();
  }
}

bool Background::sanityCheck()
{
  double min = m_leStart->text().toDouble();
  double max = m_leEnd->text().toDouble();
  if( m_ckDoRemove->isChecked() && min > max )
  {
    m_leStart->setStyleSheet("background-color: red");
    m_leEnd->setStyleSheet("background-color: red");
    return false;
  }
  else
  {
    m_leStart->setStyleSheet("background-color: white");
    m_leEnd->setStyleSheet("background-color: white");
    return true;
  }
}


