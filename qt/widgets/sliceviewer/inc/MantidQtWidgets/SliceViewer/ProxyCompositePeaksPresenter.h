// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_SLICEVIEWER_PROXYCOMPOSITEPEAKSPRESENTER_H_
#define MANTID_SLICEVIEWER_PROXYCOMPOSITEPEAKSPRESENTER_H_

#include "MantidKernel/System.h"
#include "MantidQtWidgets/SliceViewer/CompositePeaksPresenter.h"
#include "MantidQtWidgets/SliceViewer/UpdateableOnDemand.h"
#include <QObject>
#include <boost/shared_ptr.hpp>

namespace MantidQt {
namespace SliceViewer {
/*---------------------------------------------------------
ProxyCompositePeaksPresenter

Proxy wrapper of the CompositePeaksPresenter. Allows the CompositePeaksPresenter
to
be used in situations where diluted power, via a restricted API is required.
----------------------------------------------------------*/
class EXPORT_OPT_MANTIDQT_SLICEVIEWER ProxyCompositePeaksPresenter
    : public QObject,
      public UpdateableOnDemand {
public:
  ProxyCompositePeaksPresenter(
      boost::shared_ptr<CompositePeaksPresenter> compositePresenter);
  ProxyCompositePeaksPresenter();
  ~ProxyCompositePeaksPresenter() override;
  size_t size() const;
  void update();

  void
  setForegroundColor(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws,
                     PeakViewColor /*color*/);
  /// Change the background representation for the peaks of this workspace
  void
  setBackgroundColor(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws,
                     PeakViewColor /*color*/);
  /// Get the foreground colour corresponding to the workspace
  PeakViewColor getForegroundPeakViewColor(
      boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws) const;
  /// Get the background colour corresponding to the workspace
  PeakViewColor getBackgroundPeakViewColor(
      boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws) const;
  /// Determine wheter the background is shown or not.
  bool getShowBackground(
      boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws) const;
  /// Get references to all presented workspaces.
  SetPeaksWorkspaces presentedWorkspaces() const;
  /// Gets the transform name.
  std::string getTransformName() const;
  /// Change whether the background radius is shown.
  void setBackgroundRadiusShown(
      boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws,
      const bool shown);
  /// Remove the workspace and corresponding presenter.
  void remove(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> peaksWS);
  /// Hide these peaks in the plot.
  void hideInPlot(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> peaksWS,
                  const bool hide);
  /// zoom in on a peak.
  void zoomToPeak(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> peaksWS,
                  const int peakIndex);
  /// sort the peaks workspace.
  void sortPeaksWorkspace(
      boost::shared_ptr<const Mantid::API::IPeaksWorkspace> peaksWS,
      const std::string &columnToSortBy, const bool sortedAscending);
  /// Get the named peaks presenter
  PeaksPresenter *getPeaksPresenter(const QString &name);
  /// Is the workspace hidden.
  bool getIsHidden(
      boost::shared_ptr<const Mantid::API::IPeaksWorkspace> peaksWS) const;
  /// Perform a requested update.
  void performUpdate() override;
  /// Perform a peaks workspace replacement
  void
  updatePeaksWorkspace(const std::string &toName,
                       boost::shared_ptr<const Mantid::API::IPeaksWorkspace>
                           toWorkspace) override;
  /// Register an updateable view
  void registerView(UpdateableOnDemand *view);
  /// Get optional zoomed peak presenter.
  boost::optional<PeaksPresenter_sptr> getZoomedPeakPresenter() const;
  /// Get optional zoomed peak index.
  int getZoomedPeakIndex() const;
  /// Set the edit mode.
  void editCommand(EditMode editMode,
                   boost::weak_ptr<const Mantid::API::IPeaksWorkspace> target);
  /// Set the peaks size within the current projection
  void setPeakSizeOnProjection(const double fraction);
  /// Set the peaks size into the current projection
  void setPeakSizeIntoProjection(const double fraction);
  /// Get the peaks size onto the current projection
  double getPeakSizeOnProjection() const;
  /// Get the peaks size into the current projection
  double getPeakSizeIntoProjection() const;

private:
  /// Wrapped composite to delegate to.
  boost::shared_ptr<CompositePeaksPresenter> m_compositePresenter;
  /// Register an assoicated view.
  UpdateableOnDemand *m_updateableView;
};
} // namespace SliceViewer
} // namespace MantidQt

#endif
