// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"

#include <filesystem>
#include <string>
#include <vector>

namespace NXcanSASTestHelper {

struct InstrumentPolarizerTest {
  explicit InstrumentPolarizerTest(const std::string &name, const std::string &type, const std::string &valueType,
                                   double distance)
      : distanceToSample(distance), compName(name), compType(type), valueType(valueType) {}
  double distanceToSample;
  std::string compName;
  std::string compType;
  std::string valueType;
};

struct TransmissionTestParameters {
  TransmissionTestParameters() = default;
  TransmissionTestParameters(const std::string &name) : name(name), usesTransmission(true) {}
  int size{10};
  double value{12.34};
  double error{3.2345};
  double xmin{1.0};
  double xmax{10.0};
  std::string name;
  bool isHistogram{false};
  bool usesTransmission{false};
};

struct NXcanSASTestParameters {
  NXcanSASTestParameters() = default;

  int polWorkspaceNumber{4};
  int size{10};

  double value{10.23};
  double error{3.45};
  double xerror{2.3759 / 3.6};
  double xmin{1.0};
  double xmax{10.0};
  double ymin{1.0};
  double ymax{12.0};
  double beamHeight{1.0};
  double beamWidth{1.0};
  double sampleThickness{1.0};
  double magneticFieldStrength{1};
  double scaledBgSubScaleFactor{};

  std::string filename{(std::filesystem::temp_directory_path() / "testFile.h5").string()};
  std::string runNumber{"1234"};
  std::string userFile{"my_user_file"};
  std::string workspaceTitle{"sample_workspace"};
  std::string instrumentName{"SANS2D"};
  std::string radiationSource{"Spallation Neutron Source"};
  std::string geometry{"Disc"};
  std::string loadedWSName{"loadNXcanSASTestOutputWorkspace"};
  std::string idf;
  std::string sampleTransmissionRun;
  std::string sampleDirectRun;
  std::string canScatterRun;
  std::string canDirectRun;
  std::string scaledBgSubWorkspace;
  // Polarized parameters
  std::string inputSpinStates{"-1-1, -1+1, +1-1, +1+1"};
  std::string instrumentFilename{""};
  std::string wrongComponentName{"wrong"};
  std::string magneticFieldStrengthLogName{""};
  std::string magneticFieldUnit{"G"};
  std::string magneticFieldDirection{""};
  std::vector<std::string> expectedGroupSuffices{"00", "01"};
  std::vector<std::string> detectors;
  std::vector<double> referenceValues{};

  TransmissionTestParameters transmissionParameters;
  TransmissionTestParameters transmissionCanParameters;

  InstrumentPolarizerTest polarizerComponent{InstrumentPolarizerTest("test-polarizer1", "polarizer", "supermirror", 7)};
  InstrumentPolarizerTest flipperComponent{InstrumentPolarizerTest("test-flipper1", "flipper", "coil", 4)};
  InstrumentPolarizerTest analyzerComponent{InstrumentPolarizerTest("test-analyzer1", "analyzer", "MEOP", -0.5)};

  bool hasDx{true};
  bool invalidDetectors{false};
  bool is2dData{false};
  bool isPolarized{false};
  bool hasCanRuns{false};
  bool hasSampleRuns{false};
  bool hasBgSub{false};
  bool isHistogram{false};
  bool loadTransmission{false};
};

std::string concatenateStringVector(const std::vector<std::string> &stringVector);

std::string getIDFfromWorkspace(const Mantid::API::MatrixWorkspace_sptr &workspace);

void setXValuesOn1DWorkspace(const Mantid::API::MatrixWorkspace_sptr &workspace, double xmin, double xmax);

void add_sample_log(const Mantid::API::MatrixWorkspace_sptr &workspace, const std::string &logName,
                    const std::string &logValue, const std::string &logUnit = "");

void set_logs(const Mantid::API::MatrixWorkspace_sptr &workspace, const std::string &runNumber,
              const std::string &userFile);

void set_instrument(const Mantid::API::MatrixWorkspace_sptr &workspace, const std::string &instrumentName,
                    const std::string &instrumentFilename = "");

Mantid::API::WorkspaceGroup_sptr provideGroupWorkspace(Mantid::API::AnalysisDataServiceImpl &ads,
                                                       NXcanSASTestParameters &parameters);

Mantid::API::WorkspaceGroup_sptr providePolarizedGroup(Mantid::API::AnalysisDataServiceImpl &ads,
                                                       NXcanSASTestParameters &parameters);

void setPolarizedParameters(NXcanSASTestParameters &parameters);

Mantid::API::MatrixWorkspace_sptr provide1DWorkspace(const NXcanSASTestParameters &parameters);

Mantid::API::MatrixWorkspace_sptr getTransmissionWorkspace(const TransmissionTestParameters &parameters);

Mantid::API::MatrixWorkspace_sptr provide2DWorkspace(const NXcanSASTestParameters &parameters);

void set2DValues(const Mantid::API::MatrixWorkspace_sptr &ws, double value = 0);

void removeFile(const std::string &filename);
} // namespace NXcanSASTestHelper
