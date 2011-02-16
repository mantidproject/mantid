#ifndef H_CP_REBINNING
#define H_CP_REBINNING
#include <cxxtest/TestSuite.h>
#include "MDDataObjects/MDWorkspace.h"
#include "MantidMDAlgorithms/CenterpieceRebinning.h"
using namespace Mantid;
using namespace API;
using namespace Kernel;
using namespace MDDataObjects;
using namespace MDAlgorithms;

class MockMDWorkspace:public MDWorkspace
{
public:
    MockMDWorkspace(unsigned int nDim=4,unsigned int nRecDim=3):MDWorkspace(nDim,nRecDim){};

};

class testCPComponents :    public CxxTest::TestSuite
{
       std::auto_ptr<MockMDWorkspace> pOrigin;
 public:
     testPreselectCells(){
     }
};
#endif
