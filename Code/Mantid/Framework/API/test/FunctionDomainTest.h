#ifndef FUNCTIONDOMAINTEST_H_
#define FUNCTIONDOMAINTEST_H_

#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"

#include <cxxtest/TestSuite.h>
#include <iostream>

using namespace Mantid;
using namespace Mantid::API;

class FunctionDomainTest : public CxxTest::TestSuite
{
public:

  static FunctionDomainTest *createSuite() { return new FunctionDomainTest(); }
  static void destroySuite( FunctionDomainTest *suite ) { delete suite; }

  FunctionDomainTest()
  {
    data.resize(10);
    for(size_t i = 0; i < data.size(); ++i)
    {
      data[i] = 1.0 + 0.1 * double(i);
    }
  }

  void testDomain1D()
  {
    FunctionDomain1DVector domain(data);
    checkDomainVector( domain );

    FunctionDomain1DVector domainCopy( domain );
    checkDomainVector( domainCopy );

    FunctionDomain1DVector domainCopy1( 1.0 );
    domainCopy1 = domain;
    checkDomainVector( domainCopy1 );
  }

  void testDomain1D_part()
  {
    FunctionDomain1DVector domain( data.begin() + 2, data.begin() + 8 );
    checkDomainVector( domain, 2, 8 );

    FunctionDomain1DVector domainCopy( domain );
    checkDomainVector( domainCopy, 2, 8 );

    FunctionDomain1DVector domainCopy1( 1.0 );
    domainCopy1 = domain;
    checkDomainVector( domainCopy1, 2, 8 );
  }

  void test_Domain1DView()
  {
    FunctionDomain1DView domain( data.data(), data.size() );
    checkDomainVector( domain );
  }

  void test_Domain1DSpectra()
  {
    FunctionDomain1DSpectrum domain( 12, data );
    checkDomainVector( domain );
    TS_ASSERT_EQUALS( domain.getWorkspaceIndex(), 12 );
  }

  void test_Domain1DSpectra_part()
  {
    FunctionDomain1DSpectrum domain( 14, data.begin() + 3, data.begin() + 7 );
    checkDomainVector( domain, 3, 7 );
    TS_ASSERT_EQUALS( domain.getWorkspaceIndex(), 14 );
  }

private:

  void checkDomainVector(const FunctionDomain1D& domain, size_t start = 0, size_t end = 0)
  {
    if ( end == 0 ) end = data.size();
    TS_ASSERT_EQUALS( domain.size(), end - start );
    for(size_t i = start; i < end; ++i)
    {
      TS_ASSERT_EQUALS(domain[i - start], data[i]);
      TS_ASSERT_EQUALS(domain[i - start], *domain.getPointerAt(i - start));
    }
  }

  std::vector<double> data;

};

#endif /*FUNCTIONDOMAINTEST_H_*/
