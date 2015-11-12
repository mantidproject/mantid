#ifndef MANTID_CUSTOMINTERFACES_TRASNFERRESULTS_H_
#define MANTID_CUSTOMINTERFACES_TRASNFERRESULTS_H_

#include "MantidQtCustomInterfaces/DllConfig.h"
#include <vector>
#include <map>
#include <boost/make_shared.hpp>

namespace MantidQt {
namespace CustomInterfaces {
class MANTIDQT_CUSTOMINTERFACES_DLL TransferResults {
public:
    TransferResults(std::vector<std::map<std::string, std::string> > transferRuns,
        std::vector<std::map<std::string, std::string> > errorRuns);
    
    std::vector<std::map<std::string, std::string> > getTransferRuns();
    std::vector<std::map<std::string, std::string> > getErrorRuns();

    void addTransferRow(const std::map<std::string, std::string>& row);
    void addErrorRow(std::string id, std::string error);
protected:
    std::vector<std::map<std::string, std::string> > m_transferRuns;
    std::vector<std::map<std::string, std::string> > m_errorRuns;
};

}
}

#endif // MANTID_CUSTOMINTERFACES_TRASNFERRESULTS_H_!
