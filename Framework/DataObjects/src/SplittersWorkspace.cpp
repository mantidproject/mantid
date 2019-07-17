// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/SplittersWorkspace.h"
#include "MantidAPI/Column.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/IPropertyManager.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace DataObjects {
namespace {
/// static logger
Kernel::Logger g_log("SplittersWorkspace");
} // namespace
//----------------------------------------------------------------------------------------------
/** Constructor
 */
SplittersWorkspace::SplittersWorkspace() {
  this->addColumn("long64", "start");
  this->addColumn("long64", "stop");
  this->addColumn("int", "workspacegroup");
}

/*
 * Add a Splitter to
 */
void SplittersWorkspace::addSplitter(
    Mantid::Kernel::SplittingInterval splitter) {
  Mantid::API::TableRow row = this->appendRow();
  row << splitter.start().totalNanoseconds();
  row << splitter.stop().totalNanoseconds();
  row << splitter.index();
}

Kernel::SplittingInterval SplittersWorkspace::getSplitter(size_t index) {
  API::TableRow row = this->getRow(index);
  int64_t start, stop;
  int wsgroup;
  row >> start;
  row >> stop;
  row >> wsgroup;

  Kernel::SplittingInterval splitter(Types::Core::DateAndTime(start),
                                     Types::Core::DateAndTime(stop), wsgroup);

  return splitter;
}

size_t SplittersWorkspace::getNumberSplitters() const {
  return this->rowCount();
}

bool SplittersWorkspace::removeSplitter(size_t index) {
  bool removed;
  if (index >= this->rowCount()) {
    g_log.error() << "Try to delete a non-existing splitter " << index << '\n';
    removed = false;
  } else {
    this->removeRow(index);
    removed = true;
  }

  return removed;
}

} // namespace DataObjects
} // namespace Mantid

///\cond TEMPLATE

namespace Mantid {
namespace Kernel {

template <>
DLLExport Mantid::DataObjects::SplittersWorkspace_sptr
IPropertyManager::getValue<Mantid::DataObjects::SplittersWorkspace_sptr>(
    const std::string &name) const {
  auto *prop = dynamic_cast<
      PropertyWithValue<Mantid::DataObjects::SplittersWorkspace_sptr> *>(
      getPointerToProperty(name));
  if (prop) {
    return *prop;
  } else {
    std::string message =
        "Attempt to assign property " + name +
        " to incorrect type. Expected shared_ptr<SplittersWorkspace>.";
    throw std::runtime_error(message);
  }
}

template <>
DLLExport Mantid::DataObjects::SplittersWorkspace_const_sptr
IPropertyManager::getValue<Mantid::DataObjects::SplittersWorkspace_const_sptr>(
    const std::string &name) const {
  auto *prop = dynamic_cast<
      PropertyWithValue<Mantid::DataObjects::SplittersWorkspace_sptr> *>(
      getPointerToProperty(name));
  if (prop) {
    return prop->operator()();
  } else {
    std::string message =
        "Attempt to assign property " + name +
        " to incorrect type. Expected const shared_ptr<SplittersWorkspace>.";
    throw std::runtime_error(message);
  }
}

} // namespace Kernel
} // namespace Mantid
///\endcond
