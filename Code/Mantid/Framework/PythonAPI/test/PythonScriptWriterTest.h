#ifndef MANTID_PYTHONAPI_PYTHONSCRIPTWRITERTEST_H_
#define MANTID_PYTHONAPI_PYTHONSCRIPTWRITERTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidPythonAPI/PythonScriptWriter.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidKernel/Property.h"

using namespace Mantid::PythonAPI;
using namespace Mantid::API;

class PythonScriptWriterTest : public CxxTest::TestSuite
{

public:

  void test_Write_From_WorkspaceHistory()
  {
    boost::shared_ptr<WorkspaceHistory> hist = createTestHistory();
    ScriptWriter *writer = new PythonScriptWriter();
    TS_ASSERT_THROWS(const std::string script = writer->write(*hist), Mantid::Kernel::Exception::NotImplementedError);
    const std::string expected = "FirstAlgorithm\nSecondAlgorithm\n";



    delete writer;

  }

private:

  boost::shared_ptr<WorkspaceHistory> createTestHistory()
  {
    if( m_history ) return m_history;
    
    m_history = boost::shared_ptr<WorkspaceHistory>(new WorkspaceHistory);

    AlgorithmHistory alg1("FirstAlgorithm", 2);
    alg1.addProperty("FirstAlgProperty", "1",false, Mantid::Kernel::Direction::Input);
    m_history->addHistory(alg1);
    AlgorithmHistory alg2("SecondAlgorithm", 1);
    alg2.addProperty("SecondAlgProperty", "5",true, Mantid::Kernel::Direction::Input);
    alg2.addProperty("SecondAlgProperty2", "10",true, Mantid::Kernel::Direction::Output);
    m_history->addHistory(alg2);
    return m_history;
  }

  boost::shared_ptr<WorkspaceHistory> m_history;
};


#endif /* MANTID_PYTHONAPI_PYTHONSCRIPTWRITERTEST_H_ */

