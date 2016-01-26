#ifndef INSTRUMENTWINDOWTREETAB_H_
#define INSTRUMENTWINDOWTREETAB_H_

#include "InstrumentWindowTab.h"

#include <QModelIndex>

class InstrumentTreeWidget;

/**
  * Implements the instrument tree tab in InstrumentWindow
  */
class InstrumentWindowTreeTab: public InstrumentWindowTab
{
  Q_OBJECT
public:
  explicit InstrumentWindowTreeTab(InstrumentWindow *instrWindow);
  void initSurface();
public slots:
  void selectComponentByName(const QString& name);
private:
  void showEvent (QShowEvent *);
  /// Widget to display instrument tree
  InstrumentTreeWidget* m_instrumentTree;
};


#endif /*INSTRUMENTWINDOWTREETAB_H_*/
