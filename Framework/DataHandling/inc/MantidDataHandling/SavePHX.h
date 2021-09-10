// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace DataHandling {
/**
 *  Saves a workspace into an ASCII PHX file.
 *
 *   Required properties:
 *    <UL>
 *    <LI> InputWorkspace - The workspace name to save. </LI>
 *    <LI> Filename - The filename for output </LI>
 *    </UL>
 *
 *     @author Stuart Campbell, NScD, Oak Ridge National Laboratory
 *     @date 27/07/2010
 *
 *
 *
 */

class DLLExport SavePHX : public Mantid::API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "SavePHX"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Writes the detector geometry information of a workspace into a PHX "
           "format file.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"SaveSPE"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling\\SPE;Inelastic\\DataHandling"; }

  /** the method used in tests. It requested the ChildAlgorithm, which does the
     detectors
     *  position calculations to produce a target workspace. This workspace then
     can be retrieved
        from analysis data service and used to check  the results of the save
     algorithm. */
  void set_resulting_workspace(const std::string &ws_name) { det_par_ws_name = ws_name; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  /// The name of the table workpsace with detectors positions used in tests
  std::string det_par_ws_name;
};
} // namespace DataHandling
} // namespace Mantid
