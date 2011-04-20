#include "MantidVatesAPI/vtkProxyFactory.h"

namespace Mantid
{
namespace VATES
{

vtkProxyFactory::vtkProxyFactory(vtkDataSet* product) : m_product(product)
{
}

vtkProxyFactory::~vtkProxyFactory()
{
}

vtkProxyFactory::vtkProxyFactory(const vtkProxyFactory& other) : m_product(other.m_product)
{
}

vtkProxyFactory& vtkProxyFactory::operator=(const vtkProxyFactory& other)
{
  if(&other != this)
  {
    //this->m_product->Delete();
    this->m_product = other.m_product;
  }
  return *this;
}

void vtkProxyFactory::initialize(Mantid::API::IMDWorkspace_sptr workspace)
{
  throw std::runtime_error("initialize with a workspace does not apply for this type of factory.");
}

vtkDataSet* vtkProxyFactory::create() const
{
  //Essentially do nothing other than return the cached product.
  return this->m_product;
}

vtkDataSet* vtkProxyFactory::createMeshOnly() const
{
  throw std::runtime_error("::createMeshOnly() does not apply for this type of factory.");
}

vtkFloatArray* vtkProxyFactory::createScalarArray() const
{
  throw std::runtime_error("::createScalarArray() does not apply for this type of factory.");
}


}
}
