#ifndef MANTID_CUSTOMINTERFACES_ABSTRACT_MEMENTO_ITEM_H_
#define MANTID_CUSTOMINTERFACES_ABSTRACT_MEMENTO_ITEM_H_

#include <boost/shared_ptr.hpp>
#include <stdexcept>
#include "MantidKernel/System.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {

    /**  @class AbstractMementoItem
    Interface for a memento item.
   */
    class DLLExport AbstractMementoItem
    {
    public:
      virtual bool hasChanged() const = 0;
      virtual void commit() = 0;
      virtual void rollback() = 0;
      virtual bool equals(AbstractMementoItem& other) const = 0;
      virtual const std::string& getName() const = 0;
      virtual ~AbstractMementoItem() {}
      template<class T>
      void checkType()const
      {
          std::string thisTypename(get_type_info().name());
          std::string paramTypename(typeid(T).name());
          if(typeid(T) != get_type_info())
          {
            std::string message = "Type missmatch while using AbstractMementoItem. Cannot compare: '" + thisTypename + "' to : '" + paramTypename +"'";
            throw std::runtime_error(message);
          }
      }
      template<class T>
      void getValue(T& val)
      {
         checkType<T>();
         val = *static_cast<T*>(getValueVoidPtr());
      }
      virtual const std::type_info& get_type_info() const = 0;
      template<class T>
      void setValue(T& value)
      {
        checkType<T>();
        setValueVoidPtr(&value);
      }
    protected:
      virtual void* getValueVoidPtr() = 0;
      virtual void setValueVoidPtr(void* value) = 0;
    };

    typedef boost::shared_ptr<AbstractMementoItem> AbstractMementoItem_sptr;
  }
}

#endif