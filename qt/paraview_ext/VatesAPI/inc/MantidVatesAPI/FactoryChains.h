#ifndef MANTID_VATES_FACTORY_CHAINS_H
#define MANTID_VATES_FACTORY_CHAINS_H

#include "MantidKernel/System.h"

#include "MantidVatesAPI/MDLoadingView.h"
#include "MantidVatesAPI/TimeToTimeStep.h"
#include "MantidVatesAPI/WorkspaceProvider.h"
#include "MantidVatesAPI/vtkMDHexFactory.h"
#include "MantidVatesAPI/vtkMDHistoHex4DFactory.h"

#include <vtkPVClipDataSet.h>

namespace Mantid {
namespace VATES {

// Forward Decalaration
class MDLoadingPresenter;

/// Creates a facotry chain for MDHisto workspaces
std::unique_ptr<vtkMDHistoHex4DFactory<TimeToTimeStep>> DLLExport
createFactoryChainForHistoWorkspace(VisualNormalization normalization,
                                    double time);

/// Creates a factory chain for MDEvent workspaces
std::unique_ptr<vtkMDHexFactory> DLLExport createFactoryChainForEventWorkspace(
    VisualNormalization normalization, double time);

/// Function to apply the Change-of-Basis-Matrix
void DLLExport applyCOBMatrixSettingsToVtkDataSet(
    MDLoadingPresenter *presenter, vtkDataSet *dataSet,
    std::unique_ptr<Mantid::VATES::WorkspaceProvider> workspaceProvider);

/// Function to get clipped data sets.
vtkSmartPointer<vtkPVClipDataSet>
    DLLExport getClippedDataSet(const vtkSmartPointer<vtkDataSet> &dataSet);

/// Create name with timestamp attached.
std::string DLLExport createTimeStampedName(const std::string &name);
} // namespace VATES
} // namespace Mantid

#endif
