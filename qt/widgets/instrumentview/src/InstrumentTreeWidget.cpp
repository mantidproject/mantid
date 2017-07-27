#include "MantidQtMantidWidgets/InstrumentView/InstrumentTreeWidget.h"
#include "MantidQtMantidWidgets/InstrumentView/InstrumentActor.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include "MantidKernel/Exception.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/Instrument.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidQtMantidWidgets/InstrumentView/GLActor.h"
#include <queue>
#include <QMessageBox>
#include <QString>
#include <cfloat>

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
  Mantid::Geometry::Instrument_const_sptr instrument =
      m_instrWidget->getInstrumentActor().getInstrument();
  // Check whether its instrument
  boost::shared_ptr<const Mantid::Geometry::IComponent> selectedComponent;
  if (instrument->getComponentID() ==
      static_cast<Mantid::Geometry::ComponentID>(index.internalPointer()))
    selectedComponent =
        boost::dynamic_pointer_cast<const Mantid::Geometry::ICompAssembly>(
            instrument);
  else
    selectedComponent = instrument->getComponentByID(
        static_cast<Mantid::Geometry::ComponentID>(index.internalPointer()));

  // get the bounding box for the component
  xmax = ymax = zmax = -DBL_MAX;
  xmin = ymin = zmin = DBL_MAX;
  Mantid::Geometry::BoundingBox boundBox;
  std::queue<boost::shared_ptr<const Mantid::Geometry::IComponent>> CompList;
  CompList.push(selectedComponent);
  while (!CompList.empty()) {
    boost::shared_ptr<const Mantid::Geometry::IComponent> tmp =
        CompList.front();
    CompList.pop();
    boost::shared_ptr<const Mantid::Geometry::IObjComponent> tmpObj =
        boost::dynamic_pointer_cast<const Mantid::Geometry::IObjComponent>(tmp);
    if (tmpObj) {
      try {
        // std::cerr << int(tmpObj->getComponentID()) << ' ' <<
        // int(instrument->getSample()->getComponentID()) << '\n';
        if (tmpObj->getComponentID() ==
            instrument->getSample()->getComponentID()) {
          boundBox = m_instrWidget->getInstrumentActor()
                         .getWorkspace()
                         ->sample()
                         .getShape()
                         .getBoundingBox();
          boundBox.moveBy(tmpObj->getPos());
        } else {
          tmpObj->getBoundingBox(boundBox);
        }
        double txmax(boundBox.xMax()), tymax(boundBox.yMax()),
            tzmax(boundBox.zMax()), txmin(boundBox.xMin()),
            tymin(boundBox.yMin()), tzmin(boundBox.zMin());
        if (txmax > xmax)
          xmax = txmax;
        if (tymax > ymax)
          ymax = tymax;
        if (tzmax > zmax)
          zmax = tzmax;
        if (txmin < xmin)
          xmin = txmin;
        if (tymin < ymin)
          ymin = tymin;
        if (tzmin < zmin)
          zmin = tzmin;
      } catch (Mantid::Kernel::Exception::NullPointerException &) {
      }
    } else if (boost::dynamic_pointer_cast<
                   const Mantid::Geometry::ICompAssembly>(tmp)) {
      boost::shared_ptr<const Mantid::Geometry::ICompAssembly> tmpAssem =
          boost::dynamic_pointer_cast<const Mantid::Geometry::ICompAssembly>(
              tmp);
      for (int idx = 0; idx < tmpAssem->nelements(); idx++) {
        CompList.push((*tmpAssem)[idx]);
      }
    }
  }
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
  Mantid::Geometry::ComponentID id =
      static_cast<Mantid::Geometry::ComponentID>(index.internalPointer());
  auto visitor = SetVisibleComponentVisitor(id);
  m_instrWidget->getInstrumentActor().accept(visitor);
  emit componentSelected(id);
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

} // MantidWidgets
} // MantidQt
