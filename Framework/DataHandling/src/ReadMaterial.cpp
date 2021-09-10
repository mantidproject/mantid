// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/ReadMaterial.h"
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/Material.h"

namespace Mantid {
namespace DataHandling {

/**
 * Validate the parameters to build the material from, this returns
 * any errors in the inputs.
 *
 * @param params A struct containing all the parameters to be set.
 * @returns A map containing the relevent failure messages, if any.
 */
ValidationErrors ReadMaterial::validateInputs(const MaterialParameters &params) {
  ValidationErrors result;
  const bool chemicalSymbol{!params.chemicalSymbol.empty()};
  const bool atomicNumber{params.atomicNumber != 0};
  if (!chemicalSymbol && !atomicNumber) {
    if (isEmpty(params.coherentXSection)) {
      result["CoherentXSection"] = "The cross section must be specified when "
                                   "no ChemicalFormula or AtomicNumber is "
                                   "given.";
    }
    if (isEmpty(params.incoherentXSection)) {
      result["IncoherentXSection"] = "The cross section must be specified when "
                                     "no ChemicalFormula or AtomicNumber is "
                                     "given.";
    }
    if (isEmpty(params.attenuationXSection) && params.attenuationProfileFileName.empty()) {
      result["AttenuationXSection"] = "The cross section must be specified "
                                      "when no ChemicalFormula or AtomicNumber "
                                      "is given.";
    }
    if (isEmpty(params.scatteringXSection)) {
      result["ScatteringXSection"] = "The cross section must be specified when "
                                     "no ChemicalFormula or AtomicNumber is "
                                     "given.";
    }
    if (isEmpty(params.numberDensity) && isEmpty(params.numberDensityEffective) && isEmpty(params.packingFraction)) {
      result["NumberDensity"] = "The number density or effective number density must "
                                " be specified with a user-defined material";
    }

  } else if (chemicalSymbol && atomicNumber) {
    result["AtomicNumber"] = "Cannot specify both ChemicalFormula and AtomicNumber";
  }

  if (!isEmpty(params.numberDensity) && !isEmpty(params.numberDensityEffective) && !isEmpty(params.packingFraction)) {
    result["NumberDensity"] = "Number Density cannot be determined when "
                              "both the effective number density and "
                              "packing fraction are set. Only two can "
                              "be specified at most.";
  }

  if (isEmpty(params.massDensity) && isEmpty(params.zParameter) && isEmpty(params.unitCellVolume)) {
    // Checks if only the packing fraction has been specified with no other
    // way of computing the number density or eff. number density
    if (isEmpty(params.numberDensity) && isEmpty(params.numberDensityEffective) && !isEmpty(params.packingFraction)) {
      result["PackingFraction"] = "Cannot determine number density from only "
                                  " the packing fraction. The number density "
                                  " or effective number density is also needed.";
    }
  }

  // If these are all set, then number density and eff. number density can be
  // calculated. In this case, make sure the packing frac isn't set
  if (!isEmpty(params.massDensity) && !isEmpty(params.zParameter) && !isEmpty(params.unitCellVolume)) {
    if (!isEmpty(params.packingFraction)) {
      result["PackingFraction"] = "Cannot set packing fraction when both the number density "
                                  "and effective number density are determined from "
                                  "the mass density and cell volume + zParameter.";
    }
  }

  // Effective num density will be overwritten in MaterialBuilder if mass
  // density is set
  if (!isEmpty(params.massDensity)) {
    if (!isEmpty(params.numberDensityEffective)) {
      result["EffectiveNumberDensity"] = "Cannot set effective number density when the mass density "
                                         "is specified. The value specified will be overwritten "
                                         "because it will be computed from the mass density.";
    }
  }

  // Bounds check the packing fraction number [0, 2)
  if (!isEmpty(params.packingFraction)) {
    if (params.packingFraction >= 2.0) {
      result["PackingFraction"] = "Cannot have a packing fraction larger than 2";
    } else if (params.packingFraction < 0.0) {
      result["PackingFraction"] = "Cannot have a packing fraction less than 0";
    }
  }

  if (params.massNumber > 0 && params.atomicNumber <= 0)
    result["AtomicNumber"] = "Specified MassNumber without AtomicNumber";

  if (!isEmpty(params.zParameter)) {
    if (isEmpty(params.unitCellVolume)) {
      result["UnitCellVolume"] = "UnitCellVolume must be provided with ZParameter";
    }
  }
  return result;
}

/**
 * Set the parameters to build the material to the builder,
 * taking into account which values were and weren't set.
 *
 * @param params A struct containing all the parameters to be set.
 */
void ReadMaterial::setMaterialParameters(const MaterialParameters &params) {
  setMaterial(params.chemicalSymbol, params.atomicNumber, params.massNumber);

  // calculate the mass density if it wasn't provided
  double massDensity = params.massDensity;
  if (isEmpty(massDensity)) {
    if (!(isEmpty(params.mass) || isEmpty(params.volume)))
      massDensity = params.mass / params.volume;
  }

  setNumberDensity(massDensity, params.numberDensity, params.numberDensityEffective, params.packingFraction,
                   params.numberDensityUnit, params.zParameter, params.unitCellVolume);
  setScatteringInfo(params.coherentXSection, params.incoherentXSection, params.attenuationXSection,
                    params.scatteringXSection, params.attenuationProfileFileName,
                    params.xRayAttenuationProfileFileName);
}

/**
 * Construct the material,
 *
 *  @returns A unique pointer to the newly made material
 */
std::unique_ptr<Kernel::Material> ReadMaterial::buildMaterial() {
  return std::make_unique<Kernel::Material>(builder.build());
}

void ReadMaterial::setMaterial(const std::string &chemicalSymbol, const int atomicNumber, const int massNumber) {
  if (!chemicalSymbol.empty()) {
    builder.setFormula(chemicalSymbol);
  } else if (atomicNumber != 0) {
    builder.setAtomicNumber(atomicNumber);
    builder.setMassNumber(massNumber);
  }
}

void ReadMaterial::setNumberDensity(const double rho_m, const double rho, const double rho_eff, const double pFrac,
                                    Kernel::MaterialBuilder::NumberDensityUnit rhoUnit, const double zParameter,
                                    const double unitCellVolume) {
  if (!isEmpty(rho_m))
    builder.setMassDensity(rho_m);

  // These can be specified even if mass density set
  if (!isEmpty(zParameter)) {
    builder.setZParameter(zParameter);
    builder.setUnitCellVolume(unitCellVolume);
  }
  if (!isEmpty(rho)) {
    builder.setNumberDensity(rho);
    builder.setNumberDensityUnit(rhoUnit);
  }
  if (!isEmpty(rho_eff)) {
    builder.setEffectiveNumberDensity(rho_eff);
  }
  if (!isEmpty(pFrac)) {
    builder.setPackingFraction(pFrac);
  }
}

void ReadMaterial::setScatteringInfo(double coherentXSection, double incoherentXSection, double attenuationXSection,
                                     double scatteringXSection, std::string attenuationProfileFileName,
                                     std::string xRayAttenuationProfileFileName) {
  builder.setCoherentXSection(coherentXSection);       // in barns
  builder.setIncoherentXSection(incoherentXSection);   // in barns
  builder.setAbsorptionXSection(attenuationXSection);  // in barns
  builder.setTotalScatterXSection(scatteringXSection); // in barns
  builder.setAttenuationProfileFilename(attenuationProfileFileName);
  builder.setXRayAttenuationProfileFilename(xRayAttenuationProfileFileName);
}

bool ReadMaterial::isEmpty(const double toCheck) { return std::abs((toCheck - EMPTY_DBL()) / (EMPTY_DBL())) < 1e-8; }
} // namespace DataHandling
} // namespace Mantid
