#ifndef PYTHONOBJECTINSTANTIATORTEST_H_
#define PYTHONOBJECTINSTANTIATORTEST_H_

#include "MantidAPI/IAlgorithm.h"
#include "MantidKernel/make_unique.h"
#include "MantidPythonInterface/kernel/PythonObjectInstantiator.h"
#include <cxxtest/TestSuite.h>

#include <boost/python/object.hpp>

//-------------------------------------------------------------------------

using Mantid::API::IAlgorithm;
using Mantid::API::IAlgorithm_sptr;
using Mantid::PythonInterface::PythonObjectInstantiator;

class PythonAlgorithmInstantiatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PythonAlgorithmInstantiatorTest *createSuite() {
    return new PythonAlgorithmInstantiatorTest();
  }
  static void destroySuite(PythonAlgorithmInstantiatorTest *suite) {
    delete suite;
  }

public:
  void test_Bare_Pointer_Throws_On_Creation() {
    TS_ASSERT_THROWS(instantiator().createUnwrappedInstance(),
                     std::runtime_error);
  }

  void test_Shared_Pointer() {
    IAlgorithm_sptr alg;
    TS_ASSERT_THROWS_NOTHING(alg = instantiator().createInstance());
    TS_ASSERT(alg);
    if (!alg)
      return;
    TS_ASSERT_EQUALS(alg->name(), "PyAlg");
    TS_ASSERT_EQUALS(alg->version(), 1);
    TS_ASSERT_EQUALS(alg->category(), "system");
  }

private:
  using PythonAlgorithmInstantiator = PythonObjectInstantiator<IAlgorithm>;

  PythonAlgorithmInstantiator const &instantiator() {
    if (!m_creator) {
      // Assume this is where the mantid package is too
      auto code = "from mantid.api import PythonAlgorithm\n"
                  "class PyAlg(PythonAlgorithm):\n"
                  "    def category(self):\n"
                  "        return 'system'\n";
      PyRun_SimpleString(code);
      PyObject *main = PyImport_AddModule("__main__");
      TS_ASSERT(main);
      boost::python::object cls(
          boost::python::handle<>(PyObject_GetAttrString(main, "PyAlg")));
      TS_ASSERT(cls);
      m_creator = Mantid::Kernel::make_unique<PythonAlgorithmInstantiator>(cls);
    }
    return *m_creator;
  }

  /// Instantiator instance
  std::unique_ptr<PythonAlgorithmInstantiator> m_creator;
};

#endif /* PYTHONOBJECTINSTANTIATORTEST_H_ */
