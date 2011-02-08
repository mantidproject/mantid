#include "MantidVisitPresenters/vtkProxyFactory.h"

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

vtkDataSet* vtkProxyFactory::create() const
{
  //Essentially do nothing other than return the cached product.
  return this->m_product;
}

}
}
