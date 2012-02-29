#ifndef PROPERTYWITHVALUEFACTORYTEST_H_
#define PROPERTYWITHVALUEFACTORYTEST_H_

#include <cxxtest/TestSuite.h>
#ifdef _MSC_VER
  #pragma warning(push)
  #pragma warning(disable:4244)
  #pragma warning(disable:4005)
#endif
#include "MantidPythonInterface/api/PythonAlgorithm/PropertyWithValueFactory.h"
#ifdef _MSC_VER
  #pragma warning(push)
#endif

#include "MantidKernel/PropertyWithValue.h"

//-------------------------------------------------------------------------

using Mantid::PythonInterface::PropertyWithValueFactory;

class PropertyWithValueFactoryTest: public CxxTest::TestSuite
{
  public:
    
    void test_builtin_type_creates_property_without_error()
    {
      using namespace boost::python;
      using namespace Mantid::Kernel;
      object pyint = object(handle<>(PyInt_FromLong(10)));
      Property *p(NULL);
      TS_ASSERT_THROWS_NOTHING(p = PropertyWithValueFactory::create("NewProperty", pyint, Direction::Input));
      TS_ASSERT(p);
      // Is it typed
      PropertyWithValue<long> *typedProp = dynamic_cast<PropertyWithValue<long> *>(p);
      TS_ASSERT(typedProp);      
    }
};

#endif /* PROPERTYWITHVALUEFACTORYTEST_H_ */
