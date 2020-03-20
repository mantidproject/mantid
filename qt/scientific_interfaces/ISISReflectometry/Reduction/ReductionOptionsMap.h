// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include <map>
namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {
/** The ReductionOptionsMap holds information relating to the settings
 * in the Options column in the runs table. These are user-specified
 * options specified as key=value pairs.
 */
using ReductionOptionsMap = std::map<std::string, std::string>;
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt