#ifndef INSTRUMENTWINDOWRENDERTAB_H_
#define INSTRUMENTWINDOWRENDERTAB_H_
#include "../../GraphOptions.h"

#include <QFrame>

class InstrumentWindow;
class Instrument3DWidget;
class BinDialog;
//class QwtScaleWidget;
class ColorMapWidget;

class QPushButton;
class QLineEdit;
class QComboBox;
class QCheckBox;
class QAction;

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
  void minValueChanged(double);
  void maxValueChanged(double);
  void selectBinButtonClicked();
  void showResetView(int);
private:
  QFrame * setupAxisFrame();

  InstrumentWindow* m_instrWindow;
  Instrument3DWidget *mInstrumentDisplay;
  QPushButton *mSaveImage;
  BinDialog *mBinDialog;
  ColorMapWidget* m_colorMapWidget;
  QFrame* m_resetViewFrame;
  QComboBox *mAxisCombo;

  QAction *m_colorMap;
  QAction *m_backgroundColor;
  QAction *m_displayAxes;
  QAction *m_wireframe;
  QAction *m_lighting;
};


#endif /*INSTRUMENTWINDOWRENDERTAB_H_*/
