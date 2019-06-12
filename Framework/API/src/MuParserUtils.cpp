// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/MuParserUtils.h"

#include "MantidKernel/PhysicalConstants.h"

#include <gsl/gsl_sf.h>

using namespace Mantid::PhysicalConstants;

namespace Mantid {
namespace API {
namespace MuParserUtils {

// The constant names below try to follow the naming scheme of the
// scipy.constants Python module.
// In addition to these, muParser defines "_e" for the Euler's number and
// "_pi" for pi.
const std::map<double, std::string> MUPARSER_CONSTANTS = {
    {AtomicMassUnit, "atomic_mass"},
    {BoltzmannConstant, "k"},
    {E_mev_toNeutronWavenumberSq, "meV_squared_wave_number_relationship"},
    {h, "h"},
    {h_bar, "hbar"},
    {g, "g"},
    {meV, "meV"},
    {meVtoKelvin, "meV_kelvin_relationship"},
    {meVtoWavenumber, "meV_wave_number_relationship"},
    {MuonGyromagneticRatio, "muon_gyromagnetic_ratio"},
    {MuonLifetime, "muon_life_time"},
    {N_A, "N_A"},
    {NeutronMass, "m_n"},
    {NeutronMassAMU, "m_n_AMU"},
    {M_PI, "pi"}, // Another pi, without the leading '_'.
    {StandardAtmosphere, "atm"}};

/** The added constants are defined in MUPARSER_CONSTANTS.
 *  @param parser The parser to be initialized.
 */
void DLLExport addDefaultConstants(mu::Parser &parser) {
  for (const auto &constant : MUPARSER_CONSTANTS) {
    parser.DefineConst(constant.second, constant.first);
  }
}

using oneVarFun = double (*)(double); // pointer to a function of one variable
const std::map<std::string, oneVarFun> MUPARSER_ONEVAR_FUNCTIONS = {
    {"erf", gsl_sf_erf}, {"erfc", gsl_sf_erfc}};

void DLLExport extraOneVarFunctions(mu::Parser &parser) {
  for (const auto &function : MUPARSER_ONEVAR_FUNCTIONS) {
    parser.DefineFun(function.first, function.second);
  }
}

} // namespace MuParserUtils
} // namespace API
} // namespace Mantid
