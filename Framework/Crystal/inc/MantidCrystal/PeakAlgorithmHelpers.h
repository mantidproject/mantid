// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CRYSTAL_PEAKALGORITHMHELPERS_H
#define MANTID_CRYSTAL_PEAKALGORITHMHELPERS_H
#include "MantidAPI/IAlgorithm.h"

namespace Mantid::Kernel {
class V3D;
}

namespace Mantid::Crystal {

/// return -1 if convention is "Crystallography" and 1 otherwise.
double qConventionFactor(const std::string &convention);

/// convenience overload to pull the convention from the config service
double qConventionFactor();

/// Tie together a modulated peak number with its offset
using MNPOffset = std::tuple<double, double, double, Kernel::V3D>;

/// Tie together the names of the properties for the modulation vectors
struct ModulationProperties {
  inline const static std::string ModVector1{"ModVector1"};
  inline const static std::string ModVector2{"ModVector2"};
  inline const static std::string ModVector3{"ModVector3"};
  inline const static std::string MaxOrder{"MaxOrder"};
  inline const static std::string CrossTerms{"CrossTerms"};

  static void appendTo(API::IAlgorithm *alg);
  static ModulationProperties create(const API::IAlgorithm &alg);

  std::vector<MNPOffset> offsets;
  int maxOrder;
  bool crossTerms;
  bool saveOnLattice;
};

/// Create a list of valid modulation vectors from the input
std::vector<Kernel::V3D>
validModulationVectors(const std::vector<double> &modVector1,
                       const std::vector<double> &modVector2,
                       const std::vector<double> &modVector3);

/// Calculate a list of HKL offsets from the given modulation vectors.
std::vector<MNPOffset>
generateOffsetVectors(const std::vector<Kernel::V3D> &modVectors,
                      const int maxOrder, const bool crossTerms);
/// Calculate a list of HKL offsets from the given lists of offsets
std::vector<MNPOffset>
generateOffsetVectors(const std::vector<double> &hOffsets,
                      const std::vector<double> &kOffsets,
                      const std::vector<double> &lOffsets);
} // namespace Mantid::Crystal

#endif // MANTID_CRYSTAL_PEAKALGORITHMHELPERS_H
