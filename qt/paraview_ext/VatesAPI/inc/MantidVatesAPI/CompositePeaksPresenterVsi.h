// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_VATES_COMPOSITE_PEAKS_PRESENTER_VSI_H
#define MANTID_VATES_COMPOSITE_PEAKS_PRESENTER_VSI_H

#include "MantidAPI/IPeaksWorkspace_fwd.h"
#include "MantidGeometry/Crystal/PeakShape.h"
#include "MantidGeometry/Crystal/PeakTransform.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include "MantidKernel/System.h"
#include "MantidVatesAPI/PeaksPresenterVsi.h"
#include "MantidVatesAPI/ViewFrustum.h"
#include <string>
#include <vector>

namespace Mantid {
namespace VATES {
class DLLExport CompositePeaksPresenterVsi : public PeaksPresenterVsi {
public:
  Mantid::API::IPeaksWorkspace_sptr getPeaksWorkspace() const override {
    throw std::runtime_error(
        "The composite peaks presenter has no single peaks workspace.");
  }
  std::vector<Mantid::API::IPeaksWorkspace_sptr> getPeaksWorkspaces() const;
  std::vector<bool> getViewablePeaks() const override;
  void updateViewFrustum(ViewFrustum_const_sptr frustum) override;
  std::string getFrame() const override;
  std::string getPeaksWorkspaceName() const override {
    throw std::runtime_error(
        "The composite peaks presenter has no peaks workspace");
  }
  std::vector<std::string> getPeaksWorkspaceNames() const;
  void getPeaksInfo(Mantid::API::IPeaksWorkspace_sptr peaksWorkspace, int row,
                    Mantid::Kernel::V3D &position, double &radius,
                    Mantid::Kernel::SpecialCoordinateSystem
                        specialCoordinateSystem) const override;
  void addPresenter(PeaksPresenterVsi_sptr presenter);
  std::map<std::string, std::vector<bool>> getInitializedViewablePeaks() const;
  void removePresenter(const std::string &peaksWorkspaceName);
  void updateWorkspaces(const std::vector<std::string> &peaksWorkspaceNames);
  void sortPeaksWorkspace(const std::string &, const bool) override {}
  void sortPeaksWorkspace(const std::string &columnToSortBy,
                          const bool sortedAscending,
                          const Mantid::API::IPeaksWorkspace_sptr peaksWS);
  bool hasPeaks();

private:
  /// The list of presenters
  std::vector<PeaksPresenterVsi_sptr> m_peaksPresenters;
};
} // namespace VATES
} // namespace Mantid
#endif
