#ifndef MANTID_VATES_PRESENTER_UTILITIES_H
#define MANTID_VATES_PRESENTER_UTILITIES_H

#include "MantidKernel/System.h"

#include "MantidVatesAPI/MDLoadingView.h"
#include "MantidVatesAPI/TimeToTimeStep.h"
#include "MantidVatesAPI/vtkMDHistoHex4DFactory.h"
#include "MantidVatesAPI/vtkMDHexFactory.h"
#include "MantidVatesAPI/WorkspaceProvider.h"

#include <vtkPVClipDataSet.h>

namespace Mantid
{
namespace VATES
{

// Forward Decalaration
class MDLoadingPresenter;

class DLLExport EmptyWorkspaceNamePolicy
{
protected:
    std::string getWorkspaceName(Mantid::API::IMDWorkspace_sptr)
    {
        return "__EmptyWorkspaceNamePolicy";
    }
};

class DLLExport NonEmptyWorkspaceNamePolicy
{
protected:
    std::string getWorkspaceName(Mantid::API::IMDWorkspace_sptr workspace)
    {
        return workspace->name();
    }
};

/**
 * This templated function sets up an in memory loading presenter.
 * @param view: the loading view type
 * @param wsName: the name of the workspace which is to be displayed
 * @param worksapceProvider: a worksapce provider
 * @returns a new in memory loading presenter.
 */
template <class Presenter, class WorkspaceNamePolicy>
class DLLExport InMemoryPresenterFactory : private WorkspaceNamePolicy
{
    using WorkspaceNamePolicy::getWorkspaceName;

public:
    std::unique_ptr<Presenter>
    create(std::unique_ptr<MDLoadingView> view,
           Mantid::API::IMDWorkspace_sptr workspace,
           std::unique_ptr<WorkspaceProvider> workspaceProvider)
    {
        return Mantid::Kernel::make_unique<Presenter>(
            std::move(view), workspaceProvider.release(),
            getWorkspaceName(workspace));
    }
};

/// Creates a facotry chain for MDHisto workspaces
std::unique_ptr<vtkMDHistoHex4DFactory<TimeToTimeStep>> DLLExport createFactoryChainForHistoWorkspace(ThresholdRange_scptr threshold,
                                    VisualNormalization normalization,
                                    double time);

/// Creates a factory chain for MDEvent workspaces
std::unique_ptr<vtkMDHexFactory> DLLExport createFactoryChainForEventWorkspace(ThresholdRange_scptr threshold,
                                    VisualNormalization normalization,
                                    double time);

/// Function to apply the Change-of-Basis-Matrix
void DLLExport applyCOBMatrixSettingsToVtkDataSet(MDLoadingPresenter *presenter, vtkDataSet *dataSet,
    std::unique_ptr<Mantid::VATES::WorkspaceProvider> workspaceProvider);

/// Function to get clipped data sets.
vtkSmartPointer<vtkPVClipDataSet> DLLExport getClippedDataSet(vtkSmartPointer<vtkDataSet> dataSet);

/// Create name with timestamp attached.
std::string DLLExport createTimeStampedName(std::string name);
}
}

#endif
