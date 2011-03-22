#include "IFunctionWrapper.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/CompositeFunctionMW.h"
#include "MantidAPI/IPeakFunction.h"

void IFunctionWrapper::setFunction(const QString& name)
{
  try
  {
    m_function = dynamic_cast<Mantid::API::CompositeFunctionMW*>(Mantid::API::FunctionFactory::Instance().createFunction(name.toStdString()));
    m_compositeFunction = dynamic_cast<Mantid::API::CompositeFunctionMW*>(m_function);
    m_peakFunction = dynamic_cast<Mantid::API::IPeakFunction*>(m_function);
  }
  catch(...)
  {
    m_function = NULL;
    m_compositeFunction = NULL;
    m_peakFunction = NULL;
  }
}

void IFunctionWrapper::setFunction(Mantid::API::IFitFunction* function)
{
  m_function = function;
  m_compositeFunction = dynamic_cast<Mantid::API::CompositeFunctionMW*>(m_function);
  m_peakFunction = dynamic_cast<Mantid::API::IPeakFunction*>(m_function);
}
