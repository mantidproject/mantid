#ifndef MANTID_MDEVENTS_INTEGRATE_3D_EVENTS_TEST_H_
#define MANTID_MDEVENTS_INTEGRATE_3D_EVENTS_TEST_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/V3D.h"
#include "MantidDataObjects/PeakShapeEllipsoid.h"
#include "MantidMDEvents/Integrate3DEvents.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid;
using namespace Mantid::MDEvents;
using Mantid::Kernel::V3D;

//typedef Mantid::Kernel::Matrix<double>                  DblMatrix;

class Integrate3DEventsTest : public CxxTest::TestSuite
{
public:

  // Test support class for integration of events using ellipsoids aligned
  // with the principal axes of the events near a peak.  This test
  // generates some poorly distributed synthetic data, and checks that 
  // expected integration results are obtained using either fixed size
  // ellipsoids, or using ellipsoids with axis half-lengths set to 
  // three standard deviations.
  void test_1()
  { 
    double inti_all[] = { 755, 704, 603 };
    double sigi_all[] = { 27.4773, 26.533, 24.5561 };

    double inti_some[] = { 691, 648, 603 };
    double sigi_some[] = { 27.4773, 26.533, 24.5561 };

                                          // synthesize three peaks
    std::vector<std::pair<double, V3D> > peak_q_list;
    V3D peak_1( 10, 0, 0 );
    V3D peak_2(  0, 5, 0 );
    V3D peak_3(  0, 0, 4 );

    peak_q_list.push_back( std::make_pair(1., peak_1 ));
    peak_q_list.push_back( std::make_pair(1., peak_2 ));
    peak_q_list.push_back( std::make_pair(1., peak_3 ));
                                          // synthesize a UB-inverse to map
    DblMatrix UBinv(3,3,false);           // Q to h,k,l
    UBinv.setRow( 0, V3D( .1,  0,   0 ) );
    UBinv.setRow( 1, V3D(  0, .2,   0 ) );
    UBinv.setRow( 2, V3D(  0,  0, .25 ) );

                                          // synthesize events around the
                                          // peaks.  All events with in one
                                          // unit of the peak.  755 events
                                          // around peak 1, 704 events around
                                          // peak 2, and 603 events around
                                          // peak 3.
    std::vector<std::pair<double, V3D> > event_Qs;
    for ( int i = -100; i <= 100; i++ )
    {
      event_Qs.push_back( std::make_pair(1., V3D( peak_1 + V3D( (double)i/100.0, 0, 0 ) ) ) );
      event_Qs.push_back( std::make_pair(1., V3D( peak_2 + V3D( (double)i/100.0, 0, 0 ) ) ) );
      event_Qs.push_back( std::make_pair(1., V3D( peak_3 + V3D( (double)i/100.0, 0, 0 ) ) ) );

      event_Qs.push_back( std::make_pair(1., V3D( peak_1 + V3D( 0, (double)i/200.0, 0 ) ) ) );
      event_Qs.push_back( std::make_pair(1., V3D( peak_2 + V3D( 0, (double)i/200.0, 0 ) ) ) );
      event_Qs.push_back( std::make_pair(1., V3D( peak_3 + V3D( 0, (double)i/200.0, 0 ) ) ) );

      event_Qs.push_back( std::make_pair(1., V3D( peak_1 + V3D( 0, 0, (double)i/300.0 ) ) ) );
      event_Qs.push_back( std::make_pair(1., V3D( peak_2 + V3D( 0, 0, (double)i/300.0 ) ) ) );
      event_Qs.push_back( std::make_pair(1., V3D( peak_3 + V3D( 0, 0, (double)i/300.0 ) ) ) );
    }

    for ( int i = -50; i <= 50; i++ )
    {
      event_Qs.push_back(  std::make_pair(1., V3D( peak_1 + V3D( 0, (double)i/147.0, 0 ) ) ) );
      event_Qs.push_back( std::make_pair(1., V3D( peak_2 + V3D( 0, (double)i/147.0, 0 ) ) ) );
    }

    for ( int i = -25; i <= 25; i++ )
    {
      event_Qs.push_back( std::make_pair(1., V3D( peak_1 + V3D( 0, 0, (double)i/61.0 ) ) ) );
    }

    double radius = 1.3;
    Integrate3DEvents integrator( peak_q_list, UBinv, radius );

    integrator.addEvents( event_Qs );

                                    // With fixed size ellipsoids, all the
                                    // events are counted.
    bool   specify_size = true; 
    double peak_radius = 1.2;
    double back_inner_radius = 1.2;
    double back_outer_radius = 1.3;
    std::vector<double> new_sigma;
    double inti;
    double sigi;
    for ( size_t i = 0; i < peak_q_list.size(); i++ )
    {
      auto shape = integrator.ellipseIntegrateEvents( peak_q_list[i], specify_size,
                          peak_radius, back_inner_radius, back_outer_radius,
                          new_sigma, inti, sigi );
      TS_ASSERT_DELTA( inti, inti_all[i], 0.1);      
      TS_ASSERT_DELTA( sigi, sigi_all[i], 0.01);

      auto ellipsoid_shape = boost::dynamic_pointer_cast<const Mantid::DataObjects::PeakShapeEllipsoid>(shape);
      TSM_ASSERT("Expect to get back an ellipsoid shape", ellipsoid_shape);
    }

                                  // The test data is not normally distributed,
                                  // so with 3 sigma half-axis sizes, we miss
                                  // some counts
    specify_size = false;
    for ( size_t i = 0; i < peak_q_list.size(); i++ )
    {
      integrator.ellipseIntegrateEvents( peak_q_list[i], specify_size,
                          peak_radius, back_inner_radius, back_outer_radius, 
                          new_sigma, inti, sigi );
      TS_ASSERT_DELTA( inti, inti_some[i], 0.1);      
      TS_ASSERT_DELTA( sigi, sigi_some[i], 0.01);      
    }
  }


};


#endif /* MANTID_MDEVENTS_INTEGRATE_3D_EVENTS_TEST_H_ */

