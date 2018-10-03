// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CRYSTAL_COMPOSITECLUSTER_H_
#define MANTID_CRYSTAL_COMPOSITECLUSTER_H_

#include "MantidCrystal/ICluster.h"
#include "MantidKernel/System.h"
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

namespace Mantid {
namespace Crystal {

/** CompositeCluster : Cluster made by by merging other IClusters.
 */
class DLLExport CompositeCluster : public ICluster {
public:
  CompositeCluster() = default;
  CompositeCluster(const CompositeCluster &) = delete;
  CompositeCluster &operator=(const CompositeCluster &) = delete;
  /// integrate the cluster
  ICluster::ClusterIntegratedValues
  integrate(boost::shared_ptr<const Mantid::API::IMDHistoWorkspace> ws)
      const override;

  /// Apply labels to the workspace
  void
  writeTo(boost::shared_ptr<Mantid::API::IMDHistoWorkspace> ws) const override;

  /// Original label
  size_t getOriginalLabel() const override;

  /// Get the cluster label
  size_t getLabel() const override;

  /// Number of indexes tracked
  size_t size() const override;

  /// Track a linear IMDHistoWorkspace index that belongs to the cluster.
  void addIndex(const size_t &index) override;

  /// Resolve the proper label for this cluster.
  void toUniformMinimum(std::vector<DisjointElement> &disjointSet) override;

  /// Own.
  void add(boost::shared_ptr<ICluster> &toOwn);

  /// Set the root cluster
  void setRootCluster(ICluster const *root) override;

  /// Get a representative index of the cluster
  size_t getRepresentitiveIndex() const override;

  /// Is a given label part of this cluster
  bool containsLabel(const size_t &label) const override;

private:
  /// Helper method to find the minimum label.
  void findMinimum() const;

  // void validateNoRepeat(CompositeCluster*const other) const;

  /// Label used by cluster
  mutable boost::optional<size_t> m_label;
  /// Attached clusters.
  std::vector<boost::shared_ptr<ICluster>> m_ownedClusters;
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_COMPOSITECLUSTER_H_ */
