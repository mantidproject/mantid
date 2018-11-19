// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/ReadMaterial.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/Workspace.h"
#include <iostream>

namespace Mantid {
namespace DataHandling {

std::map<std::string, std::string> ReadMaterial::validateInputs(
    const std::string chemicalSymbol, const int z_number, const int a_number,
    const double sampleNumberDensity, const double zParameter,
    const double unitCellVolume, const double sampleMassDensity) {
  std::map<std::string, std::string> result;
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

std::unique_ptr<Kernel::Material> ReadMaterial::buildMaterial() {
  return std::make_unique<Kernel::Material>(builder.build());
}

void ReadMaterial::determineMaterial(const std::string chemicalSymbol,
                                     const int z_number, const int a_number) {
  if (!chemicalSymbol.empty()) {
    std::cout << "CHEM: " << chemicalSymbol << std::endl;
    builder.setFormula(chemicalSymbol);
  } else {
    builder.setAtomicNumber(z_number);
    builder.setMassNumber(a_number);
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

void ReadMaterial::setScatteringInfo(double CoherentXSection,
                                     double IncoherentXSection,
                                     double AttenuationXSection,
                                     double ScatteringXSection) {
  builder.setCoherentXSection(CoherentXSection);       // in barns
  builder.setIncoherentXSection(IncoherentXSection);   // in barns
  builder.setAbsorptionXSection(AttenuationXSection);  // in barns
  builder.setTotalScatterXSection(ScatteringXSection); // in barns
}

bool ReadMaterial::isEmpty(const double toCheck) {
  return std::abs((toCheck - EMPTY_DBL()) / (EMPTY_DBL())) < 1e-8;
}
} // namespace DataHandling
} // namespace Mantid