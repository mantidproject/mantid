#include "MantidCrystal/ClusterRegister.h"
#include "MantidCrystal/Cluster.h"
#include "MantidCrystal/CompositeCluster.h"
#include <boost/functional/hash.hpp>
#include <boost/make_shared.hpp>
#include <list>
#include <unordered_set>

namespace {
template <typename T> std::pair<T, T> ordered_pair(const T &a, const T &b) {
  T min = std::min(a, b);
  T max = std::max(a, b);
  return std::pair<T, T>(min, max);
}
} // namespace

namespace Mantid {
namespace Crystal {

class ImplClusterRegister {
public:
  /// All registered input clusters
  ClusterRegister::MapCluster m_register;

  /// Clusters that do not need merging
  ClusterRegister::MapCluster m_unique;

  /// Type for identifying label groups
  using GroupType = std::list<std::unordered_set<size_t>>;

  /// Groups of labels to maintain
  GroupType m_groups;

  /// Type for identifying labels already seen
  using LabelHash = std::unordered_set<size_t>;

  /// Hash of labels merged
  LabelHash m_labelHash;

  /// Label hasher
  boost::hash<std::pair<int, int>> m_labelHasher;

  /**
   * Inserts a pair of disjoint elements. Determines whether they can be used to
   * reorder existing sets of lables.
   * @param a : One part of pair
   * @param b : Other part of pair
   * @return : true if a new cluster was required for the insertion.
   */
  bool insert(const DisjointElement &a, const DisjointElement &b) {
    const size_t &aLabel = a.getRoot();
    const size_t &bLabel = b.getRoot();
    bool newItem = true;

    GroupType containingAny;
    GroupType containingNone;
    // ------------- Find equivalent sets
    for (auto &cluster : m_groups) {
      if (cluster.find(aLabel) != cluster.end()) {
        containingAny.push_back(cluster);
      } else if (cluster.find(bLabel) != cluster.end()) {
        containingAny.push_back(cluster);
      } else {
        containingNone.push_back(cluster); // Current iterated set contains
                                           // NEITHER of these labels. It can
                                           // therfore be ignored.
      }
    }
    // ------------ Process equivalent sets
    if (containingAny.empty()) {
      // Neither label is yet known to any set. We must add a new set for these
      GroupType::value_type newSet;
      newSet.insert(aLabel);
      newSet.insert(bLabel);
      m_groups.push_back(newSet);
    } else {
      // At least one set already contains at least one label. We merge all such
      // sets into a master set.

      // implement copy and swap. Rebuild the sets.
      GroupType temp = containingNone;
      GroupType::value_type masterSet;
      masterSet.insert(aLabel); // Incase it doesn't already contain a
      masterSet.insert(bLabel); // Incase it doesn't already contain b
      for (auto &childSet : containingAny) {
        masterSet.insert(childSet.begin(),
                         childSet.end()); // Build the master set.
      }
      temp.push_back(masterSet);
      m_groups = temp; // Swap.
      newItem = false;
    }
    return newItem;
  }

  /**
   * Make composite clusters from the merged groups.
   * @return Merged composite clusters.
   */
  std::list<boost::shared_ptr<CompositeCluster>> makeCompositeClusters() {
    std::list<boost::shared_ptr<CompositeCluster>> composites;
    for (auto &labelSet : m_groups) {
      auto composite = boost::make_shared<CompositeCluster>();
      for (auto j : labelSet) {
        boost::shared_ptr<ICluster> &cluster = m_register[j];
        composite->add(cluster);
      }
      composites.push_back(composite);
    }
    return composites;
  }
};

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ClusterRegister::ClusterRegister() : m_Impl(new ImplClusterRegister) {}

/** Destructor
 */
ClusterRegister::~ClusterRegister() = default;

/**
 * Add/register a cluster.
 * @param label : Label as key
 * @param cluster : Cluster with label
 */
void ClusterRegister::add(const size_t &label,
                          const boost::shared_ptr<ICluster> &cluster) {
  m_Impl->m_register.emplace(label, cluster);
  m_Impl->m_unique.emplace(label, cluster);
}

/**
 * Use known pairs of disjoint elements to perform cluster merges.
 * @param a : Disjoint element which is part of pair
 * @param b : Disjoint element with is part of pair
 */
void ClusterRegister::merge(const DisjointElement &a,
                            const DisjointElement &b) const {
  if (!a.isEmpty() && !b.isEmpty()) {
    const int &aId = a.getId();
    const int &bId = b.getId();

    size_t hash = m_Impl->m_labelHasher(ordered_pair(aId, bId));
    if (m_Impl->m_labelHash.find(hash) ==
        m_Impl->m_labelHash.end()) // Only if this pair combination has not
                                   // already been processed
    {
      m_Impl->insert(a, b);
      m_Impl->m_unique.erase(aId);
      m_Impl->m_unique.erase(bId);
      m_Impl->m_labelHash.insert(
          hash); // So that we don't process this pair again.
    }
  }
}

/**
 * Get the clusters
 * @return both merged and unique clusters in a single map.
 */
ClusterRegister::MapCluster ClusterRegister::clusters() const {
  MapCluster temp;
  temp.insert(m_Impl->m_unique.begin(), m_Impl->m_unique.end());
  auto mergedClusters = m_Impl->makeCompositeClusters();
  for (const auto &merged : mergedClusters) {
    temp.emplace(merged->getLabel(), merged);
  }
  return temp;
}

/**
 * Get the clusters. Also set the elements to the uniform minimum of each
 * cluster.
 * @param elements
 * @return: Map of merged clusters.
 */
ClusterRegister::MapCluster
ClusterRegister::clusters(std::vector<DisjointElement> &elements) const {
  MapCluster temp;
  temp.insert(m_Impl->m_unique.begin(), m_Impl->m_unique.end());
  auto mergedClusters = m_Impl->makeCompositeClusters();
  for (auto &merged : mergedClusters) {
    merged->toUniformMinimum(elements);
    temp.emplace(merged->getLabel(), merged);
  }
  return temp;
}

} // namespace Crystal
} // namespace Mantid
