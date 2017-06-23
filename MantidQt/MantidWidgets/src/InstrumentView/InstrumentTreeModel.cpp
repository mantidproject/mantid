#include "MantidQtMantidWidgets/InstrumentView/InstrumentTreeModel.h"
#include "MantidQtMantidWidgets/InstrumentView/InstrumentActor.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include "MantidKernel/Exception.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/Instrument.h"
#include "MantidAPI/MatrixWorkspace.h"

using Mantid::Geometry::Instrument;
using Mantid::Geometry::IComponent;
using Mantid::Geometry::ICompAssembly;
using Mantid::Geometry::IObjComponent;

namespace MantidQt {
namespace MantidWidgets {

/**
* Constructor for tree model to display instrument tree
*/
InstrumentTreeModel::InstrumentTreeModel(const InstrumentWidget *instrWidget,
                                         QObject *parent)
    : QAbstractItemModel(parent), m_instrWidget(instrWidget) {}

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
  try {
    if (parent.isValid()) {
      auto instr = m_instrWidget->getInstrumentActor().getInstrument();
      boost::shared_ptr<const IComponent> comp = instr->getComponentByID(
          static_cast<Mantid::Geometry::ComponentID>(parent.internalPointer()));
      boost::shared_ptr<const ICompAssembly> objcomp =
          boost::dynamic_pointer_cast<const ICompAssembly>(comp);
      if (objcomp)
        return 1;
      return 0;
    } else
      return 1;
  } catch (...) {
    // std::cout<<"Exception :: columnCount"<<'\n';
    return 0;
  }
}

/**
* Returns the string corresponding to the component name. the root of tree
* string will be instrument name
*/
QVariant InstrumentTreeModel::data(const QModelIndex &index, int role) const {
  try {
    if (role != Qt::DisplayRole)
      return QVariant();

    auto instr = m_instrWidget->getInstrumentActor().getInstrument();

    if (!index.isValid()) // not valid has to return the root node
      return QString(instr->getName().c_str());

    boost::shared_ptr<const IComponent> ins = instr->getComponentByID(
        static_cast<Mantid::Geometry::ComponentID>(index.internalPointer()));
    if (ins) {
      return QString(ins->getName().c_str());
    }
    return QString("Error");
  } catch (...) {
    // std::cout<<" Exception: in data"<<'\n';
    return 0;
  }
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
  //	std::cout<<"Index +++++++++ row"<<row<<" column "<<column<<" is valid
  //"<<parent.isValid()<<'\n';
  try {
    boost::shared_ptr<const ICompAssembly> parentItem;
    auto instr = m_instrWidget->getInstrumentActor().getInstrument();
    if (!parent.isValid()) // invalid parent, has to be the root node i.e
                           // instrument
      return createIndex(row, column, instr->getComponentID());

    boost::shared_ptr<const IComponent> comp = instr->getComponentByID(
        static_cast<Mantid::Geometry::ComponentID>(parent.internalPointer()));
    parentItem = boost::dynamic_pointer_cast<const ICompAssembly>(comp);
    if (!parentItem) {
      boost::shared_ptr<const IObjComponent> objcomp =
          boost::dynamic_pointer_cast<const IObjComponent>(comp);
      if (objcomp)
        return QModelIndex();
      // Not an instrument so check for Component Assembly
      parentItem = boost::dynamic_pointer_cast<const ICompAssembly>(instr);
    }
    // If component assembly pick the Component at the row index. if row index
    // is higher than number
    // of components in assembly return empty model index
    if (parentItem->nelements() < row) {
      return QModelIndex();
    } else {
      return createIndex(row, column,
                         (void *)((*parentItem)[row]->getComponentID()));
    }
  } catch (...) {
    std::cout << "InstrumentTreeModel::index(" << row << "," << column
              << ") threw an exception.\n";
  }
  return QModelIndex();
}

/**
* Returns the parent model index.
*/
QModelIndex InstrumentTreeModel::parent(const QModelIndex &index) const {
  //	std::cout<<"parent +++++++++ row"<<index.row()<<" column
  //"<<index.column()<<" is valid "<<index.isValid()<<'\n';
  try {
    if (!index.isValid()) // the index corresponds to root so there is no parent
                          // for root return empty.
      return QModelIndex();

    auto instr = m_instrWidget->getInstrumentActor().getInstrument();

    if (instr->getComponentID() ==
        static_cast<Mantid::Geometry::ComponentID>(index.internalPointer()))
      return QModelIndex();

    boost::shared_ptr<const IComponent> child = instr->getComponentByID(
        static_cast<Mantid::Geometry::ComponentID>(index.internalPointer()));
    if (child->getParent()->getComponentID() == instr->getComponentID())
      return createIndex(0, 0, instr->getComponentID());
    boost::shared_ptr<const IComponent> parent =
        instr->getComponentByID(child->getParent()->getComponentID());
    boost::shared_ptr<const IComponent> greatParent =
        instr->getComponentByID(parent->getParent()->getComponentID());
    boost::shared_ptr<const ICompAssembly> greatParentAssembly =
        boost::dynamic_pointer_cast<const ICompAssembly>(greatParent);
    int iindex = 0;
    for (int i = 0; i < greatParentAssembly->nelements(); i++)
      if ((*greatParentAssembly)[i]->getComponentID() ==
          parent->getComponentID())
        iindex = i;
    return createIndex(iindex, 0, (void *)parent->getComponentID());
  } catch (...) {
    //		std::cout<<"Exception: in parent"<<'\n';
  }
  return QModelIndex();
}

/**
* Return the row count. the number of elements in the component. for
* ObjComponent row count will be 0.
*/
int InstrumentTreeModel::rowCount(const QModelIndex &parent) const {
  //	std::cout<<"rowCount +++++++++ row"<<parent.row()<<" column
  //"<<parent.column()<<" is valid "<<parent.isValid()<<'\n';

  try {
    if (!parent.isValid()) // Root node row count is one.
    {
      return 1; // boost::dynamic_pointer_cast<ICompAssembly>(m_instrument)->nelements();
    } else {
      auto instr = m_instrWidget->getInstrumentActor().getInstrument();
      if (instr->getComponentID() == static_cast<Mantid::Geometry::ComponentID>(
                                         parent.internalPointer())) {
        return instr->nelements();
      }
      boost::shared_ptr<const IComponent> comp =
          instr->getComponentByID(static_cast<Mantid::Geometry::ComponentID>(
              parent
                  .internalPointer())); // static_cast<IComponent*>(parent.internalPointer());
      boost::shared_ptr<const ICompAssembly> assembly =
          boost::dynamic_pointer_cast<const ICompAssembly>(comp);
      if (assembly) {
        return assembly->nelements();
      }
      boost::shared_ptr<const IObjComponent> objcomp =
          boost::dynamic_pointer_cast<const IObjComponent>(comp);
      if (objcomp)
        return 0;
    }
  } catch (...) {
    // std::cout<<"Exception: in rowCount"<<'\n';
  }
  return 0;
}
} // MantidWidgets
} // MantidQt
