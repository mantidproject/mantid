// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_ReadMaterial_H_
#define MANTID_DATAHANDLING_ReadMaterial_H_

#include "MantidKernel/EmptyValues.h"
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
   SetSampleMaterial, validating the parameters before sending them on to
   MaterialBuilder
*/

class DLLExport ReadMaterial {
public:
  /**
    This struct contains the parameters for constructing a material,
    and gives them a default value for ease of testing
*/
  struct MaterialParameters {
    /// The chemical formula to set, defaults to the empty string
    std::string chemicalSymbol = "";
    /// The atomic number to set, defaults to 0
    int atomicNumber = 0;
    /// The mass number to set, defaults to 0
    int massNumber = 0;
    /// The sample number density to set, defaults to EMPTY_DBL()
    double sampleNumberDensity = EMPTY_DBL();
    /// The zParameter to set, defaults to EMPTY_DBL()
    double zParameter = EMPTY_DBL();
    /// The unit cell volume to set, defaults to EMPTY_DBL()
    double unitCellVolume = EMPTY_DBL();
    /// The sample mass density to set, defaults to EMPTY_DBL()
    double sampleMassDensity = EMPTY_DBL();
    /// The coherent scattering cross section to set, defaults to EMPTY_DBL()
    double coherentXSection = EMPTY_DBL();
    /// The incoherent scattering cross section to set, defaults to EMPTY_DBL()
    double incoherentXSection = EMPTY_DBL();
    /// The absorption cross section to set, defaults to EMPTY_DBL()
    double attenuationXSection = EMPTY_DBL();
    /// The total scattering cross section to set, defaults to EMPTY_DBL()
    double scatteringXSection = EMPTY_DBL();
    /// A flag indicating the unit of sampleNumberDensity
    Kernel::MaterialBuilder::NumberDensityUnit numberDensityUnit =
        Kernel::MaterialBuilder::NumberDensityUnit::Atoms;
  };
  /**
   * Validate the parameters to build the material from, this returns
   * any errors in the inputs.
   *
   * @param params A struct containing all the parameters to be set.
   * @returns A map containing the relevent failure messages, if any.
   */
  static ValidationErrors validateInputs(const MaterialParameters &params);
  /**
   * Set the parameters to build the material to the builder,
   * taking into account which values were and weren't set.
   *
   * @param params A struct containing all the parameters to be set.
   */
  void setMaterialParameters(const MaterialParameters &params);
  /**
   * Construct the material,
   *
   *  @returns A unique pointer to the newly made material
   */
  std::unique_ptr<Kernel::Material> buildMaterial();

private:
  /**
   * @brief The builder used to construct the material
   *
   */
  Kernel::MaterialBuilder builder;

  void setMaterial(const std::string chemicalSymbol, const int atomicNumber,
                   const int massNumber);

  void
  setNumberDensity(const double rho_m, const double rho,
                   const Kernel::MaterialBuilder::NumberDensityUnit rhoUnit,
                   const double zParameter, const double unitCellVolume);
  void setScatteringInfo(double coherentXSection, double incoherentXSection,
                         double attenuationXSection, double scatteringXSection);

  static bool isEmpty(const double toCheck);
};
} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_ReadMaterial_H_*/
