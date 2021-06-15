// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include <list>

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAlgorithms/ConvertDiffCal.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

using Mantid::Algorithms::ConvertDiffCal;
using namespace Mantid::API;
using Mantid::DataObjects::OffsetsWorkspace;
using Mantid::DataObjects::OffsetsWorkspace_sptr;
using Mantid::Kernel::V3D;

class ConvertDiffCalTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertDiffCalTest *createSuite() { return new ConvertDiffCalTest(); }
  static void destroySuite(ConvertDiffCalTest *suite) { delete suite; }

  void test_Init() {
    ConvertDiffCal alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  /* specify fake entry in offset workspace or calibration table workspace */
  class fake_entry
  {
    public:

    enum
    {
      offset, /* fake entry specifies an entry in the fake input offsets workspace */
      masked, /* fake entry specifies masked offset workspace entry */
      unmasked,
      calibration, /* fake entry specifies an entry in the input calibration table */
    };

    fake_entry( int detector_id,
                int workspace_type,
                double difc,
                int mask = fake_entry::unmasked,
                double difa = 0,
                double tzero = 0 )
      :
      detector_id(detector_id),
      workspace_type(workspace_type),
      difc(difc),
      mask(mask),
      difa(difa),
      tzero(tzero)
    {}

    int detector_id;
    int workspace_type;
    double difc;
    int mask;
    double difa;
    double tzero;
  };

  /* contains a fake offset workspace and accompanying fake previous calibration workspace */
  class fake_workspaces
  {
    public:

    fake_workspaces( OffsetsWorkspace_sptr offsets, ITableWorkspace_sptr calibration_table )
      :
      offsets( offsets ),
      calibration_table( calibration_table )
    {}

    OffsetsWorkspace_sptr offsets;

    ITableWorkspace_sptr calibration_table; 
  };

  fake_workspaces generate_test_data( std::list< class fake_entry > const &entries )
  {
    Mantid::Geometry::Instrument_sptr instrument =
    std::make_shared<Mantid::Geometry::Instrument>();

    ITableWorkspace_sptr calibration_table =
    std::make_shared<Mantid::DataObjects::TableWorkspace>(); 
    calibration_table->addColumn("int", "detid");
    calibration_table->addColumn("double", "difc");
    calibration_table->addColumn("double", "difa");
    calibration_table->addColumn("double", "tzero");

    /* loop to add detectors and ids to instrument */
    for( auto const &entry : entries )
    {
      /* add an entry to the fake offset workspace by adding a detector there */
      if( entry.workspace_type == fake_entry::offset )
      {
        /* create a detector. Is this a memory leak? Idk. Depends what instrument does with it */
        Mantid::Geometry::Detector *det =
        new Mantid::Geometry::Detector("point-detector", entry.detector_id, nullptr);
        instrument->add(det);
        instrument->markAsDetector(det);
      }

      /* add an entry to the fake calibration workspace */
      else if( entry.workspace_type == fake_entry::calibration )
      {
        Mantid::API::TableRow new_row = calibration_table->appendRow();
        new_row << entry.detector_id << entry.difc << entry.difa << entry.tzero;
      }
    }

    /* create an offset workspace with the instrument */
    OffsetsWorkspace_sptr offsets = std::make_shared<OffsetsWorkspace>(instrument);
    Mantid::Geometry::DetectorInfo &d_info = offsets->mutableDetectorInfo();

    /* Loop to apply masks */
    for( auto const &entry : entries )
    {
      if( entry.workspace_type == fake_entry::offset )
      {
        size_t internal_index = d_info.indexOf( entry.detector_id );

        if( entry.mask == fake_entry::masked )
        {
          d_info.setMasked( internal_index, true );
        }

        else if( entry.mask == fake_entry::unmasked )
        {
          d_info.setMasked( internal_index, false );
        }

        offsets->setValue( entry.detector_id, entry.difc );
      }
    }

    return fake_workspaces( offsets, calibration_table );
  }

  void test_partial_update()
  {
    /*
     * Code implementation guide:
     * 0. Add new property: previous calibration. Add it to the documentation.
     * 1. Copy previous calibration workspace to output. How? is there a copy constructor?
     * 2. Iterate through input offset workspace. Check if the detector id is masked.
     *    check if calculated offset is zero. If non-masked an non-zero, recalculate as before.
     * 3. Update documentation
     * */

    /* specify contents of fake workspaces */
    std::list< class fake_entry > fake_entries =
    {
      /* 2 entries in the table that are not in the offset workspace - should be propagated */
      fake_entry( 5, fake_entry::calibration, 5 ),
      fake_entry( 6, fake_entry::calibration, 5 ),

      /* 2 entries in the offset workspace that are not in the table - should be updated */
      fake_entry( 0,  fake_entry::offset, 1.0, fake_entry::unmasked ),
      fake_entry( 1,  fake_entry::offset, 1.0, fake_entry::unmasked ),
      /* entry that is zero but not masked - should not be updated */
      fake_entry( 2, fake_entry::offset, 0, fake_entry::unmasked ),
      /* entry that is nonzero but masked - should not be updated */
      fake_entry( 3, fake_entry::offset, 3, fake_entry::masked ),
      /* 2 entries that exist in both. Existing values should be updated */
      fake_entry( 4, fake_entry::offset, 4, fake_entry::unmasked ),
      fake_entry( 7, fake_entry::offset, 7, fake_entry::unmasked ),

      /* 2 entries that exists in both - this one should be updated */
      fake_entry( 4, fake_entry::calibration, 4 ),
      fake_entry( 7, fake_entry::calibration, 7 ),
    };

    /* generate fake workspaces */
    class fake_workspaces fake_workspaces =
    generate_test_data( fake_entries );

    /* Print offsets workspace to make sure everything is there */
    Mantid::Geometry::DetectorInfo &d_info = fake_workspaces.offsets->mutableDetectorInfo();
    std::vector< int > const &detector_ids = d_info.detectorIDs();
    std::cout << "offsets workspace contents:" << std::endl;
    for( auto id : detector_ids )
    {
      size_t internal_index = d_info.indexOf( id );

      std::cout << "id: " << id 
                << " masked: " << d_info.isMasked( internal_index )
                << " value: " << fake_workspaces.offsets->getValue( id ) << std::endl;
    }
    std::cout << std::endl;

    /* print calibration table to make sure everything is there */
    std::vector< int > column =
    fake_workspaces.calibration_table->getColumn( 0 )->numeric_fill<int>();
    std::cout << "calibration table:" << std::endl;
    for( auto i : column )
    {
      std::cout << "id: " << i << std::endl;
    }
    std::cout << std::endl;

    /* In ConvertDiffCal, you will need to create a mapping of detector_id to vector index:
     * this will allow you to easily see if a detector is in the calibration table column */
    /* what will be the time complexity of this? */
    /* Captain! Check the output and create TS_ASSERT statements where they belong */
    /* create asserts that expect correct outcomes */

    TS_ASSERT( false );
  }

  void test_exec() {
    // Create a fake offsets workspace
    auto instr = ComponentCreationHelper::createMinimalInstrument(V3D(0., 0., -10.), // source
                                                                  V3D(0., 0., 0.),   // sample
                                                                  V3D(1., 0., 0.));  // detector
    OffsetsWorkspace_sptr offsets = std::make_shared<OffsetsWorkspace>(instr);
    offsets->setValue(1, 0.); // wksp_index=0, detid=1

    // Name of the output workspace.
    std::string outWSName("ConvertDiffCalTest_OutputWS");

    ConvertDiffCal alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OffsetsWorkspace", offsets));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service. TODO: Change to your desired
    // type
    Workspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<Workspace>(outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    // test various  values
    auto table = std::dynamic_pointer_cast<ITableWorkspace>(ws);
    TS_ASSERT(table);

    std::vector<std::string> columnNames = table->getColumnNames();
    TS_ASSERT_EQUALS(columnNames.size(), 4);
    TS_ASSERT_EQUALS(columnNames[0], "detid");
    TS_ASSERT_EQUALS(columnNames[1], "difc");

    auto detid = table->getColumn("detid");
    TS_ASSERT(detid);
    TS_ASSERT_EQUALS(detid->toDouble(0), 1.);

    auto difc = table->getColumn("difc");
    TS_ASSERT(difc);
    TS_ASSERT_DELTA(difc->toDouble(0), 3932.3, .1);

    auto difa = table->getColumn("difa");
    TS_ASSERT(difa);
    TS_ASSERT_EQUALS(difa->toDouble(0), 0.);

    auto tzero = table->getColumn("tzero");
    TS_ASSERT(tzero);
    TS_ASSERT_EQUALS(tzero->toDouble(0), 0.);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }
};
