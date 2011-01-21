#ifndef H_CP_REBINNING
#define H_CP_REBINNING
#include <cxxtest/TestSuite.h>
#include "MDDataObjects/MDWorkspace.h"
#include "MantidMDAlgorithms/DynamicCPRRebinning.h"
#include "MDDataObjects/MDTestWorkspace.h"
using namespace Mantid;
using namespace API;
using namespace Kernel;
using namespace MDDataObjects;
using namespace MDAlgorithms;


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
    virtual unsigned int getNumDataChunks(void)const{
        unsigned int rez;

        rez = this->n_preselected_pix/1000;
        if(rez*1000!= this->n_preselected_pix)rez++;
        return rez;
    }
};

class testCPcomponents :    public CxxTest::TestSuite
{
    boost::shared_ptr<MDDataObjects::MDWorkspace> pOrigin;
    boost::shared_ptr<MDDataObjects::MDWorkspace> pTarget;

    std::auto_ptr<Geometry::MDGeometryDescription> pTargDescr;

    std::auto_ptr<DynamicCPRRt>    pRebin;
 public:
     void testDynamicCPRRebinningConstructor(){
         std::auto_ptr<MDTestWorkspace> tw = std::auto_ptr<MDTestWorkspace>(new MDTestWorkspace());
         pOrigin = tw->get_spWS();

         TSM_ASSERT_THROWS_NOTHING("Source WS should be constructed not throwing",pTarget= boost::shared_ptr<MDDataObjects::MDWorkspace>(new MDDataObjects::MDWorkspace(*pOrigin)));

         TSM_ASSERT_THROWS_NOTHING("Target WS descr should be constructed not throwing",pTargDescr = 
             std::auto_ptr<Geometry::MDGeometryDescription>(new Geometry::MDGeometryDescription(pOrigin->get_const_MDGeometry())));

         TSM_ASSERT_THROWS_NOTHING("Target empty WS  should be constructed not throwing",pTarget = 
             boost::shared_ptr<MDDataObjects::MDWorkspace>(new MDDataObjects::MDWorkspace()));


         TSM_ASSERT_THROWS_NOTHING("Rebinning should be constructed withoug throwing",pRebin  = 
             std::auto_ptr<DynamicCPRRt>(new DynamicCPRRt(pOrigin,pTargDescr.get(),pTarget)));

     }
};

#endif