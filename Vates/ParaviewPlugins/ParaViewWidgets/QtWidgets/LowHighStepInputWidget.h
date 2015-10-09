#ifndef LOW_STEP_HIGH_INPUT_WIDGET_H_
#define LOW_STEP_HIGH_INPUT_WIDGET_H_ 

#include "BinInputWidget.h"
class QLineEdit;

class LowHighStepInputWidget : public BinInputWidget
{  
  Q_OBJECT
public:
  virtual int getEntry(double min, double max) const;
  virtual void setEntry(int nBins, double min, double max);
  LowHighStepInputWidget();
  ~LowHighStepInputWidget();
private slots:
  void nBinsListener();
private:
   /// Low/High/Step boxes
   QLineEdit* m_step;
};

#endif