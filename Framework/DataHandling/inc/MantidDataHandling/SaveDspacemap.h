#ifndef MANTID_DATAHANDLING_SAVEDSPACEMAP_H_
#define MANTID_DATAHANDLING_SAVEDSPACEMAP_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace DataHandling {

/** Saves an OffsetsWorkspace into a POWGEN-format binary dspace map file.
 *
 * @author Janik Zikovsky (code from Vickie Lynch)
 * @date 2011-05-12
 */
class DLLExport SaveDspacemap : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "SaveDspacemap"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Saves an OffsetsWorkspace into a POWGEN-format binary dspace map "
           "file.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling\\Text"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;

  void
  CalculateDspaceFromCal(Mantid::DataObjects::OffsetsWorkspace_sptr offsetsWS,
                         std::string DFileName);
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_SAVEDSPACEMAP_H_ */
