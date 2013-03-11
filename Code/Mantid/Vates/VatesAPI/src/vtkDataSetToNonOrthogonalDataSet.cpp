#include "MantidVatesAPI/vtkDataSetToNonOrthogonalDataSet.h"

#include <vtkDataSet.h>

#include <stdexcept>

namespace Mantid
{
namespace VATES
{

void vtkDataSetToNonOrthogonalDataSet::exec(vtkDataSet *dataset)
{
  vtkDataSetToNonOrthogonalDataSet temp(dataset);
  temp.execute();
}

vtkDataSetToNonOrthogonalDataSet::vtkDataSetToNonOrthogonalDataSet(vtkDataSet *dataset) :
  m_dataSet(dataset)
{
  if (NULL == m_dataSet)
  {
    throw std::runtime_error("Cannot construct vtkDataSetToNonOrthogonalDataSet with null VTK dataset");
  }
}

/**
 * Class destructor
 */
vtkDataSetToNonOrthogonalDataSet::~vtkDataSetToNonOrthogonalDataSet()
{
}

void vtkDataSetToNonOrthogonalDataSet::execute()
{

}

} // namespace VATESI
} // namespace Mantid
