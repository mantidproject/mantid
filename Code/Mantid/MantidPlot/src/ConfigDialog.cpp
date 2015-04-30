/***************************************************************************
File                 : ConfigDialog.cpp
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
#include "ConfigDialog.h"
#include "ApplicationWindow.h"
#include "MultiLayer.h"
#include "Graph.h"
#include "Matrix.h"
#include "ColorButton.h"
#include "ColorBox.h"
#include "pixmaps.h"
#include "DoubleSpinBox.h"
#include "SendToProgramDialog.h"
#include "Mantid/MantidUI.h"
#include "MantidQtMantidWidgets/FitPropertyBrowser.h"

#include <QLocale>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include <QGroupBox>
#include <QFont>
#include <QFontDialog>
#include <QTabWidget>
#include <QTreeWidget>
#include <QStackedWidget>
#include <QWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QRadioButton>
#include <QStyleFactory>
#include <QRegExp>
#include <QMessageBox>
#include <QTranslator>
#include <QApplication>
#include <QDir>
#include <QPixmap>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidget>
#include <QFontMetrics>
#include <QFileDialog>
#include <QRegExp>
#include <QMouseEvent>
#include <QStringList>

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IBackgroundFunction.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidQtMantidWidgets/InstrumentSelector.h"
#include "MantidQtAPI/MdConstants.h"
#include "MantidQtAPI/MdSettings.h"
#include "MantidQtAPI/MdPlottingCmapsProvider.h"

#include <limits>

using Mantid::Kernel::ConfigService;

ConfigDialog::ConfigDialog( QWidget* parent, Qt::WFlags fl )
  : QDialog( parent, fl )
{
  // get current values from app window
  ApplicationWindow *app = dynamic_cast<ApplicationWindow *>(this->parentWidget());
  plot3DTitleFont = app->plot3DTitleFont;
  plot3DNumbersFont = app->plot3DNumbersFont;
  plot3DAxesFont = app->plot3DAxesFont;
  textFont = app->tableTextFont;
  headerFont = app->tableHeaderFont;
  appFont = app->appFont;
  axesFont = app->plotAxesFont;
  numbersFont = app->plotNumbersFont;
  legendFont = app->plotLegendFont;
  titleFont = app->plotTitleFont;

  // create the GUI
  generalDialog = new QStackedWidget();
  itemsList = new QListWidget();
  itemsList->setSpacing(10);
  itemsList->setAlternatingRowColors( true );

  initAppPage();
  initMantidPage();
  initTablesPage();
  initPlotsPage();
  initPlots3DPage();
  initFittingPage();
  initMdPlottingPage();

  generalDialog->addWidget(appTabWidget);
  generalDialog->addWidget(mtdTabWidget);
  generalDialog->addWidget(tables);
  generalDialog->addWidget(plotsTabWidget);
  generalDialog->addWidget(plots3D);
  generalDialog->addWidget(fitPage);
  generalDialog->addWidget(mdPlottingTabWidget);

  QVBoxLayout * rightLayout = new QVBoxLayout();
  lblPageHeader = new QLabel();
  QFont fnt = this->font();
  fnt.setPointSize(fnt.pointSize() + 3);
  fnt.setBold(true);
  lblPageHeader->setFont(fnt);
  lblPageHeader->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);

  QPalette pal = lblPageHeader->palette();
  pal.setColor( QPalette::Window, app->panelsColor );
  lblPageHeader->setPalette(pal);
  lblPageHeader->setAutoFillBackground( true );

  rightLayout->setSpacing(10);
  rightLayout->addWidget( lblPageHeader );
  rightLayout->addWidget( generalDialog );

  QHBoxLayout * topLayout = new QHBoxLayout();
  topLayout->setSpacing(5);
  topLayout->setMargin(5);
  topLayout->addWidget( itemsList );
  topLayout->addLayout( rightLayout );

  QHBoxLayout * bottomButtons = new QHBoxLayout();
  bottomButtons->addStretch();
  buttonApply = new QPushButton();
  buttonApply->setAutoDefault( true );
  bottomButtons->addWidget( buttonApply );

  buttonOk = new QPushButton();
  buttonOk->setAutoDefault( true );
  buttonOk->setDefault( true );
  bottomButtons->addWidget( buttonOk );

  buttonCancel = new QPushButton();
  buttonCancel->setAutoDefault( true );
  bottomButtons->addWidget( buttonCancel );

  QVBoxLayout * mainLayout = new QVBoxLayout( this );
  mainLayout->addLayout(topLayout);
  mainLayout->addLayout(bottomButtons);

  languageChange();

  // signals and slots connections
  connect( itemsList, SIGNAL(currentRowChanged(int)), this, SLOT(setCurrentPage(int)));
  connect( buttonOk, SIGNAL( clicked() ), this, SLOT( accept() ) );
  connect( buttonApply, SIGNAL( clicked() ), this, SLOT( apply() ) );
  connect( buttonCancel, SIGNAL( clicked() ), this, SLOT( reject() ) );
  connect( buttonTextFont, SIGNAL( clicked() ), this, SLOT( pickTextFont() ) );
  connect( buttonHeaderFont, SIGNAL( clicked() ), this, SLOT( pickHeaderFont() ) );

  setCurrentPage(0);
}

void ConfigDialog::setCurrentPage(int index)
{
  generalDialog->setCurrentIndex(index);
  if(itemsList->currentItem())
    lblPageHeader->setText(itemsList->currentItem()->text());
}

void ConfigDialog::initTablesPage()
{
  ApplicationWindow *app = dynamic_cast<ApplicationWindow *>(this->parentWidget());
  tables = new QWidget();

  QHBoxLayout * topLayout = new QHBoxLayout();
  topLayout->setSpacing(5);

  lblSeparator = new QLabel();
  topLayout->addWidget( lblSeparator );
  boxSeparator = new QComboBox();
  boxSeparator->setEditable( true );
  topLayout->addWidget( boxSeparator );

  QString help = tr("The column separator can be customized. \nThe following special codes can be used:\n\\t for a TAB character \n\\s for a SPACE");
  help += "\n"+tr("The separator must not contain the following characters: \n0-9eE.+-");

  boxSeparator->setWhatsThis(help);
  boxSeparator->setToolTip(help);
  lblSeparator->setWhatsThis(help);
  lblSeparator->setToolTip(help);

  groupBoxTableCol = new QGroupBox();
  QGridLayout * colorsLayout = new QGridLayout(groupBoxTableCol);

  lblTableBackground = new QLabel();
  colorsLayout->addWidget( lblTableBackground, 0, 0 );
  buttonBackground= new ColorButton();
  buttonBackground->setColor(app->tableBkgdColor);
  colorsLayout->addWidget( buttonBackground, 0, 1 );

  lblTextColor = new QLabel();
  colorsLayout->addWidget( lblTextColor, 1, 0 );
  buttonText = new ColorButton();
  buttonText->setColor(app->tableTextColor);
  colorsLayout->addWidget( buttonText, 1, 1 );

  lblHeaderColor = new QLabel();
  colorsLayout->addWidget( lblHeaderColor, 2, 0 );
  buttonHeader= new ColorButton();
  buttonHeader->setColor(app->tableHeaderColor);
  colorsLayout->addWidget( buttonHeader, 2, 1 );

  groupBoxTableFonts = new QGroupBox();
  QHBoxLayout * bottomLayout = new QHBoxLayout( groupBoxTableFonts );

  buttonTextFont= new QPushButton();
  bottomLayout->addWidget( buttonTextFont );
  buttonHeaderFont= new QPushButton();
  bottomLayout->addWidget( buttonHeaderFont );

  boxTableComments = new QCheckBox();
  boxTableComments->setChecked(app->d_show_table_comments);

  boxUpdateTableValues = new QCheckBox();
  boxUpdateTableValues->setChecked(app->autoUpdateTableValues());

  QVBoxLayout * tablesPageLayout = new QVBoxLayout( tables );
  tablesPageLayout->addLayout(topLayout,1);
  tablesPageLayout->addWidget(groupBoxTableCol);
  tablesPageLayout->addWidget(groupBoxTableFonts);
  tablesPageLayout->addWidget(boxTableComments);
  tablesPageLayout->addWidget(boxUpdateTableValues);
  tablesPageLayout->addStretch();
}

void ConfigDialog::initPlotsPage()
{
  ApplicationWindow *app = dynamic_cast<ApplicationWindow *>(this->parentWidget());

  plotsTabWidget = new QTabWidget();

  initOptionsPage();

  initAxesPage();
  plotsTabWidget->addTab( axesPage, QString() );

  initCurvesPage();
  plotsTabWidget->addTab( curves, QString() );

  plotTicks = new QWidget();
  QVBoxLayout * plotTicksLayout = new QVBoxLayout( plotTicks );

  QGroupBox * ticksGroupBox = new QGroupBox();
  QGridLayout * ticksLayout = new QGridLayout( ticksGroupBox );
  plotTicksLayout->addWidget( ticksGroupBox );

  lblMajTicks = new QLabel();
  ticksLayout->addWidget( lblMajTicks, 0, 0 );
  boxMajTicks = new QComboBox();
  ticksLayout->addWidget( boxMajTicks, 0, 1 );

  lblMajTicksLength = new QLabel();
  ticksLayout->addWidget( lblMajTicksLength, 0, 2 );
  boxMajTicksLength = new QSpinBox();
  boxMajTicksLength->setRange(0, 100);
  boxMajTicksLength->setValue(app->majTicksLength);
  ticksLayout->addWidget( boxMajTicksLength, 0, 3 );

  lblMinTicks = new QLabel();
  ticksLayout->addWidget( lblMinTicks, 1, 0 );
  boxMinTicks = new QComboBox();
  ticksLayout->addWidget( boxMinTicks, 1, 1 );

  lblMinTicksLength = new QLabel();
  ticksLayout->addWidget( lblMinTicksLength, 1, 2 );
  boxMinTicksLength = new QSpinBox();
  boxMinTicksLength->setRange(0, 100);
  boxMinTicksLength->setValue(app->minTicksLength);
  ticksLayout->addWidget( boxMinTicksLength, 1, 3 );

  ticksLayout->setRowStretch( 4, 1 );

  plotsTabWidget->addTab( plotTicks, QString() );

  plotFonts = new QWidget();
  QVBoxLayout * plotFontsLayout = new QVBoxLayout( plotFonts );

  QGroupBox * groupBox2DFonts = new QGroupBox();
  plotFontsLayout->addWidget( groupBox2DFonts );
  QVBoxLayout * fontsLayout = new QVBoxLayout( groupBox2DFonts );
  buttonTitleFont= new QPushButton();
  fontsLayout->addWidget( buttonTitleFont );
  buttonLegendFont= new QPushButton();
  fontsLayout->addWidget( buttonLegendFont );
  buttonAxesFont= new QPushButton();
  fontsLayout->addWidget( buttonAxesFont );
  buttonNumbersFont= new QPushButton();
  fontsLayout->addWidget( buttonNumbersFont );
  fontsLayout->addStretch();

  plotsTabWidget->addTab( plotFonts, QString() );

  plotPrint = new QWidget();
  QVBoxLayout *printLayout = new QVBoxLayout( plotPrint );

  boxScaleLayersOnPrint = new QCheckBox();
  boxScaleLayersOnPrint->setChecked(app->d_scale_plots_on_print);
  printLayout->addWidget( boxScaleLayersOnPrint );

  boxPrintCropmarks = new QCheckBox();
  boxPrintCropmarks->setChecked(app->d_print_cropmarks);
  printLayout->addWidget( boxPrintCropmarks );
  printLayout->addStretch();
  plotsTabWidget->addTab(plotPrint, QString());

  connect( boxResize, SIGNAL( clicked() ), this, SLOT( enableScaleFonts() ) );
  connect( boxFrame, SIGNAL( toggled(bool) ), this, SLOT( showFrameWidth(bool) ) );
  connect( buttonAxesFont, SIGNAL( clicked() ), this, SLOT( pickAxesFont() ) );
  connect( buttonNumbersFont, SIGNAL( clicked() ), this, SLOT( pickNumbersFont() ) );
  connect( buttonLegendFont, SIGNAL( clicked() ), this, SLOT( pickLegendFont() ) );
  connect( buttonTitleFont, SIGNAL( clicked() ), this, SLOT( pickTitleFont() ) );
}

void ConfigDialog::enableScaleFonts()
{
  if(boxResize->isChecked())
    boxScaleFonts->setEnabled(false);
  else
    boxScaleFonts->setEnabled(true);
}

void ConfigDialog::showFrameWidth(bool ok)
{
  if (!ok)
  {
    boxFrameWidth->hide();
    labelFrameWidth->hide();
  }
  else
  {
    boxFrameWidth->show();
    labelFrameWidth->show();
  }
}

void ConfigDialog::initPlots3DPage()
{
  ApplicationWindow *app = dynamic_cast<ApplicationWindow *>(this->parentWidget());
  plots3D = new QWidget();

  QGroupBox * topBox = new QGroupBox();
  QGridLayout * topLayout = new QGridLayout( topBox );
  topLayout->setSpacing(5);

  lblResolution = new QLabel();
  topLayout->addWidget( lblResolution, 0, 0 );
  boxResolution = new QSpinBox();
  boxResolution->setRange(1, 100);
  boxResolution->setValue(app->plot3DResolution);
  topLayout->addWidget( boxResolution, 0, 1 );

  boxShowLegend = new QCheckBox();
  boxShowLegend->setChecked(app->showPlot3DLegend);
  topLayout->addWidget( boxShowLegend, 1, 0 );

  boxShowProjection = new QCheckBox();
  boxShowProjection->setChecked(app->showPlot3DProjection);
  topLayout->addWidget( boxShowProjection, 1, 1 );

  boxSmoothMesh = new QCheckBox();
  boxSmoothMesh->setChecked(app->smooth3DMesh);
  topLayout->addWidget( boxSmoothMesh, 2, 0 );

  boxOrthogonal = new QCheckBox();
  boxOrthogonal->setChecked(app->orthogonal3DPlots);
  topLayout->addWidget( boxOrthogonal, 2, 1 );

  boxAutoscale3DPlots = new QCheckBox();
  boxAutoscale3DPlots->setChecked(app->autoscale3DPlots);
  topLayout->addWidget( boxAutoscale3DPlots, 3, 0 );

  groupBox3DCol = new QGroupBox();
  QGridLayout * middleLayout = new QGridLayout( groupBox3DCol );

  QStringList plot3DColors = app->plot3DColors;

  btnFromColor = new ColorButton();
  btnFromColor->setColor(QColor(plot3DColors[4]));
  middleLayout->addWidget( btnFromColor, 0, 0 );
  btnLabels = new ColorButton();
  btnLabels->setColor(QColor(plot3DColors[1]));
  middleLayout->addWidget( btnLabels, 0, 1 );
  btnMesh = new ColorButton();
  btnMesh->setColor(QColor(plot3DColors[2]));
  middleLayout->addWidget( btnMesh, 0, 2 );
  btnGrid = new ColorButton();
  btnGrid->setColor(QColor(plot3DColors[3]));
  middleLayout->addWidget( btnGrid, 0, 3 );
  btnToColor = new ColorButton();
  btnToColor->setColor(QColor(plot3DColors[0]));
  middleLayout->addWidget( btnToColor, 1, 0 );
  btnNumbers = new ColorButton();
  btnNumbers->setColor(QColor(plot3DColors[5]));
  middleLayout->addWidget( btnNumbers, 1, 1 );
  btnAxes = new ColorButton();
  btnAxes->setColor(QColor(plot3DColors[6]));
  middleLayout->addWidget( btnAxes, 1, 2 );
  btnBackground3D = new ColorButton();
  btnBackground3D->setColor(QColor(plot3DColors[7]));
  middleLayout->addWidget( btnBackground3D, 1, 3 );

  groupBox3DFonts = new QGroupBox();
  QHBoxLayout * bottomLayout = new QHBoxLayout( groupBox3DFonts );
  btnTitleFnt = new QPushButton();
  bottomLayout->addWidget( btnTitleFnt );
  btnLabelsFnt = new QPushButton();
  bottomLayout->addWidget( btnLabelsFnt );
  btnNumFnt = new QPushButton();
  bottomLayout->addWidget( btnNumFnt );

  QVBoxLayout * plots3DPageLayout = new QVBoxLayout( plots3D );
  plots3DPageLayout->addWidget(topBox);
  plots3DPageLayout->addWidget(groupBox3DCol);
  plots3DPageLayout->addWidget(groupBox3DFonts);
  plots3DPageLayout->addStretch();

  connect( btnNumFnt, SIGNAL( clicked() ), this, SLOT(pick3DNumbersFont() ) );
  connect( btnTitleFnt, SIGNAL( clicked() ), this, SLOT(pick3DTitleFont() ) );
  connect( btnLabelsFnt, SIGNAL( clicked() ), this, SLOT(pick3DAxesFont() ) );
}

void ConfigDialog::initAppPage()
{
  ApplicationWindow *app = dynamic_cast<ApplicationWindow *>(this->parentWidget());

  appTabWidget = new QTabWidget(generalDialog);
  appTabWidget->setUsesScrollButtons(false);

  application = new QWidget();
  QVBoxLayout * applicationLayout = new QVBoxLayout( application );
  QGroupBox * groupBoxApp = new QGroupBox();
  applicationLayout->addWidget(groupBoxApp);
  QGridLayout * topBoxLayout = new QGridLayout( groupBoxApp );

  lblLanguage = new QLabel();
  topBoxLayout->addWidget( lblLanguage, 0, 0 );
  boxLanguage = new QComboBox();
  insertLanguagesList();
  topBoxLayout->addWidget( boxLanguage, 0, 1 );

  lblStyle = new QLabel();
  topBoxLayout->addWidget( lblStyle, 1, 0 );
  boxStyle = new QComboBox();
  topBoxLayout->addWidget( boxStyle, 1, 1 );
  QStringList styles = QStyleFactory::keys();
  styles.sort();
  boxStyle->addItems(styles);
  boxStyle->setCurrentIndex(boxStyle->findText(app->appStyle,Qt::MatchWildcard));

  lblFonts = new QLabel();
  topBoxLayout->addWidget( lblFonts, 2, 0 );
  fontsBtn= new QPushButton();
  topBoxLayout->addWidget( fontsBtn, 2, 1 );

  lblScriptingLanguage = new QLabel();
  topBoxLayout->addWidget( lblScriptingLanguage, 3, 0 );
  boxScriptingLanguage = new QComboBox();
  QStringList llist = ScriptingLangManager::languages();
  boxScriptingLanguage->insertStringList(llist);
  boxScriptingLanguage->setCurrentItem(llist.findIndex(app->defaultScriptingLang));
  topBoxLayout->addWidget( boxScriptingLanguage, 3, 1 );

  lblUndoStackSize = new QLabel();
  topBoxLayout->addWidget( lblUndoStackSize, 4, 0 );
  undoStackSizeBox = new QSpinBox();
  undoStackSizeBox->setValue(app->matrixUndoStackSize());
  topBoxLayout->addWidget( undoStackSizeBox, 4, 1 );

  lblEndOfLine = new QLabel();
  topBoxLayout->addWidget(lblEndOfLine, 5, 0 );
  boxEndLine = new QComboBox();
  boxEndLine->addItem(tr("LF (Unix)"));
  boxEndLine->addItem(tr("CRLF (Windows)"));
  boxEndLine->addItem(tr("CR (Mac)"));
  boxEndLine->setCurrentIndex((int)app->d_eol);
  topBoxLayout->addWidget(boxEndLine, 5, 1);

  lblInitWindow = new QLabel();
  topBoxLayout->addWidget( lblInitWindow, 6, 0 );
  boxInitWindow = new QComboBox();
  topBoxLayout->addWidget( boxInitWindow, 6, 1 );

  boxSave= new QCheckBox();
  boxSave->setChecked(app->autoSave);
  topBoxLayout->addWidget( boxSave, 7, 0 );

  boxMinutes = new QSpinBox();
  boxMinutes->setRange(1, 100);
  boxMinutes->setValue(app->autoSaveTime);
  boxMinutes->setEnabled(app->autoSave);
  topBoxLayout->addWidget( boxMinutes, 7, 1 );

  boxBackupProject = new QCheckBox();
  boxBackupProject->setChecked(app->d_backup_files);
  topBoxLayout->addWidget( boxBackupProject, 8, 0, 1, 2 );

  boxSearchUpdates = new QCheckBox();
  boxSearchUpdates->setChecked(app->autoSearchUpdates);
  topBoxLayout->addWidget( boxSearchUpdates, 9, 0, 1, 2 );

  topBoxLayout->setRowStretch(10, 1);

  appTabWidget->addTab(application, QString());

  initConfirmationsPage();

  appTabWidget->addTab( confirm, QString() );

  appColors = new QWidget();
  QVBoxLayout * appColorsLayout = new QVBoxLayout( appColors );
  QGroupBox * groupBoxAppCol = new QGroupBox();
  appColorsLayout->addWidget( groupBoxAppCol );
  QGridLayout * colorsBoxLayout = new QGridLayout( groupBoxAppCol );

  lblWorkspace = new QLabel();
  colorsBoxLayout->addWidget( lblWorkspace, 0, 0 );
  btnWorkspace = new ColorButton();
  btnWorkspace->setColor(app->workspaceColor);
  colorsBoxLayout->addWidget( btnWorkspace, 0, 1 );

  lblPanels = new QLabel();
  colorsBoxLayout->addWidget( lblPanels, 1, 0 );
  btnPanels = new ColorButton();
  colorsBoxLayout->addWidget( btnPanels, 1, 1 );
  btnPanels->setColor(app->panelsColor);

  lblPanelsText = new QLabel();
  colorsBoxLayout->addWidget( lblPanelsText, 2, 0 );
  btnPanelsText = new ColorButton();
  colorsBoxLayout->addWidget( btnPanelsText, 2, 1 );
  btnPanelsText->setColor(app->panelsTextColor);

  colorsBoxLayout->setRowStretch( 3, 1 );

  appTabWidget->addTab( appColors, QString() );

  numericFormatPage = new QWidget();
  QVBoxLayout *numLayout = new QVBoxLayout( numericFormatPage );
  QGroupBox *numericFormatBox = new QGroupBox();
  numLayout->addWidget( numericFormatBox );
  QGridLayout *numericFormatLayout = new QGridLayout( numericFormatBox );

  lblAppPrecision = new QLabel();
  numericFormatLayout->addWidget(lblAppPrecision, 0, 0);
  boxAppPrecision = new QSpinBox();
  boxAppPrecision->setRange(0, 14);
  boxAppPrecision->setValue(app->d_decimal_digits);
  numericFormatLayout->addWidget(boxAppPrecision, 0, 1);

  lblDecimalSeparator = new QLabel();
  numericFormatLayout->addWidget(lblDecimalSeparator, 1, 0 );
  boxDecimalSeparator = new QComboBox();
  boxDecimalSeparator->addItem(tr("System Locale Setting"));
  boxDecimalSeparator->addItem("1,000.0");
  boxDecimalSeparator->addItem("1.000,0");
  boxDecimalSeparator->addItem("1 000,0");

  numericFormatLayout->addWidget(boxDecimalSeparator, 1, 1);

  boxThousandsSeparator = new QCheckBox();
  boxThousandsSeparator->setChecked(app->locale().numberOptions() & QLocale::OmitGroupSeparator);
  numericFormatLayout->addWidget(boxThousandsSeparator, 2, 0);

  boxUpdateSeparators = new QCheckBox();
  boxUpdateSeparators->setChecked(true);
  numericFormatLayout->addWidget(boxUpdateSeparators, 3, 0);
  numericFormatLayout->setRowStretch(4, 1);

  appTabWidget->addTab( numericFormatPage, QString() );

  initFileLocationsPage();

  // Floating windows page
  floatingWindowsPage = new QWidget();
  QVBoxLayout *floatLayout = new QVBoxLayout(floatingWindowsPage);
  QGroupBox *floatBox = new QGroupBox();
  floatLayout->addWidget(floatBox);
  QGridLayout *floatPageLayout = new QGridLayout(floatBox);

  QLabel* comment = new QLabel("Select types of windows to be floating by default.\n"
    "You can use Windows menu to make a window floating or docked.");
  floatPageLayout->addWidget(comment,0,0);

  boxFloatingGraph = new QCheckBox("Graphs"); // default to true
  boxFloatingGraph->setChecked(app->isDefaultFloating("MultiLayer"));
  floatPageLayout->addWidget(boxFloatingGraph,1,0);

  boxFloatingTable = new QCheckBox("Tables");
  boxFloatingTable->setChecked(app->isDefaultFloating("Table"));
  floatPageLayout->addWidget(boxFloatingTable,2,0);

  boxFloatingInstrumentWindow = new QCheckBox("Instrument views"); // default to true
  boxFloatingInstrumentWindow->setChecked(app->isDefaultFloating("InstrumentWindow"));
  floatPageLayout->addWidget(boxFloatingInstrumentWindow,3,0);

  boxFloatingMantidMatrix = new QCheckBox("Mantid Matrices");
  boxFloatingMantidMatrix->setChecked(app->isDefaultFloating("MantidMatrix"));
  floatPageLayout->addWidget(boxFloatingMantidMatrix,4,0);

  boxFloatingNote = new QCheckBox("Notes");
  boxFloatingNote->setChecked(app->isDefaultFloating("Note"));
  floatPageLayout->addWidget(boxFloatingNote,5,0);

  boxFloatingMatrix = new QCheckBox("Matrices");
  boxFloatingMatrix->setChecked(app->isDefaultFloating("Matrix"));
  floatPageLayout->addWidget(boxFloatingMatrix,6,0);

  boxFloatingCustomInterfaces = new QCheckBox("Custom interfaces");
  boxFloatingCustomInterfaces->setChecked(app->isDefaultFloating("MdiSubWindow"));
  floatPageLayout->addWidget(boxFloatingCustomInterfaces,7,0);

  boxFloatingTiledWindows = new QCheckBox("Tiled Windows");
  boxFloatingTiledWindows->setChecked(app->isDefaultFloating("TiledWindow"));
  floatPageLayout->addWidget(boxFloatingTiledWindows,8,0);

  floatPageLayout->setRowStretch(8,1);
  appTabWidget->addTab(floatingWindowsPage, QString());

  connect( boxLanguage, SIGNAL( activated(int) ), this, SLOT( switchToLanguage(int) ) );
  connect( fontsBtn, SIGNAL( clicked() ), this, SLOT( pickApplicationFont() ) );
  connect( boxSave, SIGNAL( toggled(bool) ), boxMinutes, SLOT( setEnabled(bool) ) );
}

/**
* Configure a Mantid page on the config dialog
*/
void ConfigDialog::initMantidPage()
{
  mtdTabWidget = new QTabWidget(generalDialog);
  mtdTabWidget->setUsesScrollButtons(false);

  instrumentPage = new QWidget();
  QVBoxLayout *instrTabLayout = new QVBoxLayout(instrumentPage);
  QGroupBox *frame = new QGroupBox();
  instrTabLayout->addWidget(frame);
  QGridLayout *grid = new QGridLayout(frame);
  mtdTabWidget->addTab(instrumentPage, QString());

  facility = new QComboBox();
  grid->addWidget(new QLabel("Facility"), 0, 0);
  grid->addWidget(facility, 0, 1);

  defInstr = new MantidQt::MantidWidgets::InstrumentSelector();
  // Here we only want the default instrument updated if the user clicks Ok/Apply
  defInstr->updateInstrumentOnSelection(false);
  grid->addWidget(new QLabel("Default Instrument"), 2, 0);
  grid->addWidget(defInstr, 2, 1);
  grid->setRowStretch(3,1);

  //Ignore paraview.
  ckIgnoreParaView = new QCheckBox("Ignore ParaView");
  ckIgnoreParaView->setToolTip("Don't bother me with anything to do with ParaView.\nRequires restart of MantidPlot to take effect.");
  auto& cfgSvc = ConfigService::Instance();
  const std::string ignoreParaViewProperty = "paraview.ignore";
  bool ignoreParaView =  cfgSvc.hasProperty(ignoreParaViewProperty) && bool(atoi(cfgSvc.getString(ignoreParaViewProperty).c_str()));
  ckIgnoreParaView->setChecked(ignoreParaView);
  grid->addWidget(ckIgnoreParaView, 3, 0);

  // Populate boxes
  auto faclist =  cfgSvc.getFacilityNames();
  for ( auto it = faclist.begin(); it != faclist.end(); ++it )
  {
    facility->addItem(QString::fromStdString(*it));
  }

  // Set default property
  QString property = QString::fromStdString( cfgSvc.getFacility().name());
  int index = facility->findText(property);
  if( index < 0 )
  {
    index = 0;
  }
  facility->setCurrentIndex(index);
  // Ensure update of instrument box with facility change
  connect(facility, SIGNAL(currentIndexChanged(const QString&)), defInstr, SLOT(fillWithInstrumentsFromFacility(const QString &)));

  initDirSearchTab();
  initCurveFittingTab();
  initSendToProgramTab();
  initMantidOptionsTab();
}

/**
 * Configure a MD Plotting Page
 */
void ConfigDialog::initMdPlottingPage()
{
  mdPlottingTabWidget = new QTabWidget(generalDialog);
  mdPlottingTabWidget->setUsesScrollButtons(false);

  // General MD Plotting tab
  initMdPlottingGeneralTab();

  // VSI tab
  initMdPlottingVsiTab();

  // Set the connections
  setupMdPlottingConnections();

  // Update the visibility of the Vsi tab if the General Md Color Map was selected the last time
  if (m_mdSettings.getUsageGeneralMdColorMap())
  {
    changeUsageGeneralMdColorMap(true);
  }

  // Update the visibility of the Vsi tab if the last session checkbox was selected.
  if (m_mdSettings.getUsageLastSession())
  {
    changeUsageLastSession(true);
  }
}

/**
 * Configure the general MD Plotting tab
 */
void ConfigDialog::initMdPlottingGeneralTab()
{
  // Ask if uniform colormap
  mdPlottingGeneralPage = new QWidget();
  QVBoxLayout *generalTabLayout = new QVBoxLayout(mdPlottingGeneralPage);
  mdPlottingGeneralFrame = new QGroupBox(mdPlottingGeneralPage);
  generalTabLayout->addWidget(mdPlottingGeneralFrame );
  mdPlottingTabWidget->addTab(mdPlottingGeneralPage, QString());

  // Color Map
  mdPlottingGeneralFrame->setTitle("Use common Color Map for Slice Viewer and VSI");
  mdPlottingGeneralFrame->setCheckable(true);
  mdPlottingGeneralFrame->setChecked(m_mdSettings.getUsageGeneralMdColorMap());

  QGridLayout *gridVsiGeneralDefaultColorMap = new QGridLayout(mdPlottingGeneralFrame);
  mdPlottingGeneralColorMap = new QComboBox();
  lblGeneralDefaultColorMap = new QLabel();
  gridVsiGeneralDefaultColorMap->addWidget(lblGeneralDefaultColorMap, 1, 0);
  gridVsiGeneralDefaultColorMap->addWidget(mdPlottingGeneralColorMap, 1, 1);

  gridVsiGeneralDefaultColorMap->setRowStretch(2,1);

  QLabel* label = new QLabel("<span style=\"font-weight:600;\">Note: Changes will not take effect until MantidPlot has been restarted.</span>");
  generalTabLayout->addWidget(label);

  // Set the color maps
  MantidQt::API::MdPlottingCmapsProvider mdPlottingCmapsProvider;
  QStringList colorMapNames;
  QStringList colorMapFiles;
  mdPlottingCmapsProvider.getColorMapsForMdPlotting(colorMapNames, colorMapFiles);

  if (colorMapNames.size() == colorMapFiles.size())
  {
    for (int index = 0; index < colorMapNames.size(); ++index)
    {
      mdPlottingGeneralColorMap->addItem(colorMapNames[index], colorMapFiles[index]);
    }
  }

  int currentIndex = mdPlottingGeneralColorMap->findData(m_mdSettings.getGeneralMdColorMapName(), Qt::DisplayRole);
  if (currentIndex != -1)
  {
    mdPlottingGeneralColorMap->setCurrentIndex(currentIndex);
  }
}

/**
 * Configure the VSI tab
 */
void ConfigDialog::initMdPlottingVsiTab()
{
  vsiPage = new QWidget();
  QVBoxLayout *vsiTabLayout = new QVBoxLayout(vsiPage);
  QGroupBox *frame = new QGroupBox();
  vsiTabLayout->addWidget(frame);
  QGridLayout *grid = new QGridLayout(frame);
  mdPlottingTabWidget->addTab(vsiPage, QString());

  // Usage of the last setting
  vsiLastSession = new QCheckBox();
  lblVsiLastSession = new QLabel();
  grid->addWidget(lblVsiLastSession , 0, 0);
  grid->addWidget(vsiLastSession , 0, 1);
  vsiLastSession->setChecked(m_mdSettings.getUsageLastSession());

  // Color Map
  vsiDefaultColorMap = new QComboBox();
  lblVsiDefaultColorMap = new QLabel();
  grid->addWidget(lblVsiDefaultColorMap, 1, 0);
  grid->addWidget(vsiDefaultColorMap, 1, 1);

  // Background Color
  vsiDefaultBackground = new ColorButton();
  lblVsiDefaultBackground = new QLabel();
  grid->addWidget(lblVsiDefaultBackground, 2, 0);
  grid->addWidget(vsiDefaultBackground, 2, 1);

  const QColor backgroundColor = m_mdSettings.getUserSettingBackgroundColor();
  vsiDefaultBackground->setColor(backgroundColor);

  // Initial View when loading into the VSI
  vsiInitialView = new QComboBox();
  lblVsiInitialView = new QLabel();
  grid->addWidget(lblVsiInitialView, 3, 0);
  grid->addWidget(vsiInitialView, 3, 1);

  grid->setRowStretch(4,1);

  QLabel* label1 = new QLabel("<span style=\"font-weight:600;\">Note: The General Tab settings take precedence over the VSI Tab settings.</span>");
  vsiTabLayout->addWidget(label1);
  QLabel* label2 = new QLabel("<span style=\"font-weight:600;\">Note: Changes will not take effect until the VSI has been restarted.</span>");
  vsiTabLayout->addWidget(label2);

  // Set the color map selection for the VSI
  QStringList maps;
  MantidQt::API::MdPlottingCmapsProvider mdPlottingCmapsProvider;
  mdPlottingCmapsProvider.getColorMapsForVSI(maps);

  MantidQt::API::MdConstants mdConstants;
  vsiDefaultColorMap->addItems(mdConstants.getVsiColorMaps());
  vsiDefaultColorMap->addItems(maps);

  int index = vsiDefaultColorMap->findData(m_mdSettings.getUserSettingColorMap(), Qt::DisplayRole);
  if (index != -1)
  {
    vsiDefaultColorMap->setCurrentIndex(index);
  }

  // Set the initial view selection for the VSI
  QStringList views;

  views = mdConstants.getAllInitialViews();
  vsiInitialView->addItems(views);

  int indexInitialView = vsiInitialView->findData(m_mdSettings.getUserSettingInitialView(), Qt::DisplayRole);

  if (index != -1)
  {
    vsiInitialView->setCurrentIndex(indexInitialView);
  }
}

/**
 * Set up the connections for Md Plotting
 */
void ConfigDialog::setupMdPlottingConnections()
{
  QObject::connect(this->mdPlottingGeneralFrame, SIGNAL(toggled(bool)), this, SLOT(changeUsageGeneralMdColorMap(bool)));
  QObject::connect(this->vsiLastSession, SIGNAL(toggled(bool)), this, SLOT(changeUsageLastSession(bool)));
}

/**
 * Handle a change of the General Md Color Map selection.
 * @param The state of the general MD color map checkbox
 */
void ConfigDialog::changeUsageGeneralMdColorMap(bool state)
{
  // Set the visibility of the default color map of the VSI
  vsiDefaultColorMap->setDisabled(state);
  lblVsiDefaultColorMap->setDisabled(state);
}

/**
 * Handle a change of the Last Session selection.
  * @param The state of the last session checkbox.
 */
void ConfigDialog::changeUsageLastSession(bool state)
{
  // Set the visibility of the default color map of the VSI
  if (!mdPlottingGeneralFrame->isChecked())
  {
    vsiDefaultColorMap->setDisabled(state);
    lblVsiDefaultColorMap->setDisabled(state);
  }

  // Set the visibility of the background color button of the VSI
  vsiDefaultBackground->setDisabled(state);
  lblVsiDefaultBackground->setDisabled(state);
}


/**
* Configure a Mantid Options page on the config dialog
*/
void ConfigDialog::initMantidOptionsTab()
{
  mantidOptionsPage = new QWidget();
  mtdTabWidget->addTab(mantidOptionsPage,"Options");
  QVBoxLayout *widgetLayout = new QVBoxLayout(mantidOptionsPage);
  QGroupBox *frame = new QGroupBox();
  widgetLayout->addWidget(frame);
  QGridLayout *grid = new QGridLayout(frame);

  // if on, for example all plotSpectrum will go to the same window.
  m_reusePlotInstances =  new QCheckBox("Re-use plot instances for different types of plots");
  m_reusePlotInstances->setChecked(false);
  grid->addWidget(m_reusePlotInstances,0,0);
  QString setting = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("MantidOptions.ReusePlotInstances"));
  if(!setting.compare("On"))
  {
    m_reusePlotInstances->setChecked(true);
  }
  else if(!setting.compare("Off"))
  {
    m_reusePlotInstances->setChecked(false);
  }
  m_reusePlotInstances->setToolTip("If on, the same plot instance will be re-used for every of the different plots available in the workspaces window (spectrum, slice, color fill, etc.)");

  //create a checkbox for invisible workspaces options
  m_invisibleWorkspaces = new QCheckBox("Show Invisible Workspaces");
  m_invisibleWorkspaces->setChecked(false);
  grid->addWidget(m_invisibleWorkspaces,1,0);

  setting = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("MantidOptions.InvisibleWorkspaces"));
  if(!setting.compare("1"))
  {
    m_invisibleWorkspaces->setChecked(true);
  }
  else if(!setting.compare("0"))
  {
    m_invisibleWorkspaces->setChecked(false);
  }

  //categories tree widget
  treeCategories = new QTreeWidget(frame);
  treeCategories->setColumnCount(1);
  treeCategories->setSortingEnabled(false);
  treeCategories->setHeaderLabel("Show Algorithm Categories");

  grid->addWidget(treeCategories,2,0);
  refreshTreeCategories();

  // create a checkbox for the instrument view OpenGL option
  m_useOpenGL = new QCheckBox("Use OpenGL in Instrument View");
  m_useOpenGL->setChecked(true);
  grid->addWidget(m_useOpenGL,4,0);

  setting = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().
    getString("MantidOptions.InstrumentView.UseOpenGL")).toUpper();
  if( setting == "ON")
  {
    m_useOpenGL->setChecked(true);
  }
  else
  {
    m_useOpenGL->setChecked(false);
  }

}


void ConfigDialog::initSendToProgramTab()
{
  mantidSendToPage = new QWidget();
  mtdTabWidget->addTab(mantidSendToPage, tr("Send To"));
  QVBoxLayout *widgetLayout = new QVBoxLayout(mantidSendToPage);
  QGroupBox *frame = new QGroupBox();
  widgetLayout ->addWidget(frame);
  QGridLayout *grid = new QGridLayout(frame);


  //Add buttons to the bottom of the widget
  deleteButton = new QPushButton(tr("Delete"));
  deleteButton->setEnabled(false);
  connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteDialog()));
  editButton = new QPushButton(tr("Edit..."));
  editButton->setEnabled(false);
  connect(editButton, SIGNAL(clicked()), this, SLOT(editDialog()));
  addButton = new QPushButton(tr("Add..."));
  connect(addButton, SIGNAL(clicked()), this, SLOT(addDialog()));

  QHBoxLayout *buttons = new QHBoxLayout;
  buttons->addStretch();
  buttons->addWidget(deleteButton);
  buttons->addWidget(editButton);
  buttons->addWidget(addButton);

  widgetLayout->addLayout(buttons);

  //create tree diagram for all known programs that can be saved to
  treePrograms = new QTreeWidget(frame);
  treePrograms->setSelectionMode( QAbstractItemView::ExtendedSelection );
  treePrograms->setColumnCount(1);
  treePrograms->setSortingEnabled(false);
  treePrograms->setHeaderLabel(tr("List of Current Programs"));

  grid->addWidget(treePrograms, 0,0);

  populateProgramTree();
  connect(treePrograms,SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(itemCheckedChanged(QTreeWidgetItem*)));
  connect(treePrograms,SIGNAL(itemSelectionChanged()), this, SLOT(enableButtons()));

}

void ConfigDialog::enableButtons()
{
  QList<QTreeWidgetItem *> selectedItems = treePrograms->selectedItems();
  //Set the buttons on whether the conditions are met. Reducing the amount of user errors
  if (selectedItems.size() == 0)
  {
    deleteButton->setEnabled(false);
    editButton->setEnabled(false);
  }
  else if (selectedItems.size() == 1)
  {
    deleteButton->setEnabled(true);
    editButton->setEnabled(true);
  }
  else
  {
    deleteButton->setEnabled(true);
    editButton->setEnabled(false);
  }
}

void ConfigDialog::itemCheckedChanged(QTreeWidgetItem* item)
{
  if (item->childCount() > 0)
  {
    treePrograms->setCurrentItem(item);
  }
  //enableButtons();
  std::string visibility = "Yes";
  if (item->checkState(0) != Qt::Checked)
  {
    visibility = "No";
  }
  auto it = m_sendToSettings.find(item->text(0).toStdString());
  if (it != m_sendToSettings.end())
  {
    it->second["visible"] = visibility;
    updateChildren(it->second, item);
  }
}


//Add a program
void ConfigDialog::addDialog()
{
  SendToProgramDialog* addProgram = new SendToProgramDialog(this);
  addProgram->setModal(true);
  if(addProgram->exec() == 1)
  {
    //Get the settings of the program the user just added
    std::pair<std::string, std::map<std::string,std::string> > tempSettings = addProgram->getSettings();
    m_sendToSettings[tempSettings.first] = tempSettings.second;
  }

  //clear the tree and repopulate it without the programs that have just been deleted
  treePrograms->clear();
  updateProgramTree();
  enableButtons();
}

//Edit a program
void ConfigDialog::editDialog()
{
  auto firstSelectedItem = treePrograms->selectedItems()[0];
  auto selectedProgram = m_sendToSettings.find(firstSelectedItem->text(0).toStdString());
  // If the program name itself isn't initially selected, recurse up.
  while ( selectedProgram == m_sendToSettings.end() )
  {
    firstSelectedItem = treePrograms->itemAbove(firstSelectedItem);
    // It shouldn't happen that we get to the top without finding anything, but should this happen just return
    if ( firstSelectedItem == treePrograms->invisibleRootItem() ) return;
    selectedProgram = m_sendToSettings.find(firstSelectedItem->text(0).toStdString());
  }

  SendToProgramDialog* editProgram = new SendToProgramDialog(this, firstSelectedItem->text(0), selectedProgram->second);

  editProgram->setWindowTitle(tr("Edit a Program"));
  editProgram->setModal(true);
  if(editProgram->exec() == 1)
  {
    //Get the settings of the program the user just edited
    std::pair<std::string, std::map<std::string,std::string> > tempSettings = editProgram->getSettings();
    m_sendToSettings[tempSettings.first] = tempSettings.second;
  }

  //clear the tree and repopulate it without the programs that have just been deleted
  treePrograms->clear();
  updateProgramTree();
  enableButtons();
}


//Deleting send to options. Deletes them off the mantid.user.properties
void ConfigDialog::deleteDialog()
{
  QList<QTreeWidgetItem *> selectedItems = treePrograms->selectedItems();
  if(selectedItems.size() > 0)
  {
    //Question box asking to continue to avoid accidental deletion of program options
    int status = QMessageBox::question(this, tr("Delete save options?"), tr("Are you sure you want to delete \nthe (%1) selected save option(s)?").arg(selectedItems.size()),
      QMessageBox::Yes|QMessageBox::Default,
      QMessageBox::No|QMessageBox::Escape,
      QMessageBox::NoButton);

    if(status == QMessageBox::Yes)
    {
      //For each program selected, remove all details from the user.properties file;
      for (int i = 0; i<selectedItems.size(); ++i)
      {
        m_sendToSettings.erase(selectedItems[i]->text(0).toStdString());
      }
      //clear the tree and repopulate it without the programs that have just been deleted
      treePrograms->clear();
      updateProgramTree();
    }
  }
}


void ConfigDialog::populateProgramTree()
{
  std::vector<std::string> programNames = Mantid::Kernel::ConfigService::Instance().getKeys("workspace.sendto.name");

  for(size_t i = 0; i<programNames.size(); i++)
  {
    //Create a map for the keys and details to go into
    std::map<std::string,std::string> programKeysAndDetails;

    //Get a list of the program detail keys (mandatory - target, saveusing) (optional - arguments, save parameters, workspace type)
    std::vector<std::string> programKeys = (Mantid::Kernel::ConfigService::Instance().getKeys("workspace.sendto." + programNames[i]));

    for (size_t j = 0; j<programKeys.size(); j++)
    {
      //Assign a key to its value using the map
      programKeysAndDetails[programKeys[j]] = (Mantid::Kernel::ConfigService::Instance().getString(("workspace.sendto." + programNames[i] + "." + programKeys[j])));
    }

    m_sendToSettings.insert(std::make_pair(programNames[i], programKeysAndDetails));
  }
  updateProgramTree();
}

void ConfigDialog::updateProgramTree()
{
  //Store into a map ready to go into config service when apply is clicked
  std::map<std::string, std::map<std::string,std::string> >::const_iterator itr = m_sendToSettings.begin();
  for( ; itr != m_sendToSettings.end(); ++itr)
  {
    //creating the map of kvps needs to happen first as createing the item requires them.
    std::map<std::string, std::string> programKeysAndDetails = itr->second;

    //Populate list
    QTreeWidgetItem *program = createCheckedTreeItem(QString::fromStdString(itr->first), (programKeysAndDetails.find("visible")->second == "Yes"));
    treePrograms->addTopLevelItem(program);
    updateChildren(programKeysAndDetails, program);
  }
}

void ConfigDialog::updateChildren(std::map<std::string, std::string> &programKeysAndDetails, QTreeWidgetItem* program)
{
  program->takeChildren();
  //get the current program's (itr) keys and values (pItr)
  std::map<std::string,std::string>::const_iterator pItr = programKeysAndDetails.begin();
  for( ; pItr != programKeysAndDetails.end(); ++pItr)
  {
    QTreeWidgetItem *item = new QTreeWidgetItem(program);
    item->setText(0, tr("   " + QString::fromStdString(pItr->first) + " --- " + QString::fromStdString(pItr->second)));
    program->addChild(item);
  }
}

void ConfigDialog::updateSendToTab()
{
  Mantid::Kernel::ConfigServiceImpl&  cfgSvc = Mantid::Kernel::ConfigService::Instance();

  //Add new values to the config service
  std::map<std::string, std::map<std::string,std::string> >::const_iterator itr = m_sendToSettings.begin();
  std::vector<std::string> programNames =  cfgSvc.getKeys("workspace.sendto.name");

  for( ; itr != m_sendToSettings.end(); ++itr)
  {
    for (size_t i = 0; i<programNames.size(); ++i)
    {
      if (programNames[i] == itr->first)
      {
        //The selected program hasn't been deleted so set to blank string (all those left without blank strings are to be deleted
        programNames[i] = "";
      }
    }

     cfgSvc.setString("workspace.sendto.name." + itr->first , "0");

    std::map<std::string, std::string> programKeysAndDetails = itr->second;

    std::map<std::string, std::string>::const_iterator pItr = programKeysAndDetails.begin();

    for( ; pItr != programKeysAndDetails.end(); ++pItr)
    {
      if(pItr->second != "")
         cfgSvc.setString("workspace.sendto." + itr->first + "." + pItr->first, pItr->second);
    }
  }

  //Delete the keys that are in the config but not in the temporary m_sendToSettings map
  for (size_t i = 0; i<programNames.size(); ++i)
  {
    if (programNames[i] != "")
    {
       cfgSvc.remove("workspace.sendto.name." + programNames[i]);
      std::vector<std::string> programKeys =  cfgSvc.getKeys("workspace.sendto." + programNames[i]);
      for (size_t j = 0; j<programKeys.size(); ++j)
      {
         cfgSvc.remove("workspace.sendto." + programNames[i] + "." +  programKeys[j]);
      }
    }
  }
}

void ConfigDialog::refreshTreeCategories()
{
  treeCategories->clear();

  typedef std::map<std::string,bool> categoriesType;
  categoriesType categoryMap = Mantid::API::AlgorithmFactory::Instance().getCategoriesWithState();

  QMap<QString,QTreeWidgetItem*> categories;// keeps track of categories added to the tree

  for(categoriesType::const_iterator i=categoryMap.begin();i!=categoryMap.end();++i)
  {
    QString catName = QString::fromStdString(i->first);
    bool isHidden = i->second;
    QStringList subCats = catName.split('\\');
    if (subCats.size() == 1)
    {
      QTreeWidgetItem *catItem = createCheckedTreeItem(catName,!isHidden);
      categories.insert(catName,catItem);
      treeCategories->addTopLevelItem(catItem);
    }
    else
    {
      QString cn = subCats[0];
      QTreeWidgetItem *catItem = 0;
      int n = subCats.size();
      for(int j=0;j<n;j++)
      {
        if (categories.contains(cn))
        {
          catItem = categories[cn];
        }
        else
        {
          QTreeWidgetItem *newCatItem = createCheckedTreeItem(subCats[j],!isHidden);
          categories.insert(cn,newCatItem);
          if (!catItem)
          {
            treeCategories->addTopLevelItem(newCatItem);
          }
          else
          {
            catItem->addChild(newCatItem);
          }
          catItem = newCatItem;
        }
        if (j != n-1) cn += "\\" + subCats[j+1];
      }
    }
  }
}

QTreeWidgetItem* ConfigDialog::createCheckedTreeItem(QString name,bool checkBoxState)
{
  QTreeWidgetItem *item = new QTreeWidgetItem(QStringList(name));
  item->setFlags(item->flags()|Qt::ItemIsUserCheckable);
  if (checkBoxState)
  {
    item->setCheckState(0,Qt::Checked);
  }
  else
  {
    item->setCheckState(0,Qt::Unchecked);
  }
  return item;
}

void ConfigDialog::initDirSearchTab()
{

  directoriesPage = new QWidget();
  QVBoxLayout *dirTabLayout = new QVBoxLayout(directoriesPage);
  QGroupBox *frame = new QGroupBox();
  dirTabLayout->addWidget(frame);
  QGridLayout *grid = new QGridLayout(frame);
  mtdTabWidget->addTab(directoriesPage, "Directories");

  /// pythonscripts.directories

  QLabel* label = new QLabel(tr("Python scripts"));
  grid->addWidget(label, 0, 0);

  std::string str = Mantid::Kernel::ConfigService::Instance().getString("pythonscripts.directories");
  lePythonScriptsDirs = new QLineEdit();
  lePythonScriptsDirs->setText(QString::fromStdString(str));
  grid->addWidget(lePythonScriptsDirs, 0, 1);

  QPushButton *button = new QPushButton();
  button->setIcon(QIcon(getQPixmap("choose_folder_xpm")));
  grid->addWidget(button, 0, 2);

  connect( button, SIGNAL(clicked()), this, SLOT(addPythonScriptsDirs()) );

  /// pythonscripts.directories

  label = new QLabel(tr("Python extensions (algorithms,fit functions)"));
  label->setWordWrap(true);
  grid->addWidget(label, 1, 0);

  str = Mantid::Kernel::ConfigService::Instance().getString("user.python.plugins.directories");
  lePythonPluginsDirs = new QLineEdit();
  lePythonPluginsDirs->setText(QString::fromStdString(str));
  grid->addWidget(lePythonPluginsDirs, 1, 1);

  button = new QPushButton();
  button->setIcon(QIcon(getQPixmap("choose_folder_xpm")));
  grid->addWidget(button, 1, 2);

  connect( button, SIGNAL(clicked()), this, SLOT(addPythonPluginDirs()) );

  /// instrumentDefinition.directory
  label = new QLabel(tr("Instrument definitions"));
  grid->addWidget(label, 2, 0);

  str = Mantid::Kernel::ConfigService::Instance().getString("instrumentDefinition.directory");
  leInstrumentDir = new QLineEdit();
  leInstrumentDir->setText(QString::fromStdString(str));
  grid->addWidget(leInstrumentDir, 2, 1);

  button = new QPushButton();
  button->setIcon(QIcon(getQPixmap("choose_folder_xpm")));
  grid->addWidget(button, 2, 2);

  connect( button, SIGNAL(clicked()), this, SLOT(addInstrumentDir()) );

  grid->setRowStretch(3,1);
}

void ConfigDialog::initCurveFittingTab()
{
  curveFittingPage = new QWidget();
  QVBoxLayout *curveTabLayout = new QVBoxLayout(curveFittingPage);
  QGroupBox *frame = new QGroupBox();
  curveTabLayout->addWidget(frame);
  QGridLayout *grid = new QGridLayout(frame);
  mtdTabWidget->addTab(curveFittingPage, "Curve Fitting");

  // Background functions list
  grid->addWidget(new QLabel(tr("Auto background")),0,0);
  backgroundFunctions = new QComboBox();
  grid->addWidget(backgroundFunctions, 0, 1);

  QLabel *label = new QLabel(tr("Background arguments"));
  QString tip = tr("A space-separated list of name=value arguments, \n"
    "i.e. a=1 b=2");
  label->setToolTip(tip);
  grid->addWidget(label,1,0);
  functionArguments = new QLineEdit();
  functionArguments->setToolTip(tip);
  grid->addWidget(functionArguments, 1,1);

  grid->addWidget(new QLabel(tr("Default peak shape")),2,0);
  defaultPeakShape = new QComboBox();
  grid->addWidget(defaultPeakShape, 2,1);

  grid->addWidget(new QLabel(tr("FindPeaks FWHM")),3,0);
  findPeaksFWHM = new QSpinBox();
  grid->addWidget(findPeaksFWHM, 3,1);

  grid->addWidget(new QLabel(tr("FindPeaks Tolerance")),4,0);
  findPeaksTolerance = new QSpinBox();
  findPeaksTolerance->setMaximum(1000000);
  grid->addWidget(findPeaksTolerance, 4,1);

  grid->addWidget(new QLabel(tr("Peak Radius (in FWHM)")),5,0);
  peakRadius = new QSpinBox();
  peakRadius->setMaximum(std::numeric_limits<int>::max());
  grid->addWidget(peakRadius, 5,1);

  grid->addWidget(new QLabel(tr("Double property decimals")),6,0);
  decimals = new QSpinBox();
  grid->addWidget(decimals, 6,1);

  grid->setRowStretch(7,1);
  label = new QLabel("<span style=\"font-weight:600;\">Note: Changes will not take effect until MantidPlot has been restarted.</span>");
  curveTabLayout->addWidget(label);

  // Find list of background functions
  // Add none option
  backgroundFunctions->addItem("None");
  Mantid::API::FunctionFactoryImpl & function_creator = Mantid::API::FunctionFactory::Instance();
  std::vector<std::string> allfunctions = function_creator.getKeys();
  size_t nfuncs = allfunctions.size();
  for( size_t i = 0; i < nfuncs; ++i )
  {
    std::string name = allfunctions[i];
    auto function = function_creator.createFunction(name);
    if( dynamic_cast<Mantid::API::IBackgroundFunction*>(function.get()) )
    {
      backgroundFunctions->addItem(QString::fromStdString(name));
    }
    if( dynamic_cast<Mantid::API::IPeakFunction*>(function.get()) )
    {
      defaultPeakShape->addItem(QString::fromStdString(name));
    }
  }

  ApplicationWindow *app = dynamic_cast<ApplicationWindow *>(this->parentWidget());

  // Set the correct default property
  QString setting = app->mantidUI->fitFunctionBrowser()->getAutoBackgroundString();
  QStringList value = setting.split(' ');
  int index(-1);
  if( value.isEmpty() )
  {
    index = 0;
  }
  else
  {
    index = backgroundFunctions->findText(value[0], Qt::MatchFixedString);// Case insensitive
    if( value.size() > 1 )
    {
      value.removeFirst();
      QString args = value.join(" ");
      functionArguments->setText(args);
    }
  }
  if( index < 0 )
  {
    backgroundFunctions->setCurrentIndex(0);
  }
  else
  {
    backgroundFunctions->setCurrentIndex(index);
  }

  setting = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("curvefitting.defaultPeak"));
  if (setting.isEmpty()) setting = "Gaussian";
  index = defaultPeakShape->findText(setting);
  if (index >= 0)
  {
    defaultPeakShape->setCurrentIndex(index);
  }

  setting = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("curvefitting.findPeaksFWHM"));
  if (!setting.isEmpty())
  {
    findPeaksFWHM->setValue(setting.toInt());
  }
  else
  {
    findPeaksFWHM->setValue(7);
  }

  setting = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("curvefitting.findPeaksTolerance"));
  if (!setting.isEmpty())
  {
    findPeaksTolerance->setValue(setting.toInt());
  }
  else
  {
    findPeaksTolerance->setValue(4);
  }

  setting = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("curvefitting.peakRadius"));
  if (!setting.isEmpty())
  {
    peakRadius->setValue(setting.toInt());
  }
  else
  {
    peakRadius->setValue(5);
  }

  decimals->setValue(app->mantidUI->fitFunctionBrowser()->getDecimals());

}


void ConfigDialog::initOptionsPage()
{
  ApplicationWindow *app = dynamic_cast<ApplicationWindow *>(this->parentWidget());

  plotOptions = new QWidget();

  QVBoxLayout * optionsTabLayout = new QVBoxLayout( plotOptions );
  optionsTabLayout->setSpacing(5);

  QGroupBox * groupBoxOptions = new QGroupBox();
  optionsTabLayout->addWidget( groupBoxOptions );

  QGridLayout * optionsLayout = new QGridLayout( groupBoxOptions );

  boxAutoscaling = new QCheckBox();
  boxAutoscaling->setChecked(app->autoscale2DPlots);
  optionsLayout->addWidget( boxAutoscaling, 0, 0 );

  boxScaleFonts = new QCheckBox();
  boxScaleFonts->setChecked(app->autoScaleFonts);
  optionsLayout->addWidget( boxScaleFonts, 0, 1 );

  boxTitle = new QCheckBox();
  boxTitle->setChecked(app->titleOn);
  optionsLayout->addWidget( boxTitle, 1, 0 );

  boxAntialiasing = new QCheckBox();
  boxAntialiasing->setChecked(app->antialiasing2DPlots);
  optionsLayout->addWidget( boxAntialiasing, 1, 1 );

  boxAspectRatio  = new QCheckBox();
  boxAspectRatio->setChecked(app->fixedAspectRatio2DPlots);
  optionsLayout->addWidget( boxAspectRatio, 2, 1);
#if QWT_VERSION < 0x050200
  boxAspectRatio->setChecked(false);
  boxAspectRatio->setVisible(true);
#endif
  boxFrame = new QCheckBox();
  boxFrame->setChecked(app->canvasFrameWidth > 0);
  optionsLayout->addWidget( boxFrame, 2, 0 );

  boxDistribution = new QCheckBox();
  boxDistribution->setChecked(app->autoDistribution1D);
  optionsLayout->addWidget( boxDistribution, 3, 0);

  labelFrameWidth = new QLabel();
  optionsLayout->addWidget( labelFrameWidth, 4, 0 );
  boxFrameWidth= new QSpinBox();
  optionsLayout->addWidget( boxFrameWidth, 4, 1 );
  boxFrameWidth->setRange(1, 100);
  boxFrameWidth->setValue(app->canvasFrameWidth);
  if (!app->canvasFrameWidth)
  {
    labelFrameWidth->hide();
    boxFrameWidth->hide();
  }

  lblMargin = new QLabel();
  optionsLayout->addWidget( lblMargin, 5, 0 );
  boxMargin= new QSpinBox();
  boxMargin->setRange(0, 1000);
  boxMargin->setSingleStep(5);
  boxMargin->setValue(app->defaultPlotMargin);
  optionsLayout->addWidget( boxMargin, 5, 1 );

  optionsLayout->setRowStretch( 7, 1 );

  boxResize = new QCheckBox();
  boxResize->setChecked(!app->autoResizeLayers);
  if(boxResize->isChecked())
    boxScaleFonts->setEnabled(false);

  optionsTabLayout->addWidget( boxResize );

  boxLabelsEditing = new QCheckBox();
  boxLabelsEditing->setChecked(!app->d_in_place_editing);
  optionsTabLayout->addWidget(boxLabelsEditing);

  plotsTabWidget->addTab( plotOptions, QString() );
}

void ConfigDialog::initAxesPage()
{
  ApplicationWindow *app = dynamic_cast<ApplicationWindow *>(this->parentWidget());
  if (!app)
    return;

  axesPage = new QWidget();

  QGroupBox * axisOptions = new QGroupBox();
  QGridLayout * axisOptionsLayout = new QGridLayout( axisOptions );

  boxBackbones = new QCheckBox();
  boxBackbones->setChecked(app->drawBackbones);
  axisOptionsLayout->addWidget(boxBackbones, 0, 0);

  boxSynchronizeScales = new QCheckBox();
  boxSynchronizeScales->setChecked(app->d_synchronize_graph_scales);
  axisOptionsLayout->addWidget(boxSynchronizeScales, 0, 1);

  lblAxesLineWidth = new QLabel();
  axisOptionsLayout->addWidget(lblAxesLineWidth, 1, 0);
  boxLineWidth= new QSpinBox();
  boxLineWidth->setRange(0, 100);
  boxLineWidth->setValue(app->axesLineWidth);
  axisOptionsLayout->addWidget(boxLineWidth, 1, 1);

  labelGraphAxesLabelsDist = new QLabel();
  axisOptionsLayout->addWidget(labelGraphAxesLabelsDist, 2, 0);
  boxAxesLabelsDist = new QSpinBox();
  boxAxesLabelsDist->setRange(0, 1000);
  boxAxesLabelsDist->setValue(app->d_graph_axes_labels_dist);
  axisOptionsLayout->addWidget(boxAxesLabelsDist, 2, 1);

  labelTickLabelsDist = new QLabel();
  axisOptionsLayout->addWidget(labelTickLabelsDist, 3, 0);
  boxTickLabelsDist = new QSpinBox();
  boxTickLabelsDist->setRange(0, 1000);
  boxTickLabelsDist->setValue(app->d_graph_tick_labels_dist);
  axisOptionsLayout->addWidget(boxTickLabelsDist, 3, 1);
  axisOptionsLayout->setRowStretch(4, 1);

  enabledAxesGroupBox = new QGroupBox();
  enabledAxesGrid = new QGridLayout( enabledAxesGroupBox );

  enableAxisLabel = new QLabel();
  enabledAxesGrid->addWidget(enableAxisLabel, 0, 2);
  showNumbersLabel = new QLabel();
  enabledAxesGrid->addWidget(showNumbersLabel, 0, 3);
  scaleLabel = new QLabel();
  enabledAxesGrid->addWidget(scaleLabel, 0, 4);

  QLabel *pixLabel = new QLabel();
  pixLabel->setPixmap (QPixmap (":/left_axis.png"));
  enabledAxesGrid->addWidget(pixLabel, 1, 0);
  yLeftLabel = new QLabel();
  enabledAxesGrid->addWidget(yLeftLabel, 1, 1);

  pixLabel = new QLabel();
  pixLabel->setPixmap (QPixmap (":/right_axis.png"));
  enabledAxesGrid->addWidget(pixLabel, 2, 0);
  yRightLabel = new QLabel();
  enabledAxesGrid->addWidget(yRightLabel, 2, 1);

  pixLabel = new QLabel();
  pixLabel->setPixmap (QPixmap (":/bottom_axis.png"));
  enabledAxesGrid->addWidget(pixLabel, 3, 0);
  xBottomLabel = new QLabel();
  enabledAxesGrid->addWidget(xBottomLabel, 3, 1);

  pixLabel = new QLabel();
  pixLabel->setPixmap (QPixmap (":/top_axis.png"));
  enabledAxesGrid->addWidget(pixLabel, 4, 0);
  xTopLabel = new QLabel();
  enabledAxesGrid->addWidget(xTopLabel, 4, 1);

  for (int i = 0; i < QwtPlot::axisCnt; i++){
    QCheckBox *box1 = new QCheckBox();
    int row = i + 1;

    enabledAxesGrid->addWidget(box1, row, 2);
    bool enabledAxis = app->d_show_axes[i];
    box1->setChecked(enabledAxis);

    QCheckBox *box2 = new QCheckBox();
    enabledAxesGrid->addWidget(box2, row, 3);
    box2->setChecked(app->d_show_axes_labels[i]);
    box2->setEnabled(enabledAxis);

    connect(box1, SIGNAL(toggled(bool)), box2, SLOT(setEnabled(bool)));

    QComboBox *box3 = new QComboBox();
    enabledAxesGrid->addWidget(box3, row, 4);
    box3->addItem(tr("linear"));
    box3->addItem("log");
    if ( app->d_axes_scales[i] == "log" ) box3->setCurrentIndex(1);
  }
  enabledAxesGrid->setColumnStretch (0, 0);
  enabledAxesGrid->setColumnStretch (1, 1);
  enabledAxesGrid->setColumnStretch (2, 1);
  enabledAxesGrid->setColumnStretch (3, 1);

  QVBoxLayout * axesPageLayout = new QVBoxLayout( axesPage );
  axesPageLayout->addWidget(axisOptions);
  axesPageLayout->addWidget(enabledAxesGroupBox);
}

void ConfigDialog::initCurvesPage()
{
  ApplicationWindow *app = dynamic_cast<ApplicationWindow *>(this->parentWidget());

  curves = new QWidget();

  QGroupBox * curvesGroupBox = new QGroupBox();
  QGridLayout * curvesBoxLayout = new QGridLayout( curvesGroupBox );

  lblCurveStyle = new QLabel();
  curvesBoxLayout->addWidget( lblCurveStyle, 0, 0 );
  boxCurveStyle = new QComboBox();
  curvesBoxLayout->addWidget( boxCurveStyle, 0, 1 );

  lblLineWidth = new QLabel();
  curvesBoxLayout->addWidget( lblLineWidth, 1, 0 );
  boxCurveLineWidth = new DoubleSpinBox('f');
  boxCurveLineWidth->setLocale(app->locale());
  boxCurveLineWidth->setSingleStep(0.1);
  boxCurveLineWidth->setRange(0.1, 100);
  boxCurveLineWidth->setValue(app->defaultCurveLineWidth);
  curvesBoxLayout->addWidget( boxCurveLineWidth, 1, 1 );

  lblSymbSize = new QLabel();
  curvesBoxLayout->addWidget( lblSymbSize, 2, 0 );
  boxSymbolSize = new QSpinBox();
  boxSymbolSize->setRange(1,100);
  boxSymbolSize->setValue(app->defaultSymbolSize/2);
  curvesBoxLayout->addWidget( boxSymbolSize, 2, 1 );

  cbApplyToMantid = new QCheckBox("Apply to Mantid");
  if (app->applyCurveStyleToMantid)
  {
    cbApplyToMantid->setChecked(true);
  }
  else
  {
    cbApplyToMantid->setChecked(false);
  }
  curvesBoxLayout->addWidget(cbApplyToMantid, 3, 1);

  cbDrawAllErrors = new QCheckBox("Draw all errors");
  cbDrawAllErrors->setChecked( app->drawAllErrors );
  curvesBoxLayout->addWidget(cbDrawAllErrors, 4, 1);

  curvesBoxLayout->setRowStretch( 5, 1 );

  QHBoxLayout * curvesPageLayout = new QHBoxLayout( curves );
  curvesPageLayout->addWidget( curvesGroupBox );
}

void ConfigDialog::initFittingPage()
{
  ApplicationWindow *app = dynamic_cast<ApplicationWindow *>(this->parentWidget());
  fitPage = new QWidget();

  groupBoxFittingCurve = new QGroupBox();
  QGridLayout * fittingCurveLayout = new QGridLayout(groupBoxFittingCurve);
  fittingCurveLayout->setSpacing(5);

  generatePointsBtn = new QRadioButton();
  generatePointsBtn->setChecked(app->generateUniformFitPoints);
  fittingCurveLayout->addWidget(generatePointsBtn, 0, 0);

  lblPoints = new QLabel();
  fittingCurveLayout->addWidget(lblPoints, 0, 1);
  generatePointsBox = new QSpinBox();
  generatePointsBox->setRange(0, 1000000);
  generatePointsBox->setSingleStep(10);
  generatePointsBox->setValue(app->fitPoints);
  fittingCurveLayout->addWidget(generatePointsBox, 0, 2);

  linearFit2PointsBox = new QCheckBox();
  linearFit2PointsBox->setChecked(app->d_2_linear_fit_points);
  fittingCurveLayout->addWidget(linearFit2PointsBox, 0, 3);

  showPointsBox(!app->generateUniformFitPoints);

  samePointsBtn = new QRadioButton();
  samePointsBtn->setChecked(!app->generateUniformFitPoints);
  fittingCurveLayout->addWidget(samePointsBtn, 1, 0);

  groupBoxMultiPeak = new QGroupBox();
  groupBoxMultiPeak->setCheckable(true);
  groupBoxMultiPeak->setChecked(app->generatePeakCurves);

  QHBoxLayout * multiPeakLayout = new QHBoxLayout(groupBoxMultiPeak);

  lblPeaksColor = new QLabel();
  multiPeakLayout->addWidget(lblPeaksColor);
  boxPeaksColor = new ColorBox(0);
  boxPeaksColor->setCurrentItem(app->peakCurvesColor);
  multiPeakLayout->addWidget(boxPeaksColor);

  groupBoxFitParameters = new QGroupBox();
  QGridLayout * fitParamsLayout = new QGridLayout(groupBoxFitParameters);

  lblPrecision = new QLabel();
  fitParamsLayout->addWidget(lblPrecision, 0, 0);
  boxPrecision = new QSpinBox();
  fitParamsLayout->addWidget(boxPrecision, 0, 1);
  boxPrecision->setValue(app->fit_output_precision);

  logBox = new QCheckBox();
  logBox->setChecked(app->writeFitResultsToLog);
  fitParamsLayout->addWidget(logBox, 1, 0);

  plotLabelBox = new QCheckBox();
  plotLabelBox->setChecked(app->pasteFitResultsToPlot);
  fitParamsLayout->addWidget(plotLabelBox, 2, 0);

  scaleErrorsBox = new QCheckBox();
  fitParamsLayout->addWidget(scaleErrorsBox, 3, 0);
  scaleErrorsBox->setChecked(app->fit_scale_errors);

  cbEnableQtiPlotFitting = new QCheckBox();
  cbEnableQtiPlotFitting->setChecked(app->m_enableQtiPlotFitting);

  QVBoxLayout* fitPageLayout = new QVBoxLayout(fitPage);
  fitPageLayout->addWidget(cbEnableQtiPlotFitting);
  fitPageLayout->addWidget(groupBoxFittingCurve);
  fitPageLayout->addWidget(groupBoxMultiPeak);
  fitPageLayout->addWidget(groupBoxFitParameters);
  fitPageLayout->addStretch();

  connect(samePointsBtn, SIGNAL(toggled(bool)), this, SLOT(showPointsBox(bool)));
  connect(generatePointsBtn, SIGNAL(toggled(bool)), this, SLOT(showPointsBox(bool)));
}

void ConfigDialog::initConfirmationsPage()
{
  ApplicationWindow *app = dynamic_cast<ApplicationWindow *>(this->parentWidget());
  confirm = new QWidget();

  groupBoxConfirm = new QGroupBox();
  QVBoxLayout * layout = new QVBoxLayout( groupBoxConfirm );

  boxFolders = new QCheckBox();
  boxFolders->setChecked(app->confirmCloseFolder);
  layout->addWidget( boxFolders );

  boxTables = new QCheckBox();
  boxTables->setChecked(app->confirmCloseTable);
  layout->addWidget( boxTables );

  boxMatrices = new QCheckBox();
  boxMatrices->setChecked(app->confirmCloseMatrix);
  layout->addWidget( boxMatrices );

  boxPlots2D = new QCheckBox();
  boxPlots2D->setChecked(app->confirmClosePlot2D);
  layout->addWidget( boxPlots2D );

  boxPlots3D = new QCheckBox();
  boxPlots3D->setChecked(app->confirmClosePlot3D);
  layout->addWidget( boxPlots3D );

  boxNotes = new QCheckBox();
  boxNotes->setChecked(app->confirmCloseNotes);
  layout->addWidget( boxNotes );

  boxInstrWindow=new QCheckBox();
  boxInstrWindow->setChecked(app->confirmCloseInstrWindow);
  layout->addWidget( boxInstrWindow );
  layout->addStretch();

  boxPromptRenameTables = new QCheckBox();
  boxPromptRenameTables->setChecked(app->d_inform_rename_table);

  boxPromptDeleteWorkspace = new QCheckBox();
  boxPromptDeleteWorkspace->setChecked(app->d_inform_delete_workspace);


  QVBoxLayout * confirmPageLayout = new QVBoxLayout( confirm );
  confirmPageLayout->addWidget(groupBoxConfirm);
  confirmPageLayout->addWidget(boxPromptDeleteWorkspace);
  confirmPageLayout->addWidget(boxPromptRenameTables);
  confirmPageLayout->addStretch();
}

void ConfigDialog::initFileLocationsPage()
{
  ApplicationWindow *app = dynamic_cast<ApplicationWindow *>(this->parentWidget());
  fileLocationsPage = new QWidget();

  QGroupBox *gb = new QGroupBox();
  QGridLayout *gl = new QGridLayout(gb);

  lblTranslationsPath = new QLabel(tr("Translations"));
  gl->addWidget(lblTranslationsPath , 0, 0);

  translationsPathLine = new QLineEdit();
  translationsPathLine->setText(app->d_translations_folder);
  gl->addWidget(translationsPathLine, 0, 1);

  QPushButton *browseTranslationsBtn = new QPushButton();
  browseTranslationsBtn->setIcon(QIcon(getQPixmap("choose_folder_xpm")));
  gl->addWidget(browseTranslationsBtn, 0, 2);

  lblHelpPath = new QLabel(tr("Help"));
  gl->addWidget(lblHelpPath, 1, 0 );

  QFileInfo hfi(app->helpFilePath);
  helpPathLine = new QLineEdit(hfi.dir().absolutePath());
  gl->addWidget( helpPathLine, 1, 1);

  QPushButton *browseHelpBtn = new QPushButton();
  browseHelpBtn->setIcon(QIcon(getQPixmap("choose_folder_xpm")));
  gl->addWidget(browseHelpBtn, 1, 2);
  gl->setRowStretch(2, 1);

  QVBoxLayout *vl = new QVBoxLayout(fileLocationsPage);
  vl->addWidget(gb);

  appTabWidget->addTab(fileLocationsPage, QString());

  connect(browseTranslationsBtn, SIGNAL(clicked()), this, SLOT(chooseTranslationsFolder()));
  connect(browseHelpBtn, SIGNAL(clicked()), this, SLOT(chooseHelpFolder()));
}

void ConfigDialog::languageChange()
{
  setWindowTitle( tr( "MantidPlot - Choose default settings" ) ); //Mantid
  ApplicationWindow *app = dynamic_cast<ApplicationWindow *>(this->parentWidget());

  // pages list
  itemsList->clear();
  itemsList->addItem( tr( "General" ) );
  itemsList->addItem( tr( "Mantid" ) );
  itemsList->addItem( tr( "Tables" ) );
  itemsList->addItem( tr( "2D Plots" ) );
  itemsList->addItem( tr( "3D Plots" ) );
  itemsList->addItem( tr( "Fitting" ) );
  itemsList->addItem( tr( "MD Plotting" ) );
  itemsList->setCurrentRow(0);
  itemsList->item(0)->setIcon(QIcon(getQPixmap("general_xpm")));
  itemsList->item(1)->setIcon(QIcon(":/MantidPlot_Icon_32offset.png"));
  itemsList->item(2)->setIcon(QIcon(getQPixmap("configTable_xpm")));
  itemsList->item(3)->setIcon(QIcon(getQPixmap("config_curves_xpm")));
  itemsList->item(4)->setIcon(QIcon(getQPixmap("logo_xpm")));
  itemsList->item(5)->setIcon(QIcon(getQPixmap("fit_xpm")));
  itemsList->item(6)->setIcon(QIcon(":/mdPlotting32x32.png"));
  itemsList->setIconSize(QSize(32,32));
  // calculate a sensible width for the items list
  // (default QListWidget size is 256 which looks too big)
  QFontMetrics fm(itemsList->font());
  int width = 32,i;
  for(i=0 ; i<itemsList->count() ; i++)
    if( fm.width(itemsList->item(i)->text()) > width)
      width = fm.width(itemsList->item(i)->text());
  itemsList->setMaximumWidth( itemsList->iconSize().width() + width + 50 );
  // resize the list to the maximum width
  itemsList->resize(itemsList->maximumWidth(),itemsList->height());

  //plots 2D page
  plotsTabWidget->setTabText(plotsTabWidget->indexOf(plotOptions), tr("Options"));
  plotsTabWidget->setTabText(plotsTabWidget->indexOf(axesPage), tr("Axes"));
  plotsTabWidget->setTabText(plotsTabWidget->indexOf(curves), tr("Curves"));
  plotsTabWidget->setTabText(plotsTabWidget->indexOf(plotTicks), tr("Ticks"));
  plotsTabWidget->setTabText(plotsTabWidget->indexOf(plotFonts), tr("Fonts"));

  boxResize->setText(tr("Do not &resize layers when window size changes"));
  boxLabelsEditing->setText(tr("&Disable in-place editing"));
  lblMinTicksLength->setText(tr("Length"));

  lblMajTicksLength->setText(tr("Length" ));
  lblMajTicks->setText(tr("Major Ticks" ));
  lblMinTicks->setText(tr("Minor Ticks" ));

  lblMargin->setText(tr("Margin" ));
  labelGraphAxesLabelsDist->setText(tr("Axes title space" ));
  labelTickLabelsDist->setText(tr("Ticks - Labels space" ));
  boxAxesLabelsDist->setSuffix(" " + tr("pixels"));
  boxTickLabelsDist->setSuffix(" " + tr("pixels"));
  labelFrameWidth->setText(tr("Frame width" ));

  boxFrame->setText(tr("Canvas Fra&me"));
  boxDistribution->setText(tr("Normalize histogram to bin width"));
  boxDistribution->setToolTip(tr("If checked, plot all spectra graphs normalised to the bin widths"));
  boxTitle->setText(tr("Show &Title"));
  boxScaleFonts->setText(tr("Scale &Fonts"));
  boxAutoscaling->setText(tr("Auto&scaling"));
  boxAntialiasing->setText(tr("Antia&liasing"));
  boxAspectRatio->setText(tr("Fixed aspect ratio on window resize"));

  // axes page
  boxBackbones->setText(tr("Axes &backbones"));
  boxSynchronizeScales->setText(tr("Synchronize scale &divisions"));
  lblAxesLineWidth->setText(tr("Axes linewidth" ));

  yLeftLabel->setText(tr("Left"));
  yRightLabel->setText(tr("Right / color map"));
  xBottomLabel->setText(tr("Bottom"));
  xTopLabel->setText(tr("Top"));

  enabledAxesGroupBox->setTitle(tr("Enabled axes" ));
  enableAxisLabel->setText(tr( "Show" ));
  showNumbersLabel->setText(tr( "Labels" ));
  scaleLabel->setText("Scale");

  boxMajTicks->clear();
  boxMajTicks->addItem(tr("None"));
  boxMajTicks->addItem(tr("Out"));
  boxMajTicks->addItem(tr("In & Out"));
  boxMajTicks->addItem(tr("In"));

  boxMinTicks->clear();
  boxMinTicks->addItem(tr("None"));
  boxMinTicks->addItem(tr("Out"));
  boxMinTicks->addItem(tr("In & Out"));
  boxMinTicks->addItem(tr("In"));

  boxMajTicks->setCurrentIndex(app->majTicksStyle);
  boxMinTicks->setCurrentIndex(app->minTicksStyle);

  plotsTabWidget->setTabText(plotsTabWidget->indexOf(plotPrint), tr("Print"));
  boxPrintCropmarks->setText(tr("Print Crop&marks"));
  boxScaleLayersOnPrint->setText(tr("&Scale layers to paper size"));

  //confirmations page
  groupBoxConfirm->setTitle(tr("Prompt on closing"));
  boxFolders->setText(tr("Folders"));
  boxTables->setText(tr("Tables"));
  boxPlots3D->setText(tr("3D Plots"));
  boxPlots2D->setText(tr("2D Plots"));
  boxMatrices->setText(tr("Matrices"));
  boxNotes->setText(tr("&Notes"));
  boxInstrWindow->setText(tr("&Instrument Window"));

  buttonOk->setText( tr( "&OK" ) );
  buttonCancel->setText( tr( "&Cancel" ) );
  buttonApply->setText( tr( "&Apply" ) );
  buttonTextFont->setText( tr( "&Text Font" ) );
  buttonHeaderFont->setText( tr( "&Labels Font" ) );
  buttonAxesFont->setText( tr( "A&xes Labels" ) );
  buttonNumbersFont->setText( tr( "Axes &Numbers" ) );
  buttonLegendFont->setText( tr( "&Legend" ) );
  buttonTitleFont->setText( tr( "T&itle" ) );
  boxPromptDeleteWorkspace->setText( tr( "Prompt when deleting Workspaces" ) );
  boxPromptRenameTables->setText( tr( "Prompt on &renaming tables when appending projects" ) );
  //application page
  appTabWidget->setTabText(appTabWidget->indexOf(application), tr("Application"));
  appTabWidget->setTabText(appTabWidget->indexOf(confirm), tr("Confirmations"));
  appTabWidget->setTabText(appTabWidget->indexOf(appColors), tr("Colors"));
  appTabWidget->setTabText(appTabWidget->indexOf(numericFormatPage), tr("Numeric Format"));
  appTabWidget->setTabText(appTabWidget->indexOf(fileLocationsPage), tr("File Locations"));
  appTabWidget->setTabText(appTabWidget->indexOf(floatingWindowsPage), tr("Floating windows"));

  //Mantid Page
  mtdTabWidget->setTabText(mtdTabWidget->indexOf(instrumentPage), tr("Instrument"));

  lblLanguage->setText(tr("Language"));
  lblStyle->setText(tr("Style"));
  lblFonts->setText(tr("Main Font"));
  fontsBtn->setText(tr("Choose &font"));
  lblWorkspace->setText(tr("Workspace"));
  lblPanelsText->setText(tr("Panels text"));
  lblPanels->setText(tr("Panels"));
  boxSave->setText(tr("Save every"));
  boxBackupProject->setText(tr("&Backup project before saving"));
  boxSearchUpdates->setText(tr("Check for new versions at startup"));
  boxMinutes->setSuffix(tr(" minutes"));
  lblScriptingLanguage->setText(tr("Default scripting language"));
  lblUndoStackSize->setText(tr("Matrix Undo Stack Size"));
  lblEndOfLine->setText(tr("Endline character"));
  lblInitWindow->setText(tr("Start New Project"));
  boxInitWindow->clear();
  boxInitWindow->addItem(tr("Empty"));
  boxInitWindow->addItem(tr("Table"));
  boxInitWindow->addItem(tr("Matrix"));
  boxInitWindow->addItem(tr("Empty Graph"));
  boxInitWindow->addItem(tr("Note"));
  boxInitWindow->setCurrentIndex((int)app->d_init_window_type);

  boxUpdateSeparators->setText(tr("Update separators in Tables/Matrices"));
  lblAppPrecision->setText(tr("Number of Decimal Digits"));
  lblDecimalSeparator->setText(tr("Decimal Separators"));
  boxDecimalSeparator->clear();
  boxDecimalSeparator->addItem(tr("System Locale Setting"));
  boxDecimalSeparator->addItem("1,000.0");
  boxDecimalSeparator->addItem("1.000,0");
  boxDecimalSeparator->addItem("1 000,0");
  boxThousandsSeparator->setText(tr("Omit Thousands Separator"));

  QLocale locale = app->locale();
  if (locale.name() == QLocale::c().name())
    boxDecimalSeparator->setCurrentIndex(1);
  else if (locale.name() == QLocale(QLocale::German).name())
    boxDecimalSeparator->setCurrentIndex(2);
  else if (locale.name() == QLocale(QLocale::French).name())
    boxDecimalSeparator->setCurrentIndex(3);

  lblTranslationsPath->setText(tr("Translations"));
  lblHelpPath->setText(tr("Help"));
  // #ifdef SCRIPTING_PYTHON
  // 	lblPythonConfigDir->setText(tr("Python Configuration Files"));
  // #endif

  //tables page
  boxUpdateTableValues->setText(tr("Automatically &Recalculate Column Values"));
  boxTableComments->setText(tr("&Display Comments in Header"));
  groupBoxTableCol->setTitle(tr("Colors"));
  lblSeparator->setText(tr("Default Column Separator"));
  boxSeparator->clear();
  boxSeparator->addItem(tr("TAB"));
  boxSeparator->addItem(tr("SPACE"));
  boxSeparator->addItem(";" + tr("TAB"));
  boxSeparator->addItem("," + tr("TAB"));
  boxSeparator->addItem(";" + tr("SPACE"));
  boxSeparator->addItem("," + tr("SPACE"));
  boxSeparator->addItem(";");
  boxSeparator->addItem(",");
  setColumnSeparator(app->columnSeparator);

  lblTableBackground->setText(tr( "Background" ));
  lblTextColor->setText(tr( "Text" ));
  lblHeaderColor->setText(tr("Labels"));
  groupBoxTableFonts->setTitle(tr("Fonts"));

  //curves page
  lblCurveStyle->setText(tr( "Default curve style" ));
  lblLineWidth->setText(tr( "Line width" ));
  lblSymbSize->setText(tr( "Symbol size" ));

  boxCurveStyle->clear();
  boxCurveStyle->addItem( getQPixmap("lPlot_xpm"), tr( " Line" ) );
  boxCurveStyle->addItem( getQPixmap("pPlot_xpm"), tr( " Scatter" ) );
  boxCurveStyle->addItem( getQPixmap("lpPlot_xpm"), tr( " Line + Symbol" ) );
  boxCurveStyle->addItem( getQPixmap("dropLines_xpm"), tr( " Vertical drop lines" ) );
  boxCurveStyle->addItem( getQPixmap("spline_xpm"), tr( " Spline" ) );
  boxCurveStyle->addItem( getQPixmap("hor_steps_xpm"), tr( " Horizontal steps" ) );
  boxCurveStyle->addItem( getQPixmap("vert_steps_xpm"), tr( " Vertical steps" ) );
  boxCurveStyle->addItem( getQPixmap("area_xpm"), tr( " Area" ) );
  boxCurveStyle->addItem( getQPixmap("vertBars_xpm"), tr( " Vertical Bars" ) );
  boxCurveStyle->addItem( getQPixmap("hBars_xpm"), tr( " Horizontal Bars" ) );

  int style = app->defaultCurveStyle;
  if (style == Graph::Line)
    boxCurveStyle->setCurrentItem(0);
  else if (style == Graph::Scatter)
    boxCurveStyle->setCurrentItem(1);
  else if (style == Graph::LineSymbols)
    boxCurveStyle->setCurrentItem(2);
  else if (style == Graph::VerticalDropLines)
    boxCurveStyle->setCurrentItem(3);
  else if (style == Graph::Spline)
    boxCurveStyle->setCurrentItem(4);
  else if (style == Graph::VerticalSteps)
    boxCurveStyle->setCurrentItem(5);
  else if (style == Graph::HorizontalSteps)
    boxCurveStyle->setCurrentItem(6);
  else if (style == Graph::Area)
    boxCurveStyle->setCurrentItem(7);
  else if (style == Graph::VerticalBars)
    boxCurveStyle->setCurrentItem(8);
  else if (style == Graph::HorizontalBars)
    boxCurveStyle->setCurrentItem(9);

  //plots 3D
  lblResolution->setText(tr("Resolution"));
  boxResolution->setSpecialValueText( "1 " + tr("(all data shown)") );
  boxShowLegend->setText(tr( "&Show Legend" ));
  boxShowProjection->setText(tr( "Show &Projection" ));
  btnFromColor->setText( tr( "&Data Max" ) );
  boxSmoothMesh->setText(tr( "Smoot&h Line" ));
  boxOrthogonal->setText(tr( "O&rthogonal" ));
  btnLabels->setText( tr( "Lab&els" ) );
  btnMesh->setText( tr( "Mesh &Line" ) );
  btnGrid->setText( tr( "&Grid" ) );
  btnToColor->setText( tr( "Data &Min" ) );
  btnNumbers->setText( tr( "&Numbers" ) );
  btnAxes->setText( tr( "A&xes" ) );
  btnBackground3D->setText( tr( "&Background" ) );
  groupBox3DCol->setTitle(tr("Colors" ));
  groupBox3DFonts->setTitle(tr("Fonts" ));
  btnTitleFnt->setText( tr( "&Title" ) );
  btnLabelsFnt->setText( tr( "&Axes Labels" ) );
  btnNumFnt->setText( tr( "&Numbers" ) );
  boxAutoscale3DPlots->setText( tr( "Autosca&ling" ) );

  //Fitting page
  cbEnableQtiPlotFitting->setText(tr("Enable QtiPlot fitting"));
  cbEnableQtiPlotFitting->setToolTip(tr("Takes effect after reopening a plot window"));
  groupBoxFittingCurve->setTitle(tr("Generated Fit Curve"));
  generatePointsBtn->setText(tr("Uniform X Function"));
  lblPoints->setText( tr("Points") );
  samePointsBtn->setText( tr( "Same X as Fitting Data" ) );
  linearFit2PointsBox->setText( tr( "2 points for linear fits" ) );
  groupBoxMultiPeak->setTitle(tr("Display Peak Curves for Multi-peak Fits"));
  groupBoxFitParameters->setTitle(tr("Parameters Output"));
  lblPrecision->setText(tr("Significant Digits"));
  logBox->setText(tr("Write Parameters to Result Log"));
  plotLabelBox->setText(tr("Paste Parameters to Plot"));
  plotLabelBox->setToolTip(tr("Adds a text box to the plot with details of fitting parameters"));
  scaleErrorsBox->setText(tr("Scale Errors with sqrt(Chi^2/doF)"));
  groupBoxMultiPeak->setTitle(tr("Display Peak Curves for Multi-peak Fits"));
  lblPeaksColor->setText(tr("Peaks Color"));

  // MDPlotting change
  mdPlottingTabWidget->setTabText(mdPlottingTabWidget->indexOf(vsiPage), tr("VSI"));
  lblVsiDefaultColorMap->setText(tr("Default Color Map"));
  lblVsiDefaultBackground->setText(tr("Background Color"));
  lblVsiLastSession->setText(tr("Use the settings of the last VSI session"));
  lblVsiInitialView->setText(tr("Initial View"));

  mdPlottingTabWidget->setTabText(mdPlottingTabWidget->indexOf(mdPlottingGeneralPage), tr("General"));
  lblGeneralDefaultColorMap->setText(tr("General Color Map"));
}

void ConfigDialog::accept()
{
  apply();
  close();
}

void ConfigDialog::apply()
{
  ApplicationWindow *app = dynamic_cast<ApplicationWindow *>(this->parentWidget());
  if (!app)
    return;

  // tables page
  QString sep = boxSeparator->currentText();
  sep.replace(tr("TAB"), "\t", false);
  sep.replace("\\t", "\t");
  sep.replace(tr("SPACE"), " ");
  sep.replace("\\s", " ");

  if (sep.contains(QRegExp("[0-9.eE+-]"))!=0){
    QMessageBox::warning(0, tr("MantidPlot - Import options error"),
      tr("The separator must not contain the following characters: 0-9eE.+-"));
    return;
  }

  app->columnSeparator = sep;
  app->setAutoUpdateTableValues(boxUpdateTableValues->isChecked());
  app->customizeTables(buttonBackground->color(), buttonText->color(),
    buttonHeader->color(), textFont, headerFont, boxTableComments->isChecked());
  // 2D plots page: options tab
  app->d_in_place_editing = !boxLabelsEditing->isChecked();
  app->titleOn=boxTitle->isChecked();
  app->autoDistribution1D = boxDistribution->isChecked();
  // Sync with config service
  ConfigService::Instance().setString("graph1d.autodistribution",
                                      boxDistribution->isChecked() ? "On" : "Off");
  if (boxFrame->isChecked())
    app->canvasFrameWidth = boxFrameWidth->value();
  else
    app->canvasFrameWidth = 0;

  app->defaultPlotMargin = boxMargin->value();
  app->d_graph_axes_labels_dist = boxAxesLabelsDist->value();
  app->d_graph_tick_labels_dist = boxTickLabelsDist->value();
  app->setGraphDefaultSettings(boxAutoscaling->isChecked(),boxScaleFonts->isChecked(),
    boxResize->isChecked(), boxAntialiasing->isChecked(), boxAspectRatio->isChecked());

  // 2D plots page: axes tab
  if (generalDialog->currentWidget() == plotsTabWidget &&
        plotsTabWidget->currentWidget() == axesPage )
  {
    app->drawBackbones = boxBackbones->isChecked();
    app->axesLineWidth = boxLineWidth->value();
    app->d_synchronize_graph_scales = boxSynchronizeScales->isChecked();

    for (int i = 0; i < QwtPlot::axisCnt; i++){
      int row = i + 1;
      QLayoutItem *item = enabledAxesGrid->itemAtPosition(row, 2);
      QCheckBox *box = qobject_cast<QCheckBox *>(item->widget());
      app->d_show_axes[i] = box->isChecked();

      item = enabledAxesGrid->itemAtPosition(row, 3);
      box = qobject_cast<QCheckBox *>(item->widget());
      app->d_show_axes_labels[i] = box->isChecked();

      item = enabledAxesGrid->itemAtPosition(row, 4);
      QComboBox *combo = qobject_cast<QComboBox *>(item->widget());
      app->d_axes_scales[i] = combo->currentText();
    }
  }

  // 2D plots page: curves tab
  app->defaultCurveStyle = curveStyle();
  app->defaultCurveLineWidth = boxCurveLineWidth->value();
  app->defaultSymbolSize = 2*boxSymbolSize->value() + 1;
  app->applyCurveStyleToMantid = cbApplyToMantid->isChecked();
  app->drawAllErrors = cbDrawAllErrors->isChecked();
  // 2D plots page: ticks tab
  app->majTicksLength = boxMajTicksLength->value();
  app->minTicksLength = boxMinTicksLength->value();
  app->majTicksStyle = boxMajTicks->currentItem();
  app->minTicksStyle = boxMinTicks->currentItem();
  // 2D plots page: fonts tab
  app->plotAxesFont=axesFont;
  app->plotNumbersFont=numbersFont;
  app->plotLegendFont=legendFont;
  app->plotTitleFont=titleFont;
  // 2D plots page: print tab
  app->d_print_cropmarks = boxPrintCropmarks->isChecked();
  app->d_scale_plots_on_print = boxScaleLayersOnPrint->isChecked();
  QList<MdiSubWindow*> windows = app->windowsList();
  foreach(MdiSubWindow *w, windows){
    if (w->isA("MultiLayer")){
      (dynamic_cast<MultiLayer*>(w))->setScaleLayersOnPrint(boxScaleLayersOnPrint->isChecked());
      (dynamic_cast<MultiLayer*>(w))->printCropmarks(boxPrintCropmarks->isChecked());
    }
  }
  // general page: application tab
  app->changeAppFont(appFont);
  setFont(appFont);
  app->changeAppStyle(boxStyle->currentText());
  app->autoSearchUpdates = boxSearchUpdates->isChecked();
  app->setSaveSettings(boxSave->isChecked(), boxMinutes->value());
  app->d_backup_files = boxBackupProject->isChecked();
  app->defaultScriptingLang = boxScriptingLanguage->currentText();
  app->d_init_window_type = (ApplicationWindow::WindowType)boxInitWindow->currentIndex();
  app->setMatrixUndoStackSize(undoStackSizeBox->value());
  app->d_eol = (ApplicationWindow::EndLineChar)boxEndLine->currentIndex();

  // general page: numeric format tab
  app->d_decimal_digits = boxAppPrecision->value();
  QLocale locale;
  switch (boxDecimalSeparator->currentIndex()){
  case 0:
    locale = QLocale::system();
    break;
  case 1:
    locale = QLocale::c();
    break;
  case 2:
    locale = QLocale(QLocale::German);
    break;
  case 3:
    locale = QLocale(QLocale::French);
    break;
  }
  if (boxThousandsSeparator->isChecked())
    locale.setNumberOptions(QLocale::OmitGroupSeparator);

  app->d_thousands_sep = !boxThousandsSeparator->isChecked();
  app->setLocale(locale);

  if (generalDialog->currentWidget() == appTabWidget &&
    appTabWidget->currentWidget() == numericFormatPage &&
    boxUpdateSeparators->isChecked()){
      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
      QList<MdiSubWindow *> windows = app->windowsList();
      foreach(MdiSubWindow *w, windows){
        w->setLocale(locale);
        if(w->isA("Table"))
          (dynamic_cast<Table *>(w))->updateDecimalSeparators();
        else if(w->isA("Matrix"))
          (dynamic_cast<Matrix *>(w))->resetView();
      }
      app->modifiedProject();
      QApplication::restoreOverrideCursor();
  }
  // general page: confirmations tab
  app->d_inform_delete_workspace = boxPromptDeleteWorkspace->isChecked();
  app->d_inform_rename_table = boxPromptRenameTables->isChecked();
  app->confirmCloseFolder = boxFolders->isChecked();
  app->updateConfirmOptions(boxTables->isChecked(), boxMatrices->isChecked(),
    boxPlots2D->isChecked(), boxPlots3D->isChecked(),
    boxNotes->isChecked(),boxInstrWindow->isChecked());
  // general page: colors tab
  app->setAppColors(btnWorkspace->color(), btnPanels->color(), btnPanelsText->color());
  // general page: floating windows tab
  app->settings.setValue("/General/FloatingWindows/MultiLayer",boxFloatingGraph->isChecked());
  app->settings.setValue("/General/FloatingWindows/Table",boxFloatingTable->isChecked());
  app->settings.setValue("/General/FloatingWindows/MantidTable",boxFloatingTable->isChecked());
  app->settings.setValue("/General/FloatingWindows/InstrumentWindow",boxFloatingInstrumentWindow->isChecked());
  app->settings.setValue("/General/FloatingWindows/MantidMatrix",boxFloatingMantidMatrix->isChecked());
  app->settings.setValue("/General/FloatingWindows/Note",boxFloatingNote->isChecked());
  app->settings.setValue("/General/FloatingWindows/Matrix",boxFloatingMatrix->isChecked());
  app->settings.setValue("/General/FloatingWindows/MdiSubWindow",boxFloatingCustomInterfaces->isChecked());
  app->settings.setValue("/General/FloatingWindows/TiledWindow",boxFloatingTiledWindows->isChecked());
  // 3D plots page
  QStringList plot3DColors = QStringList() << btnToColor->color().name() << btnLabels->color().name();
  plot3DColors << btnMesh->color().name() << btnGrid->color().name() << btnFromColor->color().name();
  plot3DColors << btnNumbers->color().name() << btnAxes->color().name() << btnBackground3D->color().name();
  app->plot3DColors = plot3DColors;

  app->showPlot3DLegend = boxShowLegend->isChecked();
  app->showPlot3DProjection = boxShowProjection->isChecked();
  app->plot3DResolution = boxResolution->value();
  app->plot3DTitleFont = plot3DTitleFont;
  app->plot3DNumbersFont = plot3DNumbersFont;
  app->plot3DAxesFont = plot3DAxesFont;
  app->orthogonal3DPlots = boxOrthogonal->isChecked();
  app->smooth3DMesh = boxSmoothMesh->isChecked();
  app->autoscale3DPlots = boxAutoscale3DPlots->isChecked();
  app->setPlot3DOptions();

  // fitting page
  app->m_enableQtiPlotFitting = cbEnableQtiPlotFitting->isChecked();
  app->fit_output_precision = boxPrecision->value();
  app->pasteFitResultsToPlot = plotLabelBox->isChecked();
  app->writeFitResultsToLog = logBox->isChecked();
  app->fitPoints = generatePointsBox->value();
  app->generateUniformFitPoints = generatePointsBtn->isChecked();
  app->generatePeakCurves = groupBoxMultiPeak->isChecked();
  app->peakCurvesColor = boxPeaksColor->currentIndex();
  app->fit_scale_errors = scaleErrorsBox->isChecked();
  app->d_2_linear_fit_points = linearFit2PointsBox->isChecked();
  app->saveSettings();

  // calculate a sensible width for the items list
  // (default QListWidget size is 256 which looks too big)
  QFontMetrics fm(itemsList->font());
  int width = 32;
  for(int i=0; i<itemsList->count(); i++)
    if( fm.width(itemsList->item(i)->text()) > width)
      width = fm.width(itemsList->item(i)->text());
  itemsList->setMaximumWidth( itemsList->iconSize().width() + width + 50 );
  // resize the list to the maximum width
  itemsList->resize(itemsList->maximumWidth(),itemsList->height());

  //Mantid
  Mantid::Kernel::ConfigServiceImpl&  cfgSvc = Mantid::Kernel::ConfigService::Instance();

   cfgSvc.setString("default.facility", facility->currentText().toStdString());
   cfgSvc.setString("default.instrument", defInstr->currentText().toStdString());
   cfgSvc.setString("paraview.ignore", QString::number(ckIgnoreParaView->isChecked()).toStdString());


  updateDirSearchSettings();
  updateCurveFitSettings();
  updateMantidOptionsTab();
  updateSendToTab();

  try
  {
     cfgSvc.saveConfig( cfgSvc.getUserFilename());
  }
  catch(std::runtime_error&)
  {
    QMessageBox::warning(this, "MantidPlot",
      "Unable to update Mantid user properties file.\n"
      "Configuration will not be saved.");
  }

  // MD Plotting
 updateMdPlottingSettings();
}

/**
 * Update the MD Plotting settings
 */
void ConfigDialog::updateMdPlottingSettings()
{
  //////// GENERAL TAB

  // Read the common color map check box
  if (mdPlottingGeneralFrame->isChecked())
  {
    m_mdSettings.setUsageGeneralMdColorMap(true);
  }
  else
  {
    m_mdSettings.setUsageGeneralMdColorMap(false);
  }

  if (mdPlottingGeneralColorMap)
  {
    QString generalTabColorMapName  = mdPlottingGeneralColorMap->currentText();
    QString generalTabColorMapFile = mdPlottingGeneralColorMap->itemData(mdPlottingGeneralColorMap->currentIndex()).toString();

    m_mdSettings.setGeneralMdColorMap(generalTabColorMapName, generalTabColorMapFile);
  }

  ///// VSI TAB

  // Read the Vsi color map
  if (vsiDefaultColorMap)
  {
    m_mdSettings.setUserSettingColorMap(vsiDefaultColorMap->currentText());
  }

  // Read if the usage of the last color map should be performed
  if (vsiLastSession)
  {
    m_mdSettings.setUsageLastSession(vsiLastSession->isChecked());
  }

  // Read the background selection
  if (vsiDefaultBackground)
  {
    m_mdSettings.setUserSettingBackgroundColor(vsiDefaultBackground->color());
  }

  // Read the initial view selection
  if (vsiInitialView)
  {
    m_mdSettings.setUserSettingIntialView(vsiInitialView->currentText());
  }
}

void ConfigDialog::updateDirSearchSettings()
{
  Mantid::Kernel::ConfigServiceImpl&  cfgSvc = Mantid::Kernel::ConfigService::Instance();

  QString setting = lePythonScriptsDirs->text();
  setting.replace('\\','/');
   cfgSvc.setString("pythonscripts.directories",setting.toStdString());

  setting = lePythonPluginsDirs->text();
  setting.replace('\\','/');
   cfgSvc.setString("user.python.plugins.directories",setting.toStdString());

  setting = leInstrumentDir->text();
  setting.replace('\\','/');
   cfgSvc.setString("instrumentDefinition.directory",setting.toStdString());

}

void ConfigDialog::updateCurveFitSettings()
{
  Mantid::Kernel::ConfigServiceImpl&  cfgSvc = Mantid::Kernel::ConfigService::Instance();

  // Form setting string from function name and parameters
  QString fname = backgroundFunctions->currentText();
  std::string setting = fname.toStdString();
  //Ignore parameters for none
  if( fname != "None" )
  {
    QString args = functionArguments->text();
    setting += std::string(" ") + args.toStdString();
  }

  ApplicationWindow *app = dynamic_cast<ApplicationWindow *>(this->parentWidget());

  // cfgSvc.setString("curvefitting.autoBackground", setting);
  app->mantidUI->fitFunctionBrowser()->setAutoBackgroundName(QString::fromStdString(setting));

  setting = defaultPeakShape->currentText().toStdString();
  // cfgSvc.setString("curvefitting.defaultPeak", setting);
  app->mantidUI->fitFunctionBrowser()->setDefaultPeakType(setting);

  setting = QString::number(findPeaksFWHM->value()).toStdString();
   cfgSvc.setString("curvefitting.findPeaksFWHM", setting);

  setting = QString::number(findPeaksTolerance->value()).toStdString();
   cfgSvc.setString("curvefitting.findPeaksTolerance", setting);

  setting = QString::number(peakRadius->value()).toStdString();
   cfgSvc.setString("curvefitting.peakRadius", setting);

  app->mantidUI->fitFunctionBrowser()->setDecimals(decimals->value());
}

void ConfigDialog::updateMantidOptionsTab()
{
  auto& cfgSvc = ConfigService::Instance();

  // re-use plot instances (spectra, slice, color-fill, etc.)
  QString reusePlotInst = m_reusePlotInstances->isChecked()? "On" : "Off";
   cfgSvc.setString("MantidOptions.ReusePlotInstances",reusePlotInst.toStdString());

  //invisible workspaces options
  QString showinvisible_ws = m_invisibleWorkspaces->isChecked()? "1" : "0";
   cfgSvc.setString("MantidOptions.InvisibleWorkspaces",showinvisible_ws.toStdString());

  //OpenGL option
  QString setting = m_useOpenGL->isChecked() ? "On" : "Off";
   cfgSvc.setString("MantidOptions.InstrumentView.UseOpenGL",setting.toStdString());

  //Hidden categories
  QString hiddenCategories = buildHiddenCategoryString().join(";");

  //store it if it has changed
  std::string hiddenCategoryString = hiddenCategories.toStdString();
  if (hiddenCategoryString !=  cfgSvc.getString("algorithms.categories.hidden"))
  {
     cfgSvc.setString("algorithms.categories.hidden",hiddenCategoryString);

    //update the algorithm tree
    ApplicationWindow *app = dynamic_cast<ApplicationWindow *>(this->parentWidget());
    app->mantidUI->updateAlgorithms();
  }
}

QStringList ConfigDialog::buildHiddenCategoryString(QTreeWidgetItem *parent)
{
  QStringList results;
  //how many children at this level
  int count = parent ? parent->childCount() : treeCategories->topLevelItemCount();

  for (int i = 0; i < count; i++)
  {
    //get the child
    QTreeWidgetItem *item = parent ? parent->child(i) : treeCategories->topLevelItem(i);

    if (item->checkState(0) == Qt::Unchecked)
    {
      results.append(item->text(0));
    }

    QStringList childResults = buildHiddenCategoryString(item);
    for (QStringList::ConstIterator it = childResults.begin();it!=childResults.end();++it)
    {
      results.append(item->text(0) + "\\" + *it);
    }
  }
  return results;
}

int ConfigDialog::curveStyle()
{
  int style = 0;
  switch (boxCurveStyle->currentItem())
  {
  case 0:
    style = Graph::Line;
    break;
  case 1:
    style = Graph::Scatter;
    break;
  case 2:
    style = Graph::LineSymbols;
    break;
  case 3:
    style = Graph::VerticalDropLines;
    break;
  case 4:
    style = Graph::Spline;
    break;
  case 5:
    style = Graph::VerticalSteps;
    break;
  case 6:
    style = Graph::HorizontalSteps;
    break;
  case 7:
    style = Graph::Area;
    break;
  case 8:
    style = Graph::VerticalBars;
    break;
  case 9:
    style = Graph::HorizontalBars;
    break;
  }
  return style;
}

void ConfigDialog::pickTextFont()
{
  bool ok;
  QFont font = QFontDialog::getFont(&ok,textFont,this);
  if ( ok ) {
    textFont = font;
  } else {
    return;
  }
}

void ConfigDialog::pickHeaderFont()
{
  bool ok;
  QFont font = QFontDialog::getFont(&ok,headerFont,this);
  if ( ok ) {
    headerFont = font;
  } else {
    return;
  }
}

void ConfigDialog::pickLegendFont()
{
  bool ok;
  QFont font = QFontDialog::getFont(&ok,legendFont,this);
  if ( ok ) {
    legendFont = font;
  } else {
    return;
  }
}

void ConfigDialog::pickAxesFont()
{
  bool ok;
  QFont font = QFontDialog::getFont(&ok,axesFont,this);
  if ( ok ) {
    axesFont = font;
  } else {
    return;
  }
}

void ConfigDialog::pickNumbersFont()
{
  bool ok;
  QFont font = QFontDialog::getFont(&ok,numbersFont,this);
  if ( ok ) {
    numbersFont = font;
  } else {
    return;
  }
}

void ConfigDialog::pickTitleFont()
{
  bool ok;
  QFont font = QFontDialog::getFont(&ok,titleFont,this);
  if ( ok )
    titleFont = font;
  else
    return;
}

void ConfigDialog::pickApplicationFont()
{
  bool ok;
  QFont font = QFontDialog::getFont(&ok,appFont,this);
  if ( ok )
    appFont = font;
  else
    return;
  fontsBtn->setFont(appFont);
}

void ConfigDialog::pick3DTitleFont()
{
  bool ok;
  QFont font = QFontDialog::getFont(&ok, plot3DTitleFont,this);
  if ( ok )
    plot3DTitleFont = font;
  else
    return;
}

void ConfigDialog::pick3DNumbersFont()
{
  bool ok;
  QFont font = QFontDialog::getFont(&ok, plot3DNumbersFont,this);
  if ( ok )
    plot3DNumbersFont = font;
  else
    return;
}

void ConfigDialog::pick3DAxesFont()
{
  bool ok;
  QFont font = QFontDialog::getFont(&ok, plot3DAxesFont,this);
  if ( ok )
    plot3DAxesFont = font;
  else
    return;
}

void ConfigDialog::setColumnSeparator(const QString& sep)
{
  if (sep=="\t")
    boxSeparator->setCurrentIndex(0);
  else if (sep==" ")
    boxSeparator->setCurrentIndex(1);
  else if (sep==";\t")
    boxSeparator->setCurrentIndex(2);
  else if (sep==",\t")
    boxSeparator->setCurrentIndex(3);
  else if (sep=="; ")
    boxSeparator->setCurrentIndex(4);
  else if (sep==", ")
    boxSeparator->setCurrentIndex(5);
  else if (sep==";")
    boxSeparator->setCurrentIndex(6);
  else if (sep==",")
    boxSeparator->setCurrentIndex(7);
  else
  {
    QString separator = sep;
    boxSeparator->setEditText(separator.replace(" ","\\s").replace("\t","\\t"));
  }
}
void ConfigDialog::gotoMantidDirectories()
{
  generalDialog->setCurrentWidget(mtdTabWidget);
  mtdTabWidget->setCurrentWidget(directoriesPage);
}

void ConfigDialog::switchToLanguage(int param)
{
  ApplicationWindow *app = dynamic_cast<ApplicationWindow *>(this->parentWidget());
  app->switchToLanguage(param);
  languageChange();
}

void ConfigDialog::insertLanguagesList()
{
  ApplicationWindow *app = dynamic_cast<ApplicationWindow *>(this->parentWidget());
  if(!app)
    return;

  boxLanguage->clear();
  QString qmPath = app->d_translations_folder;
  QDir dir(qmPath);
  QStringList locales = app->locales;
  QStringList languages;
  int lang = 0;
  for (int i=0; i < (int)locales.size(); i++)
  {
    if (locales[i] == "en")
      languages.push_back("English");
    else
    {
      QTranslator translator;
      translator.load("qtiplot_"+locales[i], qmPath);

      QString language = translator.translate("ApplicationWindow", "English");
      if (!language.isEmpty())
        languages.push_back(language);
      else
        languages.push_back(locales[i]);
    }

    if (locales[i] == app->appLanguage)
      lang = i;
  }
  boxLanguage->addItems(languages);
  boxLanguage->setCurrentIndex(lang);
}


void ConfigDialog::showPointsBox(bool)
{
  if (generatePointsBtn->isChecked())
  {
    lblPoints->show();
    generatePointsBox->show();
    linearFit2PointsBox->show();
  }
  else
  {
    lblPoints->hide();
    generatePointsBox->hide();
    linearFit2PointsBox->hide();
  }
}

void ConfigDialog::chooseTranslationsFolder()
{
  ApplicationWindow *app = dynamic_cast<ApplicationWindow *>(this->parentWidget());
  if (!app)
    return;

  QFileInfo tfi(app->d_translations_folder);
  QString dir = QFileDialog::getExistingDirectory(this, tr("Choose the location of the MantidPlot translations folder!"),
    tfi.dir().absolutePath(), 0/**QFileDialog::ShowDirsOnly*/);

  if (!dir.isEmpty()){
    app->d_translations_folder = dir;
    translationsPathLine->setText(dir);
    app->createLanguagesList();
    insertLanguagesList();
  }
}

void ConfigDialog::chooseHelpFolder()
{
  ApplicationWindow *app = dynamic_cast<ApplicationWindow *>(this->parentWidget());
  if (!app)
    return;

  app->chooseHelpFolder();

  QFileInfo hfi(app->helpFilePath);
  helpPathLine->setText(hfi.dir().absolutePath());
}

void ConfigDialog::addPythonScriptsDirs()
{
  QString dir = QFileDialog::getExistingDirectory(this, tr("Add a python scripts directory"),
    "", 0/**QFileDialog::ShowDirsOnly*/);
  if (!dir.isEmpty())
  {
    QString dirs = lePythonScriptsDirs->text();
    if (!dirs.isEmpty())
    {
      dirs += ";";
    }
    dirs += dir;
    lePythonScriptsDirs->setText(dirs);
  }
}

void ConfigDialog::addPythonPluginDirs()
{
  QString dir = QFileDialog::getExistingDirectory(this, tr("Add a python extension directory"),
    "", 0/**QFileDialog::ShowDirsOnly*/);
  if (!dir.isEmpty())
  {
    QString dirs = lePythonPluginsDirs->text();
    if (!dirs.isEmpty())
    {
      dirs += ";";
    }
    dirs += dir;
    lePythonPluginsDirs->setText(dirs);
  }
}

void ConfigDialog::addInstrumentDir()
{
  QString dir = QFileDialog::getExistingDirectory(this, tr("Select new instrument definition directory"),
    "", 0);
  if (!dir.isEmpty())
  {
    leInstrumentDir->setText(dir);
  }
}
