#ifndef PeaksTableControllerVSI_H_
#define PeaksTableControllerVSI_H_

#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"
#include "MantidVatesSimpleGuiViewWidgets/CameraManager.h"
#include "MantidVatesSimpleGuiViewWidgets/PeaksTabWidget.h"
#include "MantidVatesAPI/CompositePeaksPresenterVsi.h"
#include "MantidAPI/PeakTransformSelector.h"
#include "MantidGeometry/Crystal/PeakShape.h"
#include "MantidQtSliceViewer/PeakPalette.h"
#include "MantidKernel/SpecialCoordinateSystem.h"

#include <map>
#include <QWidget>
#include <qcolor.h>
#include <QPointer>
#include <boost/shared_ptr.hpp>

class pqPipelineSource;

namespace Mantid {
namespace Vates {
namespace SimpleGui {
class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS PeaksTableControllerVsi
    : public QWidget {
  Q_OBJECT
public:
  PeaksTableControllerVsi(boost::shared_ptr<CameraManager> cameraManager,
                          QWidget *parent = 0);
  ~PeaksTableControllerVsi();
  std::vector<bool> getViewablePeaks();
  bool hasPeaks();
  void showFullTable();
  void removeTable();
  std::string getConcatenatedWorkspaceNames(std::string delimiter);
  void updatePeaksWorkspaces(QList<QPointer<pqPipelineSource>> peakSources,
                             pqPipelineSource *splatSource);
signals:
  void setRotationToPoint(double x, double y, double z);
public slots:
  void updateViewableArea();
  void onZoomToPeak(Mantid::API::IPeaksWorkspace_sptr peaksWorkspace, int row);
  void onPeaksSorted(const std::string &columnToSortBy,
                     const bool sortAscending,
                     Mantid::API::IPeaksWorkspace_sptr ws);
  void destroySinglePeakSource();
  void onPeakMarkerDestroyed();

private:
  void addWorkspace(pqPipelineSource *source,
                    QPointer<pqPipelineSource> splatSource);
  std::vector<std::string>
  extractFrameFromSource(QPointer<pqPipelineSource> splatSource);
  void generateSinglePeaksSource(double position1, double position2,
                                 double position3, double radius);
  void resetSinglePeaksSource(double position1, double position2,
                              double position3, double radius);
  bool checkMatchingSources(pqPipelineSource *source,
                            QPointer<pqPipelineSource> splatSource);
  double getMaxRadius(Mantid::Geometry::PeakShape_sptr shape);
  void removeLayout(QWidget *widget);
  void createTable(bool full);
  void updatePeakWorkspaceColor();
  std::map<std::string, QColor> getColors();
  MantidQt::SliceViewer::PeakPalette m_peakPalette;
  boost::shared_ptr<CameraManager> m_cameraManager;
  boost::shared_ptr<Mantid::VATES::CompositePeaksPresenterVsi> m_presenter;
  /// Object for choosing a PeakTransformFactory based on the workspace type.
  Mantid::API::PeakTransformSelector m_peakTransformSelector;
  PeaksTabWidget *m_peaksTabWidget;
  pqPipelineSource *m_peakMarker;
  Mantid::Kernel::SpecialCoordinateSystem m_coordinateSystem;
};
}
}
}
#endif