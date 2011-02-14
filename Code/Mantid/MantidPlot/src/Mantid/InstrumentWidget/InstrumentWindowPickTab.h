#ifndef INSTRUMENTWINDOWPICKTAB_H_
#define INSTRUMENTWINDOWPICKTAB_H_

#include "Instrument3DWidget.h"
#include "DetSelector.h"

#include <QFrame>

class InstrumentWindow;
class Instrument3DWidget;
class CollapsiblePanel;
class OneCurvePlot;

class QPushButton;
class QTextEdit;
class QComboBox;
class QCheckBox;

/**
  * Implements the Pick tab in InstrumentWindow
  */
class InstrumentWindowPickTab: public QFrame
{
  Q_OBJECT
public:
  
  InstrumentWindowPickTab(InstrumentWindow* instrWindow);
  void updatePick(const Instrument3DWidget::DetInfo & cursorPos);
private slots:
  void plotContextMenu();
  void sumDetectors();
  void integrateTimeBins();
  void setPlotCaption();
  void setSelectionType();
private:
  void updatePlot(const Instrument3DWidget::DetInfo & cursorPos);
  void updateSelectionInfo(const Instrument3DWidget::DetInfo & cursorPos);
  void plotSingle(const Instrument3DWidget::DetInfo & cursorPos);
  void plotBox(const Instrument3DWidget::DetInfo & cursorPos);
  void plotTube(const Instrument3DWidget::DetInfo & cursorPos);

  InstrumentWindow* m_instrWindow;
  Instrument3DWidget *mInstrumentDisplay;
  /* Pick tab controls */
  OneCurvePlot* m_plot;
  QPushButton *m_one; ///< Button switching on single detector selection mode
  QPushButton *m_tube; ///< Button switching on detector's parent selection mode
  QPushButton *m_box; ///< Button switching on box selection mode
  bool m_plotSum; 
  // Actions to set integration option for the detector's parent selection mode
  QAction *m_sumDetectors;      ///< Sets summation over detectors (m_plotSum = true)
  QAction *m_integrateTimeBins; ///< Sets summation over time bins (m_plotSum = false)
  QAction *m_logY;
  QAction *m_linearY;
  CollapsiblePanel* m_plotPanel;
  QTextEdit* m_selectionInfoDisplay; ///< Text control for displaying selection information
  CollapsiblePanel* m_infoPanel;
  DetSelectionType m_selectionType;
};


#endif /*INSTRUMENTWINDOWPICKTAB_H_*/
