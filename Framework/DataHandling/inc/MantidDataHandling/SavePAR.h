// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidDataHandling/DllConfig.h"

namespace Mantid {
namespace DataHandling {
/**
 *  Saves a workspace into an ASCII PAR file.
 *
 *   Required properties:
 *    <UL>
 *    <LI> InputWorkspace - The workspace name to save. </LI>
 *    <LI> Filename - The filename for output </LI>
 *    </UL>
 *
 *     @author Alex Buts; after save PHX Sturat Cambell, ScD, Oak Ridge National
 *Laboratory
 *     @date 15/09/2011
 *
 *
 *
 */
/*!
 *    an ASCII Tobyfit par file format:
 *
 *     par(6,ndet)         contents of array
 *
 *         1st column      sample-detector distance \n
 *         2nd  &quot;          scattering angle (deg) \n
 *         3rd  &quot;          azimuthal angle (deg) \n
 *                     (west bank = 0 deg, north bank = -90 deg etc.)
 *                     (Note the reversed sign convention cf .phx files) \n
 *         4th  &quot;          width (m) \n
 *         5th  &quot;          height (m) \n
 *         6th  &quot;          DetID (m) \n
 *-----------------------------------------------------------------------
 */
class MANTID_DATAHANDLING_DLL SavePAR : public Mantid::API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "SavePAR"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Writes the detector geometry information of a workspace into a "
           "Tobyfit PAR format file.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"SaveSPE"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling\\SPE;Inelastic\\DataHandling"; }
  /** the method used in tests. It requested the ChildAlgorithm, which does the
   detectors
   *  position calculations to produce a target workspace This workspace then
   can be retrieved
      from analysis data service and used to check  the results of the save
   algorithm. */
  void set_resulting_workspace(const std::string &ws_name) { det_par_ws_name = ws_name; }

  static void writePAR(const std::string &filename, const std::vector<double> &azimuthal,
                       const std::vector<double> &polar, const std::vector<double> &azimuthal_width,
                       const std::vector<double> &polar_width, const std::vector<double> &secondary_flightpath,
                       const std::vector<size_t> &det_ID, const size_t nDetectors);

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
