#include "MantidCrystal/Cluster.h"
#include "MantidAPI/IMDHistoWorkspace.h"

namespace {
typedef std::vector<Mantid::Crystal::DisjointElement> VecElements;
}

namespace Mantid {
namespace Crystal {

/**
 * Constructor
 * @param label : Label (taken as original) for Cluster
 */
Cluster::Cluster(const size_t &label)
    : m_originalLabel(label), m_rootCluster(this) {
  m_indexes.reserve(1000);
}

/**
 * Destructor
 */
Cluster::~Cluster() {}

/**
 * Get the label
 * @return : Return label.
 */
size_t Cluster::getLabel() const {
  if (m_rootCluster != this) {
    return m_rootCluster->getLabel();
  }

  return m_originalLabel;
}

/**
 * Get the original label
 * @return : Original label
 */
size_t Cluster::getOriginalLabel() const { return m_originalLabel; }

/**
 * Get the number of indexes which are part of this cluster. i.e. the size of
 * the cluster
 * @return : Cluster size
 */
size_t Cluster::size() const { return m_indexes.size(); }

/**
 * Add the index to the cluster
 * @param index : index to add
 */
void Cluster::addIndex(const size_t &index) { m_indexes.push_back(index); }

/**
 * Apply the cluster to the image
 * @param ws : Image to stencil itself onto
 */
void Cluster::writeTo(Mantid::API::IMDHistoWorkspace_sptr ws) const {
  const size_t label = this->getLabel();
  for (size_t i = 0; i < m_indexes.size(); ++i) {
    ws->setSignalAt(m_indexes[i], static_cast<Mantid::signal_t>(label));
    ws->setErrorSquaredAt(m_indexes[i], 0);
  }
}

/**
 * Integrate the image over cluster region
 * @param ws : Image
 * @return : Integrated values
 */
ICluster::ClusterIntegratedValues
Cluster::integrate(Mantid::API::IMDHistoWorkspace_const_sptr ws) const {
  double errorIntSQ = 0;
  double sigInt = 0;
  // Integrate accross indexes owned by this workspace.
  for (size_t i = 0; i < m_indexes.size(); ++i) {
    sigInt += ws->getSignalAt(m_indexes[i]);
    double errorSQ = ws->getErrorAt(m_indexes[i]);
    errorSQ *= errorSQ;
    errorIntSQ += errorSQ;
  }
  return ClusterIntegratedValues(sigInt, errorIntSQ);
}

/**
 * Allow the cluster to adopt the uniform minimum value.
 * @param disjointSet
 */
void Cluster::toUniformMinimum(VecElements &disjointSet) {
  if (m_indexes.size() > 0) {
    size_t parentIndex = m_rootCluster->getRepresentitiveIndex();

    for (size_t i = 1; i < m_indexes.size(); ++i) {
      disjointSet[m_indexes[i]].unionWith(disjointSet[parentIndex].getParent());
    }
  }
}

/**
 * Get a representative index which is part of this cluster
 * @return
 */
size_t Cluster::getRepresentitiveIndex() const { return m_indexes.front(); }

/**
 * Set the root cluster
 * @param root : Root cluster
 */
void Cluster::setRootCluster(ICluster const *root) { m_rootCluster = root; }

/**
 * Overlaoded equals operator
 * @param other : To compare with
 * @return True only if the objects are equal
 */
bool Cluster::operator==(const Cluster &other) const {
  return getLabel() == other.getLabel();
}

/**
 * Does the cluster contain the label.
 * @param label : Label to find
 * @return : True only if the label exists.
 */
bool Cluster::containsLabel(const size_t &label) const {
  return (label == this->getLabel());
}

} // namespace Crystal
} // namespace Mantid
