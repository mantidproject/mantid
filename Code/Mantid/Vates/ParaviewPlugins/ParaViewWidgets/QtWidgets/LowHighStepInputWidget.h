#ifndef LOW_STEP_HIGH_INPUT_WIDGET_H_
#define LOW_STEP_HIGH_INPUT_WIDGET_H_ 

#include "BinInputWidget.h"

class LowHighStepInputWidget : public BinInputWidget
{
public:
  Q_OBJECT
  virtual int getNBins() const
  {
    return 1;
  }
  virtual void setValue(int value)
  {
  }
};

#endif