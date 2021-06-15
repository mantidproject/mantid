// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAlgorithms/ConvertDiffCal.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
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
                int mask,
                double diffc,
                double diffa = 0,
                double tzero = 0 )
      :
      detector_id(detector_id),
      workspace_type(workspace_type),
      mask(mask),
      diffc(diffc),
      diffa(diffa),
      tzero(tzero)
    {}

    int detector_id;
    int workspace_type;
    int mask;
    double diffc;
    double diffa;
    double tzero;
  };

  /* contains a fake offset workspace and accompanying fake previous calibration workspace */
  class fake_workspaces
  {
    public:

    fake_workspaces( OffsetsWorkspace_sptr offsets, ITableWorkspace_sptr calibration_table );

    OffsetsWorkspace_sptr offsets;

    ITableWorkspace_sptr calibration_table; 
  };

  fake_workspaces generate_test_data( std::vector< class fake_entry > const &entries )
  {
    OffsetsWorkspace_sptr offsets;

    ITableWorkspace_sptr calibration_table; 

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

    /*
     * test implementation guide:
     * 2 entries in the table that are not in the offset workspace - propagate exactly
     * 2 entries in the offset workspace that are not in the table
     * 3 entries in the offset table that are nonzero and non-masked to update.
     * 1 entry that is zero but not masked
     * 1 entry that is masked but not zero
     * */

    auto instr = ComponentCreationHelper::createMinimalInstrument(V3D(0., 0., -10.), // source
                                                                  V3D(0., 0., 0.),   // sample
                                                                  V3D(1., 0., 0.));  // detector

    Mantid::Geometry::Detector *det = 
    new Mantid::Geometry::Detector("point-detector", 420 /*detector id*/, nullptr);
    instr->add(det);
    instr->markAsDetector(det);

    /* create an offset workspace based on the instrument */
    OffsetsWorkspace_sptr offsets = std::make_shared<OffsetsWorkspace>(instr);

    /* Extract the detector ids */
    std::cout << "detector ids:" << std::endl;
    Mantid::Geometry::DetectorInfo &d_info = offsets->mutableDetectorInfo();
    std::vector< int > const &detector_ids = d_info.detectorIDs();
    for( auto id : detector_ids )
    {
      /* set them to be masked */
      size_t internal_index = d_info.indexOf( id );
      std::cout << id << " at " << internal_index << std::endl;

      /* check if masked */
      std::cout << "before, d_info.isMasked(" << internal_index << "): "
                << d_info.isMasked(internal_index) << std::endl;
      /* mask it */
      d_info.setMasked( internal_index, true );
      std::cout << "after, d_info.isMasked(" << internal_index << "): "
                << d_info.isMasked(internal_index) << std::endl;
      /* set the value to something different */
      double value = 27;  
      offsets->setValue( id, value );
      std::cout << "detector id " << id << "'s value: " << offsets->getValue( id )
                << std::endl;
    }

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
