#ifndef MANTID_DATAOBJECTS_MEMENTO_ITEM_H_
#define MANTID_DATAOBJECTS_MEMENTO_ITEM_H_

#include "MantidAPI/ITableWorkspace.h"
#include "MantidQtCustomInterfaces/AbstractMementoItem.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {
    /**
    Typedef produces distinct, non-compatible types based on an integer template arg.
    */
    template<int v>
    struct  Int2Type
    {
      enum { type_value = v };
      explicit Int2Type(int arg) : m_value(arg){ }
      operator int() const
      {
        return m_value;
      }
    private:
      int m_value;
    };

    /// Typedef to delcare a new type to act as a row
    typedef  Int2Type<1> Row;
    // Typedef to declare a new type to act as a column
    typedef  Int2Type<2> Column;

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
    template<typename ColType>
    class DLLExport WorkspaceMementoItem : public AbstractMementoItem
    {
    private:
      /// Actual/outstanding value stored in cell.
      ColType m_value;
      /// Reference to mutable table workspace.
      Mantid::API::ITableWorkspace_sptr m_data;
      /// Row onto which this column object projects.
      int m_rowIndex;
      /// Column index onto which this mementoitem maps.
      int m_colIndex;
      /// Name for the item.
      std::string name;
      /// Flag indicating that a column has been created in the table workspace for this item.
      bool m_newDefinition;

    protected:

      /**
      Getter for the type info.
      @return type_info of stored value type.
      */
      virtual const std::type_info& get_type_info() const
      {
         return typeid(ColType);
      }

      /**
      Get the stored value as a void ptr.
      @return void* to underlying data.
      */
      void* getValueVoidPtr()
      {
        return &m_value;
      }

      /**
      Set the value.
      */
      void setValueVoidPtr(void* value)
      {
        m_value = *static_cast<ColType*>(value);
      }

    public:
      /// Type of item being stored.
      typedef ColType ItemType;
      
      /**
      Constructor
      @param data : ref to table workspace storing the data.
      @param rowIndex : index of the row in the table workspace that this column/memento item. is to apply.
      @param colIndex : column index of the same table workspace.
      */
      WorkspaceMementoItem(Mantid::API::ITableWorkspace_sptr data, Row rowIndex, Column colIndex) : m_data(data), m_rowIndex(rowIndex), m_colIndex(colIndex), m_newDefinition(false)
      {
        m_value = m_data->cell<ItemType>(m_rowIndex, m_colIndex);
      }

        /**
      Constructor
      @param data : ref to table workspace storing the data.
      @param rowIndex : index of the row in the table workspace that this column/memento item. is to apply.
      @param colIndex : column index of the same table workspace.
      @param newDefinition : true if a on-the-fly definition/column in the table workspace has been created
      */
      WorkspaceMementoItem(Mantid::API::ITableWorkspace_sptr data, Row rowIndex, Column colIndex, bool newDefinition) : m_data(data), m_rowIndex(rowIndex), m_colIndex(colIndex), m_newDefinition(newDefinition)
      {
        m_value = m_data->cell<ItemType>(m_rowIndex, m_colIndex);
        name = m_data->getColumn(m_colIndex)->name();
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
        /*
        If a column has had to be deleted out of the underlying table workspace, see (bool m_newDefinition),
        then this object cannot peform comparisions against that column anymore.
        */
        if(m_data->columnCount() <= m_colIndex)
        {
          return false;
        }
        return m_data->cell<ItemType>(m_rowIndex, m_colIndex) != m_value;
      }

      /*
      Abstract equals operator. Templated type indepenedent in interface.
      @param other : Abstract memento item to compare against.
      @return true if equal.
      @throw runtime error if type mismatch.
      */
      bool equals(AbstractMementoItem& other) const
      {
        WorkspaceMementoItem* pOther = dynamic_cast<WorkspaceMementoItem<ColType>* >(&other);
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
        /*
        If a column has had to be deleted out of the underlying table workspace, see (bool m_newDefinition),
        then this object cannot peform comparisions/operations against that column anymore, so check first.
        */
        if(m_data->columnCount() > m_colIndex)
        {
          m_data->cell<ItemType>(m_rowIndex, m_colIndex) = m_value;
        }
      }

      /// Undo changes via setValue. 
      void rollback()
      {
        m_value = m_data->cell<ItemType>(m_rowIndex, m_colIndex);
        //Remove the column if it was introduced as part of a new definition
        if(m_newDefinition)
        {
          try
          {
            m_data->removeColumn(name);
          }
          catch(std::runtime_error&)
          { //m_data->getColumn(name) will throw if the column does not exist.
          }
        }
      }

      /**
      Get the value.
      @return value.
      */
      ColType getValue() const
      {
        return m_value; //TODO, should we be returning the set value or the commited value.
      }

      /**
      Getter for the item name.
      @return column/item name.
      */
      const std::string& getName() const
      {
        return m_data->getColumn(m_colIndex)->name();
      }

      /**
      Getter for the new definition flag.
      */
      bool getIsNewDefinition() const
      {
        return m_newDefinition;
      }

    };

  }
}



#endif