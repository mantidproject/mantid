#ifndef INSTRUMENTTREEWIDGET_H
#define INSTRUMENTTREEWIDGET_H

//---------------------------------------
// Includes
//--------------------------------------
#include <MantidQtWidgets/Common/WidgetDllOption.h>
#include <QTreeView>
#include "InstrumentTreeModel.h"
#include "MantidGeometry/IComponent.h"

namespace MantidQt {
namespace MantidWidgets {
//---------------------------------------
// Forward declarations
//--------------------------------------
class InstrumentActor;

/** The InstrumentTreeWidget is a tree view
*  of the components of an instrument.
*/
class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW InstrumentTreeWidget
    : public QTreeView {
  Q_OBJECT
public:
  explicit InstrumentTreeWidget(QWidget *w);
  void setInstrumentWidget(InstrumentWidget *w);
  void getSelectedBoundingBox(const QModelIndex &index, double &xmax,
                              double &ymax, double &zmax, double &xmin,
                              double &ymin, double &zmin);
  // Mantid::Kernel::V3D getSamplePos()const;
  QModelIndex findComponentByName(const QString &name) const;
  /// Find all expanded components
  QStringList
  findExpandedComponents(const QModelIndex &parent = QModelIndex()) const;
public slots:
  void sendComponentSelectedSignal(const QModelIndex);
signals:
  void componentSelected(size_t);

private:
  InstrumentWidget *m_instrWidget;
  InstrumentTreeModel *m_treeModel;
};
} // MantidWidgets
} // MantidQt

#endif // INSTRUMENTTREEWIDGET_H
