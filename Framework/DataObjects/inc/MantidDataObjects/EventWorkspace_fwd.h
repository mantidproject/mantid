// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <memory>

namespace Mantid {
namespace DataObjects {
/**
  This file provides forward declarations for Mantid::DataObjects::EventWorkspace
*/

/// forward declare of Mantid::DataObjects::EventWorkspace
class EventWorkspace;
/// shared pointer to Mantid::DataObjects::EventWorkspace
using EventWorkspace_sptr = std::shared_ptr<EventWorkspace>;
/// shared pointer to Mantid::DataObjects::EventWorkspace (const version)
using EventWorkspace_const_sptr = std::shared_ptr<const EventWorkspace>;
/// unique pointer to Mantid::DataObjects::EventWorkspace
using EventWorkspace_uptr = std::unique_ptr<EventWorkspace>;
/// unique pointer to Mantid::DataObjects::EventWorkspace (const version)
using EventWorkspace_const_uptr = std::unique_ptr<const EventWorkspace>;

} // namespace DataObjects
} // namespace Mantid
