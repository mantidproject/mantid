#ifndef INSTRUMENTWINDOWPICKTAB_H_
#define INSTRUMENTWINDOWPICKTAB_H_

#include "MantidGLWidget.h"
#include "DetSelector.h"

#include "MantidGeometry/ICompAssembly.h"
#include "MantidAPI/MatrixWorkspace.h"

#include <QFrame>

class InstrumentWindow;
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
  * Implements the Pick tab in InstrumentWindow. Allows to pick a detector
  * or a tube and display the data in it and some info.
  */
class InstrumentWindowPickTab: public QFrame
{
  Q_OBJECT
public:
  enum TubeXUnits {DETECTOR_ID = 0,LENGTH,PHI,NUMBER_OF_UNITS};
  InstrumentWindowPickTab(InstrumentWindow* instrWindow);
  void updatePick(int detid);
  bool canUpdateTouchedDetector()const;
  void init();
  void setInstrumentDisplayContextMenu(QMenu& context);
  TubeXUnits getTubeXUnits() const {return m_tubeXUnits;}
  void mouseLeftInstrmentDisplay();
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
private:
  void showEvent (QShowEvent *);
  void updatePlot(int detid);
  void updateSelectionInfo(int detid);
  void plotSingle(int detid);
  //void plotBox(const Instrument3DWidget::DetInfo & cursorPos);
  void plotTube(int detid);
  /// Calc indexes for min and max bin values defined in the instrument Actor
  void getBinMinMaxIndex(size_t wi,size_t& imin, size_t& imax);
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


  InstrumentWindow* m_instrWindow;
  MantidGLWidget *mInstrumentDisplay;
  /* Pick tab controls */
  OneCurvePlot* m_plot;
  QLabel *m_activeTool; ///< Displays a tip on which tool is currently selected
  QPushButton *m_one; ///< Button switching on single detector selection mode
  QPushButton *m_tube; ///< Button switching on detector's parent selection mode
  QPushButton *m_box; ///< Button switching on box selection mode
  QPushButton *m_peak; ///< Button switching on box selection mode
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
  DetSelectionType m_selectionType;
  int m_currentDetID;
  int m_emode;
  double m_efixed;
  double m_delta;
  TubeXUnits m_tubeXUnits; ///< quantity the time bin integrals to be plotted against
  bool m_freezePlot;
};


#endif /*INSTRUMENTWINDOWPICKTAB_H_*/
