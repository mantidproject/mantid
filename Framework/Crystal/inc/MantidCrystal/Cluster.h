#ifndef MANTID_CRYSTAL_CLUSTER_H_
#define MANTID_CRYSTAL_CLUSTER_H_

#include "MantidCrystal/DisjointElement.h"
#include "MantidCrystal/ICluster.h"
#include "MantidKernel/System.h"
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>
#include <vector>

namespace Mantid {
namespace Crystal {

/** Cluster : Image cluster used by connected component labeling

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
class DLLExport Cluster : public ICluster {

public:
  /// Constructor
  Cluster(const size_t &label);

  /// integrate the cluster
  ClusterIntegratedValues
  integrate(boost::shared_ptr<const Mantid::API::IMDHistoWorkspace> ws)
      const override;

  /// Apply labels to the workspace
  void
  writeTo(boost::shared_ptr<Mantid::API::IMDHistoWorkspace> ws) const override;

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

#endif /* MANTID_CRYSTAL_CLUSTER_H_ */
