#ifndef INSTRUMENTWINDOWRENDERTAB_H_
#define INSTRUMENTWINDOWRENDERTAB_H_

#include "InstrumentWindowTab.h"

#include "MantidQtAPI/GraphOptions.h"

class BinDialog;
class ColorMapWidget;
class MantidColorMap;

class QPushButton;
class QLineEdit;
class QComboBox;
class QCheckBox;
class QAction;
class QActionGroup;
class QMenu;

/**
  * Implements the Render tab in InstrumentWindow.
  */
class InstrumentWindowRenderTab: public InstrumentWindowTab
{
  Q_OBJECT

public:
  InstrumentWindowRenderTab(InstrumentWindow* instrWindow);
  ~InstrumentWindowRenderTab();
  void initSurface();
  void saveSettings(QSettings&)const;
  void loadSettings(const QSettings&);
  void setupColorBarScaling(const MantidColorMap&,double);
  GraphOptions::ScaleType getScaleType()const;
  void setScaleType(GraphOptions::ScaleType type);
  void setAxis(const QString& axisName);
  bool areAxesOn()const;
  void setupColorBar(const MantidColorMap&,double,double,double,bool);

signals:
  void rescaleColorMap();
  void setAutoscaling(bool);

public slots:
  void setMinValue(double value, bool apply = true);
  void setMaxValue(double value, bool apply = true);
  void setRange(double minValue, double maxValue, bool apply = true);
  void showAxes(bool on);
  void displayDetectorsOnly(bool yes);
  void enableGL(bool on);
  void setColorMapAutoscaling(bool);
  void changeColorMap(const QString & filename = "");
  void setSurfaceType(int);
  void flipUnwrappedView(bool);
  void saveImage(QString filename = "");

private slots:

  void showResetView(int);
  void showFlipControl(int);
  /// Called before the display setting menu opens. Filters out menu options.
  void displaySettingsAboutToshow();
  /// Change the type of the surfac
  void surfaceTypeChanged(int index);
  void colorMapChanged();
  void scaleTypeChanged(int);
  void glOptionChanged(bool);
  void showMenuToolTip(QAction*);

private:
  void showEvent (QShowEvent *);
  QMenu* createPeaksMenu();
  QFrame * setupAxisFrame();
  void setPrecisionMenuItemChecked(int n);
  void enable3DSurface( bool on );

  QPushButton *m_surfaceTypeButton;
  QPushButton *mSaveImage;
  ColorMapWidget* m_colorMapWidget;
  QFrame* m_resetViewFrame;
  QComboBox *mAxisCombo;
  QCheckBox *m_flipCheckBox;
  QPushButton *m_peakOverlaysButton;
  QCheckBox *m_autoscaling;

  QActionGroup *m_surfaceTypeActionGroup;
  QAction *m_full3D;
  QAction *m_cylindricalX;
  QAction *m_cylindricalY;
  QAction *m_cylindricalZ;
  QAction *m_sphericalX;
  QAction *m_sphericalY;
  QAction *m_sphericalZ;

  QAction *m_colorMap;
  QAction *m_backgroundColor;
  QAction *m_displayAxes;
  QAction *m_displayDetectorsOnly;
  QAction *m_wireframe;
  QAction *m_lighting;
  QAction *m_GLView; ///< toggle between OpenGL and simple view
  QActionGroup *m_precisionActionGroup;
  QList<QAction*> m_precisionActions;

  friend class InstrumentWindow;
  
};


#endif /*INSTRUMENTWINDOWRENDERTAB_H_*/
