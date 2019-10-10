// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef CREATE_DETECTOR_TABLE_H_
#define CREATE_DETECTOR_TABLE_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/UnitConversion.h"

namespace Mantid {
namespace Algorithm {

class CreateDetectorTable : public Mantid::API::Algorithm {
public:
  /// (Empty) Constructor
  CreateDetectorTable() : Mantid::API::Algorithm() {}
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

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  /// Creates table workspace of detector information from a given workspace
  API::ITableWorkspace_sptr CreateDetectorTable::createDetectorTableWorkspace(
      const API::MatrixWorkspace_sptr &ws, const std::vector<int> &indices,
      bool include_data);

  /// Converts a list to a string, shortened if necessary
  std::string
  CreateDetectorTable::createTruncatedList(const std::set<int> &elements);
};

} // namespace Algorithm
} // namespace Mantid

#endif /*CREATE_DETECTOR_TABLE_H_*/
