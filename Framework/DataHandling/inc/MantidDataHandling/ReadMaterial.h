// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_ReadMaterial_H_
#define MANTID_DATAHANDLING_ReadMaterial_H_

#include "MantidAPI/DistributedAlgorithm.h"
#include "MantidKernel/NeutronAtom.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/MaterialBuilder.h"

namespace Mantid {
namespace DataHandling {

/**
    This class contains code for interpreting a material input for SetSampleMaterial
*/
class DLLExport ReadMaterial {
public:
  static std::map<std::string, std::string> validateInputs(const std::string, const int, const int, const double, const double, const double, const double);
  void setScatteringInfo(const double, const double, const double, const double);
  void determineMaterial(const std::string, const int, const int);
  void setNumberDensity(const double, const double, const double, const double);
  std::unique_ptr<Kernel::Material> buildMaterial();
private:
  Kernel::MaterialBuilder builder;
  static bool isEmpty(const double toCheck);

};
} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_ReadMaterial_H_*/
