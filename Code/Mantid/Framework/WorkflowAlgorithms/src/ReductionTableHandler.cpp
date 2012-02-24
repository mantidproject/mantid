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

  ReductionTableHandler::ReductionTableHandler(TableWorkspace_sptr tableWS) : g_log(Kernel::Logger::get("ReductionHandler"))
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

  /// Create a new reduction table workspace
  void ReductionTableHandler::createTable()
  {
    ITableWorkspace_sptr table = WorkspaceFactory::Instance().createTable("TableWorkspace");
    m_reductionTable = boost::dynamic_pointer_cast<TableWorkspace>(table);
    m_reductionTable->addColumn("str","ItemName");
    m_reductionTable->addColumn("str","StringValue");
    m_reductionTable->addColumn("int","IntValue");
    m_reductionTable->addColumn("double","DoubleValue");
  }

  /// Find a string entry for the given key
  /// @param key :: key string
  std::string ReductionTableHandler::findStringEntry(const std::string& key)
  {
    try
    {
      size_t row = 0;
      m_reductionTable->find(key, row, 0);
      return m_reductionTable->String(row, STRINGENTRY_COL);
    } catch(std::out_of_range&) {}
    return "";
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

  /// Find an integer entry for the given key
  /// @param key :: key string
  int ReductionTableHandler::findIntEntry(const std::string& key)
  {
    try
    {
      size_t row = 0;
      m_reductionTable->find(key, row, 0);
      return m_reductionTable->Int(row, INTENTRY_COL);
    } catch(std::out_of_range&) {}
    return EMPTY_INT();
  }

  /// Find a double entry for the given key
  /// @param key :: key string
  double ReductionTableHandler::findDoubleEntry(const std::string& key)
  {
    try
    {
      size_t row = 0;
      m_reductionTable->find(key, row, 0);
      return m_reductionTable->Double(row, DOUBLEENTRY_COL);
    } catch(std::out_of_range&) {}
    return EMPTY_DBL();
  }

  /// Find a string entry corresponding to a workspace and return that workspace
  /// if it exists
  /// @param key :: key string
  MatrixWorkspace_sptr ReductionTableHandler::findWorkspaceEntry(const std::string& key)
  {
    MatrixWorkspace_sptr pointer;
    try
    {
      size_t row = 0;
      m_reductionTable->find(key, row, 0);
      const std::string workspaceName = m_reductionTable->String(row, STRINGENTRY_COL);
      pointer = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(workspaceName);
    } catch(std::out_of_range&) {
      // Didn't find anything, will return a NULL pointer
    }
    return pointer;
  }

  /// Add a string entry with a given key
  /// @param key :: key string
  /// @param value :: string value to add
  /// @param replace :: if true, the current value will be replaced
  void ReductionTableHandler::addEntry(const std::string& key, const std::string& value, bool replace)
  {
    const std::string oldValue = findStringEntry(key);

    // If the entry is already there with the same value, just return
    if (oldValue == value) return;

    if (oldValue.size()>0)
    {
      if (replace)
      {
        size_t irow = 0;
        m_reductionTable->find(key, irow, 0);
        m_reductionTable->removeRow(irow);
      } else {
    	  g_log.notice() << "Entry " << key << " already exists: " << oldValue
          << std::endl << "   skipping adding " << value << std::endl;
    	  return;
      }
    }

    TableRow row = m_reductionTable->appendRow();
    row << key << value << EMPTY_INT() << EMPTY_DBL();
  }

  /// Add an integer entry with a given key
  /// @param key :: key string
  /// @param value :: integer value to add
  /// @param replace :: if true, the current value will be replaced
  void ReductionTableHandler::addEntry(const std::string& key, const int& value, bool replace)
  {
    const int oldValue = findIntEntry(key);

    // If the entry is already there with the same value, just return
    if (oldValue == value) return;

    if (oldValue != EMPTY_INT())
    {
      if (replace)
      {
        size_t irow = 0;
        m_reductionTable->find(key, irow, 0);
        m_reductionTable->removeRow(irow);
      } else {
    	  g_log.notice() << "Entry " << key << " already exists: " << oldValue
            << std::endl << "   skipping adding " << value << std::endl;
    	  return;
      }
    }

    TableRow row = m_reductionTable->appendRow();
    row << key << "" << value << EMPTY_DBL();
  }

  /// Add a double entry with a given key
  /// @param key :: key string
  /// @param value :: double value to add
  /// @param replace :: if true, the current value will be replaced
  void ReductionTableHandler::addEntry(const std::string& key, const double& value, bool replace)
  {
    const double oldValue = findDoubleEntry(key);

    // If the entry is already there with the same value, just return
    if (std::fabs( (oldValue - value)/value ) < 1e-8) return;

    if (std::fabs( (oldValue - EMPTY_DBL())/(EMPTY_DBL()) ) > 1e-8)
    {
      if (replace)
      {
        size_t irow = 0;
        m_reductionTable->find(key, irow, 0);
        m_reductionTable->removeRow(irow);
      } else {
    	  g_log.notice() << "Entry " << key << " already exists: " << oldValue
            << std::endl << "   skipping adding " << value << std::endl;
      	  return;
      }
    }

    TableRow row = m_reductionTable->appendRow();
    row << key << "" << EMPTY_INT() << value;
  }


} // namespace WorkflowAlgorithms
} // namespace Mantid

