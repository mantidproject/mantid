#ifndef MANTIDQT_WIDGETS_INSTRUMENTVIEW_MINIPLOTCONTROLLER_H
#define MANTIDQT_WIDGETS_INSTRUMENTVIEW_MINIPLOTCONTROLLER_H
/*
 Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
 National Laboratory & European Spallation Source

 This file is part of Mantid.

 Mantid is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 Mantid is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
*/
#include <QObject>
#include <QPoint>
#include <vector>

class QAction;
class QActionGroup;
class QContextMenuEvent;
class QSettings;
class QSignalMapper;

namespace Mantid {
namespace Kernel {
class V3D;
}
namespace Geometry {
class ICompAssembly;
class IPeak;
}
}

namespace MantidQt {
namespace MantidWidgets {
class InstrumentActor;
class InstrumentWidget;
class MiniPlot;
struct MiniPlotCurveData;
class PeakMarker2D;

/**
 * A controller class that contains logic to display the miniplot
 */
class MiniPlotController : public QObject {
  Q_OBJECT

public:
  enum class PlotType { Single = 0, DetectorSum, TubeSum, TubeIntegral };
  enum class TubeXUnits : int {
    DETECTOR_ID = 0,
    LENGTH,
    PHI,
    OUT_OF_PLANE_ANGLE,
    NUMBER_OF_UNITS
  };

  MiniPlotController(InstrumentWidget *instrWidget, MiniPlot *miniplot);
  void loadSettings(const QSettings &settings);
  void saveSettings(QSettings &settings) const;
  void setEnabled(bool on) { m_enabled = on; }
  void setPlotData(size_t pickID);
  void setPlotData(std::vector<size_t> detIDs);
  void setPlotType(PlotType type);
  void setTubeXUnits(TubeXUnits units) { m_tubeXUnits = units; }
  void updatePlot();
  void clear();

  PlotType getPlotType() { return m_plotType; }
  TubeXUnits getTubeXUnits() { return m_tubeXUnits; }
  QString getTubeXUnitsName() const;
  QString getTubeXUnitsUnits() const;
  QString getPlotCaption() const;

public slots:
  void savePlotToWorkspace();
  void showContextMenu(QPoint pos);

signals:
  void plotTypeChanged(QString title);

private slots:
  void sumDetectors();
  void integrateTimeBins();
  void setYScaleLinear();
  void setYScaleLog();
  void setTubeXUnits(int unit);
  void removeCurve(const QString &label);
  void addPeak(double x, double y);

private:
  void initActions();

  void plotSingle(size_t detid);
  void addPeakMarker(const PeakMarker2D &marker);
  void addPeakMarker(const Mantid::Geometry::IPeak &peak);
  void plotTube(size_t detid);
  void plotTubeSums(size_t detindex);
  void plotTubeIntegrals(size_t detindex);
  MiniPlotCurveData prepareDataForSinglePlot(size_t detindex,
                                             bool includeErrors = false);
  MiniPlotCurveData prepareDataForSumsPlot(size_t detindex,
                                           bool includeErrors = false);
  MiniPlotCurveData prepareDataForIntegralsPlot(size_t detindex,
                                                bool includeErrors = false);
  static double getOutOfPlaneAngle(const Mantid::Kernel::V3D &pos,
                                   const Mantid::Kernel::V3D &origin,
                                   const Mantid::Kernel::V3D &normal);

  InstrumentWidget *m_instrWidget;
  MiniPlot *m_miniplot;

  PlotType m_plotType;
  bool m_enabled;
  /// quantity the time bin integrals to be plotted against
  TubeXUnits m_tubeXUnits;
  size_t m_currentDetID;

  // actions
  QAction *m_sumDetectors;
  QAction *m_integrateTimeBins;
  QActionGroup *m_summationType;
  QAction *m_linearY;
  QAction *m_logY;
  QActionGroup *m_yScaleActions;
  QAction *m_detidUnits, *m_lengthUnits, *m_phiUnits, *m_outOfPlaneAngleUnits;
  QActionGroup *m_unitsGroup;
  QSignalMapper *m_unitsMapper;
  QAction *m_savePlotToWorkspace;
};
}
}

#endif // MANTIDQT_WIDGETS_INSTRUMENTVIEW_MINIPLOTCONTROLLER_H
