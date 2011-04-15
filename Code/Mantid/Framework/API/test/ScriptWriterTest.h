#ifndef MANTID_API_SCRIPTWRITERTEST_H_
#define MANTID_API_SCRIPTWRITERTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/ScriptWriter.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidKernel/Property.h"

using namespace Mantid::API;

class ScriptWriterTest : public CxxTest::TestSuite
{

private:

  class TestWriter : public ScriptWriter
  {
  public:
    /// This version simply writes each algorithm name separated by a newline
    std::string write(const WorkspaceHistory & history) const
    {
      std::string script;
      const std::vector<AlgorithmHistory>& alg_info = history.getAlgorithmHistories();
      for( size_t i = 0; i < alg_info.size(); ++i )
      {
	script += alg_info[i].name() + "\n";
      }
      return script;
    }

  };

public:

  void test_Write_From_WorkspaceHistory()
  {
    boost::shared_ptr<WorkspaceHistory> hist = createTestHistory();
    ScriptWriter *writer = new TestWriter();
    const std::string script = writer->write(*hist);
    const std::string expected = "FirstAlgorithm\nSecondAlgorithm\n";
    TS_ASSERT_EQUALS(script, expected);

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
    m_history->addHistory(alg2);
    return m_history;
  }

  boost::shared_ptr<WorkspaceHistory> m_history;
};


#endif /* MANTID_API_SCRIPTWRITERTEST_H_ */

