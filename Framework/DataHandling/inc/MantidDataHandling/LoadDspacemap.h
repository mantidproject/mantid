#ifndef MANTID_DATAHANDLING_LOADDSPACEMAP_H_
#define MANTID_DATAHANDLING_LOADDSPACEMAP_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidGeometry/IDetector.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace DataHandling {

/** Loads a Dspacemap file (POWGEN binary, VULCAN binary or ascii format) into
 *an OffsetsWorkspace.
 *
 * @author Janik Zikovsky (code from Vickie Lynch)
 * @date 2011-05-10
 */
class DLLExport LoadDspacemap : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "LoadDspacemap"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads a Dspacemap file (POWGEN binary, VULCAN binary or ascii "
           "format) into an OffsetsWorkspace.";
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

  void readVulcanAsciiFile(const std::string &fileName,
                           std::map<detid_t, double> &vulcan);
  void readVulcanBinaryFile(const std::string &fileName,
                            std::map<detid_t, double> &vulcan);

  void CalculateOffsetsFromDSpacemapFile(
      const std::string DFileName,
      Mantid::DataObjects::OffsetsWorkspace_sptr offsetsWS);

  void CalculateOffsetsFromVulcanFactors(
      std::map<detid_t, double> &vulcan,
      Mantid::DataObjects::OffsetsWorkspace_sptr offsetsWS);
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADDSPACEMAP_H_ */
