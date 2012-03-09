#ifndef SIMPLE_BIN_INPUT_WIDGET_H
#define SIMPLE_BIN_INPUT_WIDGET_H 

#include "BinInputWidget.h"
class QLineEdit;

/**
class SimpleBinInputWidget
This simple implementation allows users to specify the number of bins directly.
Widget wrapper around a label and text box. 
*/
class SimpleBinInputWidget : public BinInputWidget
{
  Q_OBJECT

public:
  /// Constructor
  SimpleBinInputWidget();
  /// Setter for the entry
  virtual void setEntry(int nBins, double min, double max);
  /// Getter for the entry
  virtual int getEntry(double, double) const;
  /// Destructor
  ~SimpleBinInputWidget();
private slots:
  void nBinsListener();
private:
   /// Number of bins text box.
   QLineEdit* m_nBinsBox;
};

#endif