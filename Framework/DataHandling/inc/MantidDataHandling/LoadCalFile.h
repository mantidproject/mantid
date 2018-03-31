#ifndef MANTID_DATAHANDLING_LOADCALFILE_H_
#define MANTID_DATAHANDLING_LOADCALFILE_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace DataHandling {

/** Algorithm to load a 5-column ascii .cal file into up to 3 workspaces:
 * a GroupingWorkspace, OffsetsWorkspace and/or MaskWorkspace.
 *
 * @author Janik Zikovsky
 * @date 2011-05-09
 */
class DLLExport LoadCalFile : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "LoadCalFile"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads a 5-column ASCII .cal file into up to 3 workspaces: a "
           "GroupingWorkspace, OffsetsWorkspace and/or MaskWorkspace.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  /// Algorithm's category for identification
  const std::string category() const override {
    return R"(DataHandling\Text;Diffraction\DataHandling\CalFiles)";
  }

  static void getInstrument3WaysInit(Mantid::API::Algorithm *alg);

  static Geometry::Instrument_const_sptr
  getInstrument3Ways(API::Algorithm *alg);
  static bool instrumentIsSpecified(API::Algorithm *alg);

  static void readCalFile(const std::string &calFileName,
                          Mantid::DataObjects::GroupingWorkspace_sptr groupWS,
                          Mantid::DataObjects::OffsetsWorkspace_sptr offsetsWS,
                          Mantid::DataObjects::MaskWorkspace_sptr maskWS);

protected:
  Parallel::ExecutionMode getParallelExecutionMode(
      const std::map<std::string, Parallel::StorageMode> &storageModes)
      const override;

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;

  /// Checks if a detector ID is for a monitor on a given instrument
  static bool idIsMonitor(Mantid::Geometry::Instrument_const_sptr inst,
                          int detID);
};

} // namespace Mantid
} // namespace DataHandling

#endif /* MANTID_DATAHANDLING_LOADCALFILE_H_ */
