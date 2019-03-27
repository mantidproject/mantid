// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef PEAKSVIEWER_H
#define PEAKSVIEWER_H

#include "DllOption.h"
#include "MantidAPI/IPeaksWorkspace_fwd.h"
#include "MantidQtWidgets/SliceViewer/PeaksPresenter.h"
#include "MantidQtWidgets/SliceViewer/UpdateableOnDemand.h"
#include <QWidget>
#include <boost/shared_ptr.hpp>

namespace Mantid {
namespace API {
class IPeaksWorkspace;
}
} // namespace Mantid

namespace MantidQt {
namespace SliceViewer {
/// Forward dec.
class ProxyCompositePeaksPresenter;
class PeaksWorkspaceWidget;

/**

*/
class EXPORT_OPT_MANTIDQT_SLICEVIEWER PeaksViewer : public QWidget,
                                                    public UpdateableOnDemand {
  Q_OBJECT
public:
  PeaksViewer(QWidget *parent = nullptr);
  void setPeaksWorkspaces(const SetPeaksWorkspaces &workspaces);
  void setPresenter(boost::shared_ptr<ProxyCompositePeaksPresenter> presenter);
  void performUpdate() override;
  void
  updatePeaksWorkspace(const std::string &toName,
                       boost::shared_ptr<const Mantid::API::IPeaksWorkspace>
                           toWorkspace) override;
  bool removePeaksWorkspace(
      boost::shared_ptr<const Mantid::API::IPeaksWorkspace> toRemove);
  bool removePeaksWorkspace(const std::string &toRemove);
  void hide();
  ~PeaksViewer() override;
  bool hasThingsToShow() const;
  void clearPeaksModeRequest(PeaksWorkspaceWidget const *const originWidget,
                             const bool on);
  void addPeaksModeRequest(PeaksWorkspaceWidget const *const originWidget,
                           const bool on);
  /// Load the state of the peaks viewer from a Mantid project file
  void loadFromProject(const std::string &lines);
  /// Save the state of the peaks viewer to a Mantid project file
  std::string saveToProject() const;

public slots:
  void onPeakColorChanged(Mantid::API::IPeaksWorkspace_const_sptr /*peaksWS*/,
                          PeakViewColor /*newColor*/);
  void onBackgroundColorChanged(Mantid::API::IPeaksWorkspace_const_sptr /*peaksWS*/,
                                PeakViewColor /*newColor*/);
  void onBackgroundRadiusShown(Mantid::API::IPeaksWorkspace_const_sptr /*peaksWS*/, bool /*show*/);
  void onRemoveWorkspace(Mantid::API::IPeaksWorkspace_const_sptr /*peaksWS*/);
  void onHideInPlot(Mantid::API::IPeaksWorkspace_const_sptr peaksWS, bool /*hide*/);
  void onZoomToPeak(Mantid::API::IPeaksWorkspace_const_sptr peaksWS,
                    int peakIndex);
  void showPeaksTableColumnOptions();

private:
  /// Get visible columns on the peaks viewer
  std::set<QString> getVisibleColumns() const;
  /// Set visible columns on the peaks viewer
  void setVisibleColumns(const std::set<QString> &columns);
  /// Show a dialog to choose visible columns
  std::set<QString>
  chooseVisibleColumnsFromDialog(const std::set<QString> &columns);
  /// Get a peaks workspaces from the ADS
  Mantid::API::IPeaksWorkspace_const_sptr
  getPeaksWorkspace(const std::string &name) const;
  /// Load a presented peaks workspace and settings from a project file
  void loadPresentedWorkspace(const std::string &section);
  /// Save a presented peaks workspace and settings to a project file
  std::string
  savePresentedWorkspace(Mantid::API::IPeaksWorkspace_const_sptr ws) const;

  boost::shared_ptr<ProxyCompositePeaksPresenter> m_presenter;
};

} // namespace SliceViewer
} // namespace MantidQt
#endif // PEAKSVIEWER_H
