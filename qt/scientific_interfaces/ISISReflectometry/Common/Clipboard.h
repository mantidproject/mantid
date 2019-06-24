// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTID_CUSTOMINTERFACES_CLIPBOARD_H_
#define MANTID_CUSTOMINTERFACES_CLIPBOARD_H_

#include "MantidQtWidgets/Common/Batch/RowLocation.h"
#include "MantidQtWidgets/Common/Batch/Subtree.h"
#include "Reduction/Group.h"

namespace MantidQt {
namespace CustomInterfaces {

class Clipboard {
public:
  struct Item {
    Item(MantidQt::MantidWidgets::Batch::RowLocation location,
         MantidQt::MantidWidgets::Batch::Subtree subtree)
        : m_location(std::move(location)), m_subtree(std::move(subtree)) {}
    MantidQt::MantidWidgets::Batch::RowLocation m_location;
    MantidQt::MantidWidgets::Batch::Subtree m_subtree;
  };

  Clipboard();
  Clipboard(
      boost::optional<std::vector<MantidQt::MantidWidgets::Batch::Subtree>>
          subtrees,
      boost::optional<std::vector<MantidQt::MantidWidgets::Batch::RowLocation>>
          subtreeRoots);

  bool isInitialized() const;
  int size() const;
  bool isGroupLocation(int rootIndex) const;
  std::string groupName(int rootIndex) const;
  void setGroupName(int rootIndex, std::string const &groupName);
  Group group(int rootIndex) const;
  std::vector<boost::optional<Row>> rows() const;
  std::vector<boost::optional<Row>> rows(int rootIndex) const;

  std::vector<MantidQt::MantidWidgets::Batch::Subtree> const &subtrees() const;
  std::vector<MantidQt::MantidWidgets::Batch::Subtree> &mutableSubtrees();
  std::vector<MantidQt::MantidWidgets::Batch::RowLocation> const &
  subtreeRoots() const;
  std::vector<MantidQt::MantidWidgets::Batch::RowLocation> &
  mutableSubtreeRoots();

private:
  boost::optional<std::vector<MantidQt::MantidWidgets::Batch::Subtree>>
      m_subtrees;
  boost::optional<std::vector<MantidQt::MantidWidgets::Batch::RowLocation>>
      m_subtreeRoots;

  std::vector<boost::optional<Row>>
  rows(MantidQt::MantidWidgets::Batch::Subtree const &subtree) const;
};

bool containsGroups(Clipboard const &clipboard);
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACES_CLIPBOARD_H_
