#include "MantidVatesAPI/PresenterUtilities.h"
#include <vtkPVClipDataSet.h>
#include <vtkBox.h>

namespace Mantid
{
namespace VATES
{

/**
 * Gets a clipped object
 * @param dataSet: the unclipped data set
 * @returns a clipped object
 */
vtkSmartPointer<vtkPVClipDataSet> getClippedDataSet(vtkSmartPointer<vtkDataSet> dataSet) {
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
    //vtkDebugMacro(<< "Workspace does not have correct information to "
    //  << "plot non-orthogonal axes. " << error);
    // Add the standard change of basis matrix and set the boundaries
    presenter->setDefaultCOBandBoundaries(dataSet);
  }
}

}
}
