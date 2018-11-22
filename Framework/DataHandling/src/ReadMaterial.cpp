// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/ReadMaterial.h"
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/Material.h"

#include <iostream>

namespace Mantid {
namespace DataHandling {

ValidationErrors ReadMaterial::validateInputs(const MaterialParameters params) {
  ValidationErrors result;
  if (params.chemicalSymbol.empty()) {
    if (params.atomicNumber <= 0) {
      result["ChemicalFormula"] = "Need to specify the material";
    }
  } else {
    if (params.atomicNumber > 0)
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
          "Can not give ZParameter with SampleNumberDensity set";
    }
    if (!isEmpty(params.sampleMassDensity)) {
      result["SampleMassDensity"] =
          "Can not give SampleMassDensity with ZParameter set";
    }
  } else if (!isEmpty(params.sampleNumberDensity)) {
    if (!isEmpty(params.sampleMassDensity)) {
      result["SampleMassDensity"] =
          "Can not give SampleMassDensity with SampleNumberDensity set";
    }
  }
  return result;
}

std::unique_ptr<Kernel::Material> ReadMaterial::buildMaterial() {
  return std::make_unique<Kernel::Material>(builder.build());
}

void ReadMaterial::setMaterial(const std::string chemicalSymbol,
                               const int atomicNumber, const int massNumber) {
  if (!chemicalSymbol.empty()) {
    std::cout << "CHEM: " << chemicalSymbol << std::endl;
    builder.setFormula(chemicalSymbol);
  } else {
    builder.setAtomicNumber(atomicNumber);
    builder.setMassNumber(massNumber);
  }
}

void ReadMaterial::setNumberDensity(const double rho_m, const double rho,
                                    const double zParameter,
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