// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_VATES_MD_LOADING_PRESENTER
#define MANTID_VATES_MD_LOADING_PRESENTER

#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/System.h"
#include "MantidVatesAPI/vtkDataSetToNonOrthogonalDataSet.h"
#include "MantidVatesAPI/vtkDataSetToWsName.h"
#include <string>
#include <vector>
#include <vtkDataSet.h>
#include <vtkPVChangeOfBasisHelper.h>

class vtkUnstructuredGrid;
namespace Mantid {
namespace VATES {
class ProgressAction;
class vtkDataSetFactory;
/**
@class MDLoadingPresenter
Abstract presenters for loading conversion of MDEW workspaces into render-able
vtk objects.
@author Owen Arnold, Tessella plc
@date 05/08/2011
*/
class DLLExport MDLoadingPresenter {
public:
  virtual vtkSmartPointer<vtkDataSet>
  execute(vtkDataSetFactory *factory, ProgressAction &rebinningProgressUpdate,
          ProgressAction &drawingProgressUpdate) = 0;
  virtual void executeLoadMetadata() = 0;
  virtual bool hasTDimensionAvailable() const = 0;
  virtual std::vector<double> getTimeStepValues() const = 0;
  virtual std::string getTimeStepLabel() const = 0;
  virtual void setAxisLabels(vtkDataSet *visualDataSet) = 0;
  virtual void setDefaultCOBandBoundaries(vtkDataSet *visualDataSet);
  virtual void makeNonOrthogonal(
      vtkDataSet *visualDataSet,
      std::unique_ptr<Mantid::VATES::WorkspaceProvider> workspaceProvider,
      ProgressAction *progress);
  virtual bool canReadFile() const = 0;
  virtual const std::string &getGeometryXML() const = 0;
  virtual ~MDLoadingPresenter() {}
  virtual std::string getWorkspaceTypeName() { return "NotSet"; }
  virtual int getSpecialCoordinates() { return Kernel::None; }
  /**
   * Gets the instrument associated with the dataset.
   * @returns The instrument associated with the dataset.
   */
  virtual const std::string &getInstrument() = 0;
};
} // namespace VATES
} // namespace Mantid

#endif
