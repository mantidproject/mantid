// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_ReadMaterial_H_
#define MANTID_DATAHANDLING_ReadMaterial_H_

#include "MantidKernel/Material.h"
#include "MantidKernel/MaterialBuilder.h"

#include <map>
namespace Mantid {
namespace DataHandling {

/**
    This class contains code for interpreting a material input for
   SetSampleMaterial
*/
class DLLExport ReadMaterial {
public:
  static std::map<std::string, std::string>
  validateInputs(const std::string chemicalSymbol, const int z_number,
                 const int a_number, const double sampleNumberDensity,
                 const double zParameter, const double unitCellVolume,
                 const double sampleMassDensity);
  void determineMaterial(const std::string chemicalSymbol, const int z_number,
                         const int a_number);
  void setNumberDensity(const double rho_m, const double rho,
                        const double zParameter, const double unitCellVolume);
  void setScatteringInfo(double CoherentXSection, double IncoherentXSection,
                         double AttenuationXSection, double ScatteringXSection);
  std::unique_ptr<Kernel::Material> buildMaterial();

private:
  Kernel::MaterialBuilder builder;
  static bool isEmpty(const double toCheck);
};
} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_ReadMaterial_H_*/
