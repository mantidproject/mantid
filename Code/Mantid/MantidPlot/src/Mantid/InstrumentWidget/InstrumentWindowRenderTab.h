#ifndef INSTRUMENTWINDOWRENDERTAB_H_
#define INSTRUMENTWINDOWRENDERTAB_H_
#include "../../GraphOptions.h"

#include <QFrame>

class InstrumentWindow;
class Instrument3DWidget;
class BinDialog;
class QwtScaleWidget;

class QPushButton;
class QLineEdit;
class QComboBox;
class QCheckBox;

/**
  * Implements the Render tab in InstrumentWindow
  */
class InstrumentWindowRenderTab: public QFrame
{
  Q_OBJECT
public:
  InstrumentWindowRenderTab(InstrumentWindow* instrWindow);
  ~InstrumentWindowRenderTab();
  void setupColorBarScaling();
  void loadSettings(const QString& section);
  void saveSettings(const QString& section);
  void setMinValue(double value, bool apply = true);
  void setMaxValue(double value, bool apply = true);
  GraphOptions::ScaleType getScaleType()const;
  void setScaleType(GraphOptions::ScaleType type);
  void setAxis(const QString& axisName);
  bool areAxesOn()const;
private slots:
  void scaleTypeChanged(int);
  void changeColormap(const QString & filename = "");
  void minValueChanged();
  void maxValueChanged();
  void selectBinButtonClicked();
private:
  QFrame * setupAxisFrame();

  InstrumentWindow* m_instrWindow;
  Instrument3DWidget *mInstrumentDisplay;
  QPushButton *mSelectColormap,*mSaveImage;
  BinDialog *mBinDialog;
  QwtScaleWidget *mColorMapWidget;
  QLineEdit *mMinValueBox, *mMaxValueBox;
  QComboBox *mAxisCombo;
  QComboBox *mScaleOptions;
  QCheckBox *m3DAxesToggle;
};


#endif /*INSTRUMENTWINDOWRENDERTAB_H_*/
