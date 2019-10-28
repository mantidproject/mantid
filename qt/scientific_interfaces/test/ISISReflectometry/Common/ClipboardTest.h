// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "../../../ISISReflectometry/Common/Clipboard.h"
#include "../../../ISISReflectometry/TestHelpers/ModelCreationHelper.h"

#include <cxxtest/TestSuite.h>
#include <gtest/gtest.h>

using namespace MantidQt;
using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using namespace MantidQt::CustomInterfaces::ISISReflectometry::
    ModelCreationHelper;
using MantidQt::MantidWidgets::Batch::Cell;

class ClipboardTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ClipboardTest *createSuite() { return new ClipboardTest(); }
  static void destroySuite(ClipboardTest *suite) { delete suite; }

  void testEmptyClipboardIsNotInitialized() {
    auto clipboard = Clipboard();
    TS_ASSERT_EQUALS(clipboard.isInitialized(), false);
  }

  void testEmptyClipboardHasZeroRoots() {
    auto clipboard = Clipboard();
    TS_ASSERT_EQUALS(clipboard.numberOfRoots(), 0);
  }

  void testCheckingClipboardTypeThrowsForEmptyClipboard() {
    auto clipboard = Clipboard();
    TS_ASSERT_THROWS(clipboard.isGroupLocation(0), std::runtime_error);
  }

  void testCheckingGroupNameThrowsForEmptyClipboard() {
    auto clipboard = Clipboard();
    TS_ASSERT_THROWS(clipboard.groupName(0), std::runtime_error);
  }

  void testSettingGroupNameThrowsForEmptyClipboard() {
    auto clipboard = Clipboard();
    TS_ASSERT_THROWS(clipboard.setGroupName(0, "test group"),
                     std::runtime_error);
  }

  void testCreateGroupForRootThrowsForEmptyClipboard() {
    auto clipboard = Clipboard();
    TS_ASSERT_THROWS(clipboard.createGroupForRoot(0), std::runtime_error);
  }

  void testCreateRowsForAllRootsThrowsForEmptyClipboard() {
    auto clipboard = Clipboard();
    TS_ASSERT_THROWS(clipboard.createRowsForAllRoots(), std::runtime_error);
  }

  void testContainsGroupsThrowsForEmptyClipboard() {
    auto clipboard = Clipboard();
    TS_ASSERT_THROWS(containsGroups(clipboard), std::runtime_error);
  }

  void testClipboardIsInitializedWithRow() {
    auto clipboard = clipboardWithARow();
    TS_ASSERT_EQUALS(clipboard.isInitialized(), true);
  }

  void testIsGroupLocationReturnsFalseForRow() {
    auto clipboard = clipboardWithARow();
    TS_ASSERT_EQUALS(clipboard.isGroupLocation(0), false);
  }

  void testGettingGroupNameThrowsForRow() {
    auto clipboard = clipboardWithARow();
    TS_ASSERT_THROWS(clipboard.groupName(0), std::runtime_error);
  }

  void testSettingGroupNameThrowsForRow() {
    auto clipboard = clipboardWithARow();
    TS_ASSERT_THROWS(clipboard.setGroupName(0, "test group"),
                     std::runtime_error);
  }

  void testCreateGroupForRootThrowsForRow() {
    auto clipboard = clipboardWithARow();
    TS_ASSERT_THROWS(clipboard.createGroupForRoot(0), std::runtime_error);
  }

  void testCreateRowsForAllRootsSucceeds() {
    auto clipboard = clipboardWithARow();
    auto result = clipboard.createRowsForAllRoots();
    auto expected = std::vector<boost::optional<Row>>{makeRow("12345", 0.5)};
    TS_ASSERT_EQUALS(result, expected);
  }

  void testContainsGroupsReturnsFalseIfNoGroups() {
    auto clipboard = clipboardWithARow();
    TS_ASSERT_EQUALS(containsGroups(clipboard), false);
  }

  void testClipboardIsInitializedWithGroup() {
    auto clipboard = clipboardWithAGroup();
    TS_ASSERT_EQUALS(clipboard.isInitialized(), true);
  }

  void testIsGroupLocationReturnsTrueForGroup() {
    auto clipboard = clipboardWithAGroup();
    TS_ASSERT_EQUALS(clipboard.isGroupLocation(0), true);
  }

  void testGettingGroupNameForGroup() {
    auto clipboard = clipboardWithAGroup();
    TS_ASSERT_EQUALS(clipboard.groupName(0), "test group");
  }

  void testSettingGroupNameForGroup() {
    auto clipboard = clipboardWithAGroup();
    TS_ASSERT_THROWS_NOTHING(clipboard.setGroupName(0, "new group"));
    TS_ASSERT_EQUALS(clipboard.groupName(0), "new group");
  }

  void testCreateGroupForRootForEmptyGroup() {
    auto clipboard = clipboardWithAGroup();
    auto result = clipboard.createGroupForRoot(0);
    auto expected = Group("test group");
    TS_ASSERT_EQUALS(result, expected);
  }

  void testCreateRowsForAllRootsThrowsForGroup() {
    auto clipboard = clipboardWithAGroup();
    TS_ASSERT_THROWS(clipboard.createRowsForAllRoots(), std::runtime_error);
  }

  void testContainsGroupsReturnsTrueIfGroupsExist() {
    auto clipboard = clipboardWithAGroup();
    TS_ASSERT_EQUALS(containsGroups(clipboard), true);
  }

  void testClipboardIsInitializedWithMultiRowGroups() {
    auto clipboard = clipboardWithTwoMultiRowGroups();
    TS_ASSERT_EQUALS(clipboard.isInitialized(), true);
  }

  void testIsGroupLocationReturnsTrueForSecondGroup() {
    auto clipboard = clipboardWithTwoMultiRowGroups();
    TS_ASSERT_EQUALS(clipboard.isGroupLocation(1), true);
  }

  void testGettingGroupNameForSecondGroup() {
    auto clipboard = clipboardWithTwoMultiRowGroups();
    TS_ASSERT_EQUALS(clipboard.groupName(1), "groupB");
  }

  void testSettingGroupNameForSecondGroup() {
    auto clipboard = clipboardWithTwoMultiRowGroups();
    TS_ASSERT_THROWS_NOTHING(clipboard.setGroupName(1, "new group"));
    TS_ASSERT_EQUALS(clipboard.groupName(1), "new group");
  }

  void testCreateGroupForRootForMultiRowGroup() {
    auto clipboard = clipboardWithTwoMultiRowGroups();
    auto result = clipboard.createGroupForRoot(1);
    auto expected = Group("groupB");
    expected.appendRow(makeRow("12345", 0.5));
    expected.appendRow(makeRow("22345", 2.5));
    TS_ASSERT_EQUALS(result, expected);
  }

  void testCreateRowsForAllRootsThrowsForMultiGroupClipboard() {
    auto clipboard = clipboardWithTwoMultiRowGroups();
    TS_ASSERT_THROWS(clipboard.createRowsForAllRoots(), std::runtime_error);
  }

  void testContainsGroupsReturnsTrueIfMultipleGroupsExist() {
    auto clipboard = clipboardWithTwoMultiRowGroups();
    TS_ASSERT_EQUALS(containsGroups(clipboard), true);
  }

private:
  Cell makeCell(std::string const &text = std::string("")) {
    return Cell(text);
  }

  std::vector<Cell> makeRowCells(std::string const &run = std::string("12345"),
                                 std::string const &theta = "0.5") {
    return std::vector<Cell>{
        makeCell(run),       makeCell(theta), makeCell("Trans A"),
        makeCell("Trans B"), makeCell(),      makeCell(),
        makeCell(),          makeCell(),      makeCell()};
  }

  std::vector<Cell>
  makeGroupCells(std::string const &groupName = std::string("test group")) {
    return std::vector<Cell>{makeCell(groupName), makeCell(), makeCell(),
                             makeCell(),          makeCell(), makeCell(),
                             makeCell(),          makeCell(), makeCell()};
  }

  MantidWidgets::Batch::RowLocation makeLocation() {
    auto path = MantidWidgets::Batch::RowPath{};
    return MantidWidgets::Batch::RowLocation(path);
  }

  MantidWidgets::Batch::RowLocation makeLocation(int index) {
    auto path = MantidWidgets::Batch::RowPath{index};
    return MantidWidgets::Batch::RowLocation(path);
  }

  MantidWidgets::Batch::RowLocation makeLocation(int groupIndex, int rowIndex) {
    auto path = MantidWidgets::Batch::RowPath{groupIndex, rowIndex};
    return MantidWidgets::Batch::RowLocation(path);
  }

  MantidWidgets::Batch::Subtree makeSubtreeWithAnEmptyGroup() {
    // The group path is relative to the root (and it is the root) so does not
    // need an index
    auto group = MantidWidgets::Batch::Row(makeLocation(), makeGroupCells());
    auto subtree = MantidWidgets::Batch::Subtree{group};
    return subtree;
  }

  MantidWidgets::Batch::Subtree
  makeSubtreeWithAMultiRowGroup(std::string const &groupName) {
    // The group path is relative to the root (and it is the root) so does not
    // need an index. The rows just need a row index.
    auto group =
        MantidWidgets::Batch::Row(makeLocation(), makeGroupCells(groupName));
    auto row1 = MantidWidgets::Batch::Row(makeLocation(0),
                                          makeRowCells("12345", "0.5"));
    auto row2 = MantidWidgets::Batch::Row(makeLocation(1),
                                          makeRowCells("22345", "2.5"));
    auto subtree = MantidWidgets::Batch::Subtree{group, row1, row2};
    return subtree;
  }

  MantidWidgets::Batch::Subtree makeSubtreeWithARow(int rowIndex) {
    // The row path in the subtree is relative to the root (group) i.e. excludes
    // the group index
    auto row =
        MantidWidgets::Batch::Row(makeLocation(rowIndex), {makeRowCells()});
    auto subtree = MantidWidgets::Batch::Subtree{row};
    return subtree;
  }

  Clipboard clipboardWithAGroup() {
    auto const groupIndex = 0;
    auto subtrees = std::vector<MantidWidgets::Batch::Subtree>{
        makeSubtreeWithAnEmptyGroup()};
    // Subtree roots include the full path, i.e. with the group index
    auto subtreeRoots = std::vector<MantidWidgets::Batch::RowLocation>{
        makeLocation(groupIndex)};
    return Clipboard(subtrees, subtreeRoots);
  }

  Clipboard clipboardWithARow() {
    auto const groupIndex = 0;
    auto const rowIndex = 0;
    auto subtrees = std::vector<MantidWidgets::Batch::Subtree>{
        makeSubtreeWithARow(rowIndex)};
    // Subtree roots include the full path i.e. with group and row index
    auto subtreeRoots = std::vector<MantidWidgets::Batch::RowLocation>{
        makeLocation(groupIndex, rowIndex)};
    return Clipboard(subtrees, subtreeRoots);
  }

  Clipboard clipboardWithTwoMultiRowGroups() {
    auto subtrees = std::vector<MantidWidgets::Batch::Subtree>{
        makeSubtreeWithAMultiRowGroup("groupA"),
        makeSubtreeWithAMultiRowGroup("groupB")};
    // Subtree roots include the full path, i.e. with the group index
    auto subtreeRoots = std::vector<MantidWidgets::Batch::RowLocation>{
        makeLocation(0), makeLocation(1)};
    return Clipboard(subtrees, subtreeRoots);
  }
};
