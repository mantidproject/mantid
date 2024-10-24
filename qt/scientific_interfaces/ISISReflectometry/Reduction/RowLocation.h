// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidQtWidgets/Common/Batch/RowLocation.h"
#include "MantidQtWidgets/Common/Batch/Subtree.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

std::vector<int> groupIndexesFromSelection(std::vector<MantidWidgets::Batch::RowLocation> const &selected);
std::vector<int>
mapToContainingGroups(std::vector<MantidQt::MantidWidgets::Batch::RowLocation> const &mustNotContainRoot);
bool containsGroups(std::vector<MantidQt::MantidWidgets::Batch::RowLocation> const &locations);
bool isGroupLocation(MantidQt::MantidWidgets::Batch::RowLocation const &location);
int groupOf(MantidQt::MantidWidgets::Batch::RowLocation const &groupLocation);
bool isRowLocation(MantidWidgets::Batch::RowLocation const &location);
int rowOf(MantidQt::MantidWidgets::Batch::RowLocation const &rowLocation);
bool containsPath(std::vector<MantidQt::MantidWidgets::Batch::RowLocation> const &locations,
                  MantidQt::MantidWidgets::Batch::RowLocation const &path);
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
