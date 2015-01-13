#ifndef MANTID_CRYSTAL_COMPOSITECLUSTER_H_
#define MANTID_CRYSTAL_COMPOSITECLUSTER_H_

#include "MantidKernel/System.h"
#include "MantidCrystal/ICluster.h"
#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>

namespace Mantid {
namespace Crystal {

/** CompositeCluster : Cluster made by by merging other IClusters.

 Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
 National Laboratory & European Spallation Source

 This file is part of Mantid.

 Mantid is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 Mantid is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 File change history is stored at: <https://github.com/mantidproject/mantid>
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport CompositeCluster : public ICluster {
public:
  CompositeCluster();
  virtual ~CompositeCluster();

  /// integrate the cluster
  ICluster::ClusterIntegratedValues
  integrate(boost::shared_ptr<const Mantid::API::IMDHistoWorkspace> ws) const;

  /// Apply labels to the workspace
  void writeTo(boost::shared_ptr<Mantid::API::IMDHistoWorkspace> ws) const;

  /// Original label
  size_t getOriginalLabel() const;

  /// Get the cluster label
  size_t getLabel() const;

  /// Number of indexes tracked
  size_t size() const;

  /// Track a linear IMDHistoWorkspace index that belongs to the cluster.
  void addIndex(const size_t &index);

  /// Resolve the proper label for this cluster.
  void toUniformMinimum(std::vector<DisjointElement> &disjointSet);

  /// Own.
  void add(boost::shared_ptr<ICluster> &toOwn);

  /// Set the root cluster
  void setRootCluster(ICluster const *root);

  /// Get a representative index of the cluster
  size_t getRepresentitiveIndex() const;

  /// Is a given label part of this cluster
  bool containsLabel(const size_t &label) const;

private:
  /// Helper method to find the minimum label.
  void findMinimum() const;

  /// Disabled copy construction
  CompositeCluster(const CompositeCluster &);

  /// Disabled assignement
  CompositeCluster &operator=(const CompositeCluster &);

  // void validateNoRepeat(CompositeCluster*const other) const;

  /// Label used by cluster
  mutable boost::optional<size_t> m_label;
  /// Attached clusters.
  std::vector<boost::shared_ptr<ICluster>> m_ownedClusters;
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_COMPOSITECLUSTER_H_ */
