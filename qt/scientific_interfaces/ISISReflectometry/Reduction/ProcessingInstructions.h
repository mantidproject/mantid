// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include <boost/optional.hpp>
namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

/** For now processing instructions are just a string but we expect them to
    become more complicated so this can be changed to a class in the future
*/
using ProcessingInstructions = std::string;
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
