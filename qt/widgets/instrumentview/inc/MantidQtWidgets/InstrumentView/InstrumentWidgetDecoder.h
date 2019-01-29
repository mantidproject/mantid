// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_WIDGETS_INSTRUMENTWIDGETDECODER_H_
#define MANTIDQT_WIDGETS_INSTRUMENTWIDGETDECODER_H_

#include "MantidQtWidgets/InstrumentView/ColorBar.h"
#include "MantidQtWidgets/InstrumentView/InstrumentActor.h"
#include "MantidQtWidgets/InstrumentView/InstrumentTreeWidget.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetMaskTab.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetPickTab.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetRenderTab.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetTreeTab.h"
#include "MantidQtWidgets/InstrumentView/MaskBinsData.h"
#include "MantidQtWidgets/InstrumentView/Projection3D.h"
#include "MantidQtWidgets/InstrumentView/ProjectionSurface.h"
#include "MantidQtWidgets/InstrumentView/Shape2D.h"
#include "MantidQtWidgets/InstrumentView/Shape2DCollection.h"
#include "MantidQtWidgets/InstrumentView/Viewport.h"

#include <QList>
#include <QMap>
#include <QObject>
#include <QString>

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW InstrumentWidgetDecoder
    : public QObject {
  Q_OBJECT
public:
  InstrumentWidgetDecoder();
  void decode(const QMap<QString, QVariant> &map, InstrumentWidget &obj,
              const QString &projectPath, const bool loadMask = true);

signals:
  void shapeCreated();

private:
  void decodeTabs(const QMap<QString, QVariant> &map, InstrumentWidget &obj);

  void decodeMaskTab(const QMap<QString, QVariant> &map,
                     InstrumentWidgetMaskTab *obj);

  void decodeRenderTab(const QMap<QString, QVariant> &map,
                       InstrumentWidgetRenderTab *obj);
  void decodeColorBar(const QMap<QString, QVariant> &map, ColorBar *bar);

  void decodeTreeTab(const QMap<QString, QVariant> &map,
                     InstrumentWidgetTreeTab *obj);

  void decodePickTab(const QMap<QString, QVariant> &map,
                     InstrumentWidgetPickTab *obj);

  void decodeActor(const QMap<QString, QVariant> &map,
                   std::unique_ptr<InstrumentActor> &obj);
  void decodeBinMasks(const QList<QVariant> &list, MaskBinsData &obj);
  void decodeSurface(const QMap<QString, QVariant> &map,
                     boost::shared_ptr<ProjectionSurface> obj);
  void decodeProjection3D(const QMap<QString, QVariant> &map,
                          Projection3D &obj);
  void decodeViewPort(const QMap<QString, QVariant> &map, Viewport &obj);
  void decodeMaskShapes(const QList<QVariant> &list, Shape2DCollection &obj);

  Shape2D *decodeShape(const QMap<QString, QVariant> &map);
  Shape2D *decodeEllipse(const QMap<QString, QVariant> &map);
  Shape2D *decodeRectangle(const QMap<QString, QVariant> &map);
  Shape2D *decodeRing(const QMap<QString, QVariant> &map);
  Shape2D *decodeFree(const QMap<QString, QVariant> &map);

  void decodeAlignmentInfo(const QList<QVariant> &list,
                           boost::shared_ptr<ProjectionSurface> &obj);

  QString m_projectPath;
  QString m_workspaceName;
  bool m_loadMask;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /*MANTIDQT_WIDGETS_INSTRUMENTWIDGETDECODER_H_*/