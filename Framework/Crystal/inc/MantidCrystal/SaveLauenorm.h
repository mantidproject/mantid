#ifndef MANTID_CRYSTAL_SAVELauenorm_H_
#define MANTID_CRYSTAL_SAVELauenorm_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

namespace Mantid {
namespace Crystal {

/** Save a PeaksWorkspace to a lauenorm format
 * http://www.ccp4.ac.uk/cvs/viewvc.cgi/laue/doc/lauenorm.ptx?diff_format=s&revision=1.1.1.1&view=markup
 *
 * @author Vickie Lynch, SNS
 * @date 2014-07-24
 */

class DLLExport SaveLauenorm : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "SaveLauenorm"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Save a PeaksWorkspace to a ASCII file for each detector.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  /// Algorithm's category for identification
  const std::string category() const override {
    return "Crystal\\DataHandling;DataHandling\\Text";
  }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;

  DataObjects::PeaksWorkspace_sptr ws;
  void sizeBanks(std::string bankName, int &nCols, int &nRows);

  const std::vector<std::string> m_typeList{
      "TRICLINIC", "MONOCLINIC",   "ORTHORHOMBIC", "TETRAGONAL",
      "HEXAGONAL", "RHOMBOHEDRAL", "CUBIC"};

  const std::vector<std::string> m_centeringList{"P", "A", "B", "C",
                                                 "I", "F", "R"};
};

} // namespace Mantid
} // namespace Crystal

#endif /* MANTID_CRYSTAL_SAVELauenorm_H_ */
