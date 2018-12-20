#include "IFunctionWrapper.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"

void IFunctionWrapper::setFunction(const QString &name) {
  try {
    m_function = boost::dynamic_pointer_cast<Mantid::API::CompositeFunction>(
        Mantid::API::FunctionFactory::Instance().createFunction(
            name.toStdString()));
    m_compositeFunction =
        boost::dynamic_pointer_cast<Mantid::API::CompositeFunction>(m_function);
    m_peakFunction =
        boost::dynamic_pointer_cast<Mantid::API::IPeakFunction>(m_function);
  } catch (...) {
    m_function.reset();
    m_compositeFunction.reset();
    m_peakFunction.reset();
  }
}

void IFunctionWrapper::setFunction(
    boost::shared_ptr<Mantid::API::IFunction> function) {
  m_function = function;
  m_compositeFunction =
      boost::dynamic_pointer_cast<Mantid::API::CompositeFunction>(m_function);
  m_peakFunction =
      boost::dynamic_pointer_cast<Mantid::API::IPeakFunction>(m_function);
}
