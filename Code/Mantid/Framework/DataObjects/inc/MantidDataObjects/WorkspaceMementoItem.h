#ifndef MANTID_DATAOBJECTS_MEMENTO_ITEM_H_
#define MANTID_DATAOBJECTS_MEMENTO_ITEM_H_
#include "MantidKernel/System.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "boost/shared_ptr.hpp"

namespace Mantid
{
  namespace DataObjects
  {


    /**  @class Boolean
    As TableColumn stores its data in a std::vector bool type cannot be used 
    in the same way as the other types. Class Boolean is used instead.
*/
    class DLLExport AbstractMementoItem
    {
    public:
      virtual bool hasChanged() const = 0;
      virtual void commit() = 0;
      virtual void rollback() = 0;
      virtual bool equals(AbstractMementoItem& other) const = 0;
      virtual ~AbstractMementoItem() {}
    };


    typedef boost::shared_ptr<AbstractMementoItem> AbstractMementoItem_sptr;
    
    /** @class WorkspaceMementoItem. Unique type for column data, through which changes to cell data can be applied, stored and reverted.
    Type system ensures that no two columns are comparible, even if they store the same data.


    @author Owen Arnold
    @date 25/08/2011

    Copyright &copy; 2011-12 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
    template<int ColNumber, typename ColType>
    class DLLExport WorkspaceMementoItem : public AbstractMementoItem
    {
    private:
      /// Actual/outstanding value stored in cell.
      ColType m_value;
      /// Reference to mutable table workspace.
      TableWorkspace& m_data;
      /// Row onto which this column object projects.
      size_t m_rowIndex;

    public:
      /// Unique column index.
      enum { ColIndex = ColNumber };
      /// Type of item being stored.
      typedef ColType ItemType;
      
      /**
      Constructor
      @param data : ref to table workspace storing the data.
      @param rowIndex : index of the row in the table workspace that this column/memento item. is to apply.
      */
      WorkspaceMementoItem(TableWorkspace& data, size_t rowIndex) : m_data(data), m_rowIndex(rowIndex)
      {
        m_value = m_data.getColumn(ColIndex)->cell<WorkspaceMementoItem::ItemType>(m_rowIndex);
      }

      /**
      Copy constructor
      @param other : memento item to copy from.
      */
      WorkspaceMementoItem(const WorkspaceMementoItem& other) : m_value(other.m_value), m_data(other.m_data), m_rowIndex(other.m_rowIndex)
      {
      }
      
      /**
      Overrriden assignement operator.
      @param other : memento item to assign from.
      */
      WorkspaceMementoItem& operator=(const WorkspaceMementoItem& other)
      {
        if(&other != this)
        {
          m_value = other.m_value;
          m_rowIndex = other.m_rowIndex;
        }
        return *this;
      }

      /// Destructor
      virtual ~WorkspaceMementoItem()
      {
      }

      /*
      Getter for changed status.
      @return true if different.
      */
      virtual bool hasChanged() const 
      {
        return m_data.getColumn(ColIndex)->cell<WorkspaceMementoItem::ItemType>(m_rowIndex) != m_value;
      }

      /*
      Abstract equals operator. Templated type indepenedent in interface.
      @param other : Abstract memento item to compare against.
      @return true if equal.
      @throw runtime error if type mismatch.
      */
      bool equals(AbstractMementoItem& other) const
      {
        WorkspaceMementoItem* pOther = dynamic_cast<WorkspaceMementoItem<ColNumber, ColType>* >(&other);
        if(pOther == NULL)
        {
          throw std::runtime_error("Cannot call AbstractMementoItem::equals() on incompatible types.");
        }
        else
        {
          return equals(*pOther);
        }
      }

      /**
      Strongly typed equals operator.
      @param other : ref to other (same exact type) workspace memento item to compare against.
      @return true if same.
      */
      bool equals(const WorkspaceMementoItem& other) const
      {
        return m_value == other.m_value;
      }

      /**
      Equals operator.
      @param other : ref to other (same exact type) workspace memento item to compare against.
      @return true if same.
      */
      bool operator==(const WorkspaceMementoItem& other) const
      {
        return equals(other);
      }

      /**
      Not equals operator.
      @param other : ref to other (same exact type) workspace memento item to compare against.
      @return true if not same.
      */
      bool operator!=(const WorkspaceMementoItem& other) const
      {
        return !equals(other);
      }

      /**
      Set the internal value. This is a reversable operation.
      @value : value to set on memento item.
      */
      void setValue(ColType value)
      {
        m_value = value;
      }

     
      /// Synchronise the changes (via setvalue) with the underlying table workspace. This is a non reversible operation. 
      void commit()
      {
        m_data.getColumn(ColIndex)->cell<WorkspaceMementoItem::ItemType>(m_rowIndex) = m_value;
      }

      /// Undo changes via setValue. 
      void rollback()
      {
        m_value = m_data.getColumn(ColIndex)->cell<WorkspaceMementoItem::ItemType>(m_rowIndex);
      }

      /**
      Get the value.
      @return value.
      */
      ColType getValue() const
      {
        return m_value; //TODO, should we be returning the set value or the commited value.
      }

    };

  }
}



#endif