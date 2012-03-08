#include "SimpleBinInputWidget.h"
#include <QLabel>
#include <QLineEdit>
#include <QBoxLayout>
#include <sstream>

/**
Constructor
*/
SimpleBinInputWidget::SimpleBinInputWidget()
{
  QLabel* binLabel = new QLabel("Bins");
  m_nBinsBox = new QLineEdit;
  QHBoxLayout* layout = new QHBoxLayout;

  layout->addWidget(binLabel);
  layout->addWidget(m_nBinsBox);

  this->setLayout(layout);
  connect(m_nBinsBox, SIGNAL(editingFinished()), this, SLOT(nBinsListener()));
}

/**
Entry setter.
@param value : value that the entry should take.
*/
void SimpleBinInputWidget::entry(int value)
{
  std::stringstream stream;
  stream << value;
  m_nBinsBox->setText(stream.str().c_str());
}

/**
Getter for the current entry.
@return current entry value
*/
int SimpleBinInputWidget::entered() const
{
  return atoi(m_nBinsBox->text());
}

/// Destructor
SimpleBinInputWidget::~SimpleBinInputWidget()
{
}

/// Listener and emitter for the number of having changed.
void SimpleBinInputWidget::nBinsListener()
{
 // Raise an event that is published publically
  emit valueChanged();
}