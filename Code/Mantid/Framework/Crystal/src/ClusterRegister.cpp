#include "MantidCrystal/ClusterRegister.h"
#include "MantidCrystal/Cluster.h"
#include "MantidCrystal/CompositeCluster.h"
#include "MantidCrystal/DisjointElement.h"
#include <boost/make_shared.hpp>
#include <algorithm>
#include <set>
#include <vector>

namespace
{
  using namespace Mantid::Crystal;
  struct DisjointPair
  {
    const DisjointElement m_currentLabel;
    const DisjointElement m_neighbourLabel;
    DisjointPair(const DisjointElement& currentLabel, const DisjointElement& neighbourLabel) :
        m_currentLabel(currentLabel), m_neighbourLabel(neighbourLabel)
    {
    }

    size_t unique() const
    {
      const size_t min = std::min(m_currentLabel.getId(), m_neighbourLabel.getId());
      const size_t max = std::max(m_currentLabel.getId(), m_neighbourLabel.getId());
      return min + (max >> 32);
    }

    inline bool operator<(const DisjointPair & other) const
    {
      return (this->unique() < other.unique());
    }
  };

  typedef std::set<DisjointPair> SetDisjointPair;
}

namespace Mantid
{
  namespace Crystal
  {
    
    struct ImplClusterRegister
    {
      ClusterRegister::MapCluster m_register;

      std::vector<boost::shared_ptr<CompositeCluster> > m_merged;

      SetDisjointPair m_setPairs;
    };


    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    ClusterRegister::ClusterRegister() : m_Impl(new ImplClusterRegister)
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
     */
    ClusterRegister::~ClusterRegister()
    {
    }

    void ClusterRegister::add(const size_t& label, const boost::shared_ptr<ICluster>& cluster)
    {
      m_Impl->m_register.insert(std::make_pair(label, cluster));
    }

    void ClusterRegister::merge(const DisjointElement& a, const DisjointElement& b) const
    {
      if (!a.isEmpty() && !b.isEmpty())
      {
        DisjointPair pair(a, b);
        if (m_Impl->m_setPairs.find(pair) == m_Impl->m_setPairs.end())
        {
          const size_t& aId = a.getId();
          const size_t& bId = b.getId();
          bool newClusterRequired = true;
          for(size_t i = 0; i < m_Impl->m_merged.size(); ++i)
          {
            auto & composite = m_Impl->m_merged[i];
            if(composite->labelInSet(aId) && !composite->labelInSet(bId))
            {
              composite->add(m_Impl->m_register[bId]);
              m_Impl->m_register.erase(bId);
              newClusterRequired = false;
              break;
            }
            else if(!composite->labelInSet(aId) && composite->labelInSet(bId))
            {
              composite->add(m_Impl->m_register[aId]);
              m_Impl->m_register.erase(aId);

              newClusterRequired = false;
              break;
            }
          }
          if(newClusterRequired)
          {
            auto composite = boost::make_shared<CompositeCluster>();
            composite->add(m_Impl->m_register[aId]);
            composite->add(m_Impl->m_register[bId]);
            m_Impl->m_register.erase(aId);
            m_Impl->m_register.erase(bId);
            m_Impl->m_merged.push_back(composite);
          }

          m_Impl->m_setPairs.insert(pair);
        }
      }
    }

    void ClusterRegister::toUniformMinimum(std::vector<DisjointElement>& elements)
    {
      for(size_t i = 0; i < m_Impl->m_merged.size(); ++i)
      {
        const auto&  merged = m_Impl->m_merged[i];
        merged->toUniformMinimum(elements);
      }
    }

    ClusterRegister::MapCluster ClusterRegister::clusters() const
    {
      MapCluster temp;
      temp.insert(m_Impl->m_register.begin(), m_Impl->m_register.end());
      for(size_t i = 0; i < m_Impl->m_merged.size(); ++i)
      {
        const auto&  merged = m_Impl->m_merged[i];
        temp.insert(std::make_pair( merged->getLabel(), merged));
      }
      return temp;
    }

  } // namespace Crystal
} // namespace Mantid
