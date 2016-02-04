#ifndef VATES_API_SAVE_MD_WORKSPACE_TO_VTK_H_
#define VATES_API_SAVE_MD_WORKSPACE_TO_VTK_H_
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include <map>

namespace Mantid
{
namespace VATES
{

class SaveMDWorkspaceToVTKImpl;

class DLLExport SaveMDWorkspaceToVTK : public Mantid::API::Algorithm
{
public:
    SaveMDWorkspaceToVTK();
    ~SaveMDWorkspaceToVTK();
    virtual const std::string name() const;
    virtual int version() const;
    virtual const std::string category() const;
    virtual const std::string summary() const;

private:
    void init();
    void exec();
    std::map<std::string, std::string> validateInputs();
    std::unique_ptr<SaveMDWorkspaceToVTKImpl> pimpl;
};
}
}
#endif
