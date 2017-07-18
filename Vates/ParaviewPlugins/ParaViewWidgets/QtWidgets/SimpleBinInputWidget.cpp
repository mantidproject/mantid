#include "SimpleBinInputWidget.h"
#include <QLabel>
#include <QLineEdit>
#include <QBoxLayout>
#include <QIntValidator>
#include <sstream>

/**
Constructor
*/
SimpleBinInputWidget::SimpleBinInputWidget() {
  QLabel *binLabel = new QLabel("Bins");

  QIntValidator *validator = new QIntValidator;
  validator->setBottom(2);
  validator->setTop(1000);

  m_nBinsBox = new QLineEdit;
  m_nBinsBox->setValidator(validator);
  QHBoxLayout *layout = new QHBoxLayout;

  layout->addWidget(binLabel);
  layout->addWidget(m_nBinsBox);

  this->setLayout(layout);
  connect(m_nBinsBox, SIGNAL(editingFinished()), this, SLOT(nBinsListener()));
}

/**
Entry setter.
@param value : value that the entry should take.
*/
void SimpleBinInputWidget::setEntry(int nBins, double, double) {
  std::stringstream stream;
  stream << nBins;
  m_nBinsBox->setText(stream.str().c_str());
}

/**
Getter for the current entry.
@return current entry value
*/
int SimpleBinInputWidget::getEntry(double, double) const {
  return std::stoi(std::string(m_nBinsBox->text().toAscii()));
}

/// Destructor
SimpleBinInputWidget::~SimpleBinInputWidget() {}

/// Listener and emitter for the number of having changed.
void SimpleBinInputWidget::nBinsListener() {
  // Raise an event that is published publically
  emit valueChanged();
}
