// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/SaveCalFile.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidKernel/Timer.h"
#include <Poco/File.h>
#include <cxxtest/TestSuite.h>
#include <fstream>
#include <iosfwd>

using namespace Mantid::DataHandling;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

class SaveCalFileTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    SaveCalFile alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    // --- Get an instrument -----
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(5);

    // --- Make up some data ----
    GroupingWorkspace_sptr groupWS(new GroupingWorkspace(inst));
    OffsetsWorkspace_sptr offsetsWS(new OffsetsWorkspace(inst));
    MaskWorkspace_sptr maskWS(new MaskWorkspace(inst));
    groupWS->setValue(1, 12);
    groupWS->setValue(2, 23);
    groupWS->setValue(3, 45);
    offsetsWS->setValue(1, 0.123);
    offsetsWS->setValue(2, 0.456);
    maskWS->getSpectrum(0).clearData();
    maskWS->mutableSpectrumInfo().setMasked(0, true);

    SaveCalFile alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupingWorkspace", groupWS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OffsetsWorkspace", offsetsWS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaskWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(maskWS)));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "SaveCalFileTest.cal"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    std::string filename = alg.getPropertyValue("Filename");
    TS_ASSERT(Poco::File(filename).exists());

    std::ifstream grFile(filename.c_str());
    std::string str;
    getline(grFile, str);
    getline(grFile, str);
    getline(grFile, str);
    TS_ASSERT_EQUALS(str, "        0              1      0.1230000       0      12");
    getline(grFile, str);
    TS_ASSERT_EQUALS(str, "        1              2      0.4560000       1      23");
    getline(grFile, str);
    TS_ASSERT_EQUALS(str, "        2              3      0.0000000       1      45");

    grFile.close();
    if (Poco::File(filename).exists())
      Poco::File(filename).remove();
  }
};
