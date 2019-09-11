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
#include "MantidGeometry/Instrument/DetectorInfo.h"
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
      "Workspace of fitted incident spectrum with it's first derivative. "
      "Workspace must have instument and sample data.");
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
          "OutputWorkspace", "", Kernel::Direction::Output),
      "Workspace with the Self scattering correction");
}
//----------------------------------------------------------------------------------------------
/** Validate inputs.
 */
std::map<std::string, std::string>
CalculatePlaczekSelfScattering::validateInputs() {
  std::map<std::string, std::string> issues;
  const API::MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");
  const Geometry::DetectorInfo det_info = inWS->detectorInfo();
  if (det_info.size() == 0) {
    issues["InputWorkspace"] = 
		"Input workspace does not have detector information";
  }
  Kernel::Material::ChemicalFormula formula =
      inWS->sample().getMaterial().chemicalFormula();
  if (formula.size() == 0) {
    issues["InputWorkspace"] = "Input workspace does not have a valid sample";
  }
  return issues;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CalculatePlaczekSelfScattering::exec() {
  const API::MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");
  API::MatrixWorkspace_sptr outWS = getProperty("OutputWorkspace");
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
  const MantidVec x_lambda = inWS->readX(0);
  const MantidVec incident = inWS->readY(0);
  const MantidVec incident_prime = inWS->readY(1);
  double dx = (x_lambda[1] - x_lambda[0]) / 2.0;
  std::vector<double> phi_1;
  for (int i = 0; i < x_lambda.size() - 1; i++) {
    phi_1.push_back((x_lambda[i] + dx) * incident_prime[i] / incident[i]);
  }
  // set detector law term (eps_1)
  const double lambda_D = 1.44;
  std::vector<double> eps_1;
  for (int i = 0; i < x_lambda.size(); i++) {
    double x_term = -x_lambda[i] / lambda_D;
    eps_1.push_back(x_term * exp(x_term) / (1.0 - exp(x_term)));
  }
  /* Placzek
     Original Placzek inelastic correction Ref (for constant wavelength, reactor 
     source): Placzek, Phys. Rev v86, (1952), pp. 377-388 First Placzek 
     correction for time-of-flight, pulsed source (also shows reactor eqs.):
     Powles, Mol. Phys., v6 (1973), pp.1325-1350
     Nomenclature and calculation for this program follows Ref:
     Howe, McGreevy, and Howells, J. Phys.: Condens. Matter v1, (1989), pp. 
     3433-3451 NOTE: Powles's Equation for inelastic self-scattering is equal to 
     Howe's Equation for P(theta) by adding the elastic self-scattering
  */
  MantidVec x_lambdas;
  MantidVec placzek_correction;
  const Geometry::DetectorInfo det_info = inWS->detectorInfo();
  int NSpec = 0;
  for (int det_index = 0; det_index < det_info.size(); det_index++) {
    if (det_info.isMonitor(det_index) != true) {
      NSpec += 1;
      const double path_length = det_info.l1() + det_info.l2(det_index);
      const double f = det_info.l1() / path_length;
      const double angle_conv = M_PI / 180.0;
      const double sin_theta_by_2 =
          sin(det_info.twoTheta(det_index) * angle_conv / 2.0);
      for (int x_index = 0; x_index < x_lambda.size() - 1; x_index++) {
        const double term1 = (f + 1.0) * phi_1[x_index];
        const double term2 = f * eps_1[x_index];
        const double term3 = f - 3;
        const double inelastic_placzek_self_correction =
            2.0 * (term1 - term2 + term3) * sin_theta_by_2 * sin_theta_by_2 *
            summation_term;
        x_lambdas.push_back(x_lambda[x_index]);
        placzek_correction.push_back(inelastic_placzek_self_correction);
      }
      x_lambdas.push_back(x_lambda.back());
    }
  }
  Mantid::API::Algorithm_sptr ChildAlg =
      createChildAlgorithm("CreateWorkspace");
  ChildAlg->setProperty("DataX", x_lambdas);
  ChildAlg->setProperty("DataY", placzek_correction);
  ChildAlg->setProperty("UnitX", "Wavelength");
  ChildAlg->setProperty("NSpec", NSpec);
  ChildAlg->setProperty("ParentWorkspace", inWS);
  ChildAlg->setProperty("Distribution", true);
  ChildAlg->execute();
  outWS = ChildAlg->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace", outWS);
}

const std::map<std::string, std::map<std::string, double>>
CalculatePlaczekSelfScattering::get_sample_species_info(
    API::MatrixWorkspace_sptr ws) {
  // get sample information : mass, total scattering length, and concentration
  // of each species
  double total_stoich = 0.0;
  std::map<std::string, std::map<std::string, double>> atom_species;
  const Kernel::Material::ChemicalFormula formula =
      ws->sample().getMaterial().chemicalFormula();
  const double x_section = ws->sample().getMaterial().totalScatterXSection();
  const double b_sqrd_bar = x_section / (4.0 * M_PI);

  for (auto t = formula.begin(); t != formula.end(); t++) {
    const Kernel::Material::FormulaUnit element = *t;
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
