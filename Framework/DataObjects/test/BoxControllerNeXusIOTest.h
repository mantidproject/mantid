// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FileFinder.h"
#include "MantidDataObjects/BoxControllerNeXusIO.h"
#include "MantidFrameworkTestHelpers/MDEventsTestHelper.h"

#include <map>
#include <memory>

#include <cxxtest/TestSuite.h>

#include <nexus/NeXusFile.hpp>

#include <Poco/File.h>

class BoxControllerNeXusIOTest : public CxxTest::TestSuite {
public:
  static BoxControllerNeXusIOTest *createSuite() { return new BoxControllerNeXusIOTest(); }
  static void destroySuite(BoxControllerNeXusIOTest *suite) { delete suite; }

  Mantid::API::BoxController_sptr sc;
  std::string xxfFileName;

  BoxControllerNeXusIOTest() {
    sc = Mantid::API::BoxController_sptr(new Mantid::API::BoxController(4));
    xxfFileName = "BoxCntrlNexusIOxxfFile.nxs";
  }

  void setUp() override {
    std::string FullPathFile = Mantid::API::FileFinder::Instance().getFullPath(this->xxfFileName);
    if (!FullPathFile.empty())
      Poco::File(FullPathFile).remove();
  }

  void test_constructor_does_not_throw() { TS_ASSERT_THROWS_NOTHING(createTestBoxController()); }

  void test_constructor_setters() {
    using Mantid::DataObjects::BoxControllerNeXusIO;
    using EDV = Mantid::DataObjects::BoxControllerNeXusIO::EventDataVersion;
    auto pSaver = createTestBoxController();

    size_t CoordSize;
    std::string typeName;
    TS_ASSERT_THROWS_NOTHING(pSaver->getDataType(CoordSize, typeName));
    // default settings
    TS_ASSERT_EQUALS(4, CoordSize);
    TS_ASSERT_EQUALS("MDEvent", typeName);
    TS_ASSERT_EQUALS(EDV::EDVGoniometer, pSaver->getEventDataVersion());

    // set size
    TS_ASSERT_THROWS(pSaver->setDataType(9, typeName), const std::invalid_argument &);
    TS_ASSERT_THROWS_NOTHING(pSaver->setDataType(8, typeName));
    TS_ASSERT_THROWS_NOTHING(pSaver->getDataType(CoordSize, typeName));
    TS_ASSERT_EQUALS(8, CoordSize);
    TS_ASSERT_EQUALS("MDEvent", typeName);

    // set type
    TS_ASSERT_THROWS(pSaver->setDataType(4, "UnknownEvent"), const std::invalid_argument &);
    TS_ASSERT_THROWS_NOTHING(pSaver->setDataType(4, "MDLeanEvent"));
    TS_ASSERT_THROWS_NOTHING(pSaver->getDataType(CoordSize, typeName));
    TS_ASSERT_EQUALS(4, CoordSize);
    TS_ASSERT_EQUALS("MDLeanEvent", typeName);

    // MDLeanEvent and EDVOriginal are incompatible
    pSaver->setDataType(CoordSize, "MDLeanEvent");
    TS_ASSERT_THROWS(pSaver->setEventDataVersion(EDV::EDVOriginal), const std::invalid_argument &);

    pSaver->setDataType(CoordSize, "MDEvent");
    TS_ASSERT_THROWS_NOTHING(pSaver->setEventDataVersion(EDV::EDVOriginal));
    TS_ASSERT_EQUALS(EDV::EDVOriginal, pSaver->getEventDataVersion());
  }

  void test_eventDataVersion() {
    // initialization
    using Mantid::DataObjects::BoxControllerNeXusIO;
    using EDV = Mantid::DataObjects::BoxControllerNeXusIO::EventDataVersion;
    auto pSaver = createTestBoxController();

    // valid values
    std::map<EDV, size_t> traitsCountToEDV = {{EDV::EDVLean, 2}, {EDV::EDVOriginal, 4}, {EDV::EDVGoniometer, 5}};
    for (auto const &pair : traitsCountToEDV)
      TS_ASSERT_EQUALS(pair.first, static_cast<EDV>(pair.second));

    // some invalid values
    std::vector<size_t> invalids{3, 6, 7, 8, 9, 42};
    for (auto const &invalid : invalids)
      TS_ASSERT_THROWS(pSaver->setEventDataVersion(invalid), const std::invalid_argument &)
  }

  void test_CreateOrOpenFile() {
    using Mantid::coord_t;
    using Mantid::API::FileFinder;
    using Mantid::DataObjects::BoxControllerNeXusIO;
    using Mantid::Kernel::Exception::FileError;

    auto pSaver = createTestBoxController();
    pSaver->setDataType(sizeof(coord_t), "MDLeanEvent");
    std::string FullPathFile;

    TSM_ASSERT_THROWS("new file does not open in read mode", pSaver->openFile(this->xxfFileName, "r"),
                      const FileError &);

    TS_ASSERT_THROWS_NOTHING(pSaver->openFile(this->xxfFileName, "w"));
    TS_ASSERT_THROWS_NOTHING(FullPathFile = pSaver->getFileName());
    TS_ASSERT(pSaver->isOpened());
    TS_ASSERT_THROWS_NOTHING(pSaver->closeFile());
    TS_ASSERT(!pSaver->isOpened());

    TSM_ASSERT("file created ", !FileFinder::Instance().getFullPath(FullPathFile).empty());

    // now I can open this file for reading
    TS_ASSERT_THROWS_NOTHING(pSaver->openFile(FullPathFile, "r"));
    TS_ASSERT_THROWS_NOTHING(FullPathFile = pSaver->getFileName());
    TS_ASSERT(pSaver->isOpened());
    TS_ASSERT_THROWS_NOTHING(pSaver->closeFile());
    TS_ASSERT(!pSaver->isOpened());

    // now I can open this file for writing
    TS_ASSERT_THROWS_NOTHING(pSaver->openFile(FullPathFile, "W"));
    TS_ASSERT_THROWS_NOTHING(FullPathFile = pSaver->getFileName());
    TS_ASSERT(pSaver->isOpened());
    TS_ASSERT_THROWS_NOTHING(pSaver->closeFile());
    TS_ASSERT(!pSaver->isOpened());

    if (Poco::File(FullPathFile).exists())
      Poco::File(FullPathFile).remove();
  }

  void test_free_space_index_is_written_out_and_read_in() {
    using Mantid::DataObjects::BoxControllerNeXusIO;

    auto pSaver = createTestBoxController();
    std::string FullPathFile;

    TS_ASSERT_THROWS_NOTHING(pSaver->openFile(this->xxfFileName, "w"));
    TS_ASSERT_THROWS_NOTHING(FullPathFile = pSaver->getFileName());

    std::vector<uint64_t> freeSpaceVectorToSet;
    for (uint64_t i = 0; i < 20; i++) {
      freeSpaceVectorToSet.emplace_back(i);
    }
    pSaver->setFreeSpaceVector(freeSpaceVectorToSet);

    TS_ASSERT_THROWS_NOTHING(pSaver->closeFile());

    TS_ASSERT(!pSaver->isOpened());

    TS_ASSERT_THROWS_NOTHING(pSaver->openFile(this->xxfFileName, "w"));

    std::vector<uint64_t> freeSpaceVectorToGet;
    pSaver->getFreeSpaceVector(freeSpaceVectorToGet);

    TS_ASSERT_EQUALS(freeSpaceVectorToSet, freeSpaceVectorToGet);

    TS_ASSERT_THROWS_NOTHING(pSaver->closeFile());

    if (Poco::File(FullPathFile).exists())
      Poco::File(FullPathFile).remove();
  }

  void test_copyToFile_successfully_copies_open_file_handle() {
    using Mantid::DataObjects::BoxControllerNeXusIO;

    auto pSaver = createTestBoxController();
    pSaver->openFile(xxfFileName, "w");
    const std::string destFilename(xxfFileName + "_copied");

    TS_ASSERT_THROWS_NOTHING(pSaver->copyFileTo(destFilename));

    TSM_ASSERT("File not copied successfully.", Poco::File(destFilename).exists());

    if (Poco::File(destFilename).exists())
      Poco::File(destFilename).remove();
  }

  //---------------------------------------------------------------------------------------------------------
  // tests to read/write double/vs float events
  template <typename FROM, typename TO>
  struct IF // if in/out formats are different we can not read different data
            // format from it
  {
  public:
    static void compareReadTheSame(Mantid::API::IBoxControllerIO *pSaver, const std::vector<FROM> & /*inputData*/,
                                   size_t /*nEvents*/, size_t /*nColumns*/) {
      TS_ASSERT(pSaver->isOpened());
      TS_ASSERT_THROWS_NOTHING(pSaver->closeFile());
      TS_ASSERT(!pSaver->isOpened());
    }
  };
  template <typename FROM>
  struct IF<FROM, FROM> // if in/out formats are the same, we can read what was
                        // written earlier
  {
  public:
    static void compareReadTheSame(Mantid::API::IBoxControllerIO *pSaver, const std::vector<FROM> &inputData,
                                   size_t nEvents, size_t nColumns) {
      std::vector<FROM> toRead;
      TS_ASSERT_THROWS_NOTHING(pSaver->loadBlock(toRead, 100, nEvents));
      for (size_t i = 0; i < nEvents * nColumns; i++) {
        TS_ASSERT_DELTA(inputData[i], toRead[i], 1.e-6);
      }

      TS_ASSERT(pSaver->isOpened());
      TS_ASSERT_THROWS_NOTHING(pSaver->closeFile());
      TS_ASSERT(!pSaver->isOpened());
    }
  };

  template <typename FROM, typename TO> void WriteReadRead() {
    using Mantid::DataObjects::BoxControllerNeXusIO;

    auto pSaver = createTestBoxController();
    pSaver->setDataType(sizeof(FROM), "MDEvent");
    std::string FullPathFile;

    TS_ASSERT_THROWS_NOTHING(pSaver->openFile(this->xxfFileName, "w"));
    TS_ASSERT_THROWS_NOTHING(FullPathFile = pSaver->getFileName());

    size_t nEvents = 20;
    // the number of colums corresponfs to
    size_t nColumns = pSaver->getNDataColums();
    std::vector<FROM> toWrite(nColumns * nEvents);
    for (size_t i = 0; i < nEvents; i++) {
      for (size_t j = 0; j < nColumns; j++) {
        toWrite[i * nColumns + j] = static_cast<FROM>(j + 10 * i);
      }
    }

    TS_ASSERT_THROWS_NOTHING(pSaver->saveBlock(toWrite, 100));

    IF<FROM, TO>::compareReadTheSame(pSaver.get(), toWrite, nEvents, nColumns);

    // open and read what was written,
    pSaver->setDataType(sizeof(TO), "MDEvent");
    TS_ASSERT_THROWS_NOTHING(pSaver->openFile(FullPathFile, "r"));
    std::vector<TO> toRead2;
    TS_ASSERT_THROWS_NOTHING(pSaver->loadBlock(toRead2, 100 + (nEvents - 1), 1));
    for (size_t i = 0; i < nColumns; i++) {
      TS_ASSERT_DELTA(toWrite[(nEvents - 1) * nColumns + i], toRead2[i], 1.e-6);
    }

    pSaver->closeFile();
    if (Poco::File(FullPathFile).exists())
      Poco::File(FullPathFile).remove();
  }

  void test_WriteFloatReadReadFloat() { this->WriteReadRead<float, float>(); }
  void test_WriteFloatReadReadDouble() { this->WriteReadRead<double, double>(); }
  void test_WriteDoubleReadFloat() { this->WriteReadRead<double, float>(); }

  void test_WriteFloatReadDouble() { this->WriteReadRead<float, double>(); }

  void test_dataEventCount() {
    using Mantid::DataObjects::BoxControllerNeXusIO;
    using EDV = BoxControllerNeXusIO::EventDataVersion;

    // MDEvents, where the dimensionality of the event coordinates is 4
    auto pSaver = createTestBoxController();

    // MDEvent cannot accept EDVLean
    TS_ASSERT_THROWS(pSaver->setEventDataVersion(EDV::EDVLean), const std::invalid_argument &);

    int64_t dataSizePerEvent(pSaver->getNDataColums());
    // MDEvent can accept EDVOriginal and EDVGoniometer
    pSaver->setEventDataVersion(EDV::EDVOriginal);
    TS_ASSERT_EQUALS(pSaver->dataEventCount(), dataSizePerEvent - 1);
    pSaver->setEventDataVersion(EDV::EDVGoniometer);
    TS_ASSERT_EQUALS(pSaver->dataEventCount(), dataSizePerEvent);
  }

  void test_adjustEventDataBlock() {

    using Mantid::DataObjects::BoxControllerNeXusIO;
    using EDV = BoxControllerNeXusIO::EventDataVersion;

    // MDEvents, where the dimensionality of the event coordinates is 4
    auto pSaver = createTestBoxController();

    // We're dealing with an old Nexus file
    pSaver->setEventDataVersion(EDV::EDVOriginal);

    // A data block has been read from the file, containing two events.
    // Each event has 8 data items
    std::vector<float> blockREAD{1., 2., 3., 4., -1., -2., -3., -4., 10., 20., 30., 40., -10., -20., -30., -40.};

    // insert goniometerIndex
    pSaver->adjustEventDataBlock(blockREAD, "READ");
    std::vector<float> expectedREAD{1.,  2.,  3.,  0., 4.,  -1.,  -2.,  -3.,  -4.,
                                    10., 20., 30., 0., 40., -10., -20., -30., -40.};

    TS_ASSERT_EQUALS(blockREAD.size(), expectedREAD.size());
    for (size_t i = 0; i < blockREAD.size(); i++)
      TS_ASSERT_DELTA(blockREAD[i], expectedREAD[i], 1.e-6);

    // A data block is to be written to the file, containing two events.
    // Each event has 9 items
    std::vector<float> blockWRITE{1.,  2.,  3.,  0., 4.0,  -1.,  -2.,  -3.,  -4.,
                                  10., 20., 30., 0., 40.0, -10., -20., -30., -40.};
    pSaver->adjustEventDataBlock(blockWRITE, "WRITE"); // remove goniometerIndex
    std::vector<float> expectedWRITE{1., 2., 3., 4.0, -1., -2., -3., -4., 10., 20., 30., 40.0, -10., -20., -30., -40.};
    TS_ASSERT_EQUALS(blockWRITE.size(), expectedWRITE.size());
    for (size_t i = 0; i < blockWRITE.size(); i++)
      TS_ASSERT_DELTA(blockWRITE[i], expectedWRITE[i], 1.e-6);
  }

private:
  /// Create a Nexus reader/writer for boxController sc
  std::unique_ptr<Mantid::DataObjects::BoxControllerNeXusIO> createTestBoxController() {
    return std::make_unique<Mantid::DataObjects::BoxControllerNeXusIO>(sc.get());
  }
};
