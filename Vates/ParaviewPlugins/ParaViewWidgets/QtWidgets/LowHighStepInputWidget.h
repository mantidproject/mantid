#ifndef LOW_STEP_HIGH_INPUT_WIDGET_H_
#define LOW_STEP_HIGH_INPUT_WIDGET_H_

#include "BinInputWidget.h"
class QLineEdit;

class LowHighStepInputWidget : public BinInputWidget {
  Q_OBJECT
public:
  int getEntry(double min, double max) const override;
  void setEntry(int nBins, double min, double max) override;
  LowHighStepInputWidget();
  ~LowHighStepInputWidget() override;
private slots:
  void nBinsListener();

private:
  /// Low/High/Step boxes
  QLineEdit *m_step;
};

#endif