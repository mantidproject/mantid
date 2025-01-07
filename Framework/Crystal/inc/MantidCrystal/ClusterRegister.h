// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidCrystal/DisjointElement.h"
#include "MantidCrystal/DllConfig.h"
#include <boost/scoped_ptr.hpp>
#include <map>
#include <memory>
#include <vector>

namespace Mantid {
namespace Crystal {
class ICluster;
class ImplClusterRegister;

/** ClusterRegister : A fly-weight ICluster regeister. Handles the logic of
  merging clusters.
*/
class MANTID_CRYSTAL_DLL ClusterRegister {
public:
  /// Cluster map
  using MapCluster = std::map<size_t, std::shared_ptr<ICluster>>;

  /// Constructor
  ClusterRegister();

  /// Add clusters
  void add(const size_t &label, const std::shared_ptr<ICluster> &cluster);

  /// Merge clusters on the basis of known pairs of disjoint elements
  void merge(const DisjointElement &a, const DisjointElement &b) const;

  /// Get all combined clusters
  MapCluster clusters(std::vector<DisjointElement> &elements) const;

  /// Get all combined clusters
  MapCluster clusters() const;

  /// Destructor
  virtual ~ClusterRegister();

private:
  /// Pointer to implementation
  boost::scoped_ptr<ImplClusterRegister> m_Impl;
};

} // namespace Crystal
} // namespace Mantid
