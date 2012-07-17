#ifndef INSTRUMENTWINDOWRENDERTAB_H_
#define INSTRUMENTWINDOWRENDERTAB_H_
#include "MantidQtAPI/GraphOptions.h"

#include <QFrame>

class InstrumentWindow;
class MantidGLWidget;
class BinDialog;
//class QwtScaleWidget;
class ColorMapWidget;
class MantidColorMap;

class QPushButton;
class QLineEdit;
class QComboBox;
class QCheckBox;
class QAction;
class QMenu;

/**
  * Implements the Render tab in InstrumentWindow
  */
class InstrumentWindowRenderTab: public QFrame
{
  Q_OBJECT
public:
  InstrumentWindowRenderTab(InstrumentWindow* instrWindow);
  ~InstrumentWindowRenderTab();
  void setupColorBarScaling(const MantidColorMap&,double);
  void loadSettings(const QString& section);
  void saveSettings(const QString& section);
  void setMinValue(double value, bool apply = true);
  void setMaxValue(double value, bool apply = true);
  GraphOptions::ScaleType getScaleType()const;
  void setScaleType(GraphOptions::ScaleType type);
  void setAxis(const QString& axisName);
  bool areAxesOn()const;
  void init();
  void updateSurfaceTypeControl(int);
  void setupColorBar(const MantidColorMap&,double,double,double,bool);
signals:
  void rescaleColorMap();
  void setAutoscaling(bool);
public slots:
  void showAxes(bool on);
  void setColorMapAutoscaling(bool);
private slots:
  void changeColormap(const QString & filename = "");
  void showResetView(int);
  void showFlipControl(int);
  void flipUnwrappedView(bool);
private:
  void showEvent (QShowEvent *);
  QMenu* createPeaksMenu();

  QFrame * setupAxisFrame();

  InstrumentWindow* m_instrWindow;
  MantidGLWidget *m_InstrumentDisplay;
  QComboBox* m_renderMode;
  QPushButton *mSaveImage;
  ColorMapWidget* m_colorMapWidget;
  QFrame* m_resetViewFrame;
  QComboBox *mAxisCombo;
  QCheckBox *m_flipCheckBox;
  QPushButton *m_peakOverlaysButton;
  QCheckBox *m_autoscaling;

  QAction *m_colorMap;
  QAction *m_backgroundColor;
  QAction *m_displayAxes;
  QAction *m_wireframe;
  QAction *m_lighting;
  QAction *m_GLView; ///< toggle between OpenGL and simple view
  
};


#endif /*INSTRUMENTWINDOWRENDERTAB_H_*/
