// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
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
ValidationErrors
ReadMaterial::validateInputs(const MaterialParameters &params) {
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
    if (isEmpty(params.attenuationXSection)) {
      result["AttenuationXSection"] = "The cross section must be specified "
                                      "when no ChemicalFormula or AtomicNumber "
                                      "is given.";
    }
    if (isEmpty(params.scatteringXSection)) {
      result["ScatteringXSection"] = "The cross section must be specified when "
                                     "no ChemicalFormula or AtomicNumber is "
                                     "given.";
    }
    if (isEmpty(params.sampleNumberDensity)) {
      result["SampleNumberDensity"] =
          "The number density must be specified with a use-defined material.";
    }
  } else if (chemicalSymbol && atomicNumber) {
    result["AtomicNumber"] =
        "Cannot specify both ChemicalFormula and AtomicNumber";
  }

  if (params.massNumber > 0 && params.atomicNumber <= 0)
    result["AtomicNumber"] = "Specified MassNumber without AtomicNumber";

  if (!isEmpty(params.zParameter)) {
    if (isEmpty(params.unitCellVolume)) {
      result["UnitCellVolume"] =
          "UnitCellVolume must be provided with ZParameter";
    }
    if (!isEmpty(params.sampleNumberDensity)) {
      result["ZParameter"] =
          "Cannot give ZParameter with SampleNumberDensity set";
    }
    if (!isEmpty(params.sampleMassDensity)) {
      result["SampleMassDensity"] =
          "Cannot give SampleMassDensity with ZParameter set";
    }
  } else if (!isEmpty(params.sampleNumberDensity)) {
    if (!isEmpty(params.sampleMassDensity)) {
      result["SampleMassDensity"] =
          "Cannot give SampleMassDensity with SampleNumberDensity set";
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
  setNumberDensity(params.sampleMassDensity, params.sampleNumberDensity,
                   params.numberDensityUnit, params.zParameter,
                   params.unitCellVolume);
  setScatteringInfo(params.coherentXSection, params.incoherentXSection,
                    params.attenuationXSection, params.scatteringXSection);
}

/**
 * Construct the material,
 *
 *  @returns A unique pointer to the newly made material
 */
std::unique_ptr<Kernel::Material> ReadMaterial::buildMaterial() {
  return std::make_unique<Kernel::Material>(builder.build());
}

void ReadMaterial::setMaterial(const std::string chemicalSymbol,
                               const int atomicNumber, const int massNumber) {
  if (!chemicalSymbol.empty()) {
    builder.setFormula(chemicalSymbol);
  } else if (atomicNumber != 0) {
    builder.setAtomicNumber(atomicNumber);
    builder.setMassNumber(massNumber);
  }
}

void ReadMaterial::setNumberDensity(
    const double rho_m, const double rho,
    Kernel::MaterialBuilder::NumberDensityUnit rhoUnit, const double zParameter,
    const double unitCellVolume) {
  if (!isEmpty(rho_m))
    builder.setMassDensity(rho_m);
  if (isEmpty(rho)) {
    if (!isEmpty(zParameter)) {
      builder.setZParameter(zParameter);
      builder.setUnitCellVolume(unitCellVolume);
    }
  } else {
    builder.setNumberDensity(rho);
    builder.setNumberDensityUnit(rhoUnit);
  }
}

void ReadMaterial::setScatteringInfo(double coherentXSection,
                                     double incoherentXSection,
                                     double attenuationXSection,
                                     double scatteringXSection) {
  builder.setCoherentXSection(coherentXSection);       // in barns
  builder.setIncoherentXSection(incoherentXSection);   // in barns
  builder.setAbsorptionXSection(attenuationXSection);  // in barns
  builder.setTotalScatterXSection(scatteringXSection); // in barns
}

bool ReadMaterial::isEmpty(const double toCheck) {
  return std::abs((toCheck - EMPTY_DBL()) / (EMPTY_DBL())) < 1e-8;
}
} // namespace DataHandling
} // namespace Mantid
