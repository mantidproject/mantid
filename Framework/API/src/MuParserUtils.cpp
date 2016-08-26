#include "MantidAPI/MuParserUtils.h"

#include "MantidKernel/make_unique.h"
#include "MantidKernel/PhysicalConstants.h"

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
  {M_PI, "pi"},                // Another pi, without the leading '_'.
  {StandardAtmosphere, "atm"}
};

/** Adds the constants from MUPARSER_CONSTANTS to the parser.
 *  @param parser The parser to be initialized.
 */
void initParser(mu::Parser& parser) {
  for (const auto constant : MUPARSER_CONSTANTS) {
    parser.DefineConst(constant.second, constant.first);
  }
}

/** Populates the parser with the default constants etc.
 *  @return A unique pointer to the default muParser.
 */
std::unique_ptr<mu::Parser> DLLExport allocateDefaultMuParser() {
  auto p = Kernel::make_unique<mu::Parser>();
  initParser(*p);
  return p;
}

/** Populates the parser with the default constants etc.
 *  @return A default muParser.
 */
mu::Parser DLLExport createDefaultMuParser() {
  mu::Parser p;
  initParser(p);
  return p;
}
} // namespace MuParserUtils
} // namespace API
} // namespace Mantid
