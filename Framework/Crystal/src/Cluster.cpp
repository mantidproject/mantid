// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/Cluster.h"
#include "MantidAPI/IMDHistoWorkspace.h"

namespace {
using VecElements = std::vector<Mantid::Crystal::DisjointElement>;
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
  for (auto index : m_indexes) {
    ws->setSignalAt(index, static_cast<Mantid::signal_t>(label));
    ws->setErrorSquaredAt(index, 0);
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
  for (auto index : m_indexes) {
    sigInt += ws->getSignalAt(index);
    double errorSQ = ws->getErrorAt(index);
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
  if (!m_indexes.empty()) {
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
