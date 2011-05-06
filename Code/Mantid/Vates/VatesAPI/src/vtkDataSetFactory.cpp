#include "MantidVatesAPI/vtkDataSetFactory.h"

namespace Mantid
{
namespace VATES
{

vtkDataSetFactory::vtkDataSetFactory()
{
}

vtkDataSetFactory::~vtkDataSetFactory()
{
}

void vtkDataSetFactory::SetSuccessor(vtkDataSetFactory* pSuccessor)
{
  m_successor = vtkDataSetFactory::SuccessorType(pSuccessor);
}

bool vtkDataSetFactory::hasSuccessor() const
{
  return NULL != m_successor.get();
}

}
}