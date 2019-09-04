// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CalculatePlaczekSelfScattering.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidKernel/Atom.h"
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
  const double factor = 1.0 / 1.66053906660e-27;
  const double neutron_mass = factor * 1.674927471e-27;
  // get sample information : mass, total scattering length, and concentration
  // of each species
  auto atom_species =
      CalculatePlaczekSelfScattering::get_sample_species_info(inWS);
  // calculate summation term w/ neutron mass over molecular mass ratio
  double summation_term = 0.0;
  for (auto t = atom_species.begin(); t != atom_species.end(); t++) {
    summation_term += t->second["concentration"] * t->second["b_sqrd_bar"] *
                      neutron_mass / t->second["mass"];
  }
  // get incident spectrum and 1st derivative
  auto x_lambda = inWS->readX(0);
  auto incident = inWS->readY(0);
  auto incident_prime = inWS->readY(0);
  std::vector<double> phi_1;
  for (int i = 0; i < x_lambda.size(); i++) {
    phi_1.push_back(x_lambda[i] * incident[i] / incident_prime[i]);
  }
  // set detector law
  double lambda_D = 1.44;
  std::vector<double> detector_law_term;
  for (int i = 0; i < x_lambda.size(); i++) {
    double x_term = -x_lambda[i] / lambda_D;
    detector_law_term.push_back(x_term * exp(x_term) / (1.0 - exp(x_term)));
  }
  /* Placzek
     Original Placzek inelastic correction Ref (for constant wavelength, reactor source):
       Placzek, Phys. Rev v86, (1952), pp. 377-388
     First Placzek correction for time-of-flight, pulsed source (also shows reactor eqs.):
       Powles, Mol. Phys., v6 (1973), pp.1325-1350
     Nomenclature and calculation for this program follows Ref:
       Howe, McGreevy, and Howells, J. Phys.: Condens. Matter v1, (1989), pp. 3433-3451
     NOTE: Powles's Equation for inelastic self-scattering is equal to Howe's Equation for P(theta)
     by adding the elastic self-scattering
  */
  std::vector<double> placzek_correction;

  setProperty("OutputWorkspace", outWS);
}

std::map<std::string, std::map<std::string, double>>
CalculatePlaczekSelfScattering::get_sample_species_info(
    API::MatrixWorkspace_sptr ws) {
  // get sample information : mass, total scattering length, and concentration
  // of each species
  double total_stoich = 0.0;
  std::map<std::string, std::map<std::string, double>> atom_species;
  const auto formula = ws->sample().getMaterial().chemicalFormula();
  const double x_section = ws->sample().getMaterial().totalScatterXSection();
  const double b_sqrd_bar = x_section / (4.0 * M_PI);

  for (auto t = formula.begin(); t != formula.end(); t++) {
    auto element = *t;
    std::map<std::string, double> atom_map;
    atom_map["mass"] = element.atom->mass;
    atom_map["stoich"] = element.multiplicity;
    atom_map["b_sqrd_bar"] = b_sqrd_bar;
    atom_species[element.atom->symbol] = atom_map;
    total_stoich += element.multiplicity;
  }
  for (auto t = atom_species.begin(); t != atom_species.end(); t++) {
    t->second["concentration"] = t->second["stoich"] / total_stoich;
  }
  return atom_species;
}

} // namespace Algorithms
} // namespace Mantid
