#ifndef MANTID_TESTHELPERS_FUNCTIONCREATIONHELPER_H_
#define MANTID_TESTHELPERS_FUNCTIONCREATIONHELPER_H_

#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"

namespace Mantid {

namespace TestHelpers {

class FunctionChangesNParams : public Mantid::API::IFunction1D,
                               public Mantid::API::ParamFunction {
public:
  FunctionChangesNParams();
  std::string name() const override;
  void iterationStarting() override;
  void iterationFinished() override;

protected:
  void function1D(double *out, const double *xValues,
                  const size_t nData) const override;
  void functionDeriv1D(Mantid::API::Jacobian *out, const double *xValues,
                       const size_t nData) override;
  size_t m_maxNParams = 5;
  bool m_canChange = false;
};

} // namespace TestHelpers
} // namespace Mantid

#endif // MANTID_TESTHELPERS_FUNCTIONCREATIONHELPER_H_
