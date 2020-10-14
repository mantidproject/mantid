// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <memory>

namespace Mantid {
namespace DataObjects {
/**
  This file provides forward declarations for
  Mantid::DataObjects::TableWorkspace
*/
/// forward declare of Mantid::DataObjects::TableWorkspace
class TableWorkspace;
/// shared pointer to Mantid::DataObjects::TableWorkspace
using TableWorkspace_sptr = std::shared_ptr<TableWorkspace>;
/// shared pointer to Mantid::DataObjects::TableWorkspace (const version)
using TableWorkspace_const_sptr = std::shared_ptr<const TableWorkspace>;
/// unique pointer to Mantid::DataObjects::TableWorkspace
using TableWorkspace_uptr = std::unique_ptr<TableWorkspace>;
/// unique pointer to Mantid::DataObjects::TableWorkspace (const version)
using TableWorkspace_const_uptr = std::unique_ptr<const TableWorkspace>;

} // namespace DataObjects
} // namespace Mantid
