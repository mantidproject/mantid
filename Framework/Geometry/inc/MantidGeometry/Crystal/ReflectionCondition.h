// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/DllConfig.h"

#ifndef Q_MOC_RUN
#include <memory>
#endif

#include <string>
#include <vector>

namespace Mantid {
namespace Geometry {

/** A class containing the Reflection Condition for a crystal.
 * e.g. Face-centered, etc.
 * determining which HKL's are allows and which are not.
 *
 * @author Janik Zikovsky
 * @date 2011-05-16 11:55:15.983855
 */
class MANTID_GEOMETRY_DLL ReflectionCondition {
public:
  virtual ~ReflectionCondition() = default;
  /// Name of the reflection condition
  virtual std::string getName() = 0;
  /// Symbol of the associated lattice centering.
  virtual std::string getSymbol() = 0;
  /// Return true if the hkl is allowed.
  virtual bool isAllowed(int h, int k, int l) = 0;
};

//------------------------------------------------------------------------
/** Primitive ReflectionCondition */
class MANTID_GEOMETRY_DLL ReflectionConditionPrimitive : public ReflectionCondition {
public:
  /// Name of the reflection condition
  std::string getName() override { return "Primitive"; }
  /// Symbol of the associated lattice centering.
  std::string getSymbol() override { return "P"; }
  /// Return true if the hkl is allowed.
  bool isAllowed(int /*h*/, int /*k*/, int /*l*/) override { return true; }
};

//------------------------------------------------------------------------
/** C-face centred ReflectionCondition */
class MANTID_GEOMETRY_DLL ReflectionConditionCFaceCentred : public ReflectionCondition {
public:
  /// Name of the reflection condition
  std::string getName() override { return "C-face centred"; }
  /// Symbol of the associated lattice centering.
  std::string getSymbol() override { return "C"; }
  /// Return true if the hkl is allowed.
  bool isAllowed(int h, int k, int /*l*/) override { return (((h + k) % 2) == 0); }
};

//------------------------------------------------------------------------
/** A-face centred ReflectionCondition */
class MANTID_GEOMETRY_DLL ReflectionConditionAFaceCentred : public ReflectionCondition {
public:
  /// Name of the reflection condition
  std::string getName() override { return "A-face centred"; }
  /// Symbol of the associated lattice centering.
  std::string getSymbol() override { return "A"; }
  /// Return true if the hkl is allowed.
  bool isAllowed(int /*h*/, int k, int l) override { return (((k + l) % 2) == 0); }
};

//------------------------------------------------------------------------
/** B-face centred ReflectionCondition */
class MANTID_GEOMETRY_DLL ReflectionConditionBFaceCentred : public ReflectionCondition {
public:
  /// Name of the reflection condition
  std::string getName() override { return "B-face centred"; }
  /// Symbol of the associated lattice centering.
  std::string getSymbol() override { return "B"; }
  /// Return true if the hkl is allowed.
  bool isAllowed(int h, int /*k*/, int l) override { return (((h + l) % 2) == 0); }
};

//------------------------------------------------------------------------
/** Body centred ReflectionCondition */
class MANTID_GEOMETRY_DLL ReflectionConditionBodyCentred : public ReflectionCondition {
public:
  /// Name of the reflection condition
  std::string getName() override { return "Body centred"; }
  /// Symbol of the associated lattice centering.
  std::string getSymbol() override { return "I"; }
  /// Return true if the hkl is allowed.
  bool isAllowed(int h, int k, int l) override { return ((h + k + l) % 2) == 0; }
};

//------------------------------------------------------------------------
/** All-face centred ReflectionCondition */
class MANTID_GEOMETRY_DLL ReflectionConditionAllFaceCentred : public ReflectionCondition {
public:
  /// Name of the reflection condition
  std::string getName() override { return "All-face centred"; }
  /// Symbol of the associated lattice centering.
  std::string getSymbol() override { return "F"; }
  /// Return true if the hkl is allowed.
  bool isAllowed(int h, int k, int l) override {
    return (((((h + k) % 2) == 0) && (((h + l) % 2) == 0) && (((k + l) % 2) == 0)) ||
            ((h % 2 == 0) && (k % 2 == 0) && (l % 2 == 0)) || ((h % 2 == 1) && (k % 2 == 1) && (l % 2 == 1)));
  }
};

//------------------------------------------------------------------------
/** Rhombohedrally centred, obverse ReflectionCondition*/
class MANTID_GEOMETRY_DLL ReflectionConditionRhombohedrallyObverse : public ReflectionCondition {
public:
  /// Name of the reflection condition
  std::string getName() override { return "Rhombohedrally centred, obverse"; }
  /// Symbol of the associated lattice centering.
  std::string getSymbol() override { return "Robv"; }
  /// Return true if the hkl is allowed.
  bool isAllowed(int h, int k, int l) override { return (((-h + k + l) % 3) == 0); }
};

//------------------------------------------------------------------------
/** Rhombohedrally centred, reverse ReflectionCondition*/
class MANTID_GEOMETRY_DLL ReflectionConditionRhombohedrallyReverse : public ReflectionCondition {
public:
  /// Name of the reflection condition
  std::string getName() override { return "Rhombohedrally centred, reverse"; }
  /// Symbol of the associated lattice centering.
  std::string getSymbol() override { return "Rrev"; }
  /// Return true if the hkl is allowed.
  bool isAllowed(int h, int k, int l) override { return (((h - k + l) % 3) == 0); }
};

//------------------------------------------------------------------------
/** Hexagonally centred, reverse ReflectionCondition*/
class MANTID_GEOMETRY_DLL ReflectionConditionHexagonallyReverse : public ReflectionCondition {
public:
  /// Name of the reflection condition
  std::string getName() override { return "Hexagonally centred, reverse"; }
  /// Symbol of the associated lattice centering.
  std::string getSymbol() override { return "H"; }
  /// Return true if the hkl is allowed.
  bool isAllowed(int h, int k, int /*l*/) override { return (((h - k) % 3) == 0); }
};

/// Shared pointer to a ReflectionCondition
using ReflectionCondition_sptr = std::shared_ptr<ReflectionCondition>;
/// A collection of reflections
using ReflectionConditions = std::vector<ReflectionCondition_sptr>;

MANTID_GEOMETRY_DLL const ReflectionConditions &getAllReflectionConditions();
MANTID_GEOMETRY_DLL std::vector<std::string> getAllReflectionConditionNames();
MANTID_GEOMETRY_DLL std::vector<std::string> getAllReflectionConditionSymbols();
MANTID_GEOMETRY_DLL ReflectionCondition_sptr getReflectionConditionByName(const std::string &name);
MANTID_GEOMETRY_DLL ReflectionCondition_sptr getReflectionConditionBySymbol(const std::string &symbol);

} // namespace Geometry
} // namespace Mantid
