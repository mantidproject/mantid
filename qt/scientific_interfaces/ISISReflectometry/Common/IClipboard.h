// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidQtWidgets/Common/Batch/RowLocation.h"
#include "MantidQtWidgets/Common/Batch/Subtree.h"
#include "Reduction/Group.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class MANTIDQT_ISISREFLECTOMETRY_DLL IClipboard {
public:
  virtual bool isInitialized() const = 0;
  virtual int numberOfRoots() const = 0;
  virtual bool isGroupLocation(int rootIndex) const = 0;
  virtual std::string groupName(int rootIndex) const = 0;
  virtual void setGroupName(int rootIndex, std::string const &groupName) = 0;
  virtual Group createGroupForRoot(int rootIndex) const = 0;
  virtual std::vector<boost::optional<Row>> createRowsForAllRoots() const = 0;

  virtual std::vector<MantidQt::MantidWidgets::Batch::Subtree> const &
  subtrees() const = 0;
  virtual std::vector<MantidQt::MantidWidgets::Batch::Subtree> &
  mutableSubtrees() = 0;
  virtual std::vector<MantidQt::MantidWidgets::Batch::RowLocation> const &
  subtreeRoots() const = 0;
  virtual std::vector<MantidQt::MantidWidgets::Batch::RowLocation> &
  mutableSubtreeRoots() = 0;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt