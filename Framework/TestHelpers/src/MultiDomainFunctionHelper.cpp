#include "MantidTestHelpers/MultiDomainFunctionHelper.h"
#include "MantidTestHelpers/FakeObjects.h"

namespace Mantid {
namespace TestHelpers {

MultiDomainFunctionTest_Function::MultiDomainFunctionTest_Function()
    : Mantid::API::IFunction1D(), Mantid::API::ParamFunction() {
  this->declareParameter("A", 0);
  this->declareParameter("B", 0);
}

void MultiDomainFunctionTest_Function::function1D(double *out,
                                                  const double *xValues,
                                                  const size_t nData) const {
  const double A = getParameter(0);
  const double B = getParameter(1);

  for (size_t i = 0; i < nData; ++i) {
    double x = xValues[i];
    out[i] = A + B * x;
  }
}
void MultiDomainFunctionTest_Function::functionDeriv1D(
    Mantid::API::Jacobian *out, const double *xValues, const size_t nData) {
  for (size_t i = 0; i < nData; ++i) {
    double x = xValues[i];
    out->set(i, 1, x);
    out->set(i, 0, 1.0);
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
  std::vector<size_t> ii(2);
  ii[0] = 0;
  ii[1] = 1;
  multi->setDomainIndices(1, ii);
  ii[0] = 0;
  ii[1] = 2;
  multi->setDomainIndices(2, ii);

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

const double A0 = 0, A1 = 1, A2 = 2;
const double B0 = 1, B1 = 2, B2 = 3;

Mantid::API::MatrixWorkspace_sptr makeMultiDomainWorkspace1() {
  Mantid::API::MatrixWorkspace_sptr ws1 = boost::make_shared<WorkspaceTester>();
  ws1->initialize(1, 10, 10);
  {
    Mantid::MantidVec &x = ws1->dataX(0);
    Mantid::MantidVec &y = ws1->dataY(0);
    // Mantid::MantidVec& e = ws1->dataE(0);
    for (size_t i = 0; i < ws1->blocksize(); ++i) {
      x[i] = 0.1 * double(i);
      y[i] = A0 + A1 + A2 + (B0 + B1 + B2) * x[i];
    }
  }

  return ws1;
}

Mantid::API::MatrixWorkspace_sptr makeMultiDomainWorkspace2() {
  Mantid::API::MatrixWorkspace_sptr ws2 = boost::make_shared<WorkspaceTester>();
  ws2->initialize(1, 10, 10);
  {
    Mantid::MantidVec &x = ws2->dataX(0);
    Mantid::MantidVec &y = ws2->dataY(0);
    // Mantid::MantidVec& e = ws2->dataE(0);
    for (size_t i = 0; i < ws2->blocksize(); ++i) {
      x[i] = 1 + 0.1 * double(i);
      y[i] = A0 + A1 + (B0 + B1) * x[i];
    }
  }

  return ws2;
}

Mantid::API::MatrixWorkspace_sptr makeMultiDomainWorkspace3() {
  Mantid::API::MatrixWorkspace_sptr ws3 = boost::make_shared<WorkspaceTester>();
  ws3->initialize(1, 10, 10);
  {
    Mantid::MantidVec &x = ws3->dataX(0);
    Mantid::MantidVec &y = ws3->dataY(0);
    // Mantid::MantidVec& e = ws3->dataE(0);
    for (size_t i = 0; i < ws3->blocksize(); ++i) {
      x[i] = 2 + 0.1 * double(i);
      y[i] = A0 + A2 + (B0 + B2) * x[i];
    }
  }

  return ws3;
}
} // namespace TestHelpers
} // namespace Mantid
