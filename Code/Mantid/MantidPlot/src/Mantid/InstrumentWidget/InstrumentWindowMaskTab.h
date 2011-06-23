#ifndef INSTRUMENTWINDOWMASKTAB_H_
#define INSTRUMENTWINDOWMASKTAB_H_

#include "MantidGLWidget.h"
#include "DetSelector.h"

#include <QFrame>
#include <QMap>

class InstrumentWindow;
class Instrument3DWidget;
class CollapsiblePanel;
class OneCurvePlot;
class Shape2D;

class QPushButton;
class QTextEdit;
class QComboBox;
class QCheckBox;
class QLabel;

class QtTreePropertyBrowser;
class QtGroupPropertyManager;
class QtDoublePropertyManager;
class QtIntPropertyManager;
class QtBoolPropertyManager;
class QtStringPropertyManager;
class QtEnumPropertyManager;
class QtProperty;
class QtBrowserItem;

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
  void shapeSelected();
  void shapesDeselected();
  void shapeChanged();

  void doubleChanged(QtProperty*);
protected:
  void showEvent (QShowEvent *);

  void clearProperties();
  void setProperties();

  InstrumentWindow* m_instrumentWindow;
  MantidGLWidget *m_instrumentDisplay;

  Activity m_activity;

  // buttons
  QPushButton* m_move;
  QPushButton* m_pointer;
  QPushButton* m_ellipse;
  QPushButton* m_rectangle;
  QPushButton* m_ring_ellipse;
  QPushButton* m_ring_rectangle;

  // properties
  bool m_userEditing;
  QtGroupPropertyManager  *m_groupManager;
  QtStringPropertyManager *m_stringManager;
  QtDoublePropertyManager *m_doubleManager;

  QtTreePropertyBrowser* m_browser;

  QtProperty *m_left;
  QtProperty *m_top;
  QtProperty *m_right;
  QtProperty *m_bottom;

  QMap<QtProperty *,QString> m_doublePropertyMap;
  QMap<QString,QtProperty *> m_pointPropertyMap;
  QMap<QtProperty *,QString> m_pointComponentsMap;
};


#endif /*INSTRUMENTWINDOWMASKTAB_H_*/
