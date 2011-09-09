//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidWorkflowAlgorithms/ReductionTableHandler.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/EmptyValues.h"

namespace Mantid
{
namespace WorkflowAlgorithms
{
  using namespace Kernel;
  using namespace API;
  using namespace DataObjects;

  ReductionTableHandler::ReductionTableHandler(TableWorkspace_sptr tableWS)
  {
    if(tableWS)
      m_reductionTable = tableWS;
    else
      createTable();
  }

  ReductionTableHandler::ReductionTableHandler()
  {
    createTable();
  }

  void ReductionTableHandler::createTable()
  {
    ITableWorkspace_sptr table = WorkspaceFactory::Instance().createTable("TableWorkspace");
    m_reductionTable = boost::dynamic_pointer_cast<TableWorkspace>(table);
    m_reductionTable->addColumn("str","ItemName");
    m_reductionTable->addColumn("str","StringValue");
    m_reductionTable->addColumn("int","IntValue");
    m_reductionTable->addColumn("double","DoubleValue");
  }

  std::string ReductionTableHandler::findStringEntry(const std::string& key)
  {
    try
    {
      int row = 0;
      m_reductionTable->find(key, row, 0);
      return m_reductionTable->String(row, STRINGENTRY_COL);
    } catch(std::out_of_range&) {
      return "";
    }
  }

  int ReductionTableHandler::findIntEntry(const std::string& key)
  {
    try
    {
      int row = 0;
      m_reductionTable->find(key, row, 0);
      return m_reductionTable->Int(row, INTENTRY_COL);
    } catch(std::out_of_range&) {
      return EMPTY_INT();
    }
  }

  double ReductionTableHandler::findDoubleEntry(const std::string& key)
  {
    try
    {
      int row = 0;
      m_reductionTable->find(key, row, 0);
      return m_reductionTable->Double(row, DOUBLEENTRY_COL);
    } catch(std::out_of_range&) {
      return EMPTY_DBL();
    }
  }

  MatrixWorkspace_sptr ReductionTableHandler::findWorkspaceEntry(const std::string& key)
  {
    MatrixWorkspace_sptr pointer;
    try
    {
      int row = 0;
      m_reductionTable->find(key, row, 0);
      const std::string workspaceName = m_reductionTable->String(row, STRINGENTRY_COL);
      pointer = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(workspaceName));
    } catch(std::out_of_range&) {
      // Didn't find anything, will return a NULL pointer
    }
    return pointer;
  }

  void ReductionTableHandler::addEntry(const std::string& key, const std::string& value)
  {
    TableRow row = m_reductionTable->appendRow();
    row << key << value << EMPTY_INT() << EMPTY_DBL();
  }

  void ReductionTableHandler::addEntry(const std::string& key, const int& value)
  {
    TableRow row = m_reductionTable->appendRow();
    row << key << "" << value << EMPTY_DBL();
  }

  void ReductionTableHandler::addEntry(const std::string& key, const double& value)
  {
    TableRow row = m_reductionTable->appendRow();
    row << key << "" << EMPTY_INT() << value;
  }


} // namespace WorkflowAlgorithms
} // namespace Mantid

