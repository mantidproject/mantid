#include "MantidQtWidgets/InstrumentView/InstrumentTreeWidget.h"
#include "MantidQtWidgets/InstrumentView/InstrumentActor.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidKernel/Exception.h"
#include <QMessageBox>
#include <QString>
#include <cfloat>
#include <queue>

namespace MantidQt {
namespace MantidWidgets {

InstrumentTreeWidget::InstrumentTreeWidget(QWidget *w)
    : QTreeView(w), m_instrWidget(nullptr), m_treeModel(nullptr) {
  connect(this, SIGNAL(clicked(const QModelIndex)), this,
          SLOT(sendComponentSelectedSignal(const QModelIndex)));
}

void InstrumentTreeWidget::setInstrumentWidget(InstrumentWidget *w) {
  m_instrWidget = w;
  m_treeModel = new InstrumentTreeModel(w, this);
  setModel(m_treeModel);
  setSelectionMode(SingleSelection);
  setSelectionBehavior(SelectRows);
}

void InstrumentTreeWidget::getSelectedBoundingBox(const QModelIndex &index,
                                                  double &xmax, double &ymax,
                                                  double &zmax, double &xmin,
                                                  double &ymin, double &zmin) {
  const auto &componentInfo =
      m_instrWidget->getInstrumentActor().componentInfo();
  auto compIndex = InstrumentTreeModel::extractIndex(index);
  auto bb = componentInfo.boundingBox(compIndex);

  xmax = bb.xMax();
  ymax = bb.yMax();
  zmax = bb.zMax();
  xmin = bb.xMin();
  ymin = bb.yMin();
  zmin = bb.zMin();
}

QModelIndex
InstrumentTreeWidget::findComponentByName(const QString &name) const {
  if (!m_treeModel)
    return QModelIndex();
  // The data is in a tree model so recursively search until we find the string
  // we want. Note the match is NOT
  // case sensitive
  QModelIndexList matches = m_treeModel->match(
      m_treeModel->index(0, 0, QModelIndex()), Qt::DisplayRole, name, 1,
      Qt::MatchFixedString | Qt::MatchRecursive);
  if (matches.isEmpty())
    return QModelIndex();
  return matches.first();
}

void InstrumentTreeWidget::sendComponentSelectedSignal(
    const QModelIndex index) {
  auto selectedIndex = InstrumentTreeModel::extractIndex(index);
  m_instrWidget->getInstrumentActor().setComponentVisible(selectedIndex);
  emit componentSelected(selectedIndex);
}

/** Get a list of components that have been expanded
 * @param parent :: the parent index to start searching from
 * @return a list of component names as strings
 */
QStringList
InstrumentTreeWidget::findExpandedComponents(const QModelIndex &parent) const {
  QStringList retval;
  int rowCount = model()->rowCount(parent);

  for (int i = 0; i < rowCount; ++i) {
    QModelIndex idx = model()->index(i, 0, parent);
    if (idx.isValid() && isExpanded(idx)) {
      retval << idx.data(Qt::DisplayRole).toString();
      retval << findExpandedComponents(idx);
    }
  }

  return retval;
}

} // namespace MantidWidgets
} // namespace MantidQt
