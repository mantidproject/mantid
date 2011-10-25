/**
    @author Mathieu Doucet
    @date 12/10/2011

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidWorkflowAlgorithms/ReductionTableHandler.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/EmptyValues.h"
#include <MantidAPI/FileFinder.h>
#include "Poco/File.h"
#include <vector>

namespace Mantid
{
namespace WorkflowAlgorithms
{
  using namespace Kernel;
  using namespace API;
  using namespace DataObjects;

  ReductionTableHandler::ReductionTableHandler(TableWorkspace_sptr tableWS) : g_log(Kernel::Logger::get("Algorithm"))
  {
    if(tableWS)
      m_reductionTable = tableWS;
    else
      createTable();
  }

  ReductionTableHandler::ReductionTableHandler() : g_log(Kernel::Logger::get("Algorithm"))
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

  /// Find a file path for the given name
  /// @param name :: key for which to look up the file
  /// @param hint :: hint to prepend to the key, usually the instrument name
  std::string ReductionTableHandler::findFileEntry(const std::string& name, const std::string& hint) {
    std::string path = FileFinder::Instance().getFullPath(name);
    if (Poco::File(path).exists()) return path;

    try
    {
      std::vector<std::string> paths = FileFinder::Instance().findRuns(hint+name);
      if (Poco::File(paths[0]).exists()) return paths[0];
    } catch (...) {};

    return "";
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

  void ReductionTableHandler::addEntry(const std::string& key, const std::string& value, bool replace)
  {
    try
    {
      int row = 0;
      m_reductionTable->find(key, row, 0);
      if (replace) m_reductionTable->removeRow(row);
      else g_log.error() << "Entry " << key << "already exists: " << m_reductionTable->String(row, STRINGENTRY_COL)
          << std::endl << "   adding: " << value << std::endl;
    } catch(std::out_of_range&) {}

    TableRow row = m_reductionTable->appendRow();
    row << key << value << EMPTY_INT() << EMPTY_DBL();
  }

  void ReductionTableHandler::addEntry(const std::string& key, const int& value, bool replace)
  {
    try
    {
      int row = 0;
      m_reductionTable->find(key, row, 0);
      if (replace) m_reductionTable->removeRow(row);
      else g_log.error() << "Entry " << key << "already exists: " << m_reductionTable->Int(row, INTENTRY_COL)
          << std::endl << "   adding: " << value << std::endl;
    } catch(std::out_of_range&) {}

    TableRow row = m_reductionTable->appendRow();
    row << key << "" << value << EMPTY_DBL();
  }

  void ReductionTableHandler::addEntry(const std::string& key, const double& value, bool replace)
  {
    try
    {
      int row = 0;
      m_reductionTable->find(key, row, 0);
      if (replace) m_reductionTable->removeRow(row);
      else g_log.error() << "Entry " << key << "already exists: " << m_reductionTable->Double(row, DOUBLEENTRY_COL)
          << std::endl << "   adding: " << value << std::endl;
    } catch(std::out_of_range&) {}

    TableRow row = m_reductionTable->appendRow();
    row << key << "" << EMPTY_INT() << value;
  }


} // namespace WorkflowAlgorithms
} // namespace Mantid

