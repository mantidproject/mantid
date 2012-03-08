#ifndef SIMPLE_BIN_INPUT_WIDGET_H
#define SIMPLE_BIN_INPUT_WIDGET_H 

#include "BinInputWidget.h"
#include <QLabel>
#include <QLineEdit>
#include <QBoxLayout>
#include <sstream>

class SimpleBinInputWidget : public BinInputWidget
{
  Q_OBJECT

public:

  SimpleBinInputWidget()
  {
    QLabel* binLabel = new QLabel("Bins");
    m_nBinsBox = new QLineEdit;

    QHBoxLayout* layout = new QHBoxLayout;
    
    layout->addWidget(binLabel);
    layout->addWidget(m_nBinsBox);

    this->setLayout(layout);
    connect(m_nBinsBox, SIGNAL(editingFinished()), this, SLOT(nBinsListener()));
  }
  virtual void setValue(int value)
  {
    std::stringstream stream;
    stream << value;
    m_nBinsBox->setText(stream.str().c_str());
  }
  virtual int getNBins() const
  {
    return atoi(m_nBinsBox->text());
  }
  ~SimpleBinInputWidget()
  {
  }
private slots:
  void nBinsListener()
  {
    emit valueChanged();
  }
private:
   QLineEdit* m_nBinsBox;
};

#endif