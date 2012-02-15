#ifndef FUNCTIONDOMAINTEST_H_
#define FUNCTIONDOMAINTEST_H_

#include "MantidAPI/FunctionDomain1D.h"

#include <cxxtest/TestSuite.h>
#include <iostream>

using namespace Mantid;
using namespace Mantid::API;

class FunctionDomainTest : public CxxTest::TestSuite
{
public:

  void testDomain1D()
  {
    std::vector<double> x(10);
    for(size_t i = 0; i < x.size(); ++i)
    {
      x[i] = 1.0 + 0.1 * i;
    }
    FunctionDomain1D domain(x);
    TS_ASSERT_EQUALS(domain.size(), x.size());
    for(size_t i = 0; i < x.size(); ++i)
    {
      TS_ASSERT_EQUALS(domain[i], x[i]);
      TS_ASSERT_EQUALS(domain[i], *domain.getPointerAt(i));
    }
  }

};

#endif /*FUNCTIONDOMAINTEST_H_*/
