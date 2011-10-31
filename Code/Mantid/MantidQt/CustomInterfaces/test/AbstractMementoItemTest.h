#ifndef CUSTOM_INTERFACES_ABSTRACT_MEMENTO_ITEM_TEST_H_
#define CUSTOM_INTERFACES_ABSTRACT_MEMENTO_ITEM_TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidQtCustomInterfaces/AbstractMementoItem.h"
using namespace MantidQt::CustomInterfaces;

class AbstractMementoItemTest : public CxxTest::TestSuite
{

private:

    // Helper type. Providing a minimal concrete implementation of an AbstractMementoItem wrapping a double.
    class DoubleMementoItem : public AbstractMementoItem
    {
    public:
      DoubleMementoItem() : m_val(1) {}
      virtual bool hasChanged() const {return false;}
      virtual void commit(){};
      virtual void rollback(){};
      virtual const std::string& getName() const{throw std::runtime_error("Not Implemented");}
      virtual bool equals(AbstractMementoItem&) const {return true;};
      virtual ~DoubleMementoItem() {}
    protected:
      virtual void* getValueVoidPtr()
      {
        return &m_val;
      }
      virtual void setValueVoidPtr(void*) {};
      virtual const std::type_info& get_type_info() const
      {
        return typeid(m_val);
      }
      double m_val;
    };

public:

//=====================================================================================
// Functional tests
//=====================================================================================
   void testTypeChecking()
   {
     DoubleMementoItem item;
     TSM_ASSERT_THROWS_NOTHING("item uses std::string, type check should pass!", item.checkType<double>());
     TSM_ASSERT_THROWS("Wrong type. Should throw.", item.checkType<int>(), std::runtime_error);
     TSM_ASSERT_THROWS("Wrong type. Should throw.", item.checkType<float>(), std::runtime_error);
     TSM_ASSERT_THROWS("Wrong type. Should throw.", item.checkType<std::string>(), std::runtime_error);
   }
};

#endif