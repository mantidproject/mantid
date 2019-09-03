// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CalculatePlaczekSelfScattering.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidAPI/WorkspaceFactory.h"

#include <utility>

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(CalculatePlaczekSelfScattering)

void CalculatePlaczekSelfScattering::init() {
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
          "InputWorkspace", "", Kernel::Direction::Input),
      "");
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
          "OutputWorkspace", "", Kernel::Direction::Output),
      "");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CalculatePlaczekSelfScattering::exec() {
  API::MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");
  API::Workspace_sptr outWS = API::WorkspaceFactory::Instance().create(inWS);

  setProperty("OutputWorkspace", outWS);
}

std::map<std::string, std::map<std::string, float>>
CalculatePlaczekSelfScattering::get_sample_species_info() {
//get sample information : mass, total scattering length, and concentration of each species
  float total_stoich = 0.0;
std::map<std::string, std::map<std::string, float>> 
        atom_species = collections.OrderedDict()
        for atom, stoich in zip(material[0], material[1]):
# <b ^ 2> == sigma_s / 4 * pi(in barns)
            b_sqrd_bar = self._input_ws.sample().getMaterial().totalScatterXSection() / (4. * np.pi)
            atom_species[atom.symbol] = {'mass': atom.mass,
                                         'stoich': stoich,
                                         'b_sqrd_bar': b_sqrd_bar}
            total_stoich += stoich

        for atom, props in atom_species.items():
#inefficient in py2, but works with py3
            props['concentration'] = props['stoich'] / total_stoich
}

}  // namespace Algorithms
}  // namespace Mantid
