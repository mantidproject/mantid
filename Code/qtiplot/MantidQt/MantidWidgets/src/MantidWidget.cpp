#include "MantidQtMantidWidgets/MantidWidget.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/PropertyWithValue.h"
#include <QPalette>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

void MantidWidget::renameWorkspace(const QString &oldName, const QString &newName)
{
  IAlgorithm_sptr rename =
    AlgorithmManager::Instance().createUnmanaged("RenameWorkspace");
  rename->initialize();
  rename->setPropertyValue("InputWorkspace", oldName.toStdString());
  rename->setPropertyValue("OutputWorkspace", newName.toStdString());

  rename->execute();
}
void MantidWidget::setupValidator(QLabel *star)
{
  QPalette pal = star->palette();
  pal.setColor(QPalette::WindowText, Qt::darkRed);
  star->setPalette(pal);
}
QLabel* MantidWidget::newStar(const QGroupBox * const UI, int valRow, int valCol)
{// use new to create the QLabel the layout will take ownership and delete it later
  QLabel *validLbl = new QLabel("*");
  setupValidator(validLbl);
  // link the validator into the location specified by the user
  QGridLayout *grid = qobject_cast<QGridLayout*>(UI->layout());
  grid->addWidget(validLbl, valRow, valCol);
  return validLbl;
}
QLabel* MantidWidget::newStar(QGridLayout * const lay, int valRow, int valCol)
{// use new to create the QLabel the layout will take ownership and delete it later
  QLabel *validLbl = new QLabel("*");
  setupValidator(validLbl);
  // link the validator into the location specified by the user
  lay->addWidget(validLbl, valRow, valCol);
  return validLbl;
}
void MantidWidget::hideValidators()
{// loop through all the validators in the map
  QHash<const QWidget * const, QLabel *>::iterator
	  vali = m_validators.begin();
  for ( ; vali != m_validators.end(); ++vali)
  {
    vali.value()->hide();    
  }
}