#include "MantidQtWidgets/InstrumentView/InstrumentTreeModel.h"
#include "MantidQtWidgets/InstrumentView/InstrumentActor.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidKernel/Exception.h"

using Mantid::Geometry::ICompAssembly;
using Mantid::Geometry::IComponent;
using Mantid::Geometry::IObjComponent;

namespace MantidQt {
namespace MantidWidgets {
/**
 * Constructor for tree model to display instrument tree
 */
InstrumentTreeModel::InstrumentTreeModel(const InstrumentWidget *instrWidget,
                                         QObject *parent)
    : QAbstractItemModel(parent), m_instrWidget(instrWidget) {
  const auto &componentInfo =
      m_instrWidget->getInstrumentActor().componentInfo();
  m_componentIndices.resize(componentInfo.size());
  std::iota(m_componentIndices.begin(), m_componentIndices.end(), 0);
}

/**
 * Destructor for instrument display tree
 */
InstrumentTreeModel::~InstrumentTreeModel() {}

/**
 * Column count for the instrument tree.
 *  Returns a count of 1 for the Component Assembly = I think this means "I have
 * child nodes"
 *  Returns 0 for the ObjComponent = I'm an end point.
 */
int InstrumentTreeModel::columnCount(const QModelIndex &parent) const {
  if (!parent.isValid())
    return 1;

  const auto &componentInfo =
      m_instrWidget->getInstrumentActor().componentInfo();
  auto index = extractIndex(parent);
  if (componentInfo.children(index).size() > 0)
    return 1;

  return 0;
}

/**
 * Returns the string corresponding to the component name. the root of tree
 * string will be instrument name
 */
QVariant InstrumentTreeModel::data(const QModelIndex &index, int role) const {
  if (role != Qt::DisplayRole)
    return QVariant();

  const auto &componentInfo =
      m_instrWidget->getInstrumentActor().componentInfo();

  if (!index.isValid()) // not valid has to return the root node
    return QString::fromStdString(componentInfo.name(componentInfo.root()));

  auto compIndex = extractIndex(index);
  return QString::fromStdString(componentInfo.name(compIndex));
}

/**
 * Flags whether node in the tree is selectable or not. In instrument tree all
 * components are selectable.
 */
Qt::ItemFlags InstrumentTreeModel::flags(const QModelIndex &index) const {
  (void)index; // avoid compiler warning
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

/**
 * Instrument header. returns nothing. no header for tree
 */
QVariant InstrumentTreeModel::headerData(int section,
                                         Qt::Orientation orientation,
                                         int role) const {
  (void)section;     // avoid compiler warning
  (void)orientation; // avoid compiler warning
  (void)role;        // avoid compiler warning
  return QVariant();
}

/**
 * Returns the ModelIndex at a give row and column and the parent.
 */
QModelIndex InstrumentTreeModel::index(int row, int column,
                                       const QModelIndex &parent) const {
  const auto &componentInfo =
      m_instrWidget->getInstrumentActor().componentInfo();
  if (!parent.isValid()) { // invalid parent, has to be the root node i.e
                           // instrument
    return createIndex(row, column, &m_componentIndices[componentInfo.root()]);
  }
  auto index = extractIndex(parent);
  const auto &children = componentInfo.children(index);

  if (index == componentInfo.source() || index == componentInfo.sample() ||
      static_cast<int>(children.size()) <= row)
    return QModelIndex();

  return createIndex(row, column, &m_componentIndices[children[row]]);
}

/**
 * Returns the parent model index.
 */
QModelIndex InstrumentTreeModel::parent(const QModelIndex &index) const {
  if (!index.isValid()) // the index corresponds to root so there is no parent
                        // for root return empty.
    return QModelIndex();

  const auto &componentInfo =
      m_instrWidget->getInstrumentActor().componentInfo();
  auto compIndex = extractIndex(index);

  if (compIndex == componentInfo.root())
    return QModelIndex();

  auto parent = componentInfo.parent(compIndex);
  if (parent == componentInfo.root())
    return createIndex(0, 0, &m_componentIndices[componentInfo.root()]);

  auto grandParent = componentInfo.parent(parent);
  const auto &grandParentElems = componentInfo.children(grandParent);

  int row = 0;
  for (auto child : grandParentElems) {
    if (child == parent)
      break;
    row++;
  }

  return createIndex(row, 0, &m_componentIndices[parent]);
}

/**
 * Return the row count. the number of elements in the component. for
 * ObjComponent row count will be 0.
 */
int InstrumentTreeModel::rowCount(const QModelIndex &parent) const {
  if (!parent.isValid()) // Root node row count is one.
    return 1;

  const auto &componentInfo =
      m_instrWidget->getInstrumentActor().componentInfo();
  auto index = extractIndex(parent);
  const auto &children = componentInfo.children(index);
  if (children.size() > 0)
    return static_cast<int>(children.size());

  return 0;
}

size_t InstrumentTreeModel::extractIndex(const QModelIndex &index) {
  auto indexPtr = static_cast<size_t *>(index.internalPointer());
  return *indexPtr;
}
} // namespace MantidWidgets
} // namespace MantidQt
