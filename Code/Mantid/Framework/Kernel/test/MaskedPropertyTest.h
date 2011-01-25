#ifndef MASKEDPROPERTYTEST_H_
#define MASKEDPROPERTYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/MaskedProperty.h"

using namespace Mantid::Kernel;

class MaskedPropertyTest:public CxxTest::TestSuite
{
public:
  void testMaskProperty()
  {
    MaskedProperty<std::string> m_Property1("property1","value");
    TS_ASSERT(! m_Property1.name().compare("property1") );
    TS_ASSERT(! m_Property1.value().compare("value") );
    PropertyHistory prohist=m_Property1.createHistory();
    TS_ASSERT(! prohist.value().compare("*****") );
  }

  void testMaskProperty1()
  {
    MaskedProperty<std::string> m_Property2("property2","");
    TS_ASSERT(! m_Property2.name().compare("property2") );
    TS_ASSERT(! m_Property2.value().compare("") );
    PropertyHistory prohist=m_Property2.createHistory();
    TS_ASSERT(! prohist.value().compare("") );
  }
};

#endif
