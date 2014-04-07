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
      m_signalInt(boost::none),
      m_errorSQInt(boost::none)
    {
      m_indexes.reserve(1000); 
    }

    size_t Cluster::getLabel() const
    {
      return m_label;
    }

    double Cluster::getSignalInt() const
    {
      if(!m_signalInt.is_initialized())
      {
        throw std::runtime_error("Cluster has not been integrated.");
      }
      return m_signalInt.get();
    }

    double Cluster::getErrorSQInt() const
    {
      if(!m_errorSQInt.is_initialized())
      {
        throw std::runtime_error("Cluster has not been integrated.");
      }
      return m_errorSQInt.get();
    }

    size_t Cluster::size()
    {
      return m_indexes.size();
    }

    void Cluster::addIndex(const size_t& index)
    {
      m_indexes.push_back(index);
    }

    void Cluster::writeTo(Mantid::API::IMDHistoWorkspace_sptr ws) const
    {
      for(size_t i = 0; i< m_indexes.size(); ++i)
      {
        ws->setSignalAt(m_indexes[i], Mantid::signal_t(m_label));
        ws->setErrorSquaredAt(m_indexes[i], 0);
      }
    }

    void Cluster::integrate(Mantid::API::IMDHistoWorkspace_const_sptr ws)
    {
      double errorIntSQ = 0;
      double sigInt = 0;
      for(size_t i = 0; i< m_indexes.size(); ++i)
      {
        sigInt += ws->getSignalAt(m_indexes[i]);
        double errorSQ;
        errorSQ = ws->getErrorAt(i);
        errorSQ *= errorSQ;
        errorIntSQ += errorSQ;
      }
      m_errorSQInt = errorIntSQ;
      m_signalInt = sigInt;
    }

    void Cluster::toUniformMinimum(VecElements& disjointSet) 
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

    bool Cluster::operator==(const Cluster& other) const
    {
      return getLabel() == other.getLabel();
    }

    void Cluster::consumeCluster(Cluster& other)
    {
      if(other.getLabel() != this->getLabel())
      {
        throw std::runtime_error("Label Ids differ. Cannot combine Clusters.");
      }
      m_indexes.insert(m_indexes.end(), other.m_indexes.begin(), other.m_indexes.end());
    }

  } // namespace Crystal
} // namespace Mantid