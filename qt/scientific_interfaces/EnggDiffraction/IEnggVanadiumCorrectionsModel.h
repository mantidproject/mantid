#ifndef MANTIDQT_CUSTOMINTERFACES_IENGGVANADIUMCORRECTIONSMODEL_H_
#define MANTIDQT_CUSTOMINTERFACES_IENGGVANADIUMCORRECTIONSMODEL_H_

#include "EnggDiffCalibSettings.h"

#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

namespace MantidQt {
namespace CustomInterfaces {

/// Model for calculating and caching vanadium correction feature workspaces
/// using EnggVanadiumCorrections. This functionality is used by both the
/// Calibrate tab and the Focus tab
class IEnggVanadiumCorrectionsModel {

public:
  virtual ~IEnggVanadiumCorrectionsModel() = default;

  virtual void setCalibSettings(const EnggDiffCalibSettings &calibSettings) = 0;

  virtual void setCurrentInstrument(const std::string &currentInstrument) = 0;

  /// Get the vanadium correction workspaces. If workspaces for this run have
  /// been calculated before then load them, calculate them if not
  virtual std::pair<Mantid::API::ITableWorkspace_sptr,
                    Mantid::API::MatrixWorkspace_sptr>
  fetchCorrectionWorkspaces(const std::string &vanadiumRunNumber) const = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQT_CUSTOMINTERFACES_IENGGVANADIUMCORRECTIONSMODEL_H_
