#ifndef MANTID_CRYSTAL_CONNECTEDCOMPONENTLABELING_H_
#define MANTID_CRYSTAL_CONNECTEDCOMPONENTLABELING_H_

#include "MantidAPI/IMDHistoWorkspace_fwd.h"
#include "MantidCrystal/DisjointElement.h"
#include "MantidKernel/System.h"
#include "MantidKernel/V3D.h"
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>
#include <map>
#include <unordered_set>

namespace Mantid {
namespace API {
class Progress;
}

namespace Crystal {
class ICluster;
/**
 * Namespace containing useful typedefs
 */
namespace ConnectedComponentMappingTypes {
using SignalErrorSQPair = boost::tuple<double, double>;
using LabelIdIntensityMap = std::map<size_t, SignalErrorSQPair>;
using PositionToLabelIdMap = std::map<Mantid::Kernel::V3D, size_t>;
using VecIndexes = std::vector<size_t>;
using VecElements = std::vector<DisjointElement>;
using SetIds = std::unordered_set<size_t>;
using ClusterMap =
    std::map<size_t, boost::shared_ptr<Mantid::Crystal::ICluster>>;
using ClusterTuple =
    boost::tuple<Mantid::API::IMDHistoWorkspace_sptr, ClusterMap>;
} // namespace ConnectedComponentMappingTypes

class BackgroundStrategy;

/** ConnectedComponentLabelling : Implements connected component labeling on
 MDHistoWorkspaces.

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
class DLLExport ConnectedComponentLabeling {

public:
  /// Constructor
  ConnectedComponentLabeling(const size_t &startId = 1,
                             const boost::optional<int> nThreads = boost::none);

  /// Getter for the start label id
  size_t getStartLabelId() const;

  /// Setter for the label id
  void startLabelingId(const size_t &id);

  /// Execute and return clusters
  boost::shared_ptr<Mantid::API::IMDHistoWorkspace>
  execute(Mantid::API::IMDHistoWorkspace_sptr ws,
          BackgroundStrategy *const strategy,
          Mantid::API::Progress &progress) const;

  /// Execute and return clusters, as well as maps to integratable clusters.
  ConnectedComponentMappingTypes::ClusterTuple
  executeAndFetchClusters(Mantid::API::IMDHistoWorkspace_sptr ws,
                          BackgroundStrategy *const strategy,
                          Mantid::API::Progress &progress) const;

  /// Destructor
  virtual ~ConnectedComponentLabeling();

private:
  /// Get the number of threads to use.
  int getNThreads() const;

  /// Calculate the disjoint element tree across the image.
  ConnectedComponentMappingTypes::ClusterMap
  calculateDisjointTree(Mantid::API::IMDHistoWorkspace_sptr ws,
                        BackgroundStrategy *const baseStrategy,
                        Mantid::API::Progress &progress) const;

  /// Start labeling index
  size_t m_startId;

  /// Run multithreaded
  const boost::optional<int> m_nThreads;
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_CONNECTEDCOMPONENTLABELING_H_ */
