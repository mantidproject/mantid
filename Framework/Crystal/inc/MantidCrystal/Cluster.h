// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidCrystal/DisjointElement.h"
#include "MantidCrystal/DllConfig.h"
#include "MantidCrystal/ICluster.h"
#include <boost/tuple/tuple.hpp>
#include <memory>
#include <vector>

namespace Mantid {
namespace Crystal {

/** Cluster : Image cluster used by connected component labeling
 */
class MANTID_CRYSTAL_DLL Cluster : public ICluster {

public:
  /// Constructor
  Cluster(const size_t &label);

  /// integrate the cluster
  ClusterIntegratedValues integrate(std::shared_ptr<const Mantid::API::IMDHistoWorkspace> ws) const override;

  /// Apply labels to the workspace
  void writeTo(std::shared_ptr<Mantid::API::IMDHistoWorkspace> ws) const override;

  /// Get the cluster label
  size_t getLabel() const override;

  /// Get the original label
  size_t getOriginalLabel() const override;

  /// Number of indexes tracked
  size_t size() const override;

  /// Track a linear IMDHistoWorkspace index that belongs to the cluster.
  void addIndex(const size_t &index) override;

  /// Resolve the proper label for this cluster.
  void toUniformMinimum(std::vector<DisjointElement> &disjointSet) override;

  /// Overloaded equals.
  bool operator==(const Cluster &other) const;

  /// Set the root cluster
  void setRootCluster(ICluster const *root) override;

  /// Get a representative index of the cluster
  size_t getRepresentitiveIndex() const override;

  /// Does the cluster contain the label.
  bool containsLabel(const size_t &label) const override;

private:
  /// Disabled copy construction
  Cluster(const Cluster &);
  /// Disabled assignement
  Cluster &operator=(const Cluster &);
  /// original label on cluster
  size_t m_originalLabel;
  /// indexes belonging to cluster. This is how we track cluster objects.
  std::vector<size_t> m_indexes;
  /// Root cluster.
  ICluster const *m_rootCluster;
};

} // namespace Crystal
} // namespace Mantid
