#ifndef LOW_STEP_HIGH_INPUT_WIDGET_H_
#define LOW_STEP_HIGH_INPUT_WIDGET_H_ 

#include "BinInputWidget.h"

class LowHighStepInputWidget : public BinInputWidget
{
public:
  Q_OBJECT
  virtual int entered() const
  {
    return 1;
  }
  virtual void entry(int value)
  {
  }
};

#endif