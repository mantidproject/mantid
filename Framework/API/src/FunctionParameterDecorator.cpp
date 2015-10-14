#include "MantidAPI/FunctionParameterDecorator.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/ParameterReference.h"

namespace Mantid {
namespace API {

void FunctionParameterDecorator::setDecoratedFunction(
    const std::string &wrappedFunctionName) {
  IFunction_sptr fn =
      FunctionFactory::Instance().createFunction(wrappedFunctionName);

  beforeDecoratedFunctionSet(fn);

  setDecoratedFunctionPrivate(fn);
}

IFunction_sptr FunctionParameterDecorator::getDecoratedFunction() const {
  return m_wrappedFunction;
}

IFunction_sptr FunctionParameterDecorator::clone() const {
  FunctionParameterDecorator_sptr cloned =
      boost::dynamic_pointer_cast<FunctionParameterDecorator>(
          FunctionFactory::Instance().createFunction(name()));

  if (!cloned) {
    throw std::runtime_error(
        "Cloned function is not of type FunctionParameterDecorator, aborting.");
  }

  IFunction_sptr decoratedFn = getDecoratedFunction();

  if (decoratedFn) {
    cloned->setDecoratedFunctionPrivate(decoratedFn->clone());
  }

  return cloned;
}

void FunctionParameterDecorator::setWorkspace(
    boost::shared_ptr<const Workspace> ws) {
  throwIfNoFunctionSet();

  m_wrappedFunction->setWorkspace(ws);
}

void FunctionParameterDecorator::setMatrixWorkspace(
    boost::shared_ptr<const MatrixWorkspace> workspace, size_t wi,
    double startX, double endX) {
  throwIfNoFunctionSet();

  m_wrappedFunction->setMatrixWorkspace(workspace, wi, startX, endX);
}

void FunctionParameterDecorator::setParameter(size_t i, const double &value,
                                              bool explicitlySet) {
  throwIfNoFunctionSet();

  m_wrappedFunction->setParameter(i, value, explicitlySet);
}

void FunctionParameterDecorator::setParameterDescription(
    size_t i, const std::string &description) {
  throwIfNoFunctionSet();

  m_wrappedFunction->setParameterDescription(i, description);
}

double FunctionParameterDecorator::getParameter(size_t i) const {
  throwIfNoFunctionSet();

  return m_wrappedFunction->getParameter(i);
}

void FunctionParameterDecorator::setParameter(const std::string &name,
                                              const double &value,
                                              bool explicitlySet) {
  throwIfNoFunctionSet();

  m_wrappedFunction->setParameter(name, value, explicitlySet);
}

void FunctionParameterDecorator::setParameterDescription(
    const std::string &name, const std::string &description) {
  throwIfNoFunctionSet();

  m_wrappedFunction->setParameterDescription(name, description);
}

double FunctionParameterDecorator::activeParameter(size_t i) const {
  throwIfNoFunctionSet();

  return m_wrappedFunction->activeParameter(i);
}

void FunctionParameterDecorator::setActiveParameter(size_t i, double value) {
  throwIfNoFunctionSet();

  m_wrappedFunction->setActiveParameter(i, value);
}

double FunctionParameterDecorator::getParameter(const std::string &name) const {
  throwIfNoFunctionSet();

  return m_wrappedFunction->getParameter(name);
}

size_t FunctionParameterDecorator::nParams() const {
  if (!m_wrappedFunction) {
    return 0;
  }

  return m_wrappedFunction->nParams();
}

size_t
FunctionParameterDecorator::parameterIndex(const std::string &name) const {
  throwIfNoFunctionSet();

  return m_wrappedFunction->parameterIndex(name);
}

std::string FunctionParameterDecorator::parameterName(size_t i) const {
  throwIfNoFunctionSet();

  return m_wrappedFunction->parameterName(i);
}

std::string FunctionParameterDecorator::parameterDescription(size_t i) const {
  throwIfNoFunctionSet();

  return m_wrappedFunction->parameterDescription(i);
}

bool FunctionParameterDecorator::isExplicitlySet(size_t i) const {
  throwIfNoFunctionSet();

  return m_wrappedFunction->isExplicitlySet(i);
}

double FunctionParameterDecorator::getError(size_t i) const {
  throwIfNoFunctionSet();

  return m_wrappedFunction->getError(i);
}

void FunctionParameterDecorator::setError(size_t i, double err) {
  throwIfNoFunctionSet();

  return m_wrappedFunction->setError(i, err);
}

bool FunctionParameterDecorator::isFixed(size_t i) const {
  throwIfNoFunctionSet();

  return m_wrappedFunction->isFixed(i);
}

void FunctionParameterDecorator::fix(size_t i) {
  throwIfNoFunctionSet();

  m_wrappedFunction->fix(i);
}

void FunctionParameterDecorator::unfix(size_t i) {
  throwIfNoFunctionSet();

  m_wrappedFunction->unfix(i);
}

size_t FunctionParameterDecorator::getParameterIndex(
    const ParameterReference &ref) const {
  throwIfNoFunctionSet();

  if (boost::dynamic_pointer_cast<CompositeFunction>(m_wrappedFunction)) {
    return m_wrappedFunction->getParameterIndex(ref);
  }

  if (ref.getFunction() == this && ref.getIndex() < nParams()) {
    return ref.getIndex();
  }

  return nParams();
}

size_t FunctionParameterDecorator::nAttributes() const {
  if (!m_wrappedFunction) {
    return 0;
  }

  return m_wrappedFunction->nAttributes();
}

std::vector<std::string> FunctionParameterDecorator::getAttributeNames() const {
  throwIfNoFunctionSet();

  return m_wrappedFunction->getAttributeNames();
}

IFunction::Attribute
FunctionParameterDecorator::getAttribute(const std::string &attName) const {
  throwIfNoFunctionSet();

  return m_wrappedFunction->getAttribute(attName);
}

void FunctionParameterDecorator::setAttribute(
    const std::string &attName, const IFunction::Attribute &attValue) {
  throwIfNoFunctionSet();

  m_wrappedFunction->setAttribute(attName, attValue);
}

bool FunctionParameterDecorator::hasAttribute(
    const std::string &attName) const {
  throwIfNoFunctionSet();

  return m_wrappedFunction->hasAttribute(attName);
}

ParameterTie *FunctionParameterDecorator::tie(const std::string &parName,
                                              const std::string &expr,
                                              bool isDefault) {
  throwIfNoFunctionSet();

  return m_wrappedFunction->tie(parName, expr, isDefault);
}

void FunctionParameterDecorator::applyTies() {
  throwIfNoFunctionSet();

  m_wrappedFunction->applyTies();
}

void FunctionParameterDecorator::clearTies() {
  throwIfNoFunctionSet();

  m_wrappedFunction->clearTies();
}

void FunctionParameterDecorator::removeTie(const std::string &parName) {
  throwIfNoFunctionSet();

  m_wrappedFunction->removeTie(parName);
}

bool FunctionParameterDecorator::removeTie(size_t i) {
  throwIfNoFunctionSet();

  return m_wrappedFunction->removeTie(i);
}

ParameterTie *FunctionParameterDecorator::getTie(size_t i) const {
  throwIfNoFunctionSet();

  return m_wrappedFunction->getTie(i);
}

void FunctionParameterDecorator::addConstraint(IConstraint *ic) {
  throwIfNoFunctionSet();

  m_wrappedFunction->addConstraint(ic);
}

IConstraint *FunctionParameterDecorator::getConstraint(size_t i) const {
  throwIfNoFunctionSet();

  return m_wrappedFunction->getConstraint(i);
}

void FunctionParameterDecorator::removeConstraint(const std::string &parName) {
  throwIfNoFunctionSet();

  m_wrappedFunction->removeConstraint(parName);
}

void FunctionParameterDecorator::setUpForFit() {
  throwIfNoFunctionSet();

  m_wrappedFunction->setUpForFit();
}

/// Throws std::runtime_error when m_wrappedFunction is not set.
void FunctionParameterDecorator::throwIfNoFunctionSet() const {
  if (!m_wrappedFunction) {
    throw std::runtime_error("No wrapped function set, aborting.");
  }
}

/// Does nothing, function does not have parameters.
void FunctionParameterDecorator::declareParameter(
    const std::string &name, double initValue, const std::string &description) {
  UNUSED_ARG(name);
  UNUSED_ARG(initValue);
  UNUSED_ARG(description);
}

/// Forwads addTie-call to the decorated function.
void FunctionParameterDecorator::addTie(ParameterTie *tie) {
  throwIfNoFunctionSet();

  m_wrappedFunction->addTie(tie);
}

/**
 * @brief Function that is called before the decorated function is set
 *
 * This function is called before the decorated function is actually set, with
 * the function object in question as a parameter. The base implementation does
 * nothing. Re-implementations could for example check whether the function
 * has a certain type and throw an exception otherwise.
 *
 * @param fn :: Function that is going to be decorated.
 */
void FunctionParameterDecorator::beforeDecoratedFunctionSet(
    const IFunction_sptr &fn) {
  UNUSED_ARG(fn);
}

void FunctionParameterDecorator::setDecoratedFunctionPrivate(
    const IFunction_sptr &fn) {
  m_wrappedFunction = fn;
}

} // namespace API
} // namespace Mantid
