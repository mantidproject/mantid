#ifndef MANTID_CRYSTAL_SAVEISAWPEAKS_H_
#define MANTID_CRYSTAL_SAVEISAWPEAKS_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Crystal {

/** Save a PeaksWorkspace to a ISAW-style ASCII .peaks file.
 *
 * @author Janik Zikovsky
 * @date 2011-05-25
 */
class DLLExport SaveIsawPeaks : public API::Algorithm {
public:
  SaveIsawPeaks();
  ~SaveIsawPeaks();

  /// Algorithm's name for identification
  virtual const std::string name() const { return "SaveIsawPeaks"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Save a PeaksWorkspace to a ISAW-style ASCII .peaks file.";
  }

  /// Algorithm's version for identification
  virtual int version() const { return 1; };
  /// Algorithm's category for identification
  virtual const std::string category() const {
    return "Crystal;DataHandling\\Isaw";
  }

private:
  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();
  /// find position for rectangular and non-rectangular
  Kernel::V3D findPixelPos(std::string bankName, int col, int row);
  void sizeBanks(std::string bankName, int &NCOLS, int &NROWS, double &xsize,
                 double &ysize);
  Geometry::Instrument_const_sptr inst;
};

} // namespace Mantid
} // namespace Crystal

#endif /* MANTID_CRYSTAL_SAVEISAWPEAKS_H_ */
