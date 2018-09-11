#include "MantidDataHandling/SetSampleMaterial.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/Workspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/Atom.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/MaterialBuilder.h"
#include "MantidKernel/PhysicalConstants.h"

#include <boost/scoped_ptr.hpp>

#include <cmath>
#include <iostream>

using namespace Mantid::PhysicalConstants;

namespace Mantid {
namespace DataHandling {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SetSampleMaterial)

const std::string SetSampleMaterial::name() const {
  return "SetSampleMaterial";
}

int SetSampleMaterial::version() const { return (1); }

const std::string SetSampleMaterial::category() const { return "Sample"; }

using namespace Mantid::DataHandling;
using namespace Mantid::API;
using namespace Kernel;

/**
 * Initialize the algorithm
 */
void SetSampleMaterial::init() {
  using namespace Mantid::Kernel;
  declareProperty(make_unique<WorkspaceProperty<Workspace>>(
                      "InputWorkspace", "", Direction::InOut),
                  "The workspace with which to associate the sample ");
  declareProperty("ChemicalFormula", "",
                  "The chemical formula, see examples in documentation");
  declareProperty("AtomicNumber", 0, "The atomic number");
  declareProperty("MassNumber", 0,
                  "Mass number if ion (use 0 for default mass sensity)");
  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty("SampleNumberDensity", EMPTY_DBL(), mustBePositive,
                  "This number density of the sample in number of "
                  "atoms per cubic angstrom will be used instead of "
                  "calculated");
  declareProperty("ZParameter", EMPTY_DBL(), mustBePositive,
                  "Number of formula units in unit cell");
  declareProperty("UnitCellVolume", EMPTY_DBL(), mustBePositive,
                  "Unit cell volume in Angstoms^3. Will be calculated from the "
                  "OrientedLattice if not supplied.");
  declareProperty("CoherentXSection", EMPTY_DBL(), mustBePositive,
                  "Optional:  This coherent cross-section for the sample "
                  "material in barns will be used instead of tabulated");
  declareProperty("IncoherentXSection", EMPTY_DBL(), mustBePositive,
                  "Optional:  This incoherent cross-section for the sample "
                  "material in barns will be used instead of tabulated");
  declareProperty("AttenuationXSection", EMPTY_DBL(), mustBePositive,
                  "Optional:  This absorption cross-section for the sample "
                  "material in barns will be used instead of tabulated");
  declareProperty("ScatteringXSection", EMPTY_DBL(), mustBePositive,
                  "Optional:  This total scattering cross-section (coherent + "
                  "incoherent) for the sample material in barns will be used "
                  "instead of tabulated");
  declareProperty("SampleMassDensity", EMPTY_DBL(), mustBePositive,
                  "Measured mass density in g/cubic cm of the sample "
                  "to be used to calculate the number density.");

  // Perform Group Associations.
  std::string formulaGrp("By Formula or Atomic Number");
  setPropertyGroup("ChemicalFormula", formulaGrp);
  setPropertyGroup("AtomicNumber", formulaGrp);
  setPropertyGroup("MassNumber", formulaGrp);

  std::string densityGrp("Sample Density");
  setPropertyGroup("SampleNumberDensity", densityGrp);
  setPropertyGroup("ZParameter", densityGrp);
  setPropertyGroup("UnitCellVolume", densityGrp);
  setPropertyGroup("SampleMassDensity", densityGrp);

  std::string specificValuesGrp("Override Cross Section Values");
  setPropertyGroup("CoherentXSection", specificValuesGrp);
  setPropertyGroup("IncoherentXSection", specificValuesGrp);
  setPropertyGroup("AttenuationXSection", specificValuesGrp);
  setPropertyGroup("ScatteringXSection", specificValuesGrp);

  // Extra property settings
  setPropertySettings("ChemicalFormula",
                      make_unique<Kernel::EnabledWhenProperty>(
                          "AtomicNumber", Kernel::IS_DEFAULT));
  setPropertySettings("AtomicNumber",
                      make_unique<Kernel::EnabledWhenProperty>(
                          "ChemicalFormula", Kernel::IS_DEFAULT));
  setPropertySettings("MassNumber", make_unique<Kernel::EnabledWhenProperty>(
                                        "ChemicalFormula", Kernel::IS_DEFAULT));
}

std::map<std::string, std::string> SetSampleMaterial::validateInputs() {
  std::map<std::string, std::string> result;
  const std::string chemicalSymbol = getProperty("ChemicalFormula");
  const int z_number = getProperty("AtomicNumber");
  const int a_number = getProperty("MassNumber");
  if (chemicalSymbol.empty()) {
    if (z_number <= 0) {
      result["ChemicalFormula"] = "Need to specify the material";
    }
  } else {
    if (z_number > 0)
      result["AtomicNumber"] =
          "Cannot specify both ChemicalFormula and AtomicNumber";
  }

  if (a_number > 0 && z_number <= 0)
    result["AtomicNumber"] = "Specified MassNumber without AtomicNumber";

  const double sampleNumberDensity = getProperty("SampleNumberDensity");
  const double zParameter = getProperty("ZParameter");
  const double unitCellVolume = getProperty("UnitCellVolume");
  const double sampleMassDensity = getProperty("SampleMassDensity");

  if (!isEmpty(zParameter)) {
    if (isEmpty(unitCellVolume)) {
      result["UnitCellVolume"] =
          "UnitCellVolume must be provided with ZParameter";
    }
    if (!isEmpty(sampleNumberDensity)) {
      result["ZParameter"] =
          "Can not give ZParameter with SampleNumberDensity set";
    }
    if (!isEmpty(sampleMassDensity)) {
      result["SampleMassDensity"] =
          "Can not give SampleMassDensity with ZParameter set";
    }
  } else if (!isEmpty(sampleNumberDensity)) {
    if (!isEmpty(sampleMassDensity)) {
      result["SampleMassDensity"] =
          "Can not give SampleMassDensity with SampleNumberDensity set";
    }
  }

  return result;
}

/**
 * Add the cross sections to the neutron atom if they are not-empty
 * numbers. All values are in barns.
 *
 * @param neutron The neutron to update
 * @param coh_xs Coherent cross section
 * @param inc_xs Incoherent cross section
 * @param abs_xs Absorption cross section
 * @param tot_xs Total scattering cross section
 */
void SetSampleMaterial::fixNeutron(NeutronAtom &neutron, double coh_xs,
                                   double inc_xs, double abs_xs,
                                   double tot_xs) {
  if (!isEmpty(coh_xs))
    neutron.coh_scatt_xs = coh_xs;
  if (!isEmpty(inc_xs))
    neutron.inc_scatt_xs = inc_xs;
  if (!isEmpty(abs_xs))
    neutron.abs_scatt_xs = abs_xs;
  if (!isEmpty(tot_xs))
    neutron.tot_scatt_xs = tot_xs;
}

/**
 * Execute the algorithm
 */
void SetSampleMaterial::exec() {
  // Get the input workspace
  Workspace_sptr workspace = getProperty("InputWorkspace");
  // an ExperimentInfo object has a sample
  ExperimentInfo_sptr expInfo =
      boost::dynamic_pointer_cast<ExperimentInfo>(workspace);
  if (!bool(expInfo)) {
    throw std::runtime_error("InputWorkspace does not have a sample object");
  }

  boost::scoped_ptr<Material> material;
  MaterialBuilder builder;

  // determine the material
  const std::string chemicalSymbol = getProperty("ChemicalFormula");
  const int z_number = getProperty("AtomicNumber");
  const int a_number = getProperty("MassNumber");
  if (!chemicalSymbol.empty()) {
    std::cout << "CHEM: " << chemicalSymbol << std::endl;
    builder.setFormula(chemicalSymbol);
  } else {
    builder.setAtomicNumber(z_number);
    builder.setMassNumber(a_number);
  }

  // determine the sample number density
  double rho_m = getProperty("SampleMassDensity"); // in g/cc
  if (!isEmpty(rho_m))
    builder.setMassDensity(rho_m);
  double rho = getProperty("SampleNumberDensity"); // in atoms / Angstroms^3
  if (isEmpty(rho)) {
    double zParameter = getProperty("ZParameter"); // number of atoms
    if (!isEmpty(zParameter)) {
      builder.setZParameter(zParameter);
      double unitCellVolume = getProperty("UnitCellVolume");
      builder.setUnitCellVolume(unitCellVolume);
    }
  } else {
    builder.setNumberDensity(rho);
  }

  // get the scattering information - this will override table values
  builder.setCoherentXSection(getProperty("CoherentXSection"));      // in barns
  builder.setIncoherentXSection(getProperty("IncoherentXSection"));  // in barns
  builder.setAbsorptionXSection(getProperty("AttenuationXSection")); // in barns
  builder.setTotalScatterXSection(
      getProperty("ScatteringXSection")); // in barns

  // create the material
  material.reset(new Material(builder.build()));

  // calculate derived values
  const double bcoh_avg_sq = material->cohScatterLengthSqrd();   // <b>
  const double btot_sq_avg = material->totalScatterLengthSqrd(); // <b^2>
  double normalizedLaue = (btot_sq_avg - bcoh_avg_sq) / bcoh_avg_sq;
  if (btot_sq_avg == bcoh_avg_sq)
    normalizedLaue = 0.;

  // set the material but leave the geometry unchanged
  auto shapeObject = boost::shared_ptr<Geometry::IObject>(
      expInfo->sample().getShape().cloneWithMaterial(*material));
  expInfo->mutableSample().setShape(shapeObject);
  g_log.information() << "Sample number density ";
  if (isEmpty(material->numberDensity())) {
    g_log.information() << "was not specified\n";
  } else {
    g_log.information() << "= " << material->numberDensity()
                        << " atoms/Angstrom^3\n";
  }
  g_log.information() << "Cross sections for wavelength = "
                      << NeutronAtom::ReferenceLambda << " Angstroms\n"
                      << "    Coherent " << material->cohScatterXSection()
                      << " barns\n"
                      << "    Incoherent " << material->incohScatterXSection()
                      << " barns\n"
                      << "    Total " << material->totalScatterXSection()
                      << " barns\n"
                      << "    Absorption " << material->absorbXSection()
                      << " barns\n"
                      << "PDF terms\n"
                      << "    <b_coh>^2 = " << bcoh_avg_sq << "\n"
                      << "    <b_tot^2> = " << btot_sq_avg << "\n"
                      << "    L         = " << normalizedLaue << "\n";

  if (isEmpty(rho)) {
    g_log.information("Unknown value for number density");
  } else {
    double smu =
        material->totalScatterXSection(NeutronAtom::ReferenceLambda) * rho;
    double amu = material->absorbXSection(NeutronAtom::ReferenceLambda) * rho;
    g_log.information() << "Anvred LinearScatteringCoef = " << smu << " 1/cm\n"
                        << "Anvred LinearAbsorptionCoef = " << amu << " 1/cm\n";
  }
  // Done!
  progress(1);
}
} // namespace DataHandling
} // namespace Mantid
