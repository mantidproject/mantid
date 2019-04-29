// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SetSampleMaterial.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/Workspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/Atom.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/PhysicalConstants.h"

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
                  "Mass number if ion (use 0 for default mass number)");
  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty(
      "SampleNumberDensity", EMPTY_DBL(), mustBePositive,
      "This number density of the sample in number of "
      "atoms or formula units per cubic Angstrom will be used instead of "
      "calculated");
  declareProperty("ZParameter", EMPTY_DBL(), mustBePositive,
                  "Number of formula units in unit cell");
  declareProperty("UnitCellVolume", EMPTY_DBL(), mustBePositive,
                  "Unit cell volume in Angstoms^3. Will be calculated from the "
                  "OrientedLattice if not supplied.");
  declareProperty("CoherentXSection", EMPTY_DBL(), mustBePositive,
                  "This coherent cross-section for the sample "
                  "material in barns will be used instead of tabulated");
  declareProperty("IncoherentXSection", EMPTY_DBL(), mustBePositive,
                  "This incoherent cross-section for the sample "
                  "material in barns will be used instead of tabulated");
  declareProperty("AttenuationXSection", EMPTY_DBL(), mustBePositive,
                  "This absorption cross-section for the sample "
                  "material in barns will be used instead of tabulated");
  declareProperty("ScatteringXSection", EMPTY_DBL(), mustBePositive,
                  "Optional:  This total scattering cross-section (coherent + "
                  "incoherent) for the sample material in barns will be used "
                  "instead of tabulated");
  declareProperty("SampleMassDensity", EMPTY_DBL(), mustBePositive,
                  "Measured mass density in g/cubic cm of the sample "
                  "to be used to calculate the number density.");
  const std::vector<std::string> units({"Atoms", "Formula Units"});
  declareProperty("NumberDensityUnit", units.front(),
                  boost::make_shared<StringListValidator>(units),
                  "Choose which units SampleNumberDensity referes to.");

  // Perform Group Associations.
  std::string formulaGrp("By Formula or Atomic Number");
  setPropertyGroup("ChemicalFormula", formulaGrp);
  setPropertyGroup("AtomicNumber", formulaGrp);
  setPropertyGroup("MassNumber", formulaGrp);

  std::string densityGrp("Sample Density");
  setPropertyGroup("SampleNumberDensity", densityGrp);
  setPropertyGroup("NumberDensityUnit", densityGrp);
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
  setPropertySettings("NumberDensityUnit",
                      make_unique<Kernel::EnabledWhenProperty>(
                          "SampleNumberDensity", Kernel::IS_NOT_DEFAULT));
}

std::map<std::string, std::string> SetSampleMaterial::validateInputs() {
  params.chemicalSymbol = getPropertyValue("ChemicalFormula");
  params.atomicNumber = getProperty("AtomicNumber");
  params.massNumber = getProperty("MassNumber");
  params.sampleNumberDensity = getProperty("SampleNumberDensity");
  params.zParameter = getProperty("ZParameter");
  params.unitCellVolume = getProperty("UnitCellVolume");
  params.sampleMassDensity = getProperty("SampleMassDensity");
  params.coherentXSection = getProperty("CoherentXSection");
  params.incoherentXSection = getProperty("IncoherentXSection");
  params.attenuationXSection = getProperty("AttenuationXSection");
  params.scatteringXSection = getProperty("ScatteringXSection");
  const std::string numberDensityUnit = getProperty("NumberDensityUnit");
  if (numberDensityUnit == "Atoms") {
    params.numberDensityUnit = MaterialBuilder::NumberDensityUnit::Atoms;
  } else {
    params.numberDensityUnit = MaterialBuilder::NumberDensityUnit::FormulaUnits;
  }
  auto result = ReadMaterial::validateInputs(params);

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
  if (!expInfo) {
    throw std::runtime_error("InputWorkspace does not have a sample object");
  }

  ReadMaterial reader;
  reader.setMaterialParameters(params);

  // get the scattering information - this will override table values
  // create the material
  auto material = reader.buildMaterial();

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

  if (isDefault("SampleNumberDensity") && isDefault("SampleMassDensity")) {
    g_log.information("Unknown value for number density");
  } else {
    const double rho = material->numberDensity();
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
