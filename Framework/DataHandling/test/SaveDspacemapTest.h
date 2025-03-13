// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
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
};
