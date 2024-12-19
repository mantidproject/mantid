// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataHandling/LoadDspacemap.h"
#include "MantidDataHandling/SaveDspacemap.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidKernel/Timer.h"
#include <Poco/File.h>
#include <cxxtest/TestSuite.h>

using namespace Mantid::DataHandling;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using Mantid::Geometry::Instrument_sptr;

class SaveDspacemapTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    SaveDspacemap alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  /**
   * @param pad :: padding parameter
   * @param expectedSize :: expected file size
   * @param removeFile :: delete the file
   * @return
   */
  std::string do_test(int pad, int expectedSize, bool removeFile) {
    // Name of the output workspace.
    std::string filename("./SaveDspacemapTest_Output.dat");

    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(1);
    OffsetsWorkspace_sptr offsetsWS(new OffsetsWorkspace(inst));
    offsetsWS->setValue(1, 0.10);
    offsetsWS->setValue(2, 0.20);
    offsetsWS->setValue(3, 0.30);

    SaveDspacemap alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", offsetsWS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DspacemapFile", filename));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PadDetID", pad));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());
    filename = alg.getPropertyValue("DspacemapFile");

    TS_ASSERT(Poco::File(filename).exists());

    if (Poco::File(filename).exists()) {
      // We can only check that the size is right, more detailed checks are
      // tricky due to weird format.
      TS_ASSERT_EQUALS(Poco::File(filename).getSize(), expectedSize);
      if (removeFile)
        Poco::File(filename).remove();
    }
    return filename;
  }

  void test_nopadding() { do_test(0, 9 * 8, true); }

  void test_padding() { do_test(1000, 1000 * 8, true); }

  void test_save_then_load() {
    std::string filename("./SaveDspacemapTest_Output.dat");

    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(1);
    OffsetsWorkspace_sptr offsetsWS(new OffsetsWorkspace(inst));
    offsetsWS->setValue(1, 0.10);
    offsetsWS->setValue(2, 0.20);
    offsetsWS->setValue(3, 0.30);

    SaveDspacemap alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", offsetsWS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DspacemapFile", filename));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());
    filename = alg.getPropertyValue("DspacemapFile");

    LoadDspacemap load;
    TS_ASSERT_THROWS_NOTHING(load.initialize())
    TS_ASSERT(load.isInitialized())
    TS_ASSERT_THROWS_NOTHING(load.setProperty("InputWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(offsetsWS)));
    TS_ASSERT_THROWS_NOTHING(load.setPropertyValue("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(load.setPropertyValue("FileType", "POWGEN"));
    TS_ASSERT_THROWS_NOTHING(load.setPropertyValue("OutputWorkspace", "dummy"));
    TS_ASSERT_THROWS_NOTHING(load.execute(););
    TS_ASSERT(load.isExecuted());

    OffsetsWorkspace_sptr out;
    out = AnalysisDataService::Instance().retrieveWS<OffsetsWorkspace>("dummy");
    TS_ASSERT(out);
    if (!out)
      return;
    TS_ASSERT_DELTA(out->getValue(1), 0.10, 1e-5);
    TS_ASSERT_DELTA(out->getValue(2), 0.20, 1e-5);
    TS_ASSERT_DELTA(out->getValue(3), 0.30, 1e-5);

    if (Poco::File(filename).exists())
      Poco::File(filename).remove();
  }
};
