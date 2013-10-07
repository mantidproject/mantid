#ifndef INSTRUMENTWINDOWPICKTAB_H_
#define INSTRUMENTWINDOWPICKTAB_H_

#include "InstrumentWindowTab.h"
#include "MantidGLWidget.h"

#include "MantidGeometry/ICompAssembly.h"
#include "MantidAPI/MatrixWorkspace.h"

class Instrument3DWidget;
class InstrumentActor;
class CollapsiblePanel;
class OneCurvePlot;

class QPushButton;
class QTextEdit;
class QComboBox;
class QCheckBox;
class QLabel;
class QActionGroup;
class QSignalMapper;
class QMenu;

/**
  * Implements the Pick tab in InstrumentWindow.
  * Contains a set of tools which allow one to:
  *
  *  - pick a detector or a tube and display the data in it and some info
  *  - add a peak to a peaks workspace and display an overlay of markers
  *  - select and remove peaks
  *
  */
class InstrumentWindowPickTab: public InstrumentWindowTab
{
  Q_OBJECT
public:
  /// Activity type the tab can be in:
  ///   Single:  select and display info for a single detector when pointed by the mouse
  ///   Tube:    select and display info for a tube of detectors. The immediate parent
  ///            of a detector is considered a tube
  ///   AddPeak: Click on a detector and then on the miniplot to add a peak marker and
  ///            a peak to the attached peaks workspace
  ///   SelectPeak: click on a peak marker or draw a rubber-band selector to select peak
  ///               markers. Selected peaks can be deleted by pressing the Delete key.
  enum SelectionType {Single=0,AddPeak,ErasePeak,SingleDetectorSelection,Tube, Draw};
  enum ToolType {Zoom,PixelSelect,TubeSelect,PeakSelect,PeakErase, DrawEllipse, DrawRectangle, EditShape};
  enum TubeXUnits {DETECTOR_ID = 0,LENGTH,PHI,NUMBER_OF_UNITS};

  InstrumentWindowPickTab(InstrumentWindow* instrWindow);
  void updatePick(int detid);
  bool canUpdateTouchedDetector()const;
  TubeXUnits getTubeXUnits() const {return m_tubeXUnits;}
  void mouseLeftInstrmentDisplay();
  void initSurface();
  void saveSettings(QSettings& settings) const;
  void loadSettings(const QSettings& settings);
  bool addToDisplayContextMenu(QMenu&) const;
  void selectTool(const ToolType tool);

public slots:
  void setTubeXUnits(int units);
  void changedIntegrationRange(double,double);
private slots:
  void plotContextMenu();
  void sumDetectors();
  void integrateTimeBins();
  void setPlotCaption();
  void setSelectionType();
  void addPeak(double,double);
  void storeCurve();
  void removeCurve(const QString &);
  void savePlotToWorkspace();
  void singleDetectorTouched(int detid);
  void singleDetectorPicked(int detid);
  void updateSelectionInfoDisplay();
  void shapeCreated();
  void updatePlotMultipleDetectors();
private:
  void showEvent (QShowEvent *);
  void updatePlot(int detid);
  void updateSelectionInfo(int detid);
  void plotSingle(int detid);
  void plotTube(int detid);
  void plotTubeSums(int detid);
  void plotTubeIntegrals(int detid);
  void prepareDataForSinglePlot(
    int detid,
    std::vector<double>&x,
    std::vector<double>&y,
    std::vector<double>* err = NULL);
  void prepareDataForSumsPlot(
    int detid,
    std::vector<double>&x,
    std::vector<double>&y,
    std::vector<double>* err = NULL);
  void prepareDataForIntegralsPlot(
    int detid,
    std::vector<double>&x,
    std::vector<double>&y,
    std::vector<double>* err = NULL);
    TubeXUnits getTubeXUnits(const QString& name) const;
    QString getTubeXUnitsName(TubeXUnits unit) const;
  QString getNonDetectorInfo();
  QColor getShapeBorderColor() const;

  /* Pick tab controls */
  OneCurvePlot* m_plot; ///< Miniplot to display data in the detectors
  QLabel *m_activeTool; ///< Displays a tip on which tool is currently selected
  QPushButton *m_zoom;  ///< Button switching on navigation mode
  QPushButton *m_one;   ///< Button switching on single detector selection mode
  QPushButton *m_tube;  ///< Button switching on detector's parent selection mode
  QPushButton *m_peak;  ///< Button switching on peak creation mode
  QPushButton *m_peakSelect;   ///< Button switching on peak selection mode
  QPushButton *m_rectangle;    ///< Button switching on drawing a rectangular selection region
  QPushButton *m_ellipse;      ///< Button switching on drawing a elliptical selection region
  QPushButton *m_ring_ellipse; ///< Button switching on drawing a elliptical ring selection region
  QPushButton *m_ring_rectangle; ///< Button switching on drawing a rectangular ring selection region
  QPushButton *m_edit;           ///< Button switching on edditing the selection region
  bool m_plotSum;

  // Actions to set integration option for the detector's parent selection mode
  QAction *m_sumDetectors;      ///< Sets summation over detectors (m_plotSum = true)
  QAction *m_integrateTimeBins; ///< Sets integration over time bins (m_plotSum = false)
  QActionGroup *m_summationType;
  QAction *m_logY;
  QAction *m_linearY;
  QActionGroup *m_yScale;
  QActionGroup* m_unitsGroup;
  QAction *m_detidUnits,*m_lengthUnits,*m_phiUnits;
  QSignalMapper *m_unitsMapper;

  // Instrument display context menu actions
  QAction *m_storeCurve; ///< add the current curve to the list of permanently displayed curves
  QAction *m_savePlotToWorkspace; ///< Save data plotted on the miniplot into a MatrixWorkspace

  CollapsiblePanel* m_plotPanel;
  QTextEdit* m_selectionInfoDisplay; ///< Text control for displaying selection information
  CollapsiblePanel* m_infoPanel;
  SelectionType m_selectionType;
  int m_currentDetID;
  TubeXUnits m_tubeXUnits; ///< quantity the time bin integrals to be plotted against
  mutable bool m_freezePlot;
};


#endif /*INSTRUMENTWINDOWPICKTAB_H_*/
