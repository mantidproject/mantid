// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/PeakAlgorithmHelpers.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/V3D.h"

using Mantid::Kernel::ArrayLengthValidator;
using Mantid::Kernel::ArrayProperty;
using Mantid::Kernel::BoundedValidator;
using Mantid::Kernel::Direction;
using Mantid::Kernel::V3D;

namespace Mantid::Crystal {

/**
 * Append the common set of properties that relate to modulation vectors
 * to the given algorithm
 * @param alg A pointer to the algorithm that will receive the properties
 */
void ModulationProperties::appendTo(API::IAlgorithm *alg) {
  auto mustBeLengthThree = boost::make_shared<ArrayLengthValidator<double>>(3);
  alg->declareProperty(
      std::make_unique<ArrayProperty<double>>(ModulationProperties::ModVector1,
                                              "0.0,0.0,0.0", mustBeLengthThree),
      "Modulation Vector 1: dh, dk, dl");
  alg->declareProperty(
      std::make_unique<ArrayProperty<double>>(ModulationProperties::ModVector2,
                                              "0.0,0.0,0.0", mustBeLengthThree),
      "Modulation Vector 2: dh, dk, dl");
  alg->declareProperty(
      std::make_unique<ArrayProperty<double>>(ModulationProperties::ModVector3,
                                              "0.0,0.0,0.0", mustBeLengthThree),
      "Modulation Vector 3: dh, dk, dl");
  auto mustBePositiveOrZero = boost::make_shared<BoundedValidator<int>>();
  mustBePositiveOrZero->setLower(0);
  alg->declareProperty(ModulationProperties::MaxOrder, 0, mustBePositiveOrZero,
                       "Maximum order to apply Modulation Vectors. Default = 0",
                       Direction::Input);
  alg->declareProperty(
      ModulationProperties::CrossTerms, false,
      "Include combinations of modulation vectors in satellite search",
      Direction::Input);
}

/**
 * Create a ModulationProperties object from an algorithm
 * @param alg An algorithm containing the user input
 * @return A new ModulationProperties object
 */
ModulationProperties ModulationProperties::create(const API::IAlgorithm &alg) {
  const int maxOrder{alg.getProperty(ModulationProperties::MaxOrder)};
  const bool crossTerms{alg.getProperty(ModulationProperties::CrossTerms)};
  auto offsets = generateOffsetVectors(
      validModulationVectors(alg.getProperty(ModulationProperties::ModVector1),
                             alg.getProperty(ModulationProperties::ModVector2),
                             alg.getProperty(ModulationProperties::ModVector3)),
      maxOrder, crossTerms);
  const bool saveOnLattice{true};
  return {std::move(offsets), maxOrder, crossTerms, saveOnLattice};
}

/**
 * Check each input is a valid modulation and add it to a list
 * to return.
 * @param modVector1 List of 3 doubles specifying an offset
 * @param modVector2 List of 3 doubles specifying an offset
 * @param modVector3 List of 3 doubles specifying an offset
 * @return A list of valid modulation vectors
 */
std::vector<Kernel::V3D>
validModulationVectors(const std::vector<double> &modVector1,
                       const std::vector<double> &modVector2,
                       const std::vector<double> &modVector3) {
  std::vector<V3D> modVectors;
  auto addIfNonZero = [&modVectors](const auto &modVec) {
    if (std::fabs(modVec[0]) > 0 || std::fabs(modVec[1]) > 0 ||
        std::fabs(modVec[2]) > 0)
      modVectors.emplace_back(V3D(modVec[0], modVec[1], modVec[2]));
  };
  addIfNonZero(modVector1);
  addIfNonZero(modVector2);
  addIfNonZero(modVector3);
  return modVectors;
}

/**
 * @param maxOrder Integer specifying the multiples of the
 * modulation vector.
 * @param modVectors A list of modulation vectors form the user
 * @param crossTerms If true then compute products of the
 * modulation vectors
 * @return A list of (m, n, p, V3D) were m,n,p specifies the
 * modulation structure number and V3D specifies the offset to
 * be tested
 */
std::vector<MNPOffset>
generateOffsetVectors(const std::vector<Kernel::V3D> &modVectors,
                      const int maxOrder, const bool crossTerms) {
  assert(modVectors.size() <= 3);

  std::vector<MNPOffset> offsets;
  if (crossTerms && modVectors.size() > 1) {
    const auto &modVector0{modVectors[0]}, modVector1{modVectors[1]};
    if (modVectors.size() == 2) {
      // Calculate m*mod_vec1 + n*mod_vec2 for combinations of
      // m, n in
      // [-maxOrder,maxOrder]
      offsets.reserve(2 * maxOrder);
      for (auto m = -maxOrder; m <= maxOrder; ++m) {
        for (auto n = -maxOrder; n <= maxOrder; ++n) {
          if (m == 0 && n == 0)
            continue;
          offsets.emplace_back(
              std::make_tuple(m, n, 0, modVector0 * m + modVector1 * n));
        }
      }
    } else {
      // Calculate m*mod_vec1 + n*mod_vec2 + p*mod_vec3 for
      // combinations of m, n, p in [-maxOrder,maxOrder]
      const auto &modVector2{modVectors[2]};
      offsets.reserve(3 * maxOrder);
      for (auto m = -maxOrder; m <= maxOrder; ++m) {
        for (auto n = -maxOrder; n <= maxOrder; ++n) {
          for (auto p = -maxOrder; p <= maxOrder; ++p) {
            if (m == 0 && n == 0 && p == 0)
              continue;
            offsets.emplace_back(std::make_tuple(
                m, n, p, modVector0 * m + modVector1 * n + modVector2 * p));
          }
        }
      }
    }
  } else {
    // No cross terms: Compute coeff*mod_vec_i for each
    // modulation vector separately for coeff in [-maxOrder,
    // maxOrder]
    for (auto i = 0u; i < modVectors.size(); ++i) {
      const auto &modVector = modVectors[i];
      for (int order = -maxOrder; order <= maxOrder; ++order) {
        if (order == 0)
          continue;
        V3D offset{modVector * order};
        switch (i) {
        case 0:
          offsets.emplace_back(std::make_tuple(order, 0, 0, std::move(offset)));
          break;
        case 1:
          offsets.emplace_back(std::make_tuple(0, order, 0, std::move(offset)));
          break;
        case 2:
          offsets.emplace_back(std::make_tuple(0, 0, order, std::move(offset)));
          break;
        }
      }
    }
  }

  return offsets;
}

/**
 * The final offset vector is computed as
 * (hoffsets[i],koffsets[j],loffsets[k]) for each combination of
 * i,j,k. All offsets are considered order 1
 * @param hOffsets A list of offsets in the h direction
 * @param kOffsets A list of offsets in the k direction
 * @param lOffsets A list of offsets in the l direction
 * @return A list of (1, 1, 1, V3D) were m,n,p specifies the
 * modulation structure number and V3D specifies the offset to
 * be tested */
std::vector<MNPOffset>
generateOffsetVectors(const std::vector<double> &hOffsets,
                      const std::vector<double> &kOffsets,
                      const std::vector<double> &lOffsets) {
  std::vector<MNPOffset> offsets;
  for (double hOffset : hOffsets) {
    for (double kOffset : kOffsets) {
      for (double lOffset : lOffsets) {
        // mnp = 0, 0, 0 as
        // it's not quite clear how to interpret them as mnp indices
        offsets.emplace_back(
            std::make_tuple(0, 0, 0, V3D(hOffset, kOffset, lOffset)));
      }
    }
  }
  return offsets;
}

} // namespace Mantid::Crystal
