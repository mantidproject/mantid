#ifndef MANTID_WORKFLOWALGORITHMS_REDUCTIONTABLEHANDLER_H_
#define MANTID_WORKFLOWALGORITHMS_REDUCTIONTABLEHANDLER_H_

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
    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
  class DLLExport ReductionTableHandler : public DataObjects::TableWorkspace
  {
  public:
    ReductionTableHandler();
    ReductionTableHandler(DataObjects::TableWorkspace_sptr tableWS);
    DataObjects::TableWorkspace_sptr getTable() { return m_reductionTable; }
    std::string findStringEntry(const std::string& key);
    int findIntEntry(const std::string& key);
    double findDoubleEntry(const std::string& key);
    API::MatrixWorkspace_sptr findWorkspaceEntry(const std::string& key);

    void addEntry(const std::string& key, const std::string& value);
    void addEntry(const std::string& key, const int& value);
    void addEntry(const std::string& key, const double& value);
  private:
    void createTable();
    DataObjects::TableWorkspace_sptr m_reductionTable;
    static const int STRINGENTRY_COL = 1;
    static const int INTENTRY_COL = 2;
    static const int DOUBLEENTRY_COL = 3;
  };
}
}
#endif /*MANTID_WORKFLOWALGORITHMS_REDUCTIONTABLEHANDLER_H_*/
