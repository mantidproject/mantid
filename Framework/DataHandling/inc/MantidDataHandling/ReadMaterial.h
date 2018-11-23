// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_ReadMaterial_H_
#define MANTID_DATAHANDLING_ReadMaterial_H_

#include "MantidKernel/MaterialBuilder.h"

#include <map>
namespace Mantid {
namespace Kernel {
class Material;
}
namespace DataHandling {
using ValidationErrors = std::map<std::string, std::string>;
/**
    This class contains code for interpreting a material input for
   SetSampleMaterial
*/

class DLLExport ReadMaterial {
public:
  struct MaterialParameters {
    std::string chemicalSymbol = "";
    int atomicNumber = 0;
    int massNumber = 0;
    double sampleNumberDensity = EMPTY_DOUBLE_VAL;
    double zParameter = EMPTY_DOUBLE_VAL;
    double unitCellVolume = EMPTY_DOUBLE_VAL;
    double sampleMassDensity = EMPTY_DOUBLE_VAL;
    double coherentXSection = EMPTY_DOUBLE_VAL;
    double incoherentXSection = EMPTY_DOUBLE_VAL;
    double attenuationXSection = EMPTY_DOUBLE_VAL;
    double scatteringXSection = EMPTY_DOUBLE_VAL;
  };

  static ValidationErrors validateInputs(MaterialParameters params);
  void setMaterialParameters(MaterialParameters params);
  std::unique_ptr<Kernel::Material> buildMaterial();

private:
  Kernel::MaterialBuilder builder;
  static constexpr double EMPTY_DOUBLE_VAL = 8.9884656743115785e+307;
  void setMaterial(const std::string chemicalSymbol, const int atomicNumber,
                   const int massNumber);
  void setNumberDensity(const double rho_m, const double rho,
                        const double zParameter, const double unitCellVolume);
  void setScatteringInfo(double coherentXSection, double incoherentXSection,
                         double attenuationXSection, double scatteringXSection);

  static bool isEmpty(const double toCheck);
};
} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_ReadMaterial_H_*/
