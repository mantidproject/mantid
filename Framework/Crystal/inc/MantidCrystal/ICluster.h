// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CRYSTAL_ICLUSTER_H_
#define MANTID_CRYSTAL_ICLUSTER_H_

#include "MantidCrystal/DisjointElement.h"
#include "MantidKernel/System.h"
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>
#include <vector>

namespace Mantid {
namespace API {
class IMDHistoWorkspace;
}
namespace Crystal {

/** ICluster : Abstract cluster. Identifies neighbour elements in an image that
 are connected.
 */
class DLLExport ICluster {
public:
  using ClusterIntegratedValues = boost::tuple<double, double>;

  /// integrate the cluster
  virtual ClusterIntegratedValues integrate(
      boost::shared_ptr<const Mantid::API::IMDHistoWorkspace> ws) const = 0;

  /// Apply labels to the workspace
  virtual void
  writeTo(boost::shared_ptr<Mantid::API::IMDHistoWorkspace> ws) const = 0;

  /// Get the originally set label
  virtual size_t getOriginalLabel() const = 0;

  /// Get the cluster label
  virtual size_t getLabel() const = 0;

  /// Number of indexes tracked
  virtual size_t size() const = 0;

  /// Track a linear IMDHistoWorkspace index that belongs to the cluster.
  virtual void addIndex(const size_t &index) = 0;

  /// Resolve the proper label for this cluster.
  virtual void toUniformMinimum(std::vector<DisjointElement> &disjointSet) = 0;

  /// Virtual destructor
  virtual ~ICluster() = default;

  /// Set the root cluster
  virtual void setRootCluster(ICluster const *root) = 0;

  /// Get a represetiative index of the cluster
  virtual size_t getRepresentitiveIndex() const = 0;

  /// Is the label contained in the cluster
  virtual bool containsLabel(const size_t &label) const = 0;
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_ICLUSTER_H_ */
