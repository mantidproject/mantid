// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidCrystal/DllConfig.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"

namespace Mantid {
namespace Crystal {

/** Find the centroid of single-crystal peaks in a 2D Workspace, in order to
 *refine their positions.
 *
 * @author Janik Zikovsky
 * @date 2011-06-01
 */
class MANTID_CRYSTAL_DLL CentroidPeaks final : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "CentroidPeaks"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Find the centroid of single-crystal peaks in a 2D Workspace, in "
           "order to refine their positions.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"CentroidPeaksMD", "PeakIntegration"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Crystal\\Peaks"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
  void integrate();
  void integrateEvent();
  int findPixelID(const std::string &bankName, int col, int row);
  void removeEdgePeaks(Mantid::DataObjects::PeaksWorkspace &peakWS);
  void sizeBanks(const std::string &bankName, int &nCols, int &nRows);
  Geometry::Instrument_const_sptr m_inst;

  /// Input 2D Workspace
  API::MatrixWorkspace_sptr m_inWS;
  DataObjects::EventWorkspace_const_sptr m_eventW;
  Mantid::detid2index_map m_wi_to_detid_map;
};

} // namespace Crystal
} // namespace Mantid
