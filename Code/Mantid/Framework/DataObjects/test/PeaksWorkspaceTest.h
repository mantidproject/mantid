#ifndef MANTID_DATAOBJECTS_PEAKSWORKSPACETEST_H_
#define MANTID_DATAOBJECTS_PEAKSWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FileProperty.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <stdio.h>
#include <cmath>
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/V3D.h"

using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Geometry;

class PeaksWorkspaceTest : public CxxTest::TestSuite
{
public:

  int removeFile( std::string outfile)
  {
     return remove( outfile.c_str());
  }

  bool sameFileContents( std::string file1, std::string file2)
  {
    std::ifstream in1( file1.c_str() );
    std::ifstream in2( file2.c_str() );

    bool done = false;
    char c1,c2;
    while( !done)
    {
      if( !in1.good() || !in2.good())
        done = true;
      else
      {
        c1= in1.get();
        c2= in2.get();
        if( c1 != c2)
          return false;
      }
    }
    if( in1.good() || in2.good())
      return false;
    return true;
    return true;
  }
  void test_Something()
  {
    std::vector<std::string>ext;
    ext.push_back(std::string("peaks"));
    Mantid::API::FileProperty fProp("Filename","",FileProperty::Load,ext, Mantid::Kernel::Direction::Input );
    fProp.setValue("Ni1172A.peaks");

    PeaksWorkspace pw;

    std::string infile(fProp.value());

    TS_ASSERT_THROWS_NOTHING( pw.append(infile));

    std::string outfile( infile);


    outfile.append("1");
    removeFile( outfile );
    TS_ASSERT_THROWS_NOTHING( pw.write(outfile ));

    TS_ASSERT( sameFileContents(infile,outfile));

    TS_ASSERT_EQUALS( removeFile( outfile ),0);

    //Check base data read in correctly
    V3D hkl=pw.get_hkl(6);
    V3D test(5,3,-3);
    V3D save(hkl);
    hkl -= test;
    TS_ASSERT_LESS_THAN(hkl.norm(),.00001);
    TS_ASSERT( save==pw.get_hkl(6));

    double pi = 3.1415926535897932384626433832795;
    V3D position =pw.getPosition(6);
    V3D ptest;
    ptest.spherical(.45647,1.3748*180/pi,2.52165*180/pi);
    V3D psave(position);

    position -=ptest;
    TS_ASSERT_LESS_THAN(position.norm(),.001);
    TS_ASSERT( psave==pw.getPosition(6));

    TS_ASSERT_LESS_THAN(abs( 187.25-pw.get_column(6)),.05 );
    TS_ASSERT_LESS_THAN(abs( 121.29-pw.get_row(6)),.05 );
    TS_ASSERT_LESS_THAN(abs( 283.13-pw.get_time_channel(6)),.05 );
    TS_ASSERT_LESS_THAN(abs( 17-pw.getPeakCellCount(6)),.05 );
    TS_ASSERT_LESS_THAN(abs( 4571.82-pw.getPeakIntegrationCount(6)),.05 );
    TS_ASSERT_LESS_THAN(abs( 88.13-pw.getPeakIntegrationError(6)),.01 );
    TS_ASSERT_LESS_THAN(abs(10 -pw.getReflag(6)),.001 );


    TS_ASSERT_EQUALS( 1172,pw.cell<int>(6,pw.IrunNumCol) );
    TS_ASSERT_EQUALS(3 , pw.get_Bank(6));
    TS_ASSERT_LESS_THAN(abs( 10000-pw.getMonitorCount(6)),.1 );

    TS_ASSERT_LESS_THAN( abs(18-pw.get_L1(6)),.0001);
    TS_ASSERT_LESS_THAN( abs(0-pw.get_time_offset(6)),.001);

    V3D sampOrient = pw.getSampleOrientation(6);
    V3D soSave(sampOrient);
    V3D soTest(164.96,45,0);
    soTest *=pi/180;
    sampOrient -=soTest;
    TS_ASSERT_LESS_THAN( sampOrient.norm(), .001);
    TS_ASSERT( soSave ==pw.getSampleOrientation(6) );

    TS_ASSERT_LESS_THAN( abs(.5203-pw.get_dspacing(6)),.001);

    TS_ASSERT_LESS_THAN( abs(.660962-pw.get_wavelength(6)),.001);

    TS_ASSERT_LESS_THAN( abs(1/.5203-pw.get_Qmagnitude(6)),.004);

    //unrot Q (ISAW)= 1.1155452(beam) , 0.55290407(back) , 1.4642019(up)
    //getQ  not adjusted for samp orient -1.220807(beam) , -1.2082262(back) , 0.8624681(up)

    V3D Qlab= pw.get_Qlab(6);
 //  McStas==back,up,beam
    V3D QlabTest(-1.2082262,.8624681,-1.220807);
    Qlab -=QlabTest;
    TS_ASSERT_LESS_THAN( Qlab.norm(), .001);

    V3D QlabR= pw.get_QXtal(6);
     //  McStas==back,up,beam
    V3D QlabRTest(.55290407,1.4642019,1.1155452);
     QlabR -=QlabRTest;
     TS_ASSERT_LESS_THAN( QlabR.norm(), .001);


     //Now test out the various sets. Not all sets are possible.
     V3D testV(3,5,-6);
     pw.sethkl( testV, 6);
     TS_ASSERT( pw.get_hkl(6)==testV);

     pw.setPeakCount(23,6);
     TS_ASSERT_EQUALS( pw.getPeakCellCount(6),23);

     pw.setPeakIntegrateCount( 235,6);
     TS_ASSERT_EQUALS( pw.getPeakIntegrationCount(6),235);

     //add set row, col ,chan, time

     pw.setPeakIntegrateError( 15,6);
     TS_ASSERT_EQUALS(pw.getPeakIntegrationError(6),15 );


     pw.setReflag(35,6);
     TS_ASSERT_EQUALS( pw.getReflag(6) , 35);

     V3D pos(12,3,-5);
     pw.setPeakPos( pos, 6);
     TS_ASSERT_EQUALS( pw.getPosition(6), pos);

     pw.setTime( 1280,6);
     TS_ASSERT_EQUALS( pw.getTime( 6), 1280 );

     pw.setRowColChan( 5,8,200,6);
     TS_ASSERT_EQUALS( pw.get_row(6) , 5);
     TS_ASSERT_EQUALS( pw.get_column(6) , 8);
     TS_ASSERT_EQUALS( pw.get_time_channel(6) , 200);


  }


};


#endif /* MANTID_DATAOBJECTS_PEAKSWORKSPACETEST_H_ */

