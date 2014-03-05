#include "MantidCrystal/ClusterItem.h"

namespace Mantid
{
  namespace Crystal
  {

    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    ClusterItem::ClusterItem(const int id) :
        m_parent(this), m_depth(0), m_rank(0), m_id(id)
    {
    }

    ClusterItem::ClusterItem(const int id, ClusterItem * const parent) :
        m_parent(parent), m_depth(parent->getDepth() + 1), m_rank(0), m_id(id)
    {
      m_parent->incrementRank();
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
     */
    ClusterItem::~ClusterItem()
    {
      m_parent->decrementRank();
    }

    int ClusterItem::getId() const
    {
      return m_id;
    }

    int ClusterItem::getDepth() const
    {
      return m_depth;
    }

    ClusterItem * ClusterItem::getParent() const
    {
      return m_parent;
    }

    ClusterItem::ClusterItem(const ClusterItem& other) :
        m_parent(other.m_parent), m_depth(other.m_depth), m_rank(other.m_rank), m_id(other.m_id)
    {
      if(other.hasParent())
      {
        m_parent->incrementRank();
      }
      else // If the cluster item we are copying from is a root node, then this one is too.
      {
        m_parent = this;
      }
    }

    ClusterItem& ClusterItem::operator=(const ClusterItem& other)
    {
      if (this != &other)
      {
        setParent(other.m_parent);
        m_depth = other.m_depth;
        m_rank = other.m_rank;
        // Note. Cannot change id once set.
      }
      return *this;
    }

    int ClusterItem::compress()
    {
      ClusterItem * temp = m_parent;
      while (temp->hasParent())
      {
        temp->decrementRank(); // Decrease the rank (a.k.a unlink this parent), but don't delete it.
        temp = temp->getParent();
      }
      m_parent = temp;
      m_depth = m_parent->getDepth() + 1;
      return m_parent->getRoot();
    }

    bool ClusterItem::hasParent() const
    {
      return m_parent != this;
    }

    int ClusterItem::getRoot() const
    {
      if (m_parent == this)
      {
        return m_id;
      }
      else
      {
        return m_parent->getRoot();
      }
    }

    void ClusterItem::setParent(ClusterItem * other)
    {
      m_parent->decrementRank();
      m_parent = other;
      m_parent->incrementRank();
    }

    void ClusterItem::incrementRank()
    {
      ++m_rank;
    }

    void ClusterItem::decrementRank()
    {
      --m_rank;
    }

    int ClusterItem::getRank() const
    {
      return m_rank;
    }

    void ClusterItem::unionWith(ClusterItem& other)
    {
      const auto rootA = this->compress();
      const auto rootB = other.compress();
      if (rootA != rootB)
      {
        if (other.getDepth() < this->getDepth()) //is the rank of Root2 less than that of Root1 ?
        {
          other.setParent(this);
        }
        else //rank of Root2 is greater than or equal to that of Root1
        {
          this->setParent(&other);

           if (this->getDepth() == other.getDepth())
           {
             other.incrementRank();
           }
        }
      }
    }

  } // namespace Crystal
} // namespace Mantid
