#ifndef MANTID_DATAHANDLING_SAVECALFILE_H_
#define MANTID_DATAHANDLING_SAVECALFILE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace DataHandling {

/** Algorithm to save a 5-column ascii .cal file from  to 3 workspaces:
 * a GroupingWorkspace, OffsetsWorkspace and/or MaskWorkspace.
 *
 * @author
 * @date 2011-05-10 09:48:31.796980
 */
class DLLExport SaveCalFile : public API::Algorithm {
public:
  SaveCalFile();
  ~SaveCalFile();

  /// Algorithm's name for identification
  virtual const std::string name() const { return "SaveCalFile"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Saves a 5-column ASCII .cal file from up to 3 workspaces: a "
           "GroupingWorkspace, OffsetsWorkspace and/or MaskWorkspace.";
  }

  /// Algorithm's version for identification
  virtual int version() const { return 1; };
  /// Algorithm's category for identification
  virtual const std::string category() const {
    return "DataHandling\\Text;Diffraction";
  }

  void saveCalFile(const std::string &calFileName,
                   Mantid::DataObjects::GroupingWorkspace_sptr groupWS,
                   Mantid::DataObjects::OffsetsWorkspace_sptr offsetsWS,
                   Mantid::DataObjects::MaskWorkspace_sptr maskWS);

private:
  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();

  /// Offset precision
  int m_precision;
};

} // namespace Mantid
} // namespace DataHandling

#endif /* MANTID_DATAHANDLING_SAVECALFILE_H_ */
