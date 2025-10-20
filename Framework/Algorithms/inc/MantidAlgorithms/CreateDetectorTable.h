// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/EnabledWhenWorkspaceIsType.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/UnitConversion.h"

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
  API::ITableWorkspace_sptr createDetectorTableWorkspace();
  std::vector<std::pair<std::string, std::string>> createColumns();
  void populateTable();
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
  bool signedThetaParamRetrieved;
  bool showSignedTwoTheta; // If true, signedVersion of the two theta
  Geometry::PointingAlong beamAxisIndex;
  double sampleDist;
  int nrows;
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

/// Converts a list to a string, shortened if necessary
std::string createTruncatedList(const std::set<int> &elements);

} // namespace Algorithms
} // namespace Mantid
