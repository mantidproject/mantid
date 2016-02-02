#ifndef MANTID_VATES_PRESENTER_UTILITIES_H
#define MANTID_VATES_PRESENTER_UTILITIES_H

#include "MantidVatesAPI/MDLoadingView.h"
#include "MantidVatesAPI/MDLoadingPresenter.h"
#include "MantidVatesAPI/ThresholdRange.h"
#include "MantidVatesAPI/TimeToTimeStep.h"
#include "MantidVatesAPI/Normalization.h"
#include "MantidVatesAPI/WorkspaceProvider.h"

#include "MantidVatesAPI/vtkMDHistoLineFactory.h"
#include "MantidVatesAPI/vtkMDHistoQuadFactory.h"
#include "MantidVatesAPI/vtkMDHistoHexFactory.h"
#include "MantidVatesAPI/vtkMDHistoHex4DFactory.h"
#include "MantidVatesAPI/vtkMD0DFactory.h"
#include "MantidVatesAPI/vtkMDHexFactory.h"
#include "MantidVatesAPI/vtkMDQuadFactory.h"
#include "MantidVatesAPI/vtkMDLineFactory.h"

#include "MantidKernel/make_unique.h"
#include <vtkPVClipDataSet.h>

namespace Mantid {
namespace VATES {

/**
 * This templated function sets up an in memory loading presenter.
 * @param view: the loading view type
 * @param wsName: the name of the workspace which is to be displayed
 * @param worksapceProvider: a worksapce provider
 * @returns a new in memory loading presenter.
 */
template<class Presenter>
std::unique_ptr<Presenter> createInMemoryPresenter(std::unique_ptr<MDLoadingView> view,
  Mantid::API::IMDWorkspace_sptr workspace, std::unique_ptr<WorkspaceProvider> workspaceProvider) {
  return Mantid::Kernel::make_unique<Presenter>(std::move(view), workspaceProvider.release(), workspace->name());
}

/// Creates a facotry chain for MDHisto workspaces
std::unique_ptr<vtkMDHistoHex4DFactory<TimeToTimeStep>> createFactoryChainForHistoWorkspace(ThresholdRange_scptr threshold,
                                                                                            VisualNormalization normalization,
                                                                                            double time);

/// Creates a factory chain for MDEvent workspaces
std::unique_ptr<vtkMDHexFactory> createFactoryChainForEventWorkspace(ThresholdRange_scptr threshold, VisualNormalization normalization, double time);

/// Function to apply the Change-of-Basis-Matrix
void applyCOBMatrixSettingsToVtkDataSet(Mantid::VATES::MDLoadingPresenter* presenter, vtkDataSet* dataSet);

/// Function to get clipped data sets.
vtkSmartPointer<vtkPVClipDataSet> getClippedDataSet(vtkSmartPointer<vtkDataSet> dataSet) ;
}
}

#endif
