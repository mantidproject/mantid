#include "MantidCrystal/ClusterRegister.h"
#include "MantidCrystal/Cluster.h"
#include "MantidCrystal/CompositeCluster.h"
#include "MantidCrystal/DisjointElement.h"
#include <boost/make_shared.hpp>
#include <algorithm>
#include <set>
#include <list>

namespace Mantid
{
  namespace Crystal
  {

    class ImplClusterRegister
    {
    public:

      /// All registered input clusters
      ClusterRegister::MapCluster m_register;

      /// Clusters that do not need merging
      ClusterRegister::MapCluster m_unique;

      typedef std::list<std::set<size_t> > GroupType;
      GroupType m_groups;

      bool insert(const DisjointElement& a, const DisjointElement& b)
      {
        const size_t& aLabel = a.getRoot();
        const size_t& bLabel = b.getRoot();
        bool newItem = true;

        GroupType containingAny;
        GroupType containingNone;
        // Find equivalent sets
        for(GroupType::iterator i = m_groups.begin(); i != m_groups.end(); ++i)
        {
          GroupType::value_type& cluster =*i;
          if(cluster.find(aLabel) != cluster.end())
          {
            containingAny.push_back(cluster);
          }
          else if(cluster.find(bLabel) != cluster.end())
          {
            containingAny.push_back(cluster);
          }
          else
          {
            containingNone.push_back(cluster);
          }
        }
        // Process equivalent sets
        if(containingAny.empty())
        {
          GroupType::value_type newSet;
          newSet.insert(aLabel);
          newSet.insert(bLabel);
          m_groups.push_back(newSet);
        }
        else
        {
          // implement copy and swap. Rebuild the sets.
          GroupType temp = containingNone;
          GroupType::value_type masterSet;
          masterSet.insert(aLabel);
          masterSet.insert(bLabel);
          for(auto i = containingAny.begin(); i != containingAny.end(); ++i)
          {
            GroupType::value_type& childSet = *i;
            masterSet.insert(childSet.begin(), childSet.end());
          }
          temp.push_back(masterSet);
          m_groups = temp;
          newItem = false;
        }
        return newItem;
      }

      std::list<boost::shared_ptr<CompositeCluster> >  makeCompositeClusters()
      {
        std::list<boost::shared_ptr<CompositeCluster> >  composites;
        for(auto i = m_groups.begin(); i != m_groups.end(); ++i)
        {
          GroupType::value_type& labelSet = *i;
          auto composite = boost::make_shared<CompositeCluster>();
          for(auto j = labelSet.begin(); j != labelSet.end(); ++j)
          {
            boost::shared_ptr<ICluster>& cluster = m_register[(*j)];
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
      m_Impl->m_unique.insert(std::make_pair(label, cluster));
    }


    void ClusterRegister::merge(const DisjointElement& a, const DisjointElement& b) const
    {
      if (!a.isEmpty() && !b.isEmpty())
      {
        m_Impl->insert(a, b);
        m_Impl->m_unique.erase(a.getId());
        m_Impl->m_unique.erase(b.getId());
      }
    }

    ClusterRegister::MapCluster ClusterRegister::clusters() const
    {
      MapCluster temp;
      temp.insert(m_Impl->m_unique.begin(), m_Impl->m_unique.end());
      auto mergedClusters = m_Impl->makeCompositeClusters();
      for(auto i = mergedClusters.begin(); i != mergedClusters.end(); ++i)
      {
        const auto&  merged = *i;
        temp.insert(std::make_pair( merged->getLabel(), merged));
      }
      return temp;
    }

    ClusterRegister::MapCluster ClusterRegister::clusters(std::vector<DisjointElement>& elements) const
    {
      MapCluster temp;
      temp.insert(m_Impl->m_unique.begin(), m_Impl->m_unique.end());
      auto mergedClusters = m_Impl->makeCompositeClusters();
      for(auto i = mergedClusters.begin(); i != mergedClusters.end(); ++i)
      {
        const auto&  merged = *i;
        merged->toUniformMinimum(elements);
        temp.insert(std::make_pair( merged->getLabel(), merged));
      }
      return temp;
    }

  } // namespace Crystal
} // namespace Mantid
