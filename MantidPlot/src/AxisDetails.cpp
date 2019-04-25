// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//---------------------------
// Includes
//--------------------------

#include "AxisDetails.h"
#include "ApplicationWindow.h"
#include "MantidQtWidgets/Common/DoubleSpinBox.h"
#include "MantidQtWidgets/Plotting/Qwt/ScaleEngine.h"
#include "MantidQtWidgets/Plotting/Qwt/qwt_compat.h"
#include "MyParser.h"
#include "Plot.h"
#include <qwt_scale_widget.h>

#include <ColorButton.h>
#include <QCheckBox>
#include <QComboBox>
#include <QDate>
#include <QDateTimeEdit>
#include <QFontDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QList>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QTextEdit>
#include <QTimeEdit>
#include <QVector>
#include <QWidget>
#include <TextFormatButtons.h>

/** The constructor for a single set of widgets containing parameters for the
 * labeling and format of an axis.
 *  @param app :: the containing application window
 *  @param graph :: the graph the dialog is settign the options for
 *  @param mappedaxis :: the QwtPlot::axis value that corresponds to this axis
 *  @param parent :: the QWidget that acts as this widget's parent in the
 * hierarchy
 */
AxisDetails::AxisDetails(ApplicationWindow *app, Graph *graph, int mappedaxis,
                         QWidget *parent)
    : QWidget(parent) {
  m_app = app;
  m_graph = graph;
  m_tablesList = m_app->tableNames();
  m_mappedaxis = mappedaxis;
  m_initialised = false;
  QHBoxLayout *topLayout = new QHBoxLayout();

  m_chkShowAxis = new QCheckBox(tr("Show"));
  topLayout->addWidget(m_chkShowAxis);

  m_grpTitle = new QGroupBox(tr("Title"));
  topLayout->addWidget(m_grpTitle);

  QVBoxLayout *titleBoxLayout = new QVBoxLayout(m_grpTitle);
  titleBoxLayout->setSpacing(2);

  m_txtTitle = new QTextEdit();
  QFontMetrics metrics(this->font());
  m_txtTitle->setMaximumHeight(3 * metrics.height());
  titleBoxLayout->addWidget(m_txtTitle);

  QHBoxLayout *hl = new QHBoxLayout();
  hl->setMargin(0);
  hl->setSpacing(2);
  m_btnLabelFont = new QPushButton(tr("&Font"));
  hl->addWidget(m_btnLabelFont);

  m_formatButtons =
      new TextFormatButtons(m_txtTitle, TextFormatButtons::AxisLabel);
  hl->addWidget(m_formatButtons);
  hl->addStretch();

  m_txtTitle->setMaximumWidth(m_btnLabelFont->width() +
                              m_formatButtons->width());
  titleBoxLayout->addLayout(hl);

  QHBoxLayout *bottomLayout = new QHBoxLayout();

  m_grpAxisDisplay = new QGroupBox(QString());
  bottomLayout->addWidget(m_grpAxisDisplay);
  QGridLayout *leftBoxLayout = new QGridLayout(m_grpAxisDisplay);

  leftBoxLayout->addWidget(new QLabel(tr("Type")), 0, 0);

  m_cmbAxisType = new QComboBox();
  m_cmbAxisType->addItem(tr("Numeric"));
  m_cmbAxisType->addItem(tr("Text from table"));
  m_cmbAxisType->addItem(tr("Day of the week"));
  m_cmbAxisType->addItem(tr("Month"));
  m_cmbAxisType->addItem(tr("Time"));
  m_cmbAxisType->addItem(tr("Date"));
  m_cmbAxisType->addItem(tr("Column Headings"));
  leftBoxLayout->addWidget(m_cmbAxisType, 0, 1);

  leftBoxLayout->addWidget(new QLabel(tr("Font")), 1, 0);

  m_btnAxesFont = new QPushButton();
  m_btnAxesFont->setText(tr("Axis &Font"));
  leftBoxLayout->addWidget(m_btnAxesFont, 1, 1);

  leftBoxLayout->addWidget(new QLabel(tr("Color")), 2, 0);
  m_cbtnAxisColor = new ColorButton();
  leftBoxLayout->addWidget(m_cbtnAxisColor, 2, 1);

  leftBoxLayout->addWidget(new QLabel(tr("Major Ticks")), 3, 0);

  m_cmbMajorTicksType = new QComboBox();
  m_cmbMajorTicksType->addItem(tr("None"));
  m_cmbMajorTicksType->addItem(tr("Out"));
  m_cmbMajorTicksType->addItem(tr("In & Out"));
  m_cmbMajorTicksType->addItem(tr("In"));
  leftBoxLayout->addWidget(m_cmbMajorTicksType, 3, 1);

  leftBoxLayout->addWidget(new QLabel(tr("Minor Ticks")), 4, 0);

  m_cmbMinorTicksType = new QComboBox();
  m_cmbMinorTicksType->addItem(tr("None"));
  m_cmbMinorTicksType->addItem(tr("Out"));
  m_cmbMinorTicksType->addItem(tr("In & Out"));
  m_cmbMinorTicksType->addItem(tr("In"));
  leftBoxLayout->addWidget(m_cmbMinorTicksType, 4, 1);

  leftBoxLayout->addWidget(new QLabel(tr("Stand-off")), 5, 0);
  m_spnBaseline = new QSpinBox();
  m_spnBaseline->setRange(0, 1000);
  leftBoxLayout->addWidget(m_spnBaseline);

  m_grpShowLabels = new QGroupBox(tr("Show Labels"));
  m_grpShowLabels->setCheckable(true);
  m_grpShowLabels->setChecked(true);

  bottomLayout->addWidget(m_grpShowLabels);
  QGridLayout *rightBoxLayout = new QGridLayout(m_grpShowLabels);

  m_lblColumn = new QLabel(tr("Column"));
  rightBoxLayout->addWidget(m_lblColumn, 0, 0);

  m_cmbColName = new QComboBox();
  rightBoxLayout->addWidget(m_cmbColName, 0, 1);

  m_lblTable = new QLabel(tr("Table"));
  rightBoxLayout->addWidget(m_lblTable, 1, 0);

  m_cmbTableName = new QComboBox();
  m_cmbTableName->insertItems(-1, m_tablesList);
  m_cmbColName->insertItems(-1, m_app->columnsList(Table::All));
  rightBoxLayout->addWidget(m_cmbTableName, 1, 1);

  m_lblFormat = new QLabel(tr("Format"));
  rightBoxLayout->addWidget(m_lblFormat, 2, 0);

  m_cmbFormat = new QComboBox();
  m_cmbFormat->setDuplicatesEnabled(false);
  rightBoxLayout->addWidget(m_cmbFormat, 2, 1);

  m_lblPrecision = new QLabel(tr("Precision"));
  rightBoxLayout->addWidget(m_lblPrecision, 3, 0);
  m_spnPrecision = new QSpinBox();
  m_spnPrecision->setRange(0, 10);
  rightBoxLayout->addWidget(m_spnPrecision, 3, 1);

  rightBoxLayout->addWidget(new QLabel(tr("Angle")), 4, 0);

  m_spnAngle = new QSpinBox();
  m_spnAngle->setRange(-90, 90);
  m_spnAngle->setSingleStep(5);
  rightBoxLayout->addWidget(m_spnAngle, 4, 1);

  rightBoxLayout->addWidget(new QLabel(tr("Color")), 5, 0);
  m_cbtnAxisNumColor = new ColorButton();
  rightBoxLayout->addWidget(m_cbtnAxisNumColor, 5, 1);

  m_chkShowFormula = new QCheckBox(tr("For&mula"));
  rightBoxLayout->addWidget(m_chkShowFormula, 6, 0);

  m_txtFormula = new QTextEdit();
  m_txtFormula->setMaximumHeight(3 * metrics.height());
  rightBoxLayout->addWidget(m_txtFormula, 6, 1);
  rightBoxLayout->setRowStretch(7, 1);

  QVBoxLayout *rightLayout = new QVBoxLayout(this);
  rightLayout->addLayout(topLayout);
  rightLayout->addLayout(bottomLayout);
  rightLayout->addStretch(1);

  connect(m_chkShowFormula, SIGNAL(clicked()), this, SLOT(enableFormulaBox()));
  connect(m_cmbAxisType, SIGNAL(activated(int)), this,
          SLOT(setAxisFormatOptions(int)));

  connect(m_grpShowLabels, SIGNAL(clicked(bool)), this, SLOT(showAxis()));
  connect(m_chkShowAxis, SIGNAL(clicked()), this, SLOT(showAxis()));
  connect(m_cmbFormat, SIGNAL(activated(int)), this, SLOT(showAxis()));

  connect(m_btnAxesFont, SIGNAL(clicked()), this, SLOT(setScaleFont()));
  connect(m_btnLabelFont, SIGNAL(clicked()), this, SLOT(setLabelFont()));

  initWidgets();
}

AxisDetails::~AxisDetails() {}

/** Initialisation method. Sets up all widgets and variables not done in the
 *constructor.
 *
 */
void AxisDetails::initWidgets() {
  if (m_initialised) {
    return;
  } else {
    Plot *p = m_graph->plotWidget();
    int style = (int)m_graph->axisType(m_mappedaxis);

    bool axisOn = p->axisEnabled(m_mappedaxis);
    const QList<int> majTicks = p->getMajorTicksType();
    const QList<int> minTicks = p->getMinorTicksType();

    const QwtScaleDraw *sd = p->axisScaleDraw(m_mappedaxis);
    bool labelsOn = sd->hasComponent(QwtAbstractScaleDraw::Labels);

    int format = p->axisLabelFormat(m_mappedaxis);

    // Top
    m_chkShowAxis->setChecked(axisOn);
    m_txtTitle->setPlainText(m_graph->axisTitle(m_mappedaxis));
    m_labelFont = m_graph->axisTitleFont(m_mappedaxis);

    // bottom left
    m_cmbAxisType->setCurrentIndex(style);
    setAxisFormatOptions(style);
    m_scaleFont = p->axisFont(m_mappedaxis);

    m_cbtnAxisColor->setColor(m_graph->axisColor(m_mappedaxis));

    m_cmbMajorTicksType->setCurrentIndex(majTicks[m_mappedaxis]);
    m_cmbMinorTicksType->setCurrentIndex(minTicks[m_mappedaxis]);

    QwtScaleWidget *scale =
        dynamic_cast<QwtScaleWidget *>(p->axisWidget(m_mappedaxis));
    if (scale) {
      m_spnBaseline->setValue(scale->margin());
    } else {
      m_spnBaseline->setValue(0);
    }

    // bottom right
    m_grpShowLabels->setChecked(labelsOn);

    m_cmbFormat->setEnabled(labelsOn && axisOn);
    m_cmbFormat->setCurrentIndex(format);

    if (m_cmbAxisType->currentIndex() == ScaleDraw::Numeric) {
      m_spnPrecision->setValue(p->axisLabelPrecision(m_mappedaxis));
    } else if (m_cmbAxisType->currentIndex() == ScaleDraw::Text) {
      m_cmbColName->setItemText(m_cmbColName->currentIndex(),
                                m_graph->axisFormatInfo(m_mappedaxis));
    }

    m_spnPrecision->setEnabled(format != 0);

    if (m_mappedaxis == QwtPlot::xBottom || m_mappedaxis == QwtPlot::xTop) {
      m_spnAngle->setEnabled(labelsOn && axisOn);
      m_spnAngle->setValue(m_graph->labelsRotation(m_mappedaxis));
    } else {
      m_spnAngle->setEnabled(false);
      m_spnAngle->setValue(0);
    }

    m_cbtnAxisNumColor->setColor(m_graph->axisLabelsColor(m_mappedaxis));

    QString formula = m_graph->axisFormula(m_mappedaxis);
    m_txtFormula->setFixedWidth(150);

    if (!formula.isEmpty()) {
      m_chkShowFormula->setChecked(true);
      m_txtFormula->setEnabled(true);
      m_txtFormula->setPlainText(formula);
    } else {
      m_chkShowFormula->setChecked(false);
      m_txtFormula->setEnabled(false);
    }
    showAxis();

    connect(m_chkShowFormula, SIGNAL(stateChanged(int)), this,
            SLOT(setModified()));
    connect(m_chkShowAxis, SIGNAL(stateChanged(int)), this,
            SLOT(setModified()));

    connect(m_cmbAxisType, SIGNAL(currentIndexChanged(int)), this,
            SLOT(setModified()));
    connect(m_cmbAxisType, SIGNAL(editTextChanged(QString)), this,
            SLOT(setModified()));
    connect(m_cmbMajorTicksType, SIGNAL(currentIndexChanged(int)), this,
            SLOT(setModified()));
    connect(m_cmbMajorTicksType, SIGNAL(editTextChanged(QString)), this,
            SLOT(setModified()));
    connect(m_cmbTableName, SIGNAL(currentIndexChanged(int)), this,
            SLOT(setModified()));
    connect(m_cmbTableName, SIGNAL(editTextChanged(QString)), this,
            SLOT(setModified()));
    connect(m_cmbMinorTicksType, SIGNAL(currentIndexChanged(int)), this,
            SLOT(setModified()));
    connect(m_cmbMinorTicksType, SIGNAL(editTextChanged(QString)), this,
            SLOT(setModified()));
    connect(m_cmbColName, SIGNAL(currentIndexChanged(int)), this,
            SLOT(setModified()));
    connect(m_cmbColName, SIGNAL(editTextChanged(QString)), this,
            SLOT(setModified()));
    connect(m_cmbFormat, SIGNAL(currentIndexChanged(int)), this,
            SLOT(setModified()));
    connect(m_cmbFormat, SIGNAL(editTextChanged(QString)), this,
            SLOT(setModified()));
    connect(m_grpShowLabels, SIGNAL(clicked(bool)), this, SLOT(setModified()));
    connect(m_btnAxesFont, SIGNAL(clicked()), this, SLOT(setModified()));
    connect(m_btnLabelFont, SIGNAL(clicked()), this, SLOT(setModified()));
    connect(m_txtFormula, SIGNAL(textChanged()), this, SLOT(setModified()));
    connect(m_txtTitle, SIGNAL(textChanged()), this, SLOT(setModified()));
    connect(m_formatButtons, SIGNAL(formattingModified()), this,
            SLOT(setModified()));
    connect(m_spnPrecision, SIGNAL(valueChanged(int)), this,
            SLOT(setModified()));
    connect(m_spnAngle, SIGNAL(valueChanged(int)), this, SLOT(setModified()));
    connect(m_spnBaseline, SIGNAL(valueChanged(int)), this,
            SLOT(setModified()));
    connect(m_cbtnAxisColor, SIGNAL(colorChanged()), this, SLOT(setModified()));
    connect(m_cbtnAxisNumColor, SIGNAL(colorChanged()), this,
            SLOT(setModified()));

    m_modified = false;
    m_initialised = true;
  }
}

/** Sets the modified flag to true so that the changes may be applied.
 *
 */
void AxisDetails::setModified() { m_modified = true; }

/** Checks to see if this axis has valid parameters
 *
 */
bool AxisDetails::valid() {
  if (m_cmbAxisType->currentIndex() == ScaleDraw::Numeric) {
    if (m_chkShowFormula->isChecked()) {
      QString formula = m_txtFormula->toPlainText().toLower();
      try {
        double value = 1.0;
        MyParser parser;
        if (formula.contains("x")) {
          parser.DefineVar("x", &value);
        } else if (formula.contains("y")) {
          parser.DefineVar("y", &value);
        }
        parser.SetExpr(formula.toLatin1().data());
        parser.Eval();
      } catch (mu::ParserError &e) {
        QMessageBox::critical(this, tr("MantidPlot - Formula input error"),
                              QString::fromStdString(e.GetMsg()) + "\n" +
                                  tr("Valid variables are 'x' for Top/Bottom "
                                     "axes and 'y' for Left/Right axes!"));
        return false;
      }
    }
  }
  Table *w = m_app->table(m_cmbColName->currentText());
  return m_initialised && m_graph &&
         !((m_cmbAxisType->currentIndex() == ScaleDraw::Text ||
            m_cmbAxisType->currentIndex() == ScaleDraw::ColHeader) &&
           !w);
}

/** Applies the grid parameters to the graphs
 *
 */
void AxisDetails::apply() {
  if (m_modified && valid()) {
    Table *w = m_app->table(m_cmbColName->currentText());

    QString formula = m_txtFormula->toPlainText();
    if (!m_chkShowFormula->isChecked()) {
      formula = QString();
    }
    int type = m_cmbAxisType->currentIndex();
    QString formatInfo = m_cmbColName->currentText();
    if (type == ScaleDraw::Day || type == ScaleDraw::Month) {
      formatInfo = QString::number(m_cmbFormat->currentIndex());
    } else if (type == ScaleDraw::Time || type == ScaleDraw::Date) {
      QStringList lst = (m_graph->axisFormatInfo(m_mappedaxis))
                            .split(";", QString::SkipEmptyParts);
      if ((int)lst.count() >= 2) {
        lst[1] = m_cmbFormat->currentText();
      }
      formatInfo = lst.join(";");
    } else if (type == ScaleDraw::ColHeader) {
      formatInfo = m_cmbTableName->currentText();
    }
    m_graph->showAxis(
        m_mappedaxis, m_cmbAxisType->currentIndex(), formatInfo, w,
        m_chkShowAxis->isChecked(), m_cmbMajorTicksType->currentIndex(),
        m_cmbMinorTicksType->currentIndex(), m_grpShowLabels->isChecked(),
        m_cbtnAxisColor->color(), m_cmbFormat->currentIndex(),
        m_spnPrecision->value(), m_spnAngle->value(), m_spnBaseline->value(),
        formula, m_cbtnAxisNumColor->color());
    m_graph->setAxisTitle(m_mappedaxis, m_txtTitle->toPlainText());
    m_graph->setAxisFont(m_mappedaxis, m_scaleFont);
    m_graph->setAxisTitleFont(m_mappedaxis, m_labelFont);
    m_modified = false;
  }
}

/** Applies the grid parameters to the graphs
 *
 */
void AxisDetails::showAxis() {
  bool shown = m_chkShowAxis->isChecked();
  bool labels = m_grpShowLabels->isChecked();

  m_grpShowLabels->setEnabled(shown);
  m_grpAxisDisplay->setEnabled(shown);
  m_grpTitle->setEnabled(shown);

  if (shown) {
    m_cmbFormat->setEnabled(labels);
    m_cmbColName->setEnabled(labels);
    m_chkShowFormula->setEnabled(labels);
    m_txtFormula->setEnabled(labels);

    // this should so the work of the below IF but on one line and slightly more
    // efficiently as i assume setDisabled negates that given to it
    m_spnAngle->setEnabled(
        (m_mappedaxis == QwtPlot::xBottom || m_mappedaxis == QwtPlot::xTop) &&
        labels);
    m_spnPrecision->setEnabled(
        labels && (m_cmbAxisType->currentIndex() == ScaleDraw::Numeric) &&
        (m_cmbFormat->currentIndex() != 0));

    enableFormulaBox();
  }

  emit axisShowChanged(shown);
}

/** Enables, Disables, Hides or Shows widgets appropriate to the current Axis
 *Format
 *
 */
void AxisDetails::setAxisFormatOptions(int type) {
  m_cmbFormat->clear();
  m_cmbFormat->setEditable(false);
  m_cmbFormat->hide();
  m_spnPrecision->hide();
  m_cmbColName->hide();
  m_lblColumn->hide();
  m_lblFormat->hide();
  m_lblPrecision->hide();
  m_chkShowFormula->hide();
  m_txtFormula->hide();
  m_cmbTableName->hide();
  m_lblTable->hide();

  switch (type) {
  case 0: {
    m_lblFormat->show();
    m_cmbFormat->show();
    m_cmbFormat->insertItem(m_cmbFormat->count(), tr("Automatic"));
    m_cmbFormat->insertItem(m_cmbFormat->count(), tr("Decimal: 100.0"));
    m_cmbFormat->insertItem(m_cmbFormat->count(), tr("Scientific: 1e2"));
    m_cmbFormat->insertItem(m_cmbFormat->count(), tr("Scientific: 10^2"));
    m_cmbFormat->setCurrentIndex(
        m_graph->plotWidget()->axisLabelFormat(m_mappedaxis));

    m_lblPrecision->show();
    m_spnPrecision->show();
    m_spnPrecision->setEnabled(m_cmbFormat->currentIndex() != 0);
    m_chkShowFormula->show();
    m_txtFormula->show();

    enableFormulaBox();
    break;
  }
  case 1: {
    m_lblColumn->show();
    m_cmbColName->show();
    break;
  }
  case 2: {
    int day = (QDate::currentDate()).dayOfWeek();
    m_lblFormat->show();
    m_cmbFormat->show();
    m_cmbFormat->insertItem(m_cmbFormat->count(), QDate::shortDayName(day));
    m_cmbFormat->insertItem(m_cmbFormat->count(), QDate::longDayName(day));
    m_cmbFormat->insertItem(m_cmbFormat->count(),
                            (QDate::shortDayName(day)).left(1));
    m_cmbFormat->setCurrentIndex(
        (m_graph->axisFormatInfo(m_mappedaxis)).toInt());
    break;
  }
  case 3: {
    int month = (QDate::currentDate()).month();
    m_lblFormat->show();
    m_cmbFormat->show();
    m_cmbFormat->insertItem(m_cmbFormat->count(), QDate::shortMonthName(month));
    m_cmbFormat->insertItem(m_cmbFormat->count(), QDate::longMonthName(month));
    m_cmbFormat->insertItem(m_cmbFormat->count(),
                            (QDate::shortMonthName(month)).left(1));
    m_cmbFormat->setCurrentIndex(
        (m_graph->axisFormatInfo(m_mappedaxis)).toInt());
  } break;

  case 4: {
    m_lblFormat->show();
    m_cmbFormat->show();
    m_cmbFormat->setEditable(true);

    QStringList lst = (m_graph->axisFormatInfo(m_mappedaxis))
                          .split(";", QString::KeepEmptyParts);
    if (lst.count() == 2) {
      m_cmbFormat->insertItem(m_cmbFormat->count(), lst[1]);
      m_cmbFormat->setItemText(m_cmbFormat->currentIndex(), lst[1]);
    }

    m_cmbFormat->insertItem(m_cmbFormat->count(), "h");
    m_cmbFormat->insertItem(m_cmbFormat->count(), "h ap");
    m_cmbFormat->insertItem(m_cmbFormat->count(), "h AP");
    m_cmbFormat->insertItem(m_cmbFormat->count(), "h:mm");
    m_cmbFormat->insertItem(m_cmbFormat->count(), "h:mm ap");
    m_cmbFormat->insertItem(m_cmbFormat->count(), "hh:mm");
    m_cmbFormat->insertItem(m_cmbFormat->count(), "h:mm:ss");
    m_cmbFormat->insertItem(m_cmbFormat->count(), "h:mm:ss.zzz");
    m_cmbFormat->insertItem(m_cmbFormat->count(), "mm:ss");
    m_cmbFormat->insertItem(m_cmbFormat->count(), "mm:ss.zzz");
    m_cmbFormat->insertItem(m_cmbFormat->count(), "hmm");
    m_cmbFormat->insertItem(m_cmbFormat->count(), "hmmss");
    m_cmbFormat->insertItem(m_cmbFormat->count(), "hhmmss");
    break;
  }
  case 5: {
    m_lblFormat->show();
    m_cmbFormat->show();
    m_cmbFormat->setEditable(true);

    QStringList lst = (m_graph->axisFormatInfo(m_mappedaxis))
                          .split(";", QString::KeepEmptyParts);
    if (lst.count() == 2) {
      m_cmbFormat->insertItem(m_cmbFormat->count(), lst[1]);
      m_cmbFormat->setItemText(m_cmbFormat->currentIndex(), lst[1]);
    }
    m_cmbFormat->insertItem(m_cmbFormat->count(), "yyyy-MM-dd");
    m_cmbFormat->insertItem(m_cmbFormat->count(), "dd.MM.yyyy");
    m_cmbFormat->insertItem(m_cmbFormat->count(), "ddd MMMM d yy");
    m_cmbFormat->insertItem(m_cmbFormat->count(), "dd/MM/yyyy");
    m_cmbFormat->insertItem(m_cmbFormat->count(), "HH:mm:ss");
    break;
  }
  case 6: {
    m_lblTable->show();
    QString tableName = m_graph->axisFormatInfo(m_mappedaxis);
    if (m_tablesList.contains(tableName))
      m_cmbTableName->setItemText(m_cmbTableName->currentIndex(), tableName);
    m_cmbTableName->show();
    break;
  }
  }
}

/** enables or disables the formula text entry fields
 *
 */
void AxisDetails::enableFormulaBox() {
  if (m_chkShowFormula->isChecked()) {
    m_txtFormula->setEnabled(true);
  } else {
    m_txtFormula->setEnabled(false);
  }
}

/** sets the font to use on the scale title
 *
 */
void AxisDetails::setLabelFont() {
  bool okF;
  QFont oldFont = m_graph->axisTitleFont(m_mappedaxis);
  QFont fnt = QFontDialog::getFont(&okF, oldFont, this);
  if (okF && fnt != oldFont) {
    m_labelFont = fnt;
  }
}

/** sets the font to use on the scale numbers
 *
 */
void AxisDetails::setScaleFont() {
  bool okF;
  QFont oldFont = m_graph->axisFont(m_mappedaxis);
  QFont fnt = QFontDialog::getFont(&okF, oldFont, this);
  if (okF && fnt != oldFont) {
    m_scaleFont = fnt;
  }
}
