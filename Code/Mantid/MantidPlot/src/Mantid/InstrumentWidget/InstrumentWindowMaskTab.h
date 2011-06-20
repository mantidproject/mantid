#ifndef INSTRUMENTWINDOWMASKTAB_H_
#define INSTRUMENTWINDOWMASKTAB_H_

#include "MantidGLWidget.h"
#include "DetSelector.h"

#include <QFrame>

class InstrumentWindow;
class Instrument3DWidget;
class CollapsiblePanel;
class OneCurvePlot;

class QPushButton;
class QTextEdit;
class QComboBox;
class QCheckBox;
class QLabel;

/**
  * Implements the Mask tab in InstrumentWindow
  */
class InstrumentWindowMaskTab: public QFrame
{
  Q_OBJECT
public:
  enum Activity {Move = 0, Select = 1, DrawEllipse};
  InstrumentWindowMaskTab(InstrumentWindow* instrWindow);
  void init();
protected slots:
  void setActivity();
  void shapeCreated();
protected:
  void showEvent (QShowEvent *);

  InstrumentWindow* m_instrumentWindow;
  MantidGLWidget *m_instrumentDisplay;

  Activity m_activity;
  QPushButton* m_move;
  QPushButton* m_pointer;
  QPushButton* m_ellipse;
  QPushButton* m_rectangle;
  QPushButton* m_ring_ellipse;
  QPushButton* m_ring_rectangle;
};


#endif /*INSTRUMENTWINDOWMASKTAB_H_*/
