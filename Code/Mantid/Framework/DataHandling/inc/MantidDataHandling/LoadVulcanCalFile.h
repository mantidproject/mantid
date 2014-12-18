#ifndef MANTID_DATAHANDLING_LOADVULCANCALFILE_H_
#define MANTID_DATAHANDLING_LOADVULCANCALFILE_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/System.h"

#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid {
namespace DataHandling {

enum VULCAN_OFFSET_LEVEL {
  VULCAN_OFFSET_BANK,
  VULCAN_OFFSET_MODULE,
  VULCAN_OFFSET_STACK
};

/** Algorithm to load a 5-column ascii .cal file into up to 3 workspaces:
 * a GroupingWorkspace, OffsetsWorkspace and/or MaskWorkspace.
 */
class DLLExport LoadVulcanCalFile : public API::Algorithm {
public:
  LoadVulcanCalFile();
  ~LoadVulcanCalFile();

  /// Algorithm's name for identification
  virtual const std::string name() const { return "LoadVulcanCalFile"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Loads set of VULCAN's offset files into up to 3 workspaces: a "
           "GroupingWorkspace, OffsetsWorkspace and/or MaskWorkspace.";
  }

  /// Algorithm's version for identification
  virtual int version() const { return 1; }
  /// Algorithm's category for identification
  virtual const std::string category() const {
    return "DataHandling\\Text;Diffraction";
  }

  void getInstrument3WaysInit();

  Geometry::Instrument_const_sptr getInstrument();

  static void readCalFile(const std::string &calFileName,
                          Mantid::DataObjects::GroupingWorkspace_sptr groupWS,
                          Mantid::DataObjects::OffsetsWorkspace_sptr offsetsWS,
                          Mantid::DataObjects::MaskWorkspace_sptr maskWS);

private:
  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();

  void processInOutProperites();

  /// Set up grouping workspace
  void setupGroupingWorkspace();

  /// Set up mask workspace
  void setupMaskWorkspace();

  void generateOffsetsWorkspace();

  /// Read VULCAN's offset file
  void readOffsetFile(std::map<detid_t, double> &map_detoffset);

  void processOffsets(std::map<detid_t, double> map_detoffset);

  /// Convert offsets from VUCLAN's offset to Mantid's
  void convertOffsets();

  Geometry::Instrument_const_sptr m_instrument;

  /// Type of grouping
  VULCAN_OFFSET_LEVEL m_groupingType;

  std::string m_offsetFilename;
  std::string m_badPixFilename;

  DataObjects::OffsetsWorkspace_sptr m_tofOffsetsWS;
  DataObjects::OffsetsWorkspace_sptr m_offsetsWS;
  DataObjects::GroupingWorkspace_sptr m_groupWS;
  DataObjects::MaskWorkspace_sptr m_maskWS;

  // Verification tool
  void alignEventWorkspace();

  bool m_doAlignEventWS;
  DataObjects::EventWorkspace_sptr m_eventWS;

  // Map for bank: eff_L and theta (in degree)
  std::map<int, std::pair<double, double>> m_effLTheta;
};

} // namespace Mantid
} // namespace DataHandling

#endif /* MANTID_DATAHANDLING_LOADVULCANCALFILE_H_ */
