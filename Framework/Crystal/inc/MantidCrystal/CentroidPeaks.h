#ifndef MANTID_CRYSTAL_CENTROIDPEAKS_H_
#define MANTID_CRYSTAL_CENTROIDPEAKS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Crystal {

/** Find the centroid of single-crystal peaks in a 2D Workspace, in order to
 *refine their positions.
 *
 * @author Janik Zikovsky
 * @date 2011-06-01
 */
class DLLExport CentroidPeaks : public API::Algorithm {
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
  const std::vector<std::string> seeAlso() const override {
    return {"CentroidPeaksMD", "PeakIntegration"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "Crystal\\Peaks"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
  void integrate();
  void integrateEvent();
  int findPixelID(std::string bankName, int col, int row);
  void removeEdgePeaks(Mantid::DataObjects::PeaksWorkspace &peakWS);
  void sizeBanks(const std::string &bankName, int &nCols, int &nRows);
  Geometry::Instrument_const_sptr inst;

  /// Input 2D Workspace
  API::MatrixWorkspace_sptr inWS;
  DataObjects::EventWorkspace_const_sptr eventW;
  Mantid::detid2index_map wi_to_detid_map;
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_CENTROIDPEAKS_H_ */
