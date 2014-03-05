#include "MantidCrystal/DisjointElement.h"
#include <stdexcept>
#include <iostream>
namespace Mantid
{
  namespace Crystal
  {

    /**
     * Constructor
     * @param id : Element id
     */
    DisjointElement::DisjointElement(const int id) :
        m_parent(this), m_rank(0), m_id(id)
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
     */
    DisjointElement::~DisjointElement()
    {
    }

    /**
     * Getter for the id
     * @return return the numeric id.
     */
    int DisjointElement::getId() const
    {
      return m_id;
    }

    /**
     * Getter for the parent element.
     * @return pointer to parent element
     */
    DisjointElement * DisjointElement::getParent() const
    {
      return m_parent;
    }

    /**
     * Compress the tree such that element's parent becomes it's root parent.
     *
     * Note that compression does NOT alter ranks.
     * @return root id of new parent.
     */
    int DisjointElement::compress()
    {
      DisjointElement * temp = m_parent;
      while (temp->hasParent())
      {
        temp = temp->getParent();
      }
      m_parent = temp;
      return m_parent->getRoot();
    }

    /**
     * Determine if this instance really has a parent or whether it is a singleton
     * @return False if singleton, otherwise true.
     */
    bool DisjointElement::hasParent() const
    {
      return m_parent != this;
    }

    /**
     * Getter for the root id.
     * @return Root element id.
     */
    int DisjointElement::getRoot() const
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

    /**
     * Setter for the parent element.
     * @param other : New parent
     */
    void DisjointElement::setParent(DisjointElement * other)
    {
      m_parent = other;
    }

    /**
     * Increment the rank.
     * @return the new rank.
     */
    int DisjointElement::incrementRank()
    {
      return ++m_rank;
    }

    /**
     * Getter for the rank
     * @return the current rank
     */
    int DisjointElement::getRank() const
    {
      return m_rank;
    }

    /**
     * Union disjoint sets. Union happens w.r.t the parent of on or the other sets.
     * Early exit if both have the same root node already.
     * Generally, the set with the highest rank becomes the parent.
     * If ranks are equal, then one is chosen to be the parent, and that's sets rank is incremented.
     * @param other
     */
    void DisjointElement::unionWith(DisjointElement* other)
    {

      if (this->getId() == other->getId())
      {
        throw std::logic_error("Trying to union two elements with the same Id");
      }

      if (other->getRoot() != this->getRoot()) // Check sets do not already have the same root before continuing
      {
        this->compress();
        other->compress();

        DisjointElement* x = this->getParent();
        DisjointElement* y = other->getParent();

        if (x->getRank() > y->getRank())
        {
          y->setParent(x);
        }
        else
        {
          x->setParent(y);
          if (x->getRank() == y->getRank())
          {
            y->incrementRank();
          }
        }
      }

    }

  } // namespace Crystal
} // namespace Mantid
