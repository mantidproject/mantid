#ifndef MANTID_WORKFLOWALGORITHMS_REDUCTIONTABLEHANDLER_H_
#define MANTID_WORKFLOWALGORITHMS_REDUCTIONTABLEHANDLER_H_
/*WIKI*



*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Exception.h"
#include "MantidDataObjects/TableWorkspace.h"

namespace Mantid
{
namespace WorkflowAlgorithms
{
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
  class DLLExport ReductionTableHandler : public DataObjects::TableWorkspace
  {
  public:
    ReductionTableHandler();
    ReductionTableHandler(DataObjects::TableWorkspace_sptr tableWS);
    /// Return a pointer to the workspace table
    DataObjects::TableWorkspace_sptr getTable() { return m_reductionTable; }
    /// Find a string entry for the given key
    std::string findStringEntry(const std::string& key);
    /// Find an integer entry for the given key
    int findIntEntry(const std::string& key);
    /// Find a double entry for the given key
    double findDoubleEntry(const std::string& key);
    /// Find a workspace with the name of the string entry corresponding to the given key
    API::MatrixWorkspace_sptr findWorkspaceEntry(const std::string& key);

    /// Add a string entry with a given key
    void addEntry(const std::string& key, const std::string& value);
    /// Add an integer entry with a given key
    void addEntry(const std::string& key, const int& value);
    /// Add a double entry with a given key
    void addEntry(const std::string& key, const double& value);

    /// Find a file path for the given string
    static std::string findFileEntry(const std::string& name, const std::string& hint="");

  private:
    /// Create a reduction table
    void createTable();
    /// Shared pointer to the reduction table
    DataObjects::TableWorkspace_sptr m_reductionTable;
    /// Column number of string entries
    static const int STRINGENTRY_COL = 1;
    /// Column number of integer entries
    static const int INTENTRY_COL = 2;
    /// Column number of double entries
    static const int DOUBLEENTRY_COL = 3;
  };
}
}
#endif /*MANTID_WORKFLOWALGORITHMS_REDUCTIONTABLEHANDLER_H_*/
