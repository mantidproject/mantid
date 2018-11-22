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
    std::string chemicalSymbol;
    int atomicNumber;
    int massNumber;
    double sampleNumberDensity;
    double zParameter;
    double unitCellVolume;
    double sampleMassDensity;
  };

  static ValidationErrors validateInputs(MaterialParameters params);
  void setMaterial(const std::string chemicalSymbol, const int atomicNumber,
                   const int massNumber);
  void setNumberDensity(const double rho_m, const double rho,
                        const double zParameter, const double unitCellVolume);
  void setScatteringInfo(double coherentXSection, double incoherentXSection,
                         double attenuationXSection, double scatteringXSection);
  std::unique_ptr<Kernel::Material> buildMaterial();

private:
  Kernel::MaterialBuilder builder;
  static bool isEmpty(const double toCheck);
};
} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_ReadMaterial_H_*/
