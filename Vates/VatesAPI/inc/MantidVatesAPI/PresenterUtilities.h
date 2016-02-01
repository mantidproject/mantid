#ifndef MANTID_VATES_PRESENTER_UTILITIES_H
#define MANTID_VATES_PRESENTER_UTILITIES_H

#include "MantidVatesAPI/MDLoadingView.h"
#include "MantidVatesAPI/MDLoadingPresenter.h"
#include "MantidVatesAPI/ADSWorkspaceProvider.h"
#include "MantidVatesAPI/ThresholdRange.h"
#include "MantidVatesAPI/TimeToTimeStep.h"
#include "MantidKernel/make_unique.h"

namespace Mantid {
namespace VATES {

/**
 * This templated function sets up an in memory loading presenter.
 * @param view: the loading view type
 * @param wsName: the name of the workspace which is to be displayed
 * @returns a new in memory loading presenter.
 */
template<class Presenter, class WorkspaceType>
std::unique_ptr<Presenter> createInMemoryPresenter(std::unique_ptr<MDLoadingView> view,
  std::string wsName) {
  return Mantid::Kernel::make_unique<Presenter>(std::move(view), new ADSWorkspaceProvider<WorkspaceType>, wsName);
}

/**
 * This templated function creates a vtk data set factory chain. Note that currently there is no type nor ordering checking.
 * @param threshold: The threshold range for the underlying data.
 * @param normalization: The type of (visual) data normalization.
 * @param time: the time slice at which the data is to be looked at (important for 4D data).
 */
template<template<typename TimeType> class FourDFactory, class ThreeDFactory, class TwoDFactory, class OneDFactory, class ZeroDFactory>
std::unique_ptr<FourDFactory<TimeToTimeStep>> createFactoryChain(ThresholdRange_scptr threshold, VisualNormalization normalization, double time) {
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
 * Applies the correct change of basis matrix to the vtk data set. This is especially important for
 * non-orthogonal data sets.
 * @param presenter: a pointer to a presenter instance
 * @param dataSet: the data set which holds the COB information
 */
void applyCOBMatrixSettingsToVtkDataSet(Mantid::VATES::MDLoadingPresenter* presenter, vtkDataSet* dataSet) {
  try
  {
    presenter->makeNonOrthogonal(dataSet);
  }
  catch (std::invalid_argument &e)
  {
    std::string error = e.what();
    vtkDebugMacro(<< "Workspace does not have correct information to "
      << "plot non-orthogonal axes. " << error);
    // Add the standard change of basis matrix and set the boundaries
    presenter->setDefaultCOBandBoundaries(dataSet);
  }
}

}
}

#endif
