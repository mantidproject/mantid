// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ColorBar.h"
#include "ColorMap.h"
#include "InstrumentWidgetTab.h"

#include "MantidQtWidgets/Common/GraphOptions.h"

class QPushButton;
class QLineEdit;
class QComboBox;
class QCheckBox;
class QAction;
class QActionGroup;
class QMenu;
class QLineEdit;
class QSlider;
class QLabel;
class QVBoxLayout;
class QHBoxLayout;

namespace MantidQt {
namespace MantidWidgets {
class InstrumentWidget;
class BinDialog;

/**
 * Implements the Render tab in InstrumentWidget.
 */
class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW InstrumentWidgetRenderTab : public InstrumentWidgetTab {
  Q_OBJECT

public:
  explicit InstrumentWidgetRenderTab(InstrumentWidget *instrWindow);
  ~InstrumentWidgetRenderTab();
  void initSurface() override;
  void saveSettings(QSettings & /*unused*/) const override;
  void loadSettings(const QSettings & /*unused*/) override;
  // legacy interface for MantidPlot python api
  ColorMap::ScaleType getScaleType() const;
  void setScaleType(ColorMap::ScaleType type);
  void setAxis(const QString &axisName);
  bool areAxesOn() const;
  void setupColorBar(const ColorMap & /*cmap*/, double /*minValue*/, double /*maxValue*/, double /*minPositive*/,
                     bool /*autoscaling*/);
  /// Load the render window tab settings from file.
  virtual void loadFromProject(const std::string &lines) override;
  /// Save the render window tab settings to file.
  std::string saveToProject() const override;

signals:
  void rescaleColorMap();
  void setAutoscaling(bool /*_t1*/);

public slots:
  void setMinValue(double value, bool apply = true);
  void setMaxValue(double value, bool apply = true);
  void setRange(double minValue, double maxValue, bool apply = true);
  void showAxes(bool on);
  void displayDetectorsOnly(bool yes);
  void enableGL(bool on);
  void setColorMapAutoscaling(bool /*on*/);
  void setLegendScaleType(int /*index*/);
  void changeColorMap(const QString &filename = "", const bool highlightZeroDets = false);
  void setSurfaceType(int /*index*/);
  void setMaintainAspectRatio(bool /*on*/);
  void flipUnwrappedView(bool /*on*/);
  void resetView();
  void saveImage(const QString &filename = "");

private slots:
  void showOrHideBoxes(int /*iv*/);
  /// Called before the display setting menu opens. Filters out menu options.
  void displaySettingsAboutToshow();
  /// Change the type of the surfac
  void surfaceTypeChanged(int index);
  void maintainAspectRatioChanged(bool on);
  void colorMapChanged();
  void scaleTypeChanged(int /*type*/);
  void nthPowerChanged(double /*nth_power*/);
  void glOptionChanged(bool /*on*/);
  void showMenuToolTip(QAction * /*action*/);
  void setUCorrection();
  void toggleTooltip(bool /*activate*/);
  void toggleLayerDisplay(bool on);
  void setVisibleLayer(int layer);

private: // methods
  void showEvent(QShowEvent * /*unused*/) override;
  QMenu *createPeaksMenu();
  QFrame *setupAxisFrame();
  void setPrecisionMenuItemChecked(int n);
  void enable3DSurface(bool on);
  QPointF getUCorrection() const;
  void connectInstrumentWidgetSignals() const;
  void setupSurfaceTypeOptions();
  QPushButton *setupDisplaySettings();
  void setupColorMapWidget();
  void setupUnwrappedControls(QHBoxLayout *parentLayout);
  void setupGridBankMenu(QVBoxLayout *parentLayout);
  void forceLayers(bool on);

private: // members
  QPushButton *m_surfaceTypeButton;
  QPushButton *m_resetView;
  QPushButton *mSaveImage;
  ColorBar *m_colorBarWidget;
  QFrame *m_resetViewFrame;
  QComboBox *mAxisCombo;
  QCheckBox *m_flipCheckBox;
  QPushButton *m_peakOverlaysButton;
  QCheckBox *m_autoscaling;
  QCheckBox *m_freezeRotation;

  QActionGroup *m_surfaceTypeActionGroup;
  QAction *m_full3D;
  QAction *m_cylindricalX;
  QAction *m_cylindricalY;
  QAction *m_cylindricalZ;
  QAction *m_sphericalX;
  QAction *m_sphericalY;
  QAction *m_sphericalZ;
  QAction *m_sideBySide;

  QAction *m_colorMap;
  QAction *m_backgroundColor;
  QAction *m_displayAxes;
  QAction *m_displayDetectorsOnly;
  QAction *m_wireframe;
  QAction *m_lighting;
  QAction *m_GLView; ///< toggle between OpenGL and simple view
  QAction *m_UCorrection;
  QAction *m_tooltipInfo;
  QAction *m_maintainAspectRatio;
  QActionGroup *m_precisionActionGroup;
  QList<QAction *> m_precisionActions;

  QCheckBox *m_layerCheck;
  QSlider *m_layerSlide;
  QLabel *m_layerDisplay;

  bool m_usingLayerStore;

  friend class InstrumentWidget;
  friend class InstrumentWidgetEncoder;
  friend class InstrumentWidgetDecoder;
};
} // namespace MantidWidgets
} // namespace MantidQt
