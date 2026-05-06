// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include <cstddef>

namespace Mantid {
namespace Algorithms {

class MANTID_ALGORITHMS_DLL CreateDetectorTable final : public API::Algorithm {
public:
  /// (Empty) Constructor
  CreateDetectorTable() : API::Algorithm() {}
  /// Algorithm's name
  const std::string name() const override { return "CreateDetectorTable"; }
  /// Algorithm's version
  int version() const override { return 1; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Utility\\Workspaces"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Create a table showing detector information for the given "
           "workspace and optionally the data for that detector";
  }
  std::map<std::string, std::string> validateInputs() override;

  /// Creates table workspace of detector information from a given workspace
  void setup();
  void createColumns();
  void populateTable();
  void populateTableByDetID();
  void setTableToOutput();

private:
  API::MatrixWorkspace_sptr ws;
  std::vector<int> workspaceIndices;
  bool includeData;
  bool includeDetectorPosition;
  bool oneRowPerDetectorID;
  bool isScanning;
  bool calcQ;
  bool hasDiffConstants;
  API::ITableWorkspace_sptr table;
  const API::SpectrumInfo *spectrumInfo;
  const Geometry::DetectorInfo *detectorInfo;
  bool signedThetaParamRetrieved;
  bool showSignedTwoTheta; // If true, signedVersion of the two theta
  Geometry::PointingAlong beamAxisIndex;
  double sampleDist;
  /// Initialisation code
  struct DetectorRowData {
    int wsIndex = 0;
    int specNo = 0;
    std::set<int> detIds;
    std::string timeIndexes;
    std::string isMonitor;
    double dataY0 = 0, dataE0 = 0;
    double R = 0, theta = 0, q = 0, phi = 0;
    double difa = 0, difc = 0, difcUnc = 0, tzero = 0;
    Kernel::V3D detPosition{0, 0, 0};
  };

  void init() override;
  /// Execution code
  void exec() override;

  void getSphericalCoordinates(size_t wsIndex, double &R, double &theta, double &phi);
  const std::string getTimeIndexes(size_t wsIndex);
  double getQ(size_t wsIndex);
  void getDiffConst(size_t wsIndex, double &difa, double &difc, double &difcUnc, double &tzero);
  void writeRowToTable(const int row, const DetectorRowData &data);
  DetectorRowData calculateWsIdxData(size_t wsIndex);
};

/// Converts a list to a string, shortened if necessary
std::string createTruncatedList(const std::set<int> &elements);

} // namespace Algorithms
} // namespace Mantid
