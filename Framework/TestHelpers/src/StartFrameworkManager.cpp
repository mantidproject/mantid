// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/**
 *
 * Uses a similar method to the plugin loading to ensure that the
 *FrameworkManager is
 * called when an executable loads.
 *
 * To use this simply compile this file along with the other source files that
 * generate the executable/library. When the library is loaded/executable
 *started
 * the "static" object will ensure that the FrameworkManager is started
 */
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/RegistrationHelper.h"

namespace {
Mantid::Kernel::RegistrationHelper start_framework(((Mantid::API::FrameworkManager::Instance()), 0));
}
