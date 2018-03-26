#ifndef MANTID_CRYSTAL_SAVEISAWPEAKS_H_
#define MANTID_CRYSTAL_SAVEISAWPEAKS_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"

namespace Mantid {

namespace Kernel {
class V3D;
}

namespace Crystal {

/** Save a PeaksWorkspace to a ISAW-style ASCII .peaks file.
 *
 * @author Janik Zikovsky
 * @date 2011-05-25
 */
class DLLExport SaveIsawPeaks : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "SaveIsawPeaks"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Save a PeaksWorkspace to a ISAW-style ASCII .peaks file.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"LoadIsawPeaks"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "Crystal\\DataHandling;DataHandling\\Isaw";
  }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
  /// find position for rectangular and non-rectangular
  Kernel::V3D findPixelPos(std::string bankName, int col, int row);
  void sizeBanks(std::string bankName, int &NCOLS, int &NROWS, double &xsize,
                 double &ysize);
  bool bankMasked(Geometry::IComponent_const_sptr parent,
                  const Geometry::DetectorInfo &detectorInfo);
  Geometry::Instrument_const_sptr inst;
};

} // namespace Mantid
} // namespace Crystal

#endif /* MANTID_CRYSTAL_SAVEISAWPEAKS_H_ */
