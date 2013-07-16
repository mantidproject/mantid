#include "MantidQtCustomDialogs/SmoothNeighboursDialog.h"

//Register the class with the factory
DECLARE_DIALOG(SmoothNeighboursDialog)
 
SmoothNeighboursDialog::SmoothNeighboursDialog(QWidget* parent) 
  : AlgorithmDialog(parent)
{
}
 
void SmoothNeighboursDialog::initLayout()
{
  // Set main layout
  QVBoxLayout *dialog_layout = new QVBoxLayout();
  setLayout(dialog_layout);

  // Create and add widget with all the properties
  m_algPropertiesWidget = new AlgorithmPropertiesWidget(this);
  m_algPropertiesWidget->setAlgorithm(this->getAlgorithm());
  m_algPropertiesWidget->hideOrDisableProperties();
  dialog_layout->addWidget(m_algPropertiesWidget, 1);

  // Create and add the OK/Cancel/Help. buttons
  dialog_layout->addLayout(this->createDefaultButtonLayout(), 0);
}

void SmoothNeighboursDialog::parseInput()
{
  // TODO: implement
}