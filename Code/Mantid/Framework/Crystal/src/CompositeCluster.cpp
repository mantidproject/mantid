#include "MantidCrystal/CompositeCluster.h"
#include <stdexcept>
namespace Mantid
{
  namespace Crystal
  {

    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    CompositeCluster::CompositeCluster()
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
     */
    CompositeCluster::~CompositeCluster()
    {
    }

    ICluster::ClusterIntegratedValues CompositeCluster::integrate(
        boost::shared_ptr<const Mantid::API::IMDHistoWorkspace> ws) const
    {

      double errorIntSQ = 0;
      double sigInt = 0;
      // Integrate owned clusters and add those results too.
      for (size_t i = 0; i < m_ownedClusters.size(); ++i)
      {
        auto integratedValues = m_ownedClusters[i]->integrate(ws);
        sigInt += integratedValues.get<0>();
        errorIntSQ += integratedValues.get<1>();
      }
      return ClusterIntegratedValues(sigInt, errorIntSQ);
    }

    void CompositeCluster::writeTo(boost::shared_ptr<Mantid::API::IMDHistoWorkspace> ws) const
    {
      for (size_t i = 0; i < m_ownedClusters.size(); ++i)
      {
        m_ownedClusters[i]->writeTo(ws);
      }
    }

    size_t CompositeCluster::getLabel() const
    {
      if (!m_label.is_initialized())
      {
        throw std::runtime_error("No child IClusters. CompositeCluster::getLabel() is not supported.");
      }
      else
      {
        return m_label.get(); // Assumes all are uniform.
      }
    }

    size_t CompositeCluster::size() const
    {
      size_t size = 0;
      for (size_t i = 0; i < m_ownedClusters.size(); ++i)
      {
        size += m_ownedClusters[i]->size();
      }
      return size;
    }

    void CompositeCluster::addIndex(const size_t& index)
    {
      throw std::runtime_error("addIndex not implemented on CompositeCluster");
    }

    void CompositeCluster::toUniformMinimum(std::vector<DisjointElement>& disjointSet)
    {
      if (!m_ownedClusters.empty())
      {
        ICluster* minCluster = m_ownedClusters.front().get();
        size_t minLabel = minCluster->getLabel();
        for (size_t i = 1; i < m_ownedClusters.size(); ++i)
        {
          size_t temp = m_ownedClusters[i]->getLabel();
          if (temp < minLabel)
          {
            minLabel = temp;
            minCluster = m_ownedClusters[i].get();
          }
        }
        m_label = minLabel;

        for (size_t i = 0; i < m_ownedClusters.size(); ++i)
        {
          minCluster->setAsParentOn(m_ownedClusters[i].get());// TODO
        }
      }
    }

    void CompositeCluster::add(boost::shared_ptr<ICluster>& toOwn)
    {
      m_ownedClusters.push_back(toOwn);
    }

  } // namespace Crystal
} // namespace Mantid
