// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLINGTEST_NXCANSASTESTHELPER_H
#define MANTID_DATAHANDLINGTEST_NXCANSASTESTHELPER_H

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include <Poco/TemporaryFile.h>
#include <string>
#include <vector>

namespace NXcanSASTestHelper {
struct NXcanSASTestParameters {
  NXcanSASTestParameters() { initParameters(); }

  void initParameters() {
    Poco::TemporaryFile tmpFile;
    tmpFile.keep();
    filename = tmpFile.path();
    size = 10;
    value = 10.23;
    error = 3.45;
    xerror = 2.3759 / 3.6;
    hasDx = true;
    xmin = 1.0;
    xmax = 10.0;
    runNumber = "1234";
    userFile = "my_user_file";
    workspaceTitle = "sample_worksapce";
    instrumentName = "SANS2D";
    radiationSource = "Spallation Neutron Source";
    invalidDetectors = false;
    ymin = 1.0;
    ymax = 12.0;
    is2dData = false;
    isHistogram = false;
    sampleTransmissionRun = "";
    sampleDirectRun = "";
    canScatterRun = "";
    canDirectRun = "";
    hasCanRuns = false;
    hasSampleRuns = false;
  }

  std::string filename;
  int size;
  double value;
  double error;
  double xerror;
  bool hasDx;
  double xmin;
  double xmax;
  double ymin;
  double ymax;
  std::string runNumber;
  std::string userFile;
  std::string workspaceTitle;
  std::string instrumentName;
  std::string radiationSource;
  std::vector<std::string> detectors;
  bool invalidDetectors;
  bool is2dData;
  std::string idf;
  bool isHistogram;
  std::string sampleTransmissionRun;
  std::string sampleDirectRun;
  std::string canScatterRun;
  std::string canDirectRun;
  bool hasCanRuns;
  bool hasSampleRuns;
};

struct NXcanSASTestTransmissionParameters {
  NXcanSASTestTransmissionParameters() { initParameters(); }
  void initParameters() {
    size = 10;
    value = 12.34;
    error = 3.2345;
    xmin = 1.0;
    xmax = 10.0;
    usesTransmission = false;
  }

  int size;
  double value;
  double error;
  double xmin;
  double xmax;
  std::string name;
  bool usesTransmission;
};

std::string concatenateStringVector(std::vector<std::string> stringVector);

std::string getIDFfromWorkspace(Mantid::API::MatrixWorkspace_sptr workspace);

void setXValuesOn1DWorkspaceWithPointData(
    Mantid::API::MatrixWorkspace_sptr workspace, double xmin, double xmax);

void add_sample_log(Mantid::API::MatrixWorkspace_sptr workspace,
                    const std::string &logName, const std::string &logValue);

void set_logs(Mantid::API::MatrixWorkspace_sptr workspace,
              const std::string &runNumber, const std::string &userFile);

void set_instrument(Mantid::API::MatrixWorkspace_sptr workspace,
                    const std::string &instrumentName);

Mantid::API::MatrixWorkspace_sptr
provide1DWorkspace(NXcanSASTestParameters &parameters);

Mantid::API::MatrixWorkspace_sptr
getTransmissionWorkspace(NXcanSASTestTransmissionParameters &parameters);

Mantid::API::MatrixWorkspace_sptr
provide2DWorkspace(NXcanSASTestParameters &parameters);

void set2DValues(Mantid::API::MatrixWorkspace_sptr ws);

void removeFile(std::string filename);
} // namespace NXcanSASTestHelper
#endif
