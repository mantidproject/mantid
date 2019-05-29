// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_LOADSQW2TEST_H_
#define MANTID_MDALGORITHMS_LOADSQW2TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidKernel/make_unique.h"
#include "MantidMDAlgorithms/LoadSQW2.h"

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
using Mantid::Kernel::V3D;
using Mantid::MDAlgorithms::LoadSQW2;

class LoadSQW2Test : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadSQW2Test *createSuite() { return new LoadSQW2Test(); }
  static void destroySuite(LoadSQW2Test *suite) { delete suite; }

  LoadSQW2Test()
      : CxxTest::TestSuite(), m_4DFilename("test_horace_reader.sqw"),
        m_3DCutFilename("test_horace_reader_3dcut.sqw") {}

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
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("Filename", m_4DFilename));
  }

  void test_Default_Frame_Is_Q_Sample() {
    // Empty arguments
    Arguments args;

    IMDEventWorkspace_sptr outputWS = runAlgorithm(m_4DFilename, args);

    checkGeometryAsExpected(*outputWS, "Q_sample", DataType::SQW);
    checkExperimentInfoAsExpected(*outputWS);
    checkDataAsExpected(*outputWS, args, DataType::SQW);
  }

  void test_Full_4D_As_Expected_For_HKL_Frame() {
    Arguments args;
    args.outputFrame = "HKL";
    IMDEventWorkspace_sptr outputWS = runAlgorithm(m_4DFilename, args);

    checkGeometryAsExpected(*outputWS, args.outputFrame, DataType::SQW);
    checkExperimentInfoAsExpected(*outputWS);
    checkDataAsExpected(*outputWS, args, DataType::SQW);
  }

  void test_Full_4D_Has_No_Events_When_MetaDataOnly_Selected() {
    Arguments args;
    args.metadataOnly = true;
    IMDEventWorkspace_sptr outputWS = runAlgorithm(m_4DFilename, args);

    checkGeometryAsExpected(*outputWS, "Q_sample", DataType::SQW);
    checkExperimentInfoAsExpected(*outputWS);
    checkDataAsExpected(*outputWS, args, DataType::SQW);
  }

  void test_Full_4D_Is_File_Backed_When_Requested() {
    using Poco::TemporaryFile;

    Arguments args;
    args.metadataOnly = false;
    TemporaryFile filebacking;
    args.outputFilename = filebacking.path();
    IMDEventWorkspace_sptr outputWS = runAlgorithm(m_4DFilename, args);

    checkGeometryAsExpected(*outputWS, "Q_sample", DataType::SQW);
    checkExperimentInfoAsExpected(*outputWS);
    checkDataAsExpected(*outputWS, args, DataType::SQW);
  }

  void test_Cut_File_As_Expected_For_Default_Values() {
    Arguments args;
    IMDEventWorkspace_sptr outputWS = runAlgorithm(m_3DCutFilename, args);

    checkGeometryAsExpected(*outputWS, "Q_sample", DataType::Cut3D);
    checkExperimentInfoAsExpected(*outputWS);
    checkDataAsExpected(*outputWS, args, DataType::Cut3D);
  }

  //----------------------------------------------------------------------------
  // Failure tests
  //----------------------------------------------------------------------------
  void test_Filename_Property_Throws_If_Not_Found() {
    auto alg = createAlgorithm();
    TS_ASSERT_THROWS(alg->setPropertyValue("Filename", "x.sqw"),
                     const std::invalid_argument &);
  }

  void test_Unknown_Q3DFrame_Is_Not_Accepted() {
    auto alg = createAlgorithm();
    TS_ASSERT_THROWS(alg->setPropertyValue("Q3DFrames", "Unknown"),
                     const std::invalid_argument &);
  }

  void test_Unsupported_SQW_Type_Throws_Error() {
    auto algm = createAlgorithm();
    algm->setProperty("Filename", "horace_dnd_test_file.sqw");
    algm->setRethrows(true);
    TS_ASSERT_THROWS(algm->execute(), const std::runtime_error &);
  }

private:
  struct Arguments {
    Arguments() : metadataOnly(false), outputFilename(), outputFrame() {}
    bool metadataOnly;
    std::string outputFilename;
    std::string outputFrame;
  };

  enum class DataType { SQW, Cut3D };

  struct DimensionProperties {
    using StringList = std::array<std::string, 4>;
    using DoubleList = std::array<double, 8>;
    using SizeTList = std::array<size_t, 4>;
    StringList ids, names, units, frameNames;
    DoubleList ulimits;
    SizeTList nbins;
  };

  IMDEventWorkspace_sptr runAlgorithm(std::string filename, Arguments args) {
    auto algm = createAlgorithm();
    algm->setProperty("Filename", filename);
    algm->setProperty("MetadataOnly", args.metadataOnly);
    algm->setProperty("OutputFilename", args.outputFilename);
    if (!args.outputFrame.empty()) {
      algm->setProperty("Q3DFrames", args.outputFrame);
    }
    algm->execute();
    return algm->getProperty("OutputWorkspace");
  }

  IAlgorithm_uptr createAlgorithm() {
    IAlgorithm_uptr alg(Mantid::Kernel::make_unique<LoadSQW2>());
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("OutputWorkspace", "__unused_value_for_child_algorithm");
    return alg;
  }

  void checkGeometryAsExpected(const IMDEventWorkspace &outputWS,
                               std::string outputFrame, DataType dtype) {
    TS_ASSERT_EQUALS(4, outputWS.getNumDims());
    auto expectedDim = getExpectedDimProperties(outputFrame, dtype);
    for (size_t i = 0; i < 4; ++i) {
      auto dim = outputWS.getDimension(i);
      TS_ASSERT_EQUALS(expectedDim.ids[i], dim->getDimensionId());
      TS_ASSERT_EQUALS(expectedDim.names[i], dim->getName());
      TS_ASSERT_DELTA(expectedDim.ulimits[2 * i], dim->getMinimum(), 1e-04);
      TS_ASSERT_DELTA(expectedDim.ulimits[2 * i + 1], dim->getMaximum(), 1e-04);
      TS_ASSERT_EQUALS(expectedDim.nbins[i], dim->getNBins());
      TS_ASSERT_EQUALS(expectedDim.units[i], dim->getUnits().ascii());
      TS_ASSERT_EQUALS(expectedDim.frameNames[i], dim->getMDFrame().name());
    }
  }

  GNU_DIAG_OFF("missing-braces")
  DimensionProperties getExpectedDimProperties(std::string outputFrame,
                                               DataType dtype) {
    DimensionProperties expected;
    expected.ids = {"qx", "qy", "qz", "en"};
    if (outputFrame == "HKL") {
      expected.units = {"in 2.189 A^-1", "in 2.189 A^-1", "in 2.189 A^-1",
                        "meV"};
      expected.names = {"[H,0,0]", "[0,K,0]", "[0,0,L]", "en"};
      expected.frameNames = {"HKL", "HKL", "HKL", "meV"};
    } else if (outputFrame == "Q_sample") {
      expected.units = {"Angstrom^-1", "Angstrom^-1", "Angstrom^-1", "meV"};
      expected.names = {"Q_sample_x", "Q_sample_y", "Q_sample_z", "en"};
      expected.frameNames = {"QSample", "QSample", "QSample", "meV"};
    } else {
      throw std::runtime_error("LoadSQW2Test::getExpectedDimProperties() - "
                               "Unknown output frame expected.");
    }

    if (dtype == DataType::SQW) {
      expected.nbins = {3, 3, 2, 2};
      if (outputFrame == "HKL") {
        expected.ulimits = {0.0439,  0.8959,  -0.4644, -0.4046,
                            -0.7818, -0.5071, 2.5,     142.5};
      } else {
        expected.ulimits = {0.0962,  1.9615,  -1.0168, -0.8858,
                            -1.7116, -1.1103, 2.5,     142.5};
      }
    } else if (dtype == DataType::Cut3D) {
      expected.nbins = {3, 3, 1, 3};
      if (outputFrame == "HKL") {
        expected.ulimits = {0.0439,  0.9271,  -0.4644, -0.4024,
                            -0.7818, -0.5052, 2.5,     117.5};
      } else {
        expected.ulimits = {0.0962,  1.6247,   -1.01689, -0.909366,
                            -1.7117, -1.13168, 2.5,      117.5};
      }
    }
    return expected;
  }

  GNU_DIAG_ON("missing-braces")

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

  void checkDataAsExpected(const IMDEventWorkspace &outputWS, Arguments args,
                           DataType dtype) {
    if (args.metadataOnly) {
      TS_ASSERT_EQUALS(0, outputWS.getNEvents());
    } else {
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
      // It is assumed that if the events are not transformed to the output
      // frame correctly then they will all not register into the workspace
      // correctly and the number events will be incorrect
      if (dtype == DataType::SQW) {
        TS_ASSERT_EQUALS(580, outputWS.getNEvents());
        TS_ASSERT_EQUALS(290, nexpt1);
        TS_ASSERT_EQUALS(290, nexpt2);
        // 58 events in each detector
        std::vector<int> expectedIds(10, 58);
        TS_ASSERT_EQUALS(expectedIds, ids);
      } else if (dtype == DataType::Cut3D) {
        TS_ASSERT_EQUALS(480, outputWS.getNEvents());
        TS_ASSERT_EQUALS(240, nexpt1);
        TS_ASSERT_EQUALS(240, nexpt2);
        // 48 events in each detector
        std::vector<int> expectedIds(10, 48);
        TS_ASSERT_EQUALS(expectedIds, ids);
      } else {
        throw std::runtime_error(
            "LoadSQW2Test::checkDataAsExpected - Unexpected data type.");
      }
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
  std::string m_4DFilename;
  std::string m_3DCutFilename;
};

#endif /* MANTID_MDALGORITHMS_LOADSQW2TEST_H_ */
