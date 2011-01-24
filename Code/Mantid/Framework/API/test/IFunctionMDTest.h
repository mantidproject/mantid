#ifndef IFUNCTIONMDTEST_H_
#define IFUNCTIONMDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/IFunctionMD.h"

#include <iostream>

using namespace Mantid;
using namespace Mantid::API;
using namespace WorkspaceCreationHelper;

class IFunctionMDTest : public CxxTest::TestSuite
{
public:

  void testIFunction()
  {
    int i = 1;
    TS_ASSERT(i);

    const int nx = 10;
    const int ny = 10;
    Mantid::DataObjects::Workspace2D_sptr ws = Create2DWorkspace(nx,ny);

    for(int i=0;i<nx;++i)
    {
      for(int j=0;j<ny;++j)
      {
      }
    }

    std::cerr<<"\nn="<<ws->axes()<<"\n";
  }

};

#endif /*IFUNCTIONMDTEST_H_*/
