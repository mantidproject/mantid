#ifndef MANTID_SLICEVIEWER_PROXYCOMPOSITEPEAKSPRESENTER_H_
#define MANTID_SLICEVIEWER_PROXYCOMPOSITEPEAKSPRESENTER_H_

#include "MantidKernel/System.h"
#include "MantidQtSliceViewer/CompositePeaksPresenter.h"
#include "MantidQtSliceViewer/UpdateableOnDemand.h"
#include <boost/shared_ptr.hpp>
#include <QObject>

namespace MantidQt {
namespace SliceViewer {
/*---------------------------------------------------------
ProxyCompositePeaksPresenter

Proxy wrapper of the CompositePeaksPresenter. Allows the CompositePeaksPresenter
to
be used in situations where diluted power, via a restricted API is required.
----------------------------------------------------------*/
class DLLExport ProxyCompositePeaksPresenter : public QObject,
                                               public UpdateableOnDemand {
public:
  ProxyCompositePeaksPresenter(
      boost::shared_ptr<CompositePeaksPresenter> compositePresenter);
  ProxyCompositePeaksPresenter();
  ~ProxyCompositePeaksPresenter();
  size_t size() const;
  void update();

  void
  setForegroundColour(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws,
                      QColor);
  /// Change the background representation for the peaks of this workspace
  void
  setBackgroundColour(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws,
                      QColor);
  /// Get the foreground colour corresponding to the workspace
  QColor getForegroundColour(
      boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws) const;
  /// Get the background colour corresponding to the workspace
  QColor getBackgroundColour(
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
  void performUpdate();
  /// Perform a peaks workspace replacement
  void updatePeaksWorkspace(
      const std::string &toName,
      boost::shared_ptr<const Mantid::API::IPeaksWorkspace> toWorkspace);
  /// Register an updateable view
  void registerView(UpdateableOnDemand *view);
  /// Get optional zoomed peak presenter.
  boost::optional<PeaksPresenter_sptr> getZoomedPeakPresenter() const;
  /// Get optional zoomed peak index.
  int getZoomedPeakIndex() const;

private:
  /// Wrapped composite to delegate to.
  boost::shared_ptr<CompositePeaksPresenter> m_compositePresenter;
  /// Register an assoicated view.
  UpdateableOnDemand *m_updateableView;
};
}
}

#endif
