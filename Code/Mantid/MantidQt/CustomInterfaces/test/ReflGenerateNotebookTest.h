#ifndef MANTID_CUSTOMINTERFACES_REFLGENERATENOTEBOOKTEST_H
#define MANTID_CUSTOMINTERFACES_REFLGENERATENOTEBOOKTEST_H

#include <cxxtest/TestSuite.h>

#include "MantidQtCustomInterfaces/ReflGenerateNotebook.h"
#include "MantidQtCustomInterfaces/QReflTableModel.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/ITableWorkspace.h"

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;

class ReflGenerateNotebookTest : public CxxTest::TestSuite {

private:

  ITableWorkspace_sptr createWorkspace(const std::string &wsName) {
    ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();

    auto colRuns = ws->addColumn("str", "Run(s)");
    auto colTheta = ws->addColumn("str", "ThetaIn");
    auto colTrans = ws->addColumn("str", "TransRun(s)");
    auto colQmin = ws->addColumn("str", "Qmin");
    auto colQmax = ws->addColumn("str", "Qmax");
    auto colDqq = ws->addColumn("str", "dq/q");
    auto colScale = ws->addColumn("double", "Scale");
    auto colStitch = ws->addColumn("int", "StitchGroup");
    auto colOptions = ws->addColumn("str", "Options");

    colRuns->setPlotType(0);
    colTheta->setPlotType(0);
    colTrans->setPlotType(0);
    colQmin->setPlotType(0);
    colQmax->setPlotType(0);
    colDqq->setPlotType(0);
    colScale->setPlotType(0);
    colStitch->setPlotType(0);
    colOptions->setPlotType(0);

    if (wsName.length() > 0)
      AnalysisDataService::Instance().addOrReplace(wsName, ws);

    return ws;
  }

  ITableWorkspace_sptr createPrefilledWorkspace(const std::string &wsName) {
    auto ws = createWorkspace(wsName);
    TableRow row = ws->appendRow();
    row << "12345"
    << "0.5"
    << ""
    << "0.1"
    << "1.6"
    << "0.04" << 1.0 << 0 << "";
    row = ws->appendRow();
    row << "12346"
    << "1.5"
    << ""
    << "1.4"
    << "2.9"
    << "0.04" << 1.0 << 0 << "";
    row = ws->appendRow();
    row << "24681"
    << "0.5"
    << ""
    << "0.1"
    << "1.6"
    << "0.04" << 1.0 << 1 << "";
    row = ws->appendRow();
    row << "24682"
    << "1.5"
    << ""
    << "1.4"
    << "2.9"
    << "0.04" << 1.0 << 1 << "";
    return ws;
  }

  void createModel(const std::string &wsName)
  {
    ITableWorkspace_sptr m_ws = createPrefilledWorkspace(wsName);
    m_model.reset(new QReflTableModel(m_ws));
  }

  const std::string m_wsName = "TESTWORKSPACE";
  const std::string m_instrument = "INSTRUMENT";
  QReflTableModel_sptr m_model;
  std::set<int> m_rows;
  std::map<int,std::set<int> > m_groups;
  const ColNumbers col_nums{0, 2, 8, 1, 3, 4, 5, 6, 7};

public:

  // Create a notebook to test
  void setUp()
  {
    createModel(m_wsName);

    std::set<int> rows;
    // Populate rows with every index in the model
    for(int idx = 0; idx < m_model->rowCount(); ++idx)
      rows.insert(idx);

    //Map group numbers to the set of rows in that group we want to process
    std::map<int,std::set<int> > groups;
    for(auto it = rows.begin(); it != rows.end(); ++it)
      groups[m_model->data(m_model->index(*it, col_nums.group)).toInt()].insert(*it);
  }

  void testGenerateNotebook() {

    std::unique_ptr<ReflGenerateNotebook> notebook(new ReflGenerateNotebook(m_wsName, m_model, m_instrument,
                                                   col_nums.runs, col_nums.transmission, col_nums.options, col_nums.angle,
                                                   col_nums.qmin, col_nums.qmax, col_nums.dqq, col_nums.scale, col_nums.group));

    std::string generatedNotebook = notebook->generateNotebook(m_groups, m_rows);

    std::vector<std::string> notebookLines;
    boost::split(notebookLines, generatedNotebook, boost::is_any_of("\n"));

    const std::string result[] = {
      "{",
      "   \"metadata\" : {",
      "      \"name\" : \"Mantid Notebook\"",
      "   },",
      "   \"nbformat\" : 3,",
      "   \"nbformat_minor\" : 0,",
      "   \"worksheets\" : [",
      "      {",
      "         \"cells\" : [",
      "            {",
      "               \"cell_type\" : \"markdown\",",
    };

    // Check that the first 10 lines are output as expected
    for (int i=0; i<11; ++i)
    {
      TS_ASSERT_EQUALS(notebookLines[i], result[i])
    }

  }
  
};

#endif //MANTID_CUSTOMINTERFACES_REFLGENERATENOTEBOOKTEST_H
