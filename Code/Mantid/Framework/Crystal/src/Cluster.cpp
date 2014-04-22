#include "MantidCrystal/Cluster.h"
#include "MantidCrystal/DisjointElement.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include <stdexcept>

namespace
{
  typedef std::vector<Mantid::Crystal::DisjointElement> VecElements;
}

namespace Mantid
{
  namespace Crystal
  {

    Cluster::Cluster(const size_t& label):
      m_originalLabel(label),
      m_rootCluster(this)
    {
      m_indexes.reserve(1000);  
    }

    Cluster::~Cluster()
    {
    }

    size_t Cluster::getLabel() const
    {
      if(m_rootCluster != this)
      {
        return m_rootCluster->getLabel();
      }

      return m_originalLabel;
    }

    size_t Cluster::getOriginalLabel() const
    {
      return m_originalLabel;
    }

    size_t Cluster::size() const
    {
      return m_indexes.size(); 
    }

    void Cluster::addIndex(const size_t& index)
    {
      m_indexes.push_back(index);
    }

    void Cluster::writeTo(Mantid::API::IMDHistoWorkspace_sptr ws) const
    {
      const size_t label = this->getLabel();
      for(size_t i = 0; i< m_indexes.size(); ++i)
      {
        ws->setSignalAt(m_indexes[i], static_cast<Mantid::signal_t>(label));
        ws->setErrorSquaredAt(m_indexes[i], 0);
      }
    }

    ICluster::ClusterIntegratedValues Cluster::integrate(Mantid::API::IMDHistoWorkspace_const_sptr ws) const
    {
      double errorIntSQ = 0;
      double sigInt = 0;
      // Integrate accross indexes owned by this workspace.
      for(size_t i = 0; i< m_indexes.size(); ++i)
      {
        sigInt += ws->getSignalAt(m_indexes[i]);
        double errorSQ = ws->getErrorAt(m_indexes[i]);
        errorSQ *= errorSQ;
        errorIntSQ += errorSQ;
      }
      return ClusterIntegratedValues(sigInt, errorIntSQ);
    }

    void Cluster::toUniformMinimum(VecElements& disjointSet) 
    {
      if(m_indexes.size() > 0)
      {
        size_t parentIndex = m_rootCluster->getRepresentitiveIndex();

        for(size_t i = 1; i< m_indexes.size(); ++i)
        {
          disjointSet[m_indexes[i]].unionWith(disjointSet[parentIndex].getParent());
        }
      }
    }

    size_t Cluster::getRepresentitiveIndex() const
    {
      return m_indexes.front();
    }

    void Cluster::setRootCluster(ICluster const * root)
    {
      m_rootCluster = root;
    }

    bool Cluster::operator==(const Cluster& other) const
    {
      return getLabel() == other.getLabel();
    }

    bool Cluster::containsLabel(const size_t& label) const
    {
      return (label == this->getLabel());
    }


  } // namespace Crystal
} // namespace Mantid
