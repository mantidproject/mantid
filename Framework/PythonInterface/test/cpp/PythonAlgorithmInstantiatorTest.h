#ifndef PYTHONOBJECTINSTANTIATORTEST_H_
#define PYTHONOBJECTINSTANTIATORTEST_H_

#include "MantidAPI/IAlgorithm.h"
#include "MantidKernel/make_unique.h"
#include "MantidPythonInterface/kernel/PythonObjectInstantiator.h"
#include "MantidPythonInterface/kernel/kernel.h"
#include <cxxtest/GlobalFixture.h>
#include <cxxtest/TestSuite.h>

#include "MantidKernel/ConfigService.h"
#include <boost/algorithm/string/trim.hpp>
#include <boost/python/object.hpp>

// ---------- Test world initialization ---------------------------------

/**
 * The cxxtest code ensures that the setup/tearDownWorld methods
 * are called exactly once per test-process. We use this
 * to initialize/shutdown the python interpreter
 */
class PythonProcessHandler : CxxTest::GlobalFixture {
public:
  bool setUpWorld() override {
    using Mantid::Kernel::ConfigService;
    using boost::algorithm::trim_right_copy_if;
    Py_Initialize();
    // add location of mantid module to sys.path
    std::string propDir =
        trim_right_copy_if(ConfigService::Instance().getPropertiesDir(),
                           [](char c) { return (c == '/' || c == '\\'); });
    const std::string appendMantidToPath = "import sys\n"
                                           "sys.path.insert(0, '" +
                                           propDir + "')";
    PyRun_SimpleString(appendMantidToPath.c_str());
    PyRun_SimpleString("import mantid");
#ifdef _WIN32
    // See kernel.h for the explanation
    kernel_dll_import_numpy_capi_for_unittest();
#endif
    return true;
  }

  bool tearDown() override {
    // Some test methods leave the Python error handler with an error
    // set that confuse other tests when the executable is run as a whole
    // Clear the errors after each suite method is run
    PyErr_Clear();
    return CxxTest::GlobalFixture::tearDown();
  }

  bool tearDownWorld() override {
    Py_Finalize();
    return true;
  }
};

// From the cxxtest manual:
//
// We can rely on this file being included exactly once
// and declare this global variable in the header file.
//
static PythonProcessHandler pythonProcessHandler;

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
