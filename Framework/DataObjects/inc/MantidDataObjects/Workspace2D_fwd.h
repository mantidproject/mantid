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
  This file provides forward declarations for Mantid::DataObjects::Workspace2D
*/
/// forward declare of Mantid::DataObjects::Workspace2D
class Workspace2D;
/// shared pointer to Mantid::DataObjects::Workspace2D
using Workspace2D_sptr = std::shared_ptr<Workspace2D>;
/// shared pointer to Mantid::DataObjects::Workspace2D (const version)
using Workspace2D_const_sptr = std::shared_ptr<const Workspace2D>;
/// unique pointer to Mantid::DataObjects::Workspace2D
using Workspace2D_uptr = std::unique_ptr<Workspace2D>;
/// unique pointer to Mantid::DataObjects::Workspace2D (const version)
using Workspace2D_const_uptr = std::unique_ptr<const Workspace2D>;

} // namespace DataObjects
} // namespace Mantid
