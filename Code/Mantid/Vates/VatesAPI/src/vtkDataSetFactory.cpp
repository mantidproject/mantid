#include "MantidVatesAPI/vtkDataSetFactory.h"
#include <stdexcept>

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

/**
 Set the successor factory for the chain-of-responsibility.
 @param pSuccessor :: pointer to the successor. Note RAII is used.
 @return true if addition was successful.
 */
void vtkDataSetFactory::SetSuccessor(vtkDataSetFactory* pSuccessor)
{ 
  //Assigment peformed first (RAII) to guarentee no side effects.
  m_successor = vtkDataSetFactory::SuccessorType(pSuccessor);
  //Unless overriden, successors should not be the same type as the present instance.
  if(pSuccessor->getFactoryTypeName() == this->getFactoryTypeName())
  {
    throw std::runtime_error("Cannot assign a successor to vtkDataSetFactory with the same type as the present vtkDataSetFactory type.");
  }
}

/**
 Determine when a successor is available.
 @return true if a successor is available.
 */
bool vtkDataSetFactory::hasSuccessor() const
{
  return NULL != m_successor.get();
}

}
}
