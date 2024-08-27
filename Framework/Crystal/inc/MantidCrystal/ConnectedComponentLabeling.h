// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IMDHistoWorkspace_fwd.h"
#include "MantidCrystal/DisjointElement.h"
#include "MantidCrystal/DllConfig.h"
#include "MantidKernel/System.h"
#include "MantidKernel/V3D.h"
#include <boost/tuple/tuple.hpp>
#include <map>
#include <memory>
#include <optional>
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
using ClusterMap = std::map<size_t, std::shared_ptr<Mantid::Crystal::ICluster>>;
using ClusterTuple = boost::tuple<Mantid::API::IMDHistoWorkspace_sptr, ClusterMap>;
} // namespace ConnectedComponentMappingTypes

class BackgroundStrategy;

/** ConnectedComponentLabelling : Implements connected component labeling on
 MDHistoWorkspaces.
 */
class MANTID_CRYSTAL_DLL ConnectedComponentLabeling {

public:
  /// Constructor
  ConnectedComponentLabeling(const size_t &startId = 1, std::optional<int> nThreads = std::nullopt);

  /// Getter for the start label id
  size_t getStartLabelId() const;

  /// Setter for the label id
  void startLabelingId(const size_t &id);

  /// Execute and return clusters
  std::shared_ptr<Mantid::API::IMDHistoWorkspace> execute(Mantid::API::IMDHistoWorkspace_sptr ws,
                                                          BackgroundStrategy *const strategy,
                                                          Mantid::API::Progress &progress) const;

  /// Execute and return clusters, as well as maps to integratable clusters.
  ConnectedComponentMappingTypes::ClusterTuple executeAndFetchClusters(Mantid::API::IMDHistoWorkspace_sptr ws,
                                                                       BackgroundStrategy *const strategy,
                                                                       Mantid::API::Progress &progress) const;

  /// Destructor
  virtual ~ConnectedComponentLabeling();

private:
  /// Get the number of threads to use.
  int getNThreads() const;

  /// Calculate the disjoint element tree across the image.
  ConnectedComponentMappingTypes::ClusterMap calculateDisjointTree(const Mantid::API::IMDHistoWorkspace_sptr &ws,
                                                                   BackgroundStrategy *const baseStrategy,
                                                                   Mantid::API::Progress &progress) const;

  /// Start labeling index
  size_t m_startId;

  /// Run multithreaded
  const std::optional<int> m_nThreads;
};

} // namespace Crystal
} // namespace Mantid
