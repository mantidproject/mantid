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

#include <Poco/TemporaryFile.h>
#include <string>
#include <vector>

namespace NXcanSASTestHelper {
struct NXcanSASTestParameters {
  NXcanSASTestParameters() {}

  std::string filename{Poco::TemporaryFile::tempName() + ".h5"};
  std::vector<std::string> expectedGroupSuffices{"00", "01"};
  int size{10};
  double value{10.23};
  double error{3.45};
  double xerror{2.3759 / 3.6};
  bool hasDx{true};
  double xmin{1.0};
  double xmax{10.0};
  double ymin{1.0};
  double ymax{12.0};
  std::string runNumber{"1234"};
  std::string userFile{"my_user_file"};
  std::string workspaceTitle{"sample_workspace"};
  std::string instrumentName{"SANS2D"};
  std::string radiationSource{"Spallation Neutron Source"};
  std::string geometry{"Disc"};
  double beamHeight{1.0};
  double beamWidth{1.0};
  std::vector<std::string> detectors;
  double sampleThickness{1.0};
  bool invalidDetectors{false};
  bool is2dData{false};
  std::string idf;
  bool isHistogram{false};
  std::string sampleTransmissionRun;
  std::string sampleDirectRun;
  std::string canScatterRun;
  std::string canDirectRun;
  std::string scaledBgSubWorkspace;
  double scaledBgSubScaleFactor;
  bool hasCanRuns{false};
  bool hasSampleRuns{false};
  bool hasBgSub{false};
};

struct NXcanSASTestTransmissionParameters {
  int size{10};
  double value{12.34};
  double error{3.2345};
  double xmin{1.0};
  double xmax{10.0};
  std::string name;
  bool usesTransmission{false};
  bool isHistogram{false};
};

std::string concatenateStringVector(const std::vector<std::string> &stringVector);

std::string getIDFfromWorkspace(const Mantid::API::MatrixWorkspace_sptr &workspace);

void setXValuesOn1DWorkspace(const Mantid::API::MatrixWorkspace_sptr &workspace, double xmin, double xmax);

void add_sample_log(const Mantid::API::MatrixWorkspace_sptr &workspace, const std::string &logName,
                    const std::string &logValue);

void set_logs(const Mantid::API::MatrixWorkspace_sptr &workspace, const std::string &runNumber,
              const std::string &userFile);

void set_instrument(const Mantid::API::MatrixWorkspace_sptr &workspace, const std::string &instrumentName);

Mantid::API::WorkspaceGroup_sptr provideGroupWorkspace(Mantid::API::AnalysisDataServiceImpl &ads,
                                                       NXcanSASTestParameters &parameters);

Mantid::API::MatrixWorkspace_sptr provide1DWorkspace(NXcanSASTestParameters &parameters);

Mantid::API::MatrixWorkspace_sptr getTransmissionWorkspace(NXcanSASTestTransmissionParameters &parameters);

Mantid::API::MatrixWorkspace_sptr provide2DWorkspace(NXcanSASTestParameters &parameters);

void set2DValues(const Mantid::API::MatrixWorkspace_sptr &ws);

void removeFile(const std::string &filename);
} // namespace NXcanSASTestHelper
