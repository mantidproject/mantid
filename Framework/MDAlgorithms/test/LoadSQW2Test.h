#ifndef MANTID_MDALGORITHMS_LOADSQW2TEST_H_
#define MANTID_MDALGORITHMS_LOADSQW2TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidMDAlgorithms/LoadSQW2.h"
#include "MantidKernel/make_unique.h"

#include <Poco/TemporaryFile.h>

#include <array>

using Mantid::API::ExperimentInfo;
using Mantid::API::IAlgorithm;
using Mantid::API::IAlgorithm_uptr;
using Mantid::API::IMDEventWorkspace;
using Mantid::API::IMDEventWorkspace_sptr;
using Mantid::API::IMDIterator;
using Mantid::API::Run;
using Mantid::API::Sample;
using Mantid::MDAlgorithms::LoadSQW2;
using Mantid::Kernel::make_unique;
using Mantid::Kernel::V3D;

class LoadSQW2Test : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadSQW2Test *createSuite() { return new LoadSQW2Test(); }
  static void destroySuite(LoadSQW2Test *suite) { delete suite; }

  LoadSQW2Test() : CxxTest::TestSuite(), m_filename("test_horace_reader.sqw") {}

  //----------------------------------------------------------------------------
  // Success tests
  //----------------------------------------------------------------------------
  void test_Algorithm_Initializes_Correctly() {
    IAlgorithm_uptr alg;
    TS_ASSERT_THROWS_NOTHING(alg = createAlgorithm());
    TS_ASSERT(alg->isInitialized());
  }

  void test_Algorithm_Is_Version_2_LoadSQW() {
    IAlgorithm_uptr alg = createAlgorithm();
    TS_ASSERT_EQUALS("LoadSQW", alg->name());
    TS_ASSERT_EQUALS(2, alg->version());
  }

  void test_SQW_Is_Accepted_Filename() {
    IAlgorithm_uptr alg;
    TS_ASSERT_THROWS_NOTHING(alg = createAlgorithm());
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("Filename", m_filename));
  }

  void test_OutputWorkspace_Has_Correct_Data() {
    Arguments args;
    args.metadataOnly = false;
    IMDEventWorkspace_sptr outputWS = runAlgorithm(args);

    checkGeometryAsExpected(*outputWS);
    checkExperimentInfoAsExpected(*outputWS);
    checkDataAsExpected(*outputWS, args);
  }

  void test_OutputWorkspace_Has_No_Events_When_MetaDataOnly_Selected() {
    Arguments args;
    args.metadataOnly = true;
    IMDEventWorkspace_sptr outputWS = runAlgorithm(args);

    checkGeometryAsExpected(*outputWS);
    checkExperimentInfoAsExpected(*outputWS);
    checkDataAsExpected(*outputWS, args);
  }

  void test_OutputWorkspace_Is_File_Backed_When_Requested() {
    using Poco::TemporaryFile;

    Arguments args;
    args.metadataOnly = false;
    TemporaryFile filebacking;
    args.outputFilename = filebacking.path();
    IMDEventWorkspace_sptr outputWS = runAlgorithm(args);

    checkGeometryAsExpected(*outputWS);
    checkExperimentInfoAsExpected(*outputWS);
    checkDataAsExpected(*outputWS, args);
  }

  //----------------------------------------------------------------------------
  // Failure tests
  //----------------------------------------------------------------------------
  void test_Filename_Property_Throws_If_Not_Found() {
    auto alg = createAlgorithm();
    TS_ASSERT_THROWS(alg->setPropertyValue("Filename", "x.sqw"),
                     std::invalid_argument);
  }

private:
  struct Arguments {
    Arguments() : metadataOnly(false), outputFilename() {}
    bool metadataOnly;
    std::string outputFilename;
  };

  IMDEventWorkspace_sptr runAlgorithm(Arguments args) {
    auto algm = createAlgorithm();
    algm->setProperty("Filename", m_filename);
    algm->setProperty("OutputWorkspace", "__unused_value_for_child_algorithm");
    algm->setProperty("MetadataOnly", args.metadataOnly);
    algm->setProperty("OutputFilename", args.outputFilename);
    algm->execute();
    return algm->getProperty("OutputWorkspace");
  }

  IAlgorithm_uptr createAlgorithm() {
    IAlgorithm_uptr alg(make_unique<LoadSQW2>());
    alg->initialize();
    alg->setChild(true);
    return alg;
  }

  void checkGeometryAsExpected(const IMDEventWorkspace &outputWS) {
    TS_ASSERT_EQUALS(4, outputWS.getNumDims());
    std::array<const char *, 4> ids{"Q1", "Q2", "Q3", "DeltaE"};
    std::array<const char *, 4> names{"Q_\\zeta", "Q_\\xi", "Q_\\eta", "E"};
    std::array<double, 8> ulimits{0.0439,  0.9271,  -0.4644, -0.4024,
                                  -0.7818, -0.5052, 2.5,     147.5};
    std::array<size_t, 4> nbins{3, 3, 2, 2};
    std::array<const char *, 4> units{"A\\^-1", "A\\^-1", "A\\^-1", "mev"};
    std::array<const char *, 4> frames{"HKL", "HKL", "HKL", "meV"};
    for (size_t i = 0; i < 4; ++i) {
      auto dim = outputWS.getDimension(i);
      TS_ASSERT_EQUALS(ids[i], dim->getDimensionId());
      TS_ASSERT_EQUALS(names[i], dim->getName());
      TS_ASSERT_DELTA(ulimits[2 * i], dim->getMinimum(), 1e-04);
      TS_ASSERT_DELTA(ulimits[2 * i + 1], dim->getMaximum(), 1e-04);
      TS_ASSERT_EQUALS(nbins[i], dim->getNBins());
      TS_ASSERT_EQUALS(units[i], dim->getUnits().ascii());
      TS_ASSERT_EQUALS(frames[i], dim->getMDFrame().name());
    }
  }

  void checkExperimentInfoAsExpected(const IMDEventWorkspace &outputWS) {
    TS_ASSERT_EQUALS(2, outputWS.getNumExperimentInfo());
    for (uint16_t i = 0; i < 2; ++i) {
      checkExperimentInfoAsExpected(*outputWS.getExperimentInfo(i), i);
    }
  }

  void checkExperimentInfoAsExpected(const ExperimentInfo &expt,
                                     const uint16_t index) {
    checkRunAsExpected(expt.run(), index);
    checkSampleAsExpected(expt.sample(), index);
  }

  void checkRunAsExpected(const Run &run, const size_t index) {
    const double efix(787.0);
    TS_ASSERT_DELTA(efix, run.getLogAsSingleValue("Ei"), 1e-04);
    // histogram bins
    const auto enBins = run.getBinBoundaries();
    std::vector<double> expectedBins(31);
    double en(-5.0);
    std::generate(begin(expectedBins), end(expectedBins), [&en]() {
      en += 5.0;
      return en;
    });
    TS_ASSERT_DELTA(expectedBins, enBins, 1e-4);
    // goniometer
    const auto &gR = run.getGoniometerMatrix();
    std::vector<double> expectedG;
    if (index == 0) {
      expectedG = {1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0};
    } else {
      expectedG = {1.0,  0.000304617, 0.0, -0.000304617, 1.0,
                   -0.0, -0.0,        0.0, 1.0};
    }
    TSM_ASSERT_DELTA("Goniometer for run index " + std::to_string(index) +
                         " is incorrect",
                     expectedG, gR.getVector(), 1e-04);
  }

  void checkSampleAsExpected(const Sample &sample, const size_t) {
    const auto &lattice = sample.getOrientedLattice();
    // lattice
    TS_ASSERT_DELTA(2.87, lattice.a1(), 1e-04);
    TS_ASSERT_DELTA(2.87, lattice.a2(), 1e-04);
    TS_ASSERT_DELTA(2.87, lattice.a3(), 1e-04);
    TS_ASSERT_DELTA(90.0, lattice.alpha(), 1e-04);
    TS_ASSERT_DELTA(90.0, lattice.beta(), 1e-04);
    TS_ASSERT_DELTA(90.0, lattice.gamma(), 1e-04);
    auto uVec(lattice.getuVector()), vVec(lattice.getvVector());
    uVec.normalize();
    vVec.normalize();
    TS_ASSERT_DELTA(1.0, uVec[0], 1e-04);
    TS_ASSERT_DELTA(0.0, uVec[1], 1e-04);
    TS_ASSERT_DELTA(0.0, uVec[2], 1e-04);
    TS_ASSERT_DELTA(0.0, vVec[0], 1e-04);
    TS_ASSERT_DELTA(1.0, vVec[1], 1e-04);
    TS_ASSERT_DELTA(0.0, vVec[2], 1e-04);
  }

  void checkDataAsExpected(const IMDEventWorkspace &outputWS, Arguments args) {
    if (args.metadataOnly) {
      TS_ASSERT_EQUALS(0, outputWS.getNEvents());
    } else {
      TS_ASSERT_EQUALS(580, outputWS.getNEvents());
      // equal split between experiments
      size_t nexpt1(0), nexpt2(0);
      // 10 detector Ids split evenly
      std::vector<int> ids(10, 0);
      auto iter = std::unique_ptr<IMDIterator>(outputWS.createIterator());
      do {
        auto nevents = iter->getNumEvents();
        for (size_t i = 0; i < nevents; ++i) {
          auto irun = iter->getInnerRunIndex(i);
          TSM_ASSERT("Expected run index 0 or 1. Found " + std::to_string(irun),
                     irun == 0 || irun == 1);
          if (irun == 0)
            nexpt1++;
          else
            nexpt2++;
          auto idet = iter->getInnerDetectorID(i);
          TSM_ASSERT("Expected 1 <= det ID <= 10. Found " +
                         std::to_string(idet),
                     1 <= idet || idet <= 10);
          ids[idet - 1] += 1;
        }
      } while (iter->next());
      TS_ASSERT_EQUALS(290, nexpt1);
      TS_ASSERT_EQUALS(290, nexpt2);
      // 58 events in each detector
      std::vector<int> expectedIds(10, 58);
      TS_ASSERT_EQUALS(expectedIds, ids);
    }
    if (!args.outputFilename.empty()) {
      checkOutputFile(outputWS, args.outputFilename);
    }
  }

  void checkOutputFile(const IMDEventWorkspace &outputWS,
                       std::string outputFilename) {
    TS_ASSERT(outputWS.isFileBacked());
    Poco::File fileback(outputFilename);
    TS_ASSERT(fileback.getSize() > 0);
  }

  //----------------------------------------------------------------------------
  // Private data
  //----------------------------------------------------------------------------
  std::string m_filename;
};

#endif /* MANTID_MDALGORITHMS_LOADSQW2TEST_H_ */
