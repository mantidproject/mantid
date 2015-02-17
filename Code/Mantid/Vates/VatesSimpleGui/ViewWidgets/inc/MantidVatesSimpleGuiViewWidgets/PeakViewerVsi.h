#ifndef PEAKSVIEWERVSI_H_
#define PEAKSVIEWERVSI_H_

#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"
#include "MantidVatesSimpleGuiViewWidgets/CameraManager.h"
#include "MantidVatesSimpleGuiViewWidgets/PeaksWidget.h"
#include "MantidVatesAPI/PeaksPresenterVsi.h"
#include "MantidAPI/PeakTransformSelector.h"
#include "MantidGeometry/Crystal/PeakShape.h"
#include <QWidget>
#include <QPointer>
#include <boost/shared_ptr.hpp>

class pqPipelineSource;

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{
class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS PeaksViewerVsi : public QWidget
{
  Q_OBJECT
public:
  PeaksViewerVsi(boost::shared_ptr<CameraManager> cameraManager, QWidget *parent=0);
  void addWorkspace(pqPipelineSource* source, QPointer<pqPipelineSource> splatSource);
  std::vector<bool> getViewablePeaks();
  bool hasPeaks();
  void showTable();
  void showFullTable();
  void removeTable();

public slots:
  void updateViewableArea();
  void onZoomToPeak(Mantid::API::IPeaksWorkspace_sptr peaksWorkspace, int row);

private:
  std::vector<std::string> extractFrameFromSource(QPointer<pqPipelineSource> splatSource);
  bool checkMatchingSources(pqPipelineSource* source, QPointer<pqPipelineSource> splatSource);
  double getMaxRadius(Mantid::Geometry::PeakShape_sptr shape);
  void removeLayout(QWidget *widget);
  void createTable(bool full);
  boost::shared_ptr<CameraManager> m_cameraManager;
  boost::shared_ptr<Mantid::VATES::PeaksPresenterVsi> m_presenter;
  /// Object for choosing a PeakTransformFactory based on the workspace type.
  Mantid::API::PeakTransformSelector m_peakTransformSelector;
  PeaksWidget* m_peaksWidget;
};

}
}
}
#endif