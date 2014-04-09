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
      m_label(label),
      m_originalLabel(label)
    {
      m_indexes.reserve(1000);  
    }

    size_t Cluster::getLabel() const
    {
      return m_label;
    }

    size_t Cluster::getOriginalLabel() const
    {
      return m_originalLabel;
    }

   
    size_t Cluster::size() const
    {
      size_t size = m_indexes.size(); 
      for(size_t i = 0; i < m_ownedClusters.size(); ++i)
      {
        size+=m_ownedClusters[i]->size();
      }
      return size;
    }

    void Cluster::addIndex(const size_t& index)
    {
      m_indexes.push_back(index);
    }

    void Cluster::writeTo(Mantid::API::IMDHistoWorkspace_sptr ws) const
    {
      for(size_t i = 0; i< m_indexes.size(); ++i)
      {
        ws->setSignalAt(m_indexes[i], static_cast<Mantid::signal_t>(m_label));
        ws->setErrorSquaredAt(m_indexes[i], 0);
      }
      for(size_t i = 0; i < m_ownedClusters.size(); ++i)
      {
        m_ownedClusters[i]->writeTo(ws);
      }
    }

    Cluster::ClusterIntegratedValues Cluster::integrate(Mantid::API::IMDHistoWorkspace_const_sptr ws) const
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
      // Integrate owned clusters and add those results too.
      for(size_t i = 0; i < m_ownedClusters.size(); ++i)
      {
        auto integratedValues = m_ownedClusters[i]->integrate(ws);
        sigInt += integratedValues.get<0>();
        errorIntSQ += integratedValues.get<1>();
      }
      return ClusterIntegratedValues(sigInt, errorIntSQ);
    }

    void Cluster::toUniformMinimum(VecElements& disjointSet) 
    {
      if(m_indexes.size() > 0)
      {
        size_t minLabelIndex = m_indexes.front();
        size_t minLabel= disjointSet[m_indexes.front()].getRoot();
        for(size_t i = 1; i< m_indexes.size(); ++i)
        {
          const size_t& currentLabel = disjointSet[m_indexes[i]].getRoot();
          if(currentLabel < minLabel)
          {
            minLabel = currentLabel;
            minLabelIndex = i;
          }
        }

        m_label = minLabel;
        for(size_t i = 1; i< m_indexes.size(); ++i)
        {
          disjointSet[m_indexes[i]].unionWith(disjointSet[minLabelIndex].getParent());
        }
      }
    }

    bool Cluster::operator==(const Cluster& other) const
    {
      return getLabel() == other.getLabel();
    }

    void Cluster::attachCluster(boost::shared_ptr<const Cluster>& toOwn)
    {
      m_ownedClusters.push_back(toOwn);
    }


  } // namespace Crystal
} // namespace Mantid