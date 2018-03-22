#ifndef INSTRUMENTWIDGETPICKTAB_H_
#define INSTRUMENTWIDGETPICKTAB_H_

#include "MantidQtWidgets/InstrumentView/InstrumentWidgetTab.h"
#include "MantidQtWidgets/InstrumentView/MantidGLWidget.h"

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IDTypes.h"

class Instrument3DWidget;

class QPushButton;
class QTextEdit;
class QComboBox;
class QCheckBox;
class QLabel;
class QActionGroup;
class QSignalMapper;
class QMenu;

namespace Mantid {
namespace Geometry {
class IPeak;
}
}

namespace MantidQt {
namespace MantidWidgets {
class InstrumentActor;
class CollapsiblePanel;
class ProjectionSurface;
class ComponentInfoController;
class MiniPlotController;
class MiniPlot;
struct MiniPlotCurveData;

/**
* Implements the Pick tab in InstrumentWidget.
* Contains a set of tools which allow one to:
*
*  - pick a detector or a tube and display the data in it and some info
*  - add a peak to a peaks workspace and display an overlay of markers
*  - select and remove peaks
*
*/
class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW InstrumentWidgetPickTab
    : public InstrumentWidgetTab {
  Q_OBJECT
public:
  /// Activity type the tab can be in:
  ///   Single:  select and display info for a single detector when pointed by
  ///   the mouse
  ///   Tube:    select and display info for a tube of detectors. The immediate
  ///   parent
  ///            of a detector is considered a tube
  ///   AddPeak: Click on a detector and then on the miniplot to add a peak
  ///   marker and
  ///            a peak to the attached peaks workspace
  ///   SelectPeak: click on a peak marker or draw a rubber-band selector to
  ///   select peak
  ///               markers. Selected peaks can be deleted by pressing the
  ///               Delete key.
  enum SelectionType {
    Single = 0,
    AddPeak,
    ErasePeak,
    ComparePeak,
    AlignPeak,
    SingleDetectorSelection,
    Tube,
    Draw
  };
  enum ToolType {
    Zoom,
    PixelSelect,
    TubeSelect,
    PeakSelect,
    PeakErase,
    PeakCompare,
    PeakAlign,
    DrawEllipse,
    DrawRectangle,
    DrawFree,
    EditShape
  };

  explicit InstrumentWidgetPickTab(InstrumentWidget *instrWidget);
  bool canUpdateTouchedDetector() const;
  void initSurface() override;
  void saveSettings(QSettings &settings) const override;
  void loadSettings(const QSettings &settings) override;
  bool addToDisplayContextMenu(QMenu &context) const override;
  void selectTool(const ToolType tool);
  boost::shared_ptr<ProjectionSurface> getSurface() const;
  const InstrumentWidget *getInstrumentWidget() const;
  /// Load settings for the pick tab from a project file
  virtual void loadFromProject(const std::string &lines) override;
  /// Save settings for the pick tab to a project file
  virtual std::string saveToProject() const override;

public slots:
  void changedIntegrationRange(double, double);

private slots:
  void setPlotCaption();
  void setSelectionType();
  void singleComponentTouched(size_t pickID);
  void singleComponentPicked(size_t pickID);
  void alignPeaks(const std::vector<Mantid::Kernel::V3D> &planePeaks,
                  const Mantid::Geometry::IPeak *peak);
  void
  comparePeaks(const std::pair<std::vector<Mantid::Geometry::IPeak *>,
                               std::vector<Mantid::Geometry::IPeak *>> &peaks);
  void updateSelectionInfoDisplay();
  void shapeCreated();
  void updatePlotMultipleDetectors();
  void savePlotToWorkspace();

private:
  void showEvent(QShowEvent *) override;
  QColor getShapeBorderColor() const;

  /* Pick tab controls */
  MiniPlot *m_miniplot; ///< Miniplot to display data in the detectors
  QLabel *m_activeTool; ///< Displays a tip on which tool is currently selected
  QPushButton *m_zoom;  ///< Button switching on navigation mode
  QPushButton *m_one;   ///< Button switching on single detector selection mode
  QPushButton *m_tube; ///< Button switching on detector's parent selection mode
  QPushButton *m_peak; ///< Button switching on peak creation mode
  QPushButton *m_peakSelect;  ///< Button switching on peak selection mode
  QPushButton *m_peakCompare; ///< Button switching on peak comparison mode
  QPushButton *m_peakAlign;   ///< Button switching on peak alignment mode
  QPushButton *m_rectangle;   ///< Button switching on drawing a rectangular
  /// selection region
  QPushButton *
      m_ellipse; ///< Button switching on drawing a elliptical selection region
  QPushButton *m_ring_ellipse; ///< Button switching on drawing a elliptical
  /// ring selection region
  QPushButton *m_ring_rectangle; ///< Button switching on drawing a rectangular
  /// ring selection region
  QPushButton *
      m_free_draw; ///< Button switching on drawing a region of arbitrary shape
  QPushButton *m_edit; ///< Button switching on edditing the selection region

  // Instrument display context menu actions
  QAction *m_storeCurve; ///< add the current curve to the list of permanently
  /// displayed curves
  QAction *m_savePlotToWorkspace; ///< Save data plotted on the miniplot into a
  /// MatrixWorkspace

  CollapsiblePanel *m_plotPanel;
  QTextEdit *m_selectionInfoDisplay; ///< Text control for displaying selection
  /// information
  CollapsiblePanel *m_infoPanel;
  SelectionType m_selectionType;
  mutable bool m_freezePlot;

  ComponentInfoController *m_infoController;
  MiniPlotController *m_miniplotController;
};

/**
* Class containing the logic of displaying info on the selected
* component(s) in the info text widget.
*/
class ComponentInfoController : public QObject {
  Q_OBJECT
public:
  /// Constructor.
  ComponentInfoController(InstrumentWidgetPickTab *tab,
                          const InstrumentWidget *instrWidget,
                          QTextEdit *infoDisplay);
public slots:
  void displayInfo(size_t pickID);
  void displayComparePeaksInfo(
      const std::pair<std::vector<Mantid::Geometry::IPeak *>,
                      std::vector<Mantid::Geometry::IPeak *>> &peaks);
  void displayAlignPeaksInfo(const std::vector<Mantid::Kernel::V3D> &planePeaks,
                             const Mantid::Geometry::IPeak *peak);
  void clear();

private:
  QString displayDetectorInfo(size_t index);
  QString displayNonDetectorInfo(Mantid::Geometry::ComponentID compID);
  QString displayPeakInfo(Mantid::Geometry::IPeak *peak);
  QString displayPeakAngles(const std::pair<Mantid::Geometry::IPeak *,
                                            Mantid::Geometry::IPeak *> &peaks);
  QString getPeakOverlayInfo();

  InstrumentWidgetPickTab *m_tab;
  const InstrumentWidget *m_instrWidget;
  QTextEdit *m_selectionInfoDisplay;

  bool m_freezePlot;
  bool m_instrWidgetBlocked;
  size_t m_currentPickID;
  QString m_xUnits;
};

} // MantidWidgets
} // MantidQt

#endif /*INSTRUMENTWIDGETPICKTAB_H_*/
