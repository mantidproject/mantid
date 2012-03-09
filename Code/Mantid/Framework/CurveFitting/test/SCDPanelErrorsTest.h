/*
 * SCDPanelErrorsTest.h
 *
 *  Created on: Feb 28, 2012
 *      Author: ruth
 */

#ifndef SCDCALIBRATEPANELTEST_H_
#define SCDCALIBRATEPANELTEST_H_


#include <cxxtest/TestSuite.h>

//#include "MantidCurveFitting/SCDPanelErrors.h"
//#include "MantidCrystal/LoadIsawPeaks.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/IPeak.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "../../Crystal/inc/MantidCrystal/LoadIsawPeaks.h"
#include "../inc/MantidCurveFitting/SCDPanelErrors.h"
#include "../../Geometry/inc/MantidGeometry/Instrument.h"

using namespace  Mantid;
using  namespace API;
using namespace DataObjects;
using namespace Geometry;
using namespace Crystal;
using namespace CurveFitting;



class SCDPanelErrorsTest : public CxxTest::TestSuite
{

public:

  void test_data()
  {
    LoadIsawPeaks alg;
    alg.initialize() ;
    alg.setPropertyValue("Filename", "TOPAZ_3007.peaks");
    alg.setPropertyValue("OutputWorkspace", "TOPAZ_3007");

    alg.execute();

    PeaksWorkspace_sptr Peakws =boost::dynamic_pointer_cast<PeaksWorkspace>(
        AnalysisDataService::Instance().retrieve("TOPAZ_3007") );
    AnalysisDataService::Instance().remove("TOPAZ_3007");
    std::string ComponentName("bank26");
    CurveFitting::SCDPanelErrors calib(Peakws, ComponentName,14.0,19.3,8.6,90.,105.,90.,.12);
    std::vector<std::string> banks;
    banks.push_back(std::string("bank26"));
    DataObjects::Workspace2D_sptr ws=SCDPanelErrors::calcWorkspace( Peakws,banks,.12);
    calib.setWorkspace(ws );

    const int N =(int)ws->dataX(0).size();

    double out[N];
    double xVals[N];
    MantidVec xdata =ws->dataX(0);
    for( size_t i=0; i< xdata.size();i++)
        xVals[i]= xdata[i];

    IPeak & peak0 =Peakws->getPeak(0);
    calib.setParameter("l0", peak0.getL1());

    Instrument_const_sptr instr =peak0.getInstrument();
    if( !instr)
      std::cout<<"No Instrument"<<std::endl;
    IComponent_const_sptr bank =   instr->getComponentByName("bank26");

    if( !bank)
      std::cout<<"No Bank 26"<<std::endl;

    boost::shared_ptr<const RectangularDetector> det = boost::dynamic_pointer_cast<const RectangularDetector>(bank);
    if( !det)
      std::cout<<"Cannot convert to rectangular detector"<<std::endl;

     calib.setParameter("detWidth", 1.5*det->xsize());

     calib.setParameter("detHeight", det->ysize());
    //calib.setParameter("Xoffset",1.0);
    //calib.setParameter("Yrot",90);
     std::cout<<"C"<<std::endl;
     calib.functionMW( out,xVals, (size_t)N);
     std::cout<<"D"<<std::endl;

  };



};

#endif /* SCDCALIBRATEPANELTEST_H_ */
