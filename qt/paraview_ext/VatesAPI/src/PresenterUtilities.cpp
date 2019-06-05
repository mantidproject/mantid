// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidVatesAPI/FactoryChains.h"
#include "MantidVatesAPI/PresenterFactories.h"

#include "MantidVatesAPI/MDLoadingPresenter.h"
#include "MantidVatesAPI/vtkMD0DFactory.h"
#include "MantidVatesAPI/vtkMDHistoHexFactory.h"
#include "MantidVatesAPI/vtkMDHistoLineFactory.h"
#include "MantidVatesAPI/vtkMDHistoQuadFactory.h"
#include "MantidVatesAPI/vtkMDLineFactory.h"
#include "MantidVatesAPI/vtkMDQuadFactory.h"

#include "MantidKernel/Logger.h"

#include <vtkBox.h>

#include <algorithm>
#include <chrono>
#include <ctime>

namespace {
/// Static logger
Mantid::Kernel::Logger g_log_presenter_utilities("PresenterUtilities");
} // namespace

namespace Mantid {
namespace VATES {

/**
 * Gets a clipped object
 * @param dataSet: the unclipped data set
 * @returns a clipped object
 */
vtkSmartPointer<vtkPVClipDataSet>
getClippedDataSet(const vtkSmartPointer<vtkDataSet> &dataSet) {
  auto box = vtkSmartPointer<vtkBox>::New();
  box->SetBounds(dataSet->GetBounds());
  auto clipper = vtkSmartPointer<vtkPVClipDataSet>::New();
  clipper->SetInputData(dataSet);
  clipper->SetClipFunction(box);
  clipper->SetInsideOut(true);
  clipper->Update();
  return clipper;
}

/**
 * Applies the correct change of basis matrix to the vtk data set. This is
 * especially important for
 * non-orthogonal data sets.
 * @param presenter: a pointer to a presenter instance
 * @param dataSet: the data set which holds the COB information
 * @param workspaceProvider: provides one or multiple workspaces
 */
void applyCOBMatrixSettingsToVtkDataSet(
    Mantid::VATES::MDLoadingPresenter *presenter, vtkDataSet *dataSet,
    std::unique_ptr<Mantid::VATES::WorkspaceProvider> workspaceProvider) {
  try {
    presenter->makeNonOrthogonal(dataSet, std::move(workspaceProvider),
                                 nullptr);
  } catch (std::invalid_argument &e) {
    std::string error = e.what();
    g_log_presenter_utilities.warning()
        << "PresenterUtilities: Workspace does not have correct "
           "information to "
        << "plot non-orthogonal axes: " << error;
    // Add the standard change of basis matrix and set the boundaries
    presenter->setDefaultCOBandBoundaries(dataSet);
  } catch (...) {
    g_log_presenter_utilities.warning()
        << "PresenterUtilities: Workspace does not have correct "
           "information to "
        << "plot non-orthogonal axes. Non-orthogonal axes features require "
           "three dimensions.";
  }
}

/**
 * Creates a factory chain for MDEvent workspaces
 * @param normalization: the normalization option
 * @param time: the time slice time
 * @returns a factory chain
 */
std::unique_ptr<vtkMDHexFactory>
createFactoryChainForEventWorkspace(VisualNormalization normalization,
                                    double time) {
  auto factory = std::make_unique<vtkMDHexFactory>(normalization);
  factory->setSuccessor(std::make_unique<vtkMDQuadFactory>(normalization))
      .setSuccessor(std::make_unique<vtkMDLineFactory>(normalization))
      .setSuccessor(std::make_unique<vtkMD0DFactory>());
  factory->setTime(time);
  return factory;
}

/**
 * Creates a factory chain for MDHisto workspaces
 * @param normalization: the normalization option
 * @param time: the time slice time
 * @returns a factory chain
 */
std::unique_ptr<vtkMDHistoHex4DFactory<TimeToTimeStep>>
createFactoryChainForHistoWorkspace(VisualNormalization normalization,
                                    double time) {
  auto factory = std::make_unique<vtkMDHistoHex4DFactory<TimeToTimeStep>>(
      normalization, time);
  factory->setSuccessor(std::make_unique<vtkMDHistoHexFactory>(normalization))
      .setSuccessor(std::make_unique<vtkMDHistoQuadFactory>(normalization))
      .setSuccessor(std::make_unique<vtkMDHistoLineFactory>(normalization))
      .setSuccessor(std::make_unique<vtkMD0DFactory>());
  return factory;
}

/**
 * Creates a time stamped name
 * @param name: the input name
 * @return a name with a time stamp
 */
std::string createTimeStampedName(const std::string &name) {
  auto currentTime =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::string timeInReadableFormat = std::string(std::ctime(&currentTime));
  // Replace all white space with double underscore
  std::replace(timeInReadableFormat.begin(), timeInReadableFormat.end(), ' ',
               '_');
  // Replace all colons with single underscore
  std::replace(timeInReadableFormat.begin(), timeInReadableFormat.end(), ':',
               '_');
  timeInReadableFormat.erase(std::remove(timeInReadableFormat.begin(),
                                         timeInReadableFormat.end(), '\n'),
                             timeInReadableFormat.end());
  std::string stampedName = name + "_";
  stampedName = stampedName + timeInReadableFormat;
  return stampedName;
}
} // namespace VATES
} // namespace Mantid
