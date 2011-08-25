#ifndef PYTHONOBJECTINSTANTIATORTEST_H_
#define PYTHONOBJECTINSTANTIATORTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/IAlgorithm.h"
#include "MantidPythonInterface/kernel/PythonObjectInstantiator.h"

#include <boost/python/object.hpp>

using Mantid::PythonInterface::PythonObjectInstantiator;
using Mantid::API::IAlgorithm;
using Mantid::API::IAlgorithm_sptr;

class PythonObjectInstantiatorTest: public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PythonObjectInstantiatorTest *createSuite() { return new PythonObjectInstantiatorTest(); }
  static void destroySuite( PythonObjectInstantiatorTest *suite ) { delete suite; }

public:
  //
  PythonObjectInstantiatorTest() : m_creator(NULL)
  {
    Py_Initialize();
  }

  ~PythonObjectInstantiatorTest()
  {
    delete m_creator;
    Py_Finalize();
  }

  void test_Bare_Pointer()
  {
    PythonObjectInstantiator<IAlgorithm> *factory = getInstantiator();
    IAlgorithm *alg = factory->createUnwrappedInstance();
    TS_ASSERT(alg);
    TS_ASSERT_EQUALS(alg->name(), "PyAlg");
    TS_ASSERT_EQUALS(alg->version(), 1);
    TS_ASSERT_EQUALS(alg->category(), "PythonAlgorithms");
  }

  void test_Shared_Pointer()
   {
     PythonObjectInstantiator<IAlgorithm> *factory = getInstantiator();
     IAlgorithm_sptr alg = factory->createInstance();
     TS_ASSERT(alg);
     TS_ASSERT_EQUALS(alg->name(), "PyAlg");
     TS_ASSERT_EQUALS(alg->version(), 1);
     TS_ASSERT_EQUALS(alg->category(), "PythonAlgorithms");
   }

private:
  PythonObjectInstantiator<IAlgorithm> *getInstantiator()
  {
    if( !m_creator )
    {
      std::string code = "from mantid.api import PythonAlgorithm\n"
        "class PyAlg(PythonAlgorithm):\n"
        "  pass\n";
      PyRun_SimpleString(code.c_str());
      PyObject *main = PyImport_AddModule("__main__");
      TS_ASSERT(main);
      boost::python::object cls(boost::python::handle<>(PyObject_GetAttrString(main, "PyAlg")));
      TS_ASSERT(cls);
      m_creator = new PythonObjectInstantiator<IAlgorithm>(cls);
    }
    return m_creator;
  }

  /// Instantiator instance
  PythonObjectInstantiator<IAlgorithm> *m_creator;
};

#endif /* PYTHONOBJECTINSTANTIATORTEST_H_ */
