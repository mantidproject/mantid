#ifndef BIN_INPUT_WIDGET_H_
#define BIN_INPUT_WIDGET_H_ 

#include <QWidget>

class BinInputWidget : public QWidget
{
  Q_OBJECT
public:
  virtual int getNBins() const = 0;
  virtual void setValue(int value) = 0;
  virtual ~BinInputWidget(){}
Q_SIGNALS:
  virtual void valueChanged();
};

#endif
