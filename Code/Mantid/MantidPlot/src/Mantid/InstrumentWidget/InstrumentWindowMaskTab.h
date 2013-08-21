#ifndef INSTRUMENTWINDOWMASKTAB_H_
#define INSTRUMENTWINDOWMASKTAB_H_

#include "InstrumentWindowTab.h"
#include "MantidGLWidget.h"

#include "MantidGeometry/Instrument.h"

#include <QFrame>
#include <QMap>

#include <boost/shared_ptr.hpp>

class Instrument3DWidget;
class CollapsiblePanel;
class OneCurvePlot;
class Shape2D;

class QPushButton;
class QRadioButton;
class QTextEdit;
class QComboBox;
class QCheckBox;
class QLabel;
class QAction;
class QMenu;

class QtTreePropertyBrowser;
class QtGroupPropertyManager;
class QtDoublePropertyManager;
class QtIntPropertyManager;
class QtBoolPropertyManager;
class QtStringPropertyManager;
class QtEnumPropertyManager;
class QtProperty;
class QtBrowserItem;

namespace Mantid
{
  namespace API
  {
    class MatrixWorkspace;
  }
}

/**
  * Implements the Mask/Group tab in InstrumentWindow.
  *
  * Contains controls to create, manipulate and apply masking and grouping to underlying workspace.
  *
  */
class InstrumentWindowMaskTab: public InstrumentWindowTab
{
  Q_OBJECT
public:
  enum Mode {Mask, Group};
  enum Activity {Move,Select,DrawEllipse,DrawRectangle,DrawEllipticalRing,DrawRectangularRing};

  InstrumentWindowMaskTab(InstrumentWindow* instrWindow);
  void initSurface();
  void setMode(Mode mode);
  void selectTool(Activity tool);

signals:
  void executeAlgorithm(const QString&, const QString&);
protected slots:
  void setActivity();
  void shapeCreated();
  void shapeSelected();
  void shapesDeselected();
  void shapeChanged();
  void shapesCleared();
  void clearShapes();
  void applyMask();
  void applyMaskToView();
  void storeMask();
  void clearMask();
  void saveInvertedMaskToWorkspace();
  void saveInvertedMaskToFile();
  void saveMaskToWorkspace();
  void saveMaskToFile();
  void saveMaskToCalFile();
  void saveMaskToTable();
  void saveInvertedMaskToCalFile();
  void extractDetsToWorkspace();
  void sumDetsToWorkspace();
  void saveIncludeGroupToFile();
  void saveExcludeGroupToFile();
  void showSaveMenuTooltip(QAction*);
  void toggleMaskGroup(bool);

  void doubleChanged(QtProperty*);
protected:
  void showEvent (QShowEvent *);

  void clearProperties();
  void setProperties();
  boost::shared_ptr<Mantid::API::MatrixWorkspace> createMaskWorkspace(bool invertMask, bool temp = false);
  void saveMaskingToWorkspace(bool invertMask = false);
  void saveMaskingToFile(bool invertMask = false);
  void saveMaskingToCalFile(bool invertMask = false);
  void saveMaskingToTableWorkspace(bool invertMask = false);
  std::string generateMaskWorkspaceName(bool temp = false) const;
  void enableApplyButtons();
  void setSelectActivity();
  /// True if in masking mode, flase if in grouping.
  bool isMasking() const;
  /// Get mask/group border color
  QColor getShapeBorderColor() const;
  /// Get mask/group fill color
  QColor getShapeFillColor() const;
  /// Add a double property to the shape property browser
  QtProperty* addDoubleProperty(const QString& name)const;

  /// Is it used?
  Activity m_activity;
  /// True if there is a mask not applied to the data workspace
  bool m_hasMaskToApply;

  QRadioButton* m_masking_on;
  QRadioButton* m_grouping_on;

  QLabel *m_activeTool; ///< Displays a tip on which tool is currently selected

  // buttons
  QPushButton* m_move;
  QPushButton* m_pointer;
  QPushButton* m_ellipse;
  QPushButton* m_rectangle;
  QPushButton* m_ring_ellipse;
  QPushButton* m_ring_rectangle;

  QPushButton* m_apply;
  QPushButton* m_apply_to_view;
  QPushButton* m_clear_all;
  QPushButton* m_saveButton;


  QMenu* m_saveMask;
  QAction* m_save_as_workspace_include;
  QAction* m_save_as_workspace_exclude;
  QAction* m_save_as_file_include;
  QAction* m_save_as_file_exclude;
  QAction* m_save_as_cal_file_include;
  QAction* m_save_as_cal_file_exclude;
  QAction *m_save_as_table_xrange_exclude;

  QMenu* m_saveGroup;
  QAction* m_extract_to_workspace;
  QAction* m_sum_to_workspace;
  QAction* m_save_group_file_include;
  QAction* m_save_group_file_exclude;

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
