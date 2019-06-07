// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef INSTRUMENTTREEWIDGET_H
#define INSTRUMENTTREEWIDGET_H

//---------------------------------------
// Includes
//--------------------------------------
#include "InstrumentTreeModel.h"
#include "MantidGeometry/IComponent.h"
#include "MantidQtWidgets/Common/WidgetDllOption.h"
#include <QTreeView>

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
  void sendComponentSelectedSignal(const QModelIndex /*index*/);
signals:
  void componentSelected(size_t /*_t1*/);

private:
  InstrumentWidget *m_instrWidget;
  InstrumentTreeModel *m_treeModel;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif // INSTRUMENTTREEWIDGET_H
