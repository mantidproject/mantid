#ifndef MANTID_CUSTOMINTERFACES_SLICEDREDUCTIONWORKSPACES_H_
#define MANTID_CUSTOMINTERFACES_SLICEDREDUCTIONWORKSPACES_H_
#include <string>
#include <vector>
#include "ReductionWorkspaces.h"
#include "Slicing.h"

namespace MantidQt {
namespace CustomInterfaces {

class SlicedReductionWorkspaces {
public:
  SlicedReductionWorkspaces(std::string inputWorkspace,
                            std::vector<ReductionWorkspaces> sliceWorkspaces);

  std::string const &inputWorkspace() const;
  std::vector<ReductionWorkspaces> const &sliceWorkspaces() const;

private:
  std::string m_inputWorkspace;
  std::vector<ReductionWorkspaces> m_sliceWorkspaces;
};

MANTIDQT_ISISREFLECTOMETRY_DLL SlicedReductionWorkspaces
workspaceNamesForSliced(
    std::vector<std::string> const &summedRunNumbers,
    std::pair<std::string, std::string> const &transmissionRuns,
    Slicing const &slicing);

MANTIDQT_ISISREFLECTOMETRY_DLL bool
operator==(SlicedReductionWorkspaces const &lhs,
           SlicedReductionWorkspaces const &rhs);
MANTIDQT_ISISREFLECTOMETRY_DLL bool
operator!=(SlicedReductionWorkspaces const &lhs,
           SlicedReductionWorkspaces const &rhs);
}
}
#endif // MANTID_CUSTOMINTERFACES_SLICEDREDUCTIONWORKSPACES_H_
