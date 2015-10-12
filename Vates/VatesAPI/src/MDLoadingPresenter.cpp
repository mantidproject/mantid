#include "MantidVatesAPI/MDLoadingPresenter.h"

namespace Mantid
{
namespace VATES
{
/**
 * Set the default COB Matrix and boundaries. The default COB matrix uses the
 * canonical axis directions.
 * @param visualDataSet: the input data set
 */
void MDLoadingPresenter::setDefaultCOBandBoundaries(vtkDataSet* visualDataSet) {
  // Set an identity matrix
  vtkSmartPointer<vtkMatrix4x4> cobMatrix =
    vtkSmartPointer<vtkMatrix4x4>::New();
  cobMatrix->Identity();

  if (!vtkPVChangeOfBasisHelper::AddChangeOfBasisMatrixToFieldData(visualDataSet,
                                                                    cobMatrix)) {
    g_log.warning("The Change-of-Basis-Matrix could not be added to the field "
                  "data of the scaled data set.\n");
  }

  // Set the bounds
  double boundingBox[6];
  visualDataSet->GetBounds(boundingBox);
  if (!vtkPVChangeOfBasisHelper::AddBoundingBoxInBasis(visualDataSet,
                                                        boundingBox)) {
    g_log.warning("The bounding box could not be added to the field data of "
                  "the scaled data set.\n");
  }
}

/**
 * Make the visual data set non-orthogonal
 */
void MDLoadingPresenter::makeNonOrthogonal(vtkDataSet* visualDataSet)
{
  std::string wsName = vtkDataSetToWsName::exec(visualDataSet);
  vtkDataSetToNonOrthogonalDataSet converter(visualDataSet, wsName);
  converter.execute();
}
}
}