/***************************************************************************
File                 : ConfigDialog.h
Project              : QtiPlot
--------------------------------------------------------------------
Copyright            : (C) 2006 by Ion Vasilief, Tilman Hoener zu Siederdissen
Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
Description          : Preferences dialog

***************************************************************************/

/***************************************************************************
*                                                                         *
*  This program is free software; you can redistribute it and/or modify   *
*  it under the terms of the GNU General Public License as published by   *
*  the Free Software Foundation; either version 2 of the License, or      *
*  (at your option) any later version.                                    *
*                                                                         *
*  This program is distributed in the hope that it will be useful,        *
*  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
*  GNU General Public License for more details.                           *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the Free Software           *
*   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
*   Boston, MA  02110-1301  USA                                           *
*                                                                         *
***************************************************************************/
#ifndef ConfigDialog_H
#define ConfigDialog_H

#include <QDialog>
#include <QCheckBox>
#include <map>
#include "MantidQtAPI/MdSettings.h"
#include "boost/scoped_ptr.hpp"

class QLineEdit;
class QGroupBox;
class QGridLayout;
class QPushButton;
class QTabWidget;
class QStackedWidget;
class QWidget;
class QComboBox;
class QSpinBox;
class QLabel;
class QRadioButton;
class QListWidget;
class ColorButton;
class ColorBox;
class DoubleSpinBox;
class QMouseEvent;
class QTreeWidget;
class QTreeWidgetItem;
class QStringList;

namespace MantidQt
{
  namespace MantidWidgets
  {
    class InstrumentSelector;
  }
}

//! Preferences dialog
class ConfigDialog : public QDialog
{
  Q_OBJECT

public:
  //! Constructor
  /**
  * @param parent :: parent widget (must be the application window!=
  * @param fl :: window flags
  */
  ConfigDialog( QWidget* parent, Qt::WFlags fl = 0 );
  void setColumnSeparator(const QString& sep);
  void gotoMantidDirectories();

  private slots:
    virtual void languageChange();
    void insertLanguagesList();

    void accept();
    void apply();

    void setCurrentPage(int index);

    //table fonts
    void pickTextFont();
    void pickHeaderFont();

    //graph fonts
    void pickAxesFont();
    void pickNumbersFont();
    void pickLegendFont();
    void pickTitleFont();

    void enableScaleFonts();
    void showFrameWidth(bool ok);

    //application
    void pickApplicationFont();

    //2D curves
    int curveStyle();
    void pick3DTitleFont();
    void pick3DNumbersFont();
    void pick3DAxesFont();

    //Fitting
    void showPointsBox(bool);

    void switchToLanguage(int param);

    void chooseTranslationsFolder();
    void chooseHelpFolder();

    //Mantid
    void addPythonScriptsDirs();
    void addPythonPluginDirs();
    void addInstrumentDir();
    void enableButtons();
    void itemCheckedChanged(QTreeWidgetItem* item);
    void updateChildren(std::map<std::string, std::string> &programKeysAndDetails, QTreeWidgetItem* program);
    void addDialog();
    void editDialog();
    void deleteDialog();

private:
  void initPlotsPage();
  void initOptionsPage();

  void initAxesPage();
  void initAppPage();
  // Mantid
  void initMantidPage();
  void initDirSearchTab();
  void initCurveFittingTab();

  void initCurvesPage();
  void initPlots3DPage();
  void initTablesPage();
  void initConfirmationsPage();
  void initFileLocationsPage();
  void initFittingPage();

  void updateDirSearchSettings();
  void updateCurveFitSettings();
  void updateMantidOptionsTab();
  void updateSendToTab();

  void initMantidOptionsTab(); 
  void initSendToProgramTab();
  void refreshTreeCategories();
  void populateProgramTree();
  void updateProgramTree();

  // MD Plotting
  void initMdPlottingPage();
  void initMdPlottingGeneralTab();
  void initMdPlottingVsiTab();
  void updateMdPlottingSettings();
  void setupMdPlottingConnections();

  QTreeWidgetItem* createCheckedTreeItem(QString name,bool checkBoxState);
  QStringList buildHiddenCategoryString(QTreeWidgetItem *parent = 0);

  std::map<std::string,std::map<std::string,std::string> > m_sendToSettings;

  QFont textFont, headerFont, axesFont, numbersFont, legendFont, titleFont, appFont;
  QFont plot3DTitleFont, plot3DNumbersFont, plot3DAxesFont;

  QCheckBox *boxScaleLayersOnPrint, *boxPrintCropmarks, *boxUpdateSeparators, *linearFit2PointsBox;
  QTabWidget *plotsTabWidget, *appTabWidget, *mtdTabWidget;
  ColorButton *btnBackground3D, *btnMesh, *btnAxes, *btnLabels, *btnNumbers;
  ColorButton *btnFromColor, *btnToColor, *btnGrid;
  QPushButton	*btnTitleFnt, *btnLabelsFnt, *btnNumFnt;
  QPushButton *deleteButton, *editButton, *addButton;
  ColorButton *buttonBackground, *buttonText, *buttonHeader;
  QPushButton *buttonOk, *buttonCancel, *buttonApply;
  QPushButton* buttonTextFont, *buttonHeaderFont;
  QStackedWidget * generalDialog;
  QWidget *appColors, *tables, *plotOptions, *plotTicks, *plotFonts, *confirm, *plotPrint;
  QWidget *application, *curves, *axesPage, *plots3D, *fitPage, *numericFormatPage, *floatingWindowsPage;
  //Mantid
  QWidget *instrumentPage;
  QComboBox *facility;
  MantidQt::MantidWidgets::InstrumentSelector  *defInstr;
  QCheckBox* ckIgnoreParaView;

  /// Mantid tab for setting directories
  QWidget *directoriesPage;
  QLineEdit* lePythonScriptsDirs;///< pythonscripts.directories
  QLineEdit* lePythonPluginsDirs;///< python plugins directories
  QLineEdit* leInstrumentDir;///< instrumentDefinition.directory
  // Mantid curve fitting page
  QWidget *curveFittingPage;
  QComboBox *backgroundFunctions;
  QLineEdit *functionArguments;
  QComboBox *defaultPeakShape;
  QSpinBox  *findPeaksFWHM,*findPeaksTolerance;
  QSpinBox  *peakRadius;
  QSpinBox  *decimals;
  /// mantid options page
  QWidget*  mantidOptionsPage;
  QWidget*  mantidSendToPage;
  QCheckBox *m_invisibleWorkspaces;
  QCheckBox *m_reusePlotInstances;
  QCheckBox *m_useOpenGL;
  QCheckBox *m_sendToPrograms;
  QTreeWidget *treeCategories;
  QTreeWidget *treePrograms;

  //MDPlotting
  QTabWidget* mdPlottingTabWidget;
  QWidget *vsiPage, *mdPlottingGeneralPage;
  QComboBox *vsiDefaultColorMap, *vsiInitialView, *mdPlottingGeneralColorMap;
  QLabel *lblVsiDefaultColorMap, *lblVsiDefaultBackground, *lblGeneralDefaultColorMap, *lblBoxGeneralDefaultColorMap, *lblVsiLastSession, *lblVsiInitialView;
  ColorButton *vsiDefaultBackground;
  QGroupBox* mdPlottingGeneralFrame;
  QCheckBox* vsiLastSession;
  boost::scoped_ptr<MantidQt::API::MdSettings> mdSettings;

  QPushButton* buttonAxesFont, *buttonNumbersFont, *buttonLegendFont, *buttonTitleFont, *fontsBtn;
  QCheckBox *boxSearchUpdates, *boxOrthogonal, *logBox, *plotLabelBox, *scaleErrorsBox;
  QCheckBox *boxTitle, *boxFrame, *boxDistribution, *boxPlots3D, *boxPlots2D, *boxTables, *boxNotes, *boxFolders,*boxInstrWindow;
  QCheckBox *boxSave, *boxBackbones, *boxShowLegend, *boxSmoothMesh;
  QCheckBox *boxAutoscaling, *boxShowProjection, *boxMatrices, *boxScaleFonts, *boxResize, *boxAspectRatio;
  QComboBox *boxMajTicks, *boxMinTicks, *boxStyle, *boxCurveStyle, *boxSeparator, *boxLanguage, *boxDecimalSeparator;
  QCheckBox *boxFloatingGraph, *boxFloatingTable, *boxFloatingInstrumentWindow, *boxFloatingMantidMatrix, *boxFloatingNote, *boxFloatingMatrix;
  QCheckBox *boxFloatingCustomInterfaces,*boxFloatingTiledWindows;
  QSpinBox *boxMinutes, *boxLineWidth, *boxFrameWidth, *boxResolution, *boxMargin, *boxPrecision, *boxAppPrecision;
  QSpinBox *boxSymbolSize, *boxMinTicksLength, *boxMajTicksLength, *generatePointsBox;
  DoubleSpinBox *boxCurveLineWidth;
  ColorButton *btnWorkspace, *btnPanels, *btnPanelsText;
  QListWidget * itemsList;
  QLabel *labelFrameWidth, *lblLanguage, *lblWorkspace, *lblPanels, *lblPageHeader;
  QLabel *lblPanelsText, *lblFonts, *lblStyle, *lblDecimalSeparator, *lblAppPrecision;
  QGroupBox *groupBoxConfirm;
  QGroupBox *groupBoxTableFonts, *groupBoxTableCol;
  QLabel *lblSeparator, *lblTableBackground, *lblTextColor, *lblHeaderColor;
  QLabel *lblSymbSize, *lblAxesLineWidth, *lblCurveStyle, *lblResolution, *lblPrecision;
  QGroupBox *groupBox3DFonts, *groupBox3DCol;
  QLabel *lblMargin, *lblMajTicks, *lblMajTicksLength, *lblLineWidth, *lblMinTicks, *lblMinTicksLength, *lblPoints, *lblPeaksColor;
  QGroupBox *groupBoxFittingCurve, *groupBoxFitParameters;
  QRadioButton *samePointsBtn, *generatePointsBtn;
  QGroupBox *groupBoxMultiPeak;
  ColorBox *boxPeaksColor;
  QLabel *lblScriptingLanguage, *lblInitWindow;
  QComboBox *boxScriptingLanguage, *boxInitWindow;
  QCheckBox *boxAntialiasing, *boxAutoscale3DPlots, *boxTableComments, *boxThousandsSeparator;
  QCheckBox *boxPromptRenameTables, *boxBackupProject, *boxLabelsEditing, *boxPromptDeleteWorkspace;
  QWidget *fileLocationsPage;
  QLabel *lblTranslationsPath, *lblHelpPath, *lblUndoStackSize, *lblEndOfLine;
  QLineEdit *translationsPathLine, *helpPathLine;
  QSpinBox *undoStackSizeBox;
  QComboBox *boxEndLine;
  QCheckBox* cbApplyToMantid;
  QCheckBox* cbDrawAllErrors;
  QCheckBox* cbEnableQtiPlotFitting;

  QLabel *labelGraphAxesLabelsDist, *labelTickLabelsDist;
  QSpinBox *boxAxesLabelsDist, *boxTickLabelsDist;
  QLabel *xBottomLabel, *xTopLabel, *yLeftLabel, *yRightLabel, *enableAxisLabel, *showNumbersLabel, *scaleLabel;
  QCheckBox *boxEnableAxis, *boxShowAxisLabels;
  QGroupBox * enabledAxesGroupBox;
  QGridLayout *enabledAxesGrid;
  QCheckBox *boxSynchronizeScales;


#ifdef SCRIPTING_PYTHON
  QLabel *lblPythonConfigDir;
  QLineEdit *pythonConfigDirLine;
#endif
  QCheckBox *boxUpdateTableValues;

  public slots:
    void changeUsageGeneralMdColorMap(bool state);
};

#endif // CONFIGDIALOG_H
