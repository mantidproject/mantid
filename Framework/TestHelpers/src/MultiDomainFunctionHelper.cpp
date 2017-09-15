#include "MantidTestHelpers/MultiDomainFunctionHelper.h"
#include "MantidTestHelpers/FakeObjects.h"

namespace Mantid {
namespace TestHelpers {

MultiDomainFunctionTest_Function::MultiDomainFunctionTest_Function()
    : Mantid::API::IFunction1D(), Mantid::API::ParamFunction() {
  this->declareParameter("A", 0);
  this->declareParameter("B", 0);
  this->declareAttribute("Order", Attribute(1));
}

void MultiDomainFunctionTest_Function::function1D(double *out,
                                                  const double *xValues,
                                                  const size_t nData) const {
  const double A = getParameter(0);
  const double B = getParameter(1);
  const int order = getAttribute("Order").asInt();

  for (size_t i = 0; i < nData; ++i) {
    double x = xValues[i];
    switch (order) {
    case 1:
      out[i] = A + B * x;
      break;
    case 3:
      out[i] = (A + B * x) * pow(x, 2);
      break;
    case 5:
      out[i] = (A + B * x) * pow(x, 4);
      break;
    default:
      throw std::runtime_error("Unknown attribute value.");
    };
  }
}
void MultiDomainFunctionTest_Function::functionDeriv1D(
    Mantid::API::Jacobian *out, const double *xValues, const size_t nData) {
  const int order = getAttribute("Order").asInt();
  for (size_t i = 0; i < nData; ++i) {
    double x = xValues[i];
    switch (order) {
    case 1:
      out->set(i, 0, 1.0);
      out->set(i, 1, x);
      break;
    case 3:
      out->set(i, 0, pow(x, 2));
      out->set(i, 1, pow(x, 3));
      break;
    case 5:
      out->set(i, 0, pow(x, 4));
      out->set(i, 1, pow(x, 5));
      break;
    default:
      throw std::runtime_error("Unknown attribute value.");
    };
  }
}

boost::shared_ptr<Mantid::API::MultiDomainFunction> makeMultiDomainFunction3() {
  auto multi = boost::make_shared<Mantid::API::MultiDomainFunction>();
  multi->addFunction(boost::make_shared<MultiDomainFunctionTest_Function>());
  multi->addFunction(boost::make_shared<MultiDomainFunctionTest_Function>());
  multi->addFunction(boost::make_shared<MultiDomainFunctionTest_Function>());

  multi->getFunction(0)->setParameter("A", 0);
  multi->getFunction(0)->setParameter("B", 0);

  multi->getFunction(1)->setParameter("A", 0);
  multi->getFunction(1)->setParameter("B", 0);

  multi->getFunction(2)->setParameter("A", 0);
  multi->getFunction(2)->setParameter("B", 0);

  multi->clearDomainIndices();
  multi->setDomainIndices(1, {0, 1});
  multi->setDomainIndices(2, {0, 2});

  return multi;
}

boost::shared_ptr<Mantid::API::JointDomain> makeMultiDomainDomain3() {
  auto domain = boost::make_shared<Mantid::API::JointDomain>();
  domain->addDomain(
      boost::make_shared<Mantid::API::FunctionDomain1DVector>(0, 1, 9));
  domain->addDomain(
      boost::make_shared<Mantid::API::FunctionDomain1DVector>(1, 2, 10));
  domain->addDomain(
      boost::make_shared<Mantid::API::FunctionDomain1DVector>(2, 3, 11));

  return domain;
}

const double A0 = 0.5, A1 = -4, A2 = 4;
const double B0 = 5, B1 = -20, B2 = 16;
const size_t nbins = 10;
const double dX = 0.2;

Mantid::API::MatrixWorkspace_sptr makeMultiDomainWorkspace1() {
  Mantid::API::MatrixWorkspace_sptr ws1 = boost::make_shared<WorkspaceTester>();
  ws1->initialize(1, nbins, nbins);
  auto &x = ws1->mutableX(0);
  auto &y = ws1->mutableY(0);
  for (size_t i = 0; i < ws1->blocksize(); ++i) {
    x[i] = -1.0 + dX * double(i);
    const double t = x[i];
    y[i] = A0 + B0 * t + (A1 + B1 * t) * pow(t, 2) + (A2 + B2 * t) * pow(t, 4);
  }

  return ws1;
}

Mantid::API::MatrixWorkspace_sptr makeMultiDomainWorkspace2() {
  Mantid::API::MatrixWorkspace_sptr ws2 = boost::make_shared<WorkspaceTester>();
  ws2->initialize(1, nbins, nbins);

  auto &x = ws2->mutableX(0);
  auto &y = ws2->mutableY(0);
  for (size_t i = 0; i < ws2->blocksize(); ++i) {
    x[i] = -1.0 + dX * double(i);
    const double t = x[i];
    y[i] = A0 + B0 * t + (A1 + B1 * t) * pow(t, 2);
  }

  return ws2;
}

Mantid::API::MatrixWorkspace_sptr makeMultiDomainWorkspace3() {
  Mantid::API::MatrixWorkspace_sptr ws3 = boost::make_shared<WorkspaceTester>();
  ws3->initialize(1, nbins, nbins);
  auto &x = ws3->mutableX(0);
  auto &y = ws3->mutableY(0);
  for (size_t i = 0; i < ws3->blocksize(); ++i) {
    x[i] = -1.0 + dX * double(i);
    const double t = x[i];
    y[i] = A0 + B0 * t + (A2 + B2 * t) * pow(t, 4);
  }

  return ws3;
}
} // namespace TestHelpers
} // namespace Mantid
