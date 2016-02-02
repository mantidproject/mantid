#ifndef MANTID_VATES_PRESENTER_UTILITIES_H
#define MANTID_VATES_PRESENTER_UTILITIES_H

#include "MantidVatesAPI/MDLoadingView.h"
#include "MantidVatesAPI/MDLoadingPresenter.h"
#include "MantidVatesAPI/ADSWorkspaceProvider.h"
#include "MantidVatesAPI/SingleWorkspaceProvider.h"
#include "MantidVatesAPI/ThresholdRange.h"
#include "MantidVatesAPI/TimeToTimeStep.h"
#include "MantidVatesAPI/Normalization.h"
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


/**
 * This templated function creates a vtk data set factory chain. Note that currently there is no type nor ordering checking.
 * This is for a factory chain which takes 5 factories.
 * @param threshold: The threshold range for the underlying data.
 * @param normalization: The type of (visual) data normalization.
 * @param time: the time slice at which the data is to be looked at (important for 4D data).
 */
template<template<typename TimeType> class FourDFactory, class ThreeDFactory, class TwoDFactory, class OneDFactory, class ZeroDFactory>
std::unique_ptr<FourDFactory<TimeToTimeStep>> createFactoryChain5Factories(ThresholdRange_scptr threshold, VisualNormalization normalization, double time) {
  auto factory =
    Mantid::Kernel::make_unique<FourDFactory<TimeToTimeStep>>(
      threshold, normalization, time);
  factory->setSuccessor(Mantid::Kernel::make_unique<ThreeDFactory>(
    threshold, normalization))
    .setSuccessor(Mantid::Kernel::make_unique<TwoDFactory>(
      threshold, normalization))
    .setSuccessor(Mantid::Kernel::make_unique<OneDFactory>(
      threshold, normalization))
    .setSuccessor(Mantid::Kernel::make_unique<ZeroDFactory>());
  return factory;
}

/**
 * This templated function creates a vtk data set factory chain. Note that currently there is no type nor ordering checking.
 * This is for a factory chain which takes 4 factories.
 * @param threshold: The threshold range for the underlying data.
 * @param normalization: The type of (visual) data normalization.
 * @param time: the time slice at which the data is to be looked at (important for 4D data).
 */
template<class ThreeDFactory, class TwoDFactory, class OneDFactory, class ZeroDFactory>
std::unique_ptr<ThreeDFactory> createFactoryChain4Factories(ThresholdRange_scptr threshold, VisualNormalization normalization, double time) {
  auto factory = Mantid::Kernel::make_unique<ThreeDFactory>(threshold, normalization);
  factory->setSuccessor(Mantid::Kernel::make_unique<TwoDFactory>(threshold, normalization))
    .setSuccessor(Mantid::Kernel::make_unique<OneDFactory>(threshold, normalization))
    .setSuccessor(Mantid::Kernel::make_unique<ZeroDFactory>());
    factory->setTime(time);
  return factory;
}


/// Function to apply the Change-of-Basis-Matrix
void applyCOBMatrixSettingsToVtkDataSet(Mantid::VATES::MDLoadingPresenter* presenter, vtkDataSet* dataSet);

/// Function to get clipped data sets.
vtkSmartPointer<vtkPVClipDataSet> getClippedDataSet(vtkSmartPointer<vtkDataSet> dataSet) ;

}
}

#endif
