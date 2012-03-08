#ifndef BIN_INPUT_WIDGET_H_
#define BIN_INPUT_WIDGET_H_ 

#include <QWidget>

/**
Abstract bin widget
*/
class BinInputWidget : public QWidget
{
  Q_OBJECT
public:
  /// Getter for the number of bins
  virtual int entered() const = 0;
  /// Setter for the number of bins
  virtual void entry(int value) = 0;
  /// Destructor
  virtual ~BinInputWidget(){}
Q_SIGNALS:
  /// Signal
  virtual void valueChanged();
};

#endif
