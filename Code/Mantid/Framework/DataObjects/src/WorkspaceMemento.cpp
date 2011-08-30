#include "MantidDataObjects/WorkspaceMemento.h"

namespace Mantid
{
  namespace DataObjects
  {

    /**
    Constructor
    @param ws : ref to underlying table workspace where persisted data is stored.
    @param runNumber : the run number corresponding to this memento.
    */
    WorkspaceMemento::WorkspaceMemento(TableWorkspace& ws, size_t runNumber) :
      m_data(ws), 
      m_validMemento(false), 
      m_runNumber(runNumber), 
      m_lock(new SingleOwnerLock(runNumber))
    {
    }

    /**
    Constructor with option to specify locking object.
    @param ws : ref to underlying table workspace where persisted data is stored.
    @param runNumber : the run number corresponding to this memento.
    @param lock : locking object to use internally.
    */
     WorkspaceMemento::WorkspaceMemento(TableWorkspace& ws, size_t runNumber, WorkspaceMementoLock* lock) : 
      m_data(ws), 
      m_validMemento(false), 
      m_runNumber(runNumber), 
      m_lock(lock)
    {
    }

    /// Destructor
    WorkspaceMemento::~WorkspaceMemento()
    {
    }

    /* Add a metadata item. This corresponds to a cell in the underlying table workspace.
    @param item : Abstract memento item shared pointer.
    */
    void WorkspaceMemento::addItem(AbstractMementoItem* item)
    {
      //TODO check types here. Type of item should be the same as the corresponding colum in the table.
      AbstractMementoItem_sptr temp(item);
      m_items.push_back(temp);
      m_items.size() == size_t(m_data.columnCount()) ? m_validMemento = true : m_validMemento = false;
    }

    /* Getter for the item at a column
       @return Abstract memento item shared pointer.
    */
    AbstractMementoItem_sptr WorkspaceMemento::getItem(const size_t col) const
    {
      return m_items[col];
    }

    /**
    Getter for the locked status.
    @return locked status.
    */
    bool WorkspaceMemento::locked() const
    {
      return m_lock->locked();
    }

    /// Request the memento and underlying data are locked.
    void WorkspaceMemento::lock()
    {
     return m_lock->lock();
    }

    /**
    Unlock the memento. 
    @return true if sucessfully unlocked, false if already unlocked.
    */
    bool WorkspaceMemento::unlock()
    {
      return m_lock->unlock();
    }

    /// Validate the workspace. Ensure that it is well-setup before using as a memento.
    void WorkspaceMemento::validate() const
    {
      if(!m_validMemento)
      {
        throw std::runtime_error("Cannot use this type without first having set it up using ::addItem");
      }
    }

    /**
    Getter indicating if there are changes not yet persisted to the underlying table workspace.
    @return true if outstanding changes exist.
    */
    bool WorkspaceMemento::hasChanged() const
    {
      validate();
      //Run through each memento item for its changed status.
      VecMementoItems::const_iterator it = m_items.begin();
      while(it != m_items.end())
      {
        if((*it)->hasChanged()){ return true; }
        it++;
      }
      return false;
    }

    /**
    Equals comparitor for this instance with another memento.
    @param other : another workspace memento to compare to.
    @return true if equal.
    */
    bool WorkspaceMemento::equals(const WorkspaceMemento& other) const
    {
      validate();
      bool bResult = false;
      if(this->m_items.size() != other.m_items.size())
      {
        return bResult;
      }
      for(size_t i = 0; i < m_items.size(); i++)
      {
        //Defer to each memento item.
        bResult = m_items[i]->equals(*other.m_items[i]);
        if(!bResult)
          break;
      }
      return bResult;
    }

    /**
    Equals operator overload.
    @param other : another workspace memento to compare to.
    @return true if equal.
    */
    bool WorkspaceMemento::operator==(const WorkspaceMemento& other) const
    {
      return this->equals(other);
    }

    /**
    Not equals operator overload.
    @param other : another workspace memento to compare to.
    @return false if equal.
    */
    bool WorkspaceMemento::operator!=(const WorkspaceMemento& other) const
    {
      return !this->equals(other);
    }

    /// Commit changes 
    void WorkspaceMemento::commit()
    {
      validate();
      VecMementoItems::const_iterator it = m_items.begin();
      while(it != m_items.end())
      {
        (*it)->commit();
        it++;
      }
    }

    /// Roll back changes
    void WorkspaceMemento::rollback()
    {
      validate();
      VecMementoItems::const_iterator it = m_items.begin();
      while(it != m_items.end())
      {
        (*it)->rollback();
        it++;
      }
    }

    // ------------------ TODO split following off into separate cpp ----------------

    SingleOwnerLock::LockMap SingleOwnerLock::locks;

    SingleOwnerLock::SingleOwnerLock(size_t runNumber) : m_runNumber(runNumber) {}

      /// Apply the lock.
      void SingleOwnerLock::lock()
      {
        //You could get a race condition in the following, but this code is not intended for parallel usage.
        if(locked())
        {
          throw std::runtime_error("This memento is already in use!");
        }
        SingleOwnerLock::locks[m_runNumber] = true;
      }

      /**
      Remove the lock.
      @return true if locked when unlocked. Returns false if already unlocked.
      */
      bool SingleOwnerLock::unlock()
      {
        bool existingState = SingleOwnerLock::locks[m_runNumber];
        SingleOwnerLock::locks[m_runNumber] = false;
        return existingState;
      }

      /**
      Getter for the locked status.
      @return true if locked.
      */
      bool SingleOwnerLock::locked() const
      {
        LockMap::iterator it = SingleOwnerLock::locks.find(m_runNumber);
        if(SingleOwnerLock::locks.end() != it)
        {
          return it->second;
        }
        return false;
      }

      /// Destructor
      SingleOwnerLock::~SingleOwnerLock()
      {
        //Remove the lock if not done so already.
        if(locked())
          unlock();
      }
  }
}