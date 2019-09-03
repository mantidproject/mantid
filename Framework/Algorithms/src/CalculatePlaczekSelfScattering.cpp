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
#include "MantidAPI/Sample.h"
#include "MantidKernel/Material.h"

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
CalculatePlaczekSelfScattering::get_sample_species_info(
    API::MatrixWorkspace_sptr ws) {
  //get sample information : mass, total scattering length, and concentration of each species
  float total_stoich; 
  std::map<std::string, std::map<std::string, float>> atom_species;
  Kernel::Material::ChemicalFormula material =
      ws->sample().getMaterial().chemicalFormula();
  total_stoich = 0.0;
  return atom_species;
}

}  // namespace Algorithms
}  // namespace Mantid
