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
  bool PickOneDetectorID;
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
  size_t nrows;
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  void get_spherical_coordinates(size_t wsIndex, double &R, double &theta, double &phi);
  const std::string get_time_indexes(size_t wsIndex);
  double get_q(size_t wsIndex);
  void get_diff_consts(size_t wsIndex, double &difa, double &difc, double &difcUnc, double &tzero);
  void writeRowToTable(const size_t row, const size_t wsIndex, const int specNo, const std::set<int> &detIds,
                       const std::string timeIndexes, const double dataY0, const double dataE0, const double R,
                       const double theta, const double q, const double phi, const std::string isMonitor,
                       const double difa, const double difc, const double difcUnc, const double tzero,
                       const Kernel::V3D detPosition);

  void calculateWsIdxData(const size_t wsIndex, int &specNo, std::set<int> &detIds, std::string &timeIndexes,
                          double &dataY0, double &dataE0, double &R, double &theta, double &q, double &phi,
                          std::string &isMonitor, double &difa, double &difc, double &difcUnc, double &tzero,
                          Kernel::V3D &detPosition);
};

/// Converts a list to a string, shortened if necessary
std::string createTruncatedList(const std::set<int> &elements);

} // namespace Algorithms
} // namespace Mantid
