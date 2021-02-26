// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "../../../ISISReflectometry/Common/IClipboard.h"
#include <gmock/gmock.h>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class DLLExport MockClipboard : public IClipboard {
public:
  MockClipboard() = default;

  MOCK_METHOD(bool, isInitialized, (), (const));
  MOCK_METHOD(int, numberOfRoots, (), (const));
  MOCK_METHOD(bool, isGroupLocation, (int rootIndex), (const));
  MOCK_METHOD(std::string, groupName, (int rootIndex), (const));
  MOCK_METHOD(void, setGroupName,
              (int rootIndex, std::string const &groupName));
  MOCK_METHOD(Group, createGroupForRoot, (int rootIndex), (const));
  MOCK_METHOD(std::vector<boost::optional<Row>>, createRowsForAllRoots, (),
              (const));

  MOCK_METHOD(std::vector<MantidQt::MantidWidgets::Batch::Subtree> &, subtrees,
              (), (const));
  MOCK_METHOD(std::vector<MantidQt::MantidWidgets::Batch::Subtree> &,
              mutableSubtrees, ());
  MOCK_METHOD(std::vector<MantidQt::MantidWidgets::Batch::RowLocation> &,
              subtreeRoots, (), (const));
  MOCK_METHOD(std::vector<MantidQt::MantidWidgets::Batch::RowLocation> &,
              mutableSubtreeRoots, ());
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt