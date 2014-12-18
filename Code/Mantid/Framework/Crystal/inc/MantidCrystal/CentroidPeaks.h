#ifndef MANTID_CRYSTAL_CENTROIDPEAKS_H_
#define MANTID_CRYSTAL_CENTROIDPEAKS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/EventWorkspace.h"

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
  CentroidPeaks();
  ~CentroidPeaks();

  /// Algorithm's name for identification
  virtual const std::string name() const { return "CentroidPeaks"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Find the centroid of single-crystal peaks in a 2D Workspace, in "
           "order to refine their positions.";
  }

  /// Algorithm's version for identification
  virtual int version() const { return 1; };
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Crystal"; }

private:
  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();
  void integrate();
  void integrateEvent();
  int findPixelID(std::string bankName, int col, int row);
  bool edgePixel(std::string bankName, int col, int row, int Edge);

  /// Input 2D Workspace
  API::MatrixWorkspace_sptr inWS;
  DataObjects::EventWorkspace_const_sptr eventW;
  Mantid::detid2index_map wi_to_detid_map;
};

} // namespace Mantid
} // namespace Crystal

#endif /* MANTID_CRYSTAL_CENTROIDPEAKS_H_ */
