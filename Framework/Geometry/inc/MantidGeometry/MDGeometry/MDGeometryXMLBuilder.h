// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include <algorithm>
#include <string>
#include <vector>

namespace Mantid {
namespace Geometry {

/**
 @class  MDGeometryBuliderXML
 @brief Computes Boolean algebra for simplification
 @author Owen Arnold
 @date May 2011
 @version 1.0

 Handles the generation of well formed description of a geometry based on input
 IMDDimensions.
 Outputs xml.

 */
template <typename CheckDimensionPolicy> class MANTID_GEOMETRY_DLL MDGeometryBuilderXML {

public:
  /// Constructor
  MDGeometryBuilderXML();

  /// Destructor
  ~MDGeometryBuilderXML();

  /// Add a dimension that is neither considered x, y, z or t.
  bool addOrdinaryDimension(IMDDimension_const_sptr dimensionToAdd) const;

  /// Add many ordinary dimensions.
  void addManyOrdinaryDimensions(const VecIMDDimension_sptr &manyDims) const;

  /// Add x dimension.
  bool addXDimension(const IMDDimension_const_sptr &dimension) const;

  /// Add y dimension.
  bool addYDimension(const IMDDimension_const_sptr &dimension) const;

  /// Add z dimension.
  bool addZDimension(const IMDDimension_const_sptr &dimension) const;

  /// Add t dimension.
  bool addTDimension(const IMDDimension_const_sptr &dimension) const;

  /// Copy constructor
  MDGeometryBuilderXML(const MDGeometryBuilderXML &);

  /// Assignment Operator
  MDGeometryBuilderXML &operator=(const MDGeometryBuilderXML &);

  /// Create the xml.
  const std::string &create() const;

  /// Determine if a valid x dimension has been provided.
  bool hasXDimension() const;

  /// Determine whether a valid y dimension has been provided.
  bool hasYDimension() const;

  /// Determine whether a valid z dimension has been provided.
  bool hasZDimension() const;

  /// Determine whether a valid t dimension has been provided.
  bool hasTDimension() const;

  /// Determine whether an integrated t dimension has been provided.
  bool hasIntegratedTDimension() const;

private:
  using DimensionContainerType = std::vector<IMDDimension_const_sptr>;

  mutable DimensionContainerType m_vecDimensions;

  mutable IMDDimension_const_sptr m_spXDimension;

  mutable IMDDimension_const_sptr m_spYDimension;

  mutable IMDDimension_const_sptr m_spZDimension;

  mutable IMDDimension_const_sptr m_spTDimension;

  /// Instantiate and apply the checking policy.
  void applyPolicyChecking(const IMDDimension &dimensionToAdd) const;

  /// Flag indicating that some change in the inputs has occured. Triggers full
  /// recreation.
  mutable bool m_changed;

  /// Variable suports lazy calculation.
  mutable std::string m_lastResult;
};

/*
 @class StrictDimensionPolicy
 @brief Unary operator that throws if the dimension provided is integrated.
 @author Owen Arnold
 @date May 2011
 @version 1.0
*/
struct MANTID_GEOMETRY_DLL
StrictDimensionPolicy{public : StrictDimensionPolicy(){} void operator()(const IMDDimension &item){
    if (true == item.getIsIntegrated()){std::string message = "StrictDimensionPolicy bans the use of integrated "
                                                              "IMDDimensions mapped to x, y, z or t in a "
                                                              "IMDWorkspace."
                                                              "Attempted to do so with IMDDimension: " +
                                                              item.getDimensionId();
throw std::invalid_argument(message);
} // namespace Geometry
} // namespace Mantid
}
;

/*
 @class NoDimensionPolicy
 @brief Unary operator that has no effect.
 @author Owen Arnold
 @date May 2011
*/
struct MANTID_GEOMETRY_DLL NoDimensionPolicy {
  void
  operator()(const IMDDimension &){
      // Do nothing.
  }
};
} // namespace Geometry
} // namespace Mantid
