// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "IClipboard.h"
#include "MantidQtWidgets/Common/Batch/RowLocation.h"
#include "MantidQtWidgets/Common/Batch/Subtree.h"
#include "Reduction/Group.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class MANTIDQT_ISISREFLECTOMETRY_DLL Clipboard : public IClipboard {
public:
  struct Item {
    Item(MantidQt::MantidWidgets::Batch::RowLocation location, MantidQt::MantidWidgets::Batch::Subtree subtree)
        : m_location(std::move(location)), m_subtree(std::move(subtree)) {}
    MantidQt::MantidWidgets::Batch::RowLocation m_location;
    MantidQt::MantidWidgets::Batch::Subtree m_subtree;
  };

  Clipboard();
  Clipboard(boost::optional<std::vector<MantidQt::MantidWidgets::Batch::Subtree>> subtrees,
            boost::optional<std::vector<MantidQt::MantidWidgets::Batch::RowLocation>> subtreeRoots);

  bool isInitialized() const override;
  int numberOfRoots() const override;
  bool isGroupLocation(int rootIndex) const override;
  std::string groupName(int rootIndex) const override;
  void setGroupName(int rootIndex, std::string const &groupName) override;
  Group createGroupForRoot(int rootIndex) const override;
  std::vector<boost::optional<Row>> createRowsForAllRoots() const override;

  std::vector<MantidQt::MantidWidgets::Batch::Subtree> const &
  subtrees() const override;
  std::vector<MantidQt::MantidWidgets::Batch::Subtree> &
  mutableSubtrees() override;
  std::vector<MantidQt::MantidWidgets::Batch::RowLocation> const &
  subtreeRoots() const override;
  std::vector<MantidQt::MantidWidgets::Batch::RowLocation> &
  mutableSubtreeRoots() override;

private:
  // The subtrees for each of the roots. Note that the Rows here contain
  // relative paths
  boost::optional<std::vector<MantidQt::MantidWidgets::Batch::Subtree>> m_subtrees;
  // The actual locations of the roots that were copied. This allows us to work
  // out the actual paths that were copied and determine whether items are rows
  // or groups in the reflectometry GUI sense. Note that these locations may
  // not be valid in the table if other edits have been made so this should
  // only be used for checking whether copied values were rows/groups.
  boost::optional<std::vector<MantidQt::MantidWidgets::Batch::RowLocation>> m_subtreeRoots;

  std::vector<boost::optional<Row>> createRowsForRootChildren(int rootIndex) const;
  std::vector<boost::optional<Row>> createRowsForSubtree(MantidQt::MantidWidgets::Batch::Subtree const &subtree) const;
};
bool containsGroups(IClipboard const &clipboard);
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
