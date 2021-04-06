// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidGeometry/muParser_Silent.h"

namespace Mantid {
namespace API {
namespace MuParserUtils {

/** Defines convenience methods to be used with the muParser mathematical
    expression parser.
*/

/// A map from constants to their muParser names.
extern const MANTID_API_DLL std::map<double, std::string> MUPARSER_CONSTANTS;

/// Add a set of default constants to a muParser.
void MANTID_API_DLL addDefaultConstants(mu::Parser &parser);

using oneVarFun = double (*)(double); // pointer to a function of one variable
extern const MANTID_API_DLL std::map<std::string, oneVarFun> MUPARSER_ONEVAR_FUNCTIONS;

void MANTID_API_DLL extraOneVarFunctions(mu::Parser &parser);

} // namespace MuParserUtils
} // namespace API
} // namespace Mantid
