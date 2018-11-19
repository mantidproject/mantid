// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/ReadMaterial.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/Algorithm.h"
#include <iostream>

namespace Mantid {
namespace DataHandling {

void ReadMaterial::determineMaterial(const std::string chemicalSymbol, const int z_number, const int a_number){
    if (!chemicalSymbol.empty()) {
    std::cout << "CHEM: " << chemicalSymbol << std::endl;
    builder.setFormula(chemicalSymbol);
  } else {
    builder.setAtomicNumber(z_number);
    builder.setMassNumber(a_number);
  }
  m_materialDetermined = true;
}

void ReadMaterial::setNumberDensity(const double rho_m, const double rho, const double zParameter, const double unitCellVolume){
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
  m_numberDensitySet = true;
}


void ReadMaterial::setScatteringInfo(double CoherentXSection, double IncoherentXSection, double AttenuationXSection, double ScatteringXSection){
  builder.setCoherentXSection(CoherentXSection);      // in barns
  builder.setIncoherentXSection(IncoherentXSection);  // in barns
  builder.setAbsorptionXSection(AttenuationXSection); // in barns
  builder.setTotalScatterXSection(ScatteringXSection); // in barns
}


bool ReadMaterial::isEmpty(const double toCheck) {
  return std::abs((toCheck - EMPTY_DBL()) / (EMPTY_DBL())) < 1e-8;
}
}
}