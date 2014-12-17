#include "MantidDataObjects/MementoTableWorkspace.h"

#include "MantidKernel/Logger.h"
#include "MantidAPI/WorkspaceFactory.h"

namespace Mantid {
namespace DataObjects {

DECLARE_WORKSPACE(MementoTableWorkspace)

/**
Determines whether the provided column has the same name and type as expected.
@param expected : expected column name
@param candidate : ref to column to check
@return true if all expectations are met.
*/
bool MementoTableWorkspace::expectedColumn(
    Mantid::API::Column_const_sptr expected,
    Mantid::API::Column_const_sptr candidate) {
  if (expected->name() != candidate->name()) {
    return false;
  } else if (expected->type() != candidate->type()) {
    return false;
  } else {
    return true;
  }
}

/**
Determines whether a given table workspace has columns in the same order, and
exactly matching those for the
MementoTableWorkspace schema.
@param candidate : ref to workspace to treat as a candidate for being a memento
table workspace.
@return true if it is a MementoTableWorkspace.
*/
bool MementoTableWorkspace::isMementoWorkspace(
    const Mantid::API::ITableWorkspace &candidate) {
  MementoTableWorkspace theStandard;
  size_t nCols = theStandard.columnCount();
  if (nCols != candidate.columnCount()) {
    return false;
  }
  for (size_t i = 0; i < nCols; i++) {
    if (!expectedColumn(theStandard.getColumn(i), candidate.getColumn(i))) {
      return false;
    }
  }
  return true;
}

/// Constructor
MementoTableWorkspace::MementoTableWorkspace(int nRows)
    : TableWorkspace(nRows) {
  // Configure the columns as part of the construction.
  this->addColumn("str", "WSName");
  this->addColumn("str", "ISName");
  this->addColumn("int", "RunNumber");
  this->addColumn("str", "ShapeXML");
  this->addColumn("double", "a");
  this->addColumn("double", "b");
  this->addColumn("double", "c");
  this->addColumn("double", "alpha");
  this->addColumn("double", "beta");
  this->addColumn("double", "gamma");
  this->addColumn("str", "Status");
}

/// Destructor
MementoTableWorkspace::~MementoTableWorkspace() {}
}
}
