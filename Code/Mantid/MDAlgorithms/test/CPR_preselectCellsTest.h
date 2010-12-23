#ifndef H_CP_REBINNING
#define H_CP_REBINNING
#include <cxxtest/TestSuite.h>
#include "MDDataObjects/MDWorkspace.h"
#include "MantidMDAlgorithms/DynamicCPRRebinning.h"
using namespace Mantid;
using namespace API;
using namespace Kernel;
using namespace MDDataObjects;
using namespace MDAlgorithms;
/*
class MockMDWorkspace:public MDWorkspace
{
public:
    MockMDWorkspace(unsigned int nDim=4,unsigned int nRecDim=3):MDWorkspace(nDim,nRecDim){};

};
class DynamicCPRRt: public DynamicCPRRebinning
{
public:
   DynamicCPRRt(const MDDataObjects::MDWorkspace_const_sptr &pSourceWS, 
                Geometry::MDGeometryDescription const * const pTargetDescr,
                const MDDataObjects::MDWorkspace_sptr  & TargetWS ):
   DynamicCPRRebinning(pSourceWS,pTargetDescr,TargetWS)
   {}

    virtual bool rebin_data_chunk(){return false;}
    virtual bool rebin_data_chunk_keep_pixels(){return false;}
};

class testCPcomponents :    public CxxTest::TestSuite
{
       std::auto_ptr<MockMDWorkspace> pOrigin;
       std::auto_ptr<DynamicCPRRt>    pRebin;
 public:
     void t__tDynamicCPRRebinningConstructor(){
     }
};
*/
#endif