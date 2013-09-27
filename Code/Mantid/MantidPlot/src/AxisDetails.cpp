//---------------------------
// Includes
//--------------------------

#include "AxisDetails.h"
#include "ApplicationWindow.h"
#include "DoubleSpinBox.h"
#include <qwt_scale_widget.h>
//#include <qwt_plot.h>
#include "qwt_compat.h"
#include "Plot.h"
#include "plot2D/ScaleEngine.h"

#include <QWidget>
#include <QSpinBox>
#include <QRadioButton>
#include <QCheckBox>
#include <QGroupBox>
#include <QComboBox>
#include <QLabel>
#include <QDateTimeEdit>
#include <QTimeEdit>
#include <QLayout>
#include <QDate>
#include <QList>
#include <QListWidget>
#include <QVector>
#include <QTextEdit>
#include <QPushButton>
#include <QGridLayout>
#include <QMessageBox>
#include <TextFormatButtons.h>
#include <ColorButton.h>
#include <QFontDialog>

AxisDetails::AxisDetails(ApplicationWindow* app, Graph* graph, int mappedaxis, QWidget *parent) : QWidget(parent)
{
  d_app = app;
  d_graph = graph;
  tablesList = d_app->tableNames();
  m_mappedaxis = mappedaxis;
  m_initialised = false;
  QHBoxLayout * topLayout = new QHBoxLayout();

  chkShowAxis = new QCheckBox(tr("Show"));
  topLayout->addWidget(chkShowAxis);

  grpTitle = new QGroupBox(tr("Title"));
  topLayout->addWidget(grpTitle);

  QVBoxLayout *titleBoxLayout = new QVBoxLayout(grpTitle);
  titleBoxLayout->setSpacing(2);

  txtTitle = new QTextEdit();
  txtTitle->setTextFormat(Qt::PlainText);
  QFontMetrics metrics(this->font());
  txtTitle->setMaximumHeight(3 * metrics.height());
  titleBoxLayout->addWidget(txtTitle);

  QHBoxLayout *hl = new QHBoxLayout();
  hl->setMargin(0);
  hl->setSpacing(2);
  btnLabelFont = new QPushButton(tr("&Font"));
  hl->addWidget(btnLabelFont);

  formatButtons = new TextFormatButtons(txtTitle, TextFormatButtons::AxisLabel);
  hl->addWidget(formatButtons);
  hl->addStretch();

  txtTitle->setMaximumWidth(btnLabelFont->width() + formatButtons->width());
  titleBoxLayout->addLayout(hl);

  QHBoxLayout * bottomLayout = new QHBoxLayout();

  grpAxisDisplay = new QGroupBox(QString());
  bottomLayout->addWidget(grpAxisDisplay);
  QGridLayout * leftBoxLayout = new QGridLayout(grpAxisDisplay);

  leftBoxLayout->addWidget(new QLabel(tr("Type")), 0, 0);

  cmbAxisType = new QComboBox();
  cmbAxisType->addItem(tr("Numeric"));
  cmbAxisType->addItem(tr("Text from table"));
  cmbAxisType->addItem(tr("Day of the week"));
  cmbAxisType->addItem(tr("Month"));
  cmbAxisType->addItem(tr("Time"));
  cmbAxisType->addItem(tr("Date"));
  cmbAxisType->addItem(tr("Column Headings"));
  leftBoxLayout->addWidget(cmbAxisType, 0, 1);

  leftBoxLayout->addWidget(new QLabel(tr("Font")), 1, 0);

  btnAxesFont = new QPushButton();
  btnAxesFont->setText(tr("Axis &Font"));
  leftBoxLayout->addWidget(btnAxesFont, 1, 1);

  leftBoxLayout->addWidget(new QLabel(tr("Color")), 2, 0);
  cbtnAxisColor = new ColorButton();
  leftBoxLayout->addWidget(cbtnAxisColor, 2, 1);

  leftBoxLayout->addWidget(new QLabel(tr("Major Ticks")), 3, 0);

  cmbMajorTicksType = new QComboBox();
  cmbMajorTicksType->addItem(tr("None"));
  cmbMajorTicksType->addItem(tr("Out"));
  cmbMajorTicksType->addItem(tr("In & Out"));
  cmbMajorTicksType->addItem(tr("In"));
  leftBoxLayout->addWidget(cmbMajorTicksType, 3, 1);

  leftBoxLayout->addWidget(new QLabel(tr("Minor Ticks")), 4, 0);

  cmbMinorTicksType = new QComboBox();
  cmbMinorTicksType->addItem(tr("None"));
  cmbMinorTicksType->addItem(tr("Out"));
  cmbMinorTicksType->addItem(tr("In & Out"));
  cmbMinorTicksType->addItem(tr("In"));
  leftBoxLayout->addWidget(cmbMinorTicksType, 4, 1);

  leftBoxLayout->addWidget(new QLabel(tr("Stand-off")), 5, 0);
  spnBaseline = new QSpinBox();
  spnBaseline->setRange(0, 1000);
  leftBoxLayout->addWidget(spnBaseline);

  grpShowLabels = new QGroupBox(tr("Show Labels"));
  grpShowLabels->setCheckable(true);
  grpShowLabels->setChecked(true);

  bottomLayout->addWidget(grpShowLabels);
  QGridLayout *rightBoxLayout = new QGridLayout(grpShowLabels);

  lblColumn = new QLabel(tr("Column"));
  rightBoxLayout->addWidget(lblColumn, 0, 0);

  cmbColName = new QComboBox();
  rightBoxLayout->addWidget(cmbColName, 0, 1);

  lblTable = new QLabel(tr("Table"));
  rightBoxLayout->addWidget(lblTable, 1, 0);

  cmbTableName = new QComboBox();
  cmbTableName->insertStringList(tablesList);
  cmbColName->insertStringList(d_app->columnsList(Table::All));
  rightBoxLayout->addWidget(cmbTableName, 1, 1);

  lblFormat = new QLabel(tr("Format"));
  rightBoxLayout->addWidget(lblFormat, 2, 0);

  cmbFormat = new QComboBox();
  cmbFormat->setDuplicatesEnabled(false);
  rightBoxLayout->addWidget(cmbFormat, 2, 1);

  lblPrecision = new QLabel(tr("Precision"));
  rightBoxLayout->addWidget(lblPrecision, 3, 0);
  spnPrecision = new QSpinBox();
  spnPrecision->setRange(0, 10);
  rightBoxLayout->addWidget(spnPrecision, 3, 1);

  rightBoxLayout->addWidget(new QLabel(tr("Angle")), 4, 0);

  spnAngle = new QSpinBox();
  spnAngle->setRange(-90, 90);
  spnAngle->setSingleStep(5);
  rightBoxLayout->addWidget(spnAngle, 4, 1);

  rightBoxLayout->addWidget(new QLabel(tr("Color")), 5, 0);
  cbtnAxisNumColor = new ColorButton();
  rightBoxLayout->addWidget(cbtnAxisNumColor, 5, 1);

  chkShowFormula = new QCheckBox(tr("For&mula"));
  rightBoxLayout->addWidget(chkShowFormula, 6, 0);

  txtFormula = new QTextEdit();
  txtFormula->setTextFormat(Qt::PlainText);
  txtFormula->setMaximumHeight(3 * metrics.height());
  //txtFormula->hide();
  rightBoxLayout->addWidget(txtFormula, 6, 1);
  rightBoxLayout->setRowStretch(7, 1);

  QVBoxLayout * rightLayout = new QVBoxLayout(this);
  rightLayout->addLayout(topLayout);
  rightLayout->addLayout(bottomLayout);
  rightLayout->addStretch(1);

  connect(chkShowFormula, SIGNAL(clicked()), this, SLOT(enableFormulaBox()));
  connect(cmbAxisType, SIGNAL(activated(int)), this, SLOT(setAxisFormatOptions(int)));

  connect(grpShowLabels, SIGNAL(clicked(bool)), this,  SLOT(showAxis()));
  connect(chkShowAxis, SIGNAL(clicked()), this, SLOT(showAxis()));
  connect(cmbFormat, SIGNAL(activated(int)), this,  SLOT(showAxis()));

  connect(btnAxesFont, SIGNAL(clicked()), this, SLOT(setScaleFont()));
  connect(btnLabelFont, SIGNAL(clicked()), this, SLOT(setLabelFont()));

  initWidgets();
}

AxisDetails::~AxisDetails()
{

}

void AxisDetails::initWidgets()
{
  if (m_initialised)
  {
    return;
  }
  else
  {
    Plot *p = d_graph->plotWidget();
    int style = (int) d_graph->axisType(m_mappedaxis);

    bool axisOn = p->axisEnabled(m_mappedaxis);
    const QList<int> majTicks = p->getMajorTicksType();
    const QList<int> minTicks = p->getMinorTicksType();

    const QwtScaleDraw *sd = p->axisScaleDraw(m_mappedaxis);
    bool labelsOn = sd->hasComponent(QwtAbstractScaleDraw::Labels);

    int format = p->axisLabelFormat(m_mappedaxis);

    //Top
    chkShowAxis->setChecked(axisOn);
    txtTitle->setText(d_graph->axisTitle(m_mappedaxis));
    m_labelFont = d_graph->axisTitleFont(m_mappedaxis);

    //bottom left
    cmbAxisType->setCurrentIndex(style);
    setAxisFormatOptions(style);
    m_scaleFont = p->axisFont(m_mappedaxis);

    cbtnAxisColor->setColor(d_graph->axisColor(m_mappedaxis));

    cmbMajorTicksType->setCurrentIndex(majTicks[m_mappedaxis]);
    cmbMinorTicksType->setCurrentIndex(minTicks[m_mappedaxis]);

    QwtScaleWidget *scale = dynamic_cast<QwtScaleWidget *>(p->axisWidget(m_mappedaxis));
    if (scale)
    {
      spnBaseline->setValue(scale->margin());
    }
    else
    {
      spnBaseline->setValue(0);
    }

    //bottom right
    grpShowLabels->setChecked(labelsOn);

    cmbFormat->setEnabled(labelsOn && axisOn);
    cmbFormat->setCurrentIndex(format);

    if (cmbAxisType->currentIndex() == ScaleDraw::Numeric)
    {
      spnPrecision->setValue(p->axisLabelPrecision(m_mappedaxis));
    }
    else if (cmbAxisType->currentIndex() == ScaleDraw::Text)
    {
      cmbColName->setCurrentText(d_graph->axisFormatInfo(m_mappedaxis));
    }

    spnPrecision->setEnabled(format != 0);

    if (m_mappedaxis == QwtPlot::xBottom || m_mappedaxis == QwtPlot::xTop)
    {
      spnAngle->setEnabled(labelsOn && axisOn);
      spnAngle->setValue(d_graph->labelsRotation(m_mappedaxis));
    }
    else
    {
      spnAngle->setEnabled(false);
      spnAngle->setValue(0);
    }

    cbtnAxisNumColor->setColor(d_graph->axisLabelsColor(m_mappedaxis));

    QString formula = d_graph->axisFormula(m_mappedaxis);
    txtFormula->setFixedWidth(150);

    if (!formula.isEmpty())
    {
      chkShowFormula->setChecked(true);
      txtFormula->setEnabled(true);
      txtFormula->setText(formula);
    }
    else
    {
      chkShowFormula->setChecked(false);
      txtFormula->setEnabled(false);
    }
    showAxis();

    connect(chkShowFormula, SIGNAL(stateChanged(int)), this, SLOT(setModified()));
    connect(chkShowAxis, SIGNAL(stateChanged(int)), this, SLOT(setModified()));

    connect(cmbAxisType, SIGNAL(currentIndexChanged(int)), this, SLOT(setModified()));
    connect(cmbAxisType, SIGNAL(editTextChanged(QString)), this, SLOT(setModified()));
    connect(cmbMajorTicksType, SIGNAL(currentIndexChanged(int)), this,  SLOT(setModified()));
    connect(cmbMajorTicksType, SIGNAL(editTextChanged(QString)), this, SLOT(setModified()));
    connect(cmbTableName, SIGNAL(currentIndexChanged(int)), this,  SLOT(setModified()));
    connect(cmbTableName, SIGNAL(editTextChanged(QString)), this, SLOT(setModified()));
    connect(cmbMinorTicksType, SIGNAL(currentIndexChanged(int)), this,  SLOT(setModified()));
    connect(cmbMinorTicksType, SIGNAL(editTextChanged(QString)), this, SLOT(setModified()));
    connect(cmbColName, SIGNAL(currentIndexChanged(int)), this,  SLOT(setModified()));
    connect(cmbColName, SIGNAL(editTextChanged(QString)), this, SLOT(setModified()));
    connect(cmbFormat, SIGNAL(currentIndexChanged(int)), this,  SLOT(setModified()));
    connect(cmbFormat, SIGNAL(editTextChanged(QString)), this, SLOT(setModified()));
    connect(grpShowLabels, SIGNAL(clicked(bool)), this,  SLOT(setModified()));
    connect(btnAxesFont, SIGNAL(clicked()), this, SLOT(setModified()));
    connect(btnLabelFont, SIGNAL(clicked()), this, SLOT(setModified()));
    connect(txtFormula, SIGNAL(textChanged()), this, SLOT(setModified()));
    connect(txtTitle, SIGNAL(textChanged()), this, SLOT(setModified()));
    connect(formatButtons, SIGNAL(formattingModified()), this, SLOT(setModified()));
    connect(spnPrecision,SIGNAL(valueChanged(int)), this, SLOT(setModified()));
    connect(spnAngle,SIGNAL(valueChanged(int)), this, SLOT(setModified()));
    connect(spnBaseline,SIGNAL(valueChanged(int)), this, SLOT(setModified()));
    connect(cbtnAxisColor, SIGNAL(colorChanged()), this, SLOT(setModified()));
    connect(cbtnAxisNumColor, SIGNAL(colorChanged()), this, SLOT(setModified()));

    m_modified = false;
    m_initialised = true;
  }
}

void AxisDetails::setModified()
{
  m_modified = true;
}

bool AxisDetails::valid()
{
  Table *w = d_app->table(cmbColName->currentText());
  return m_initialised && d_app && d_graph && !((cmbAxisType->currentIndex() == ScaleDraw::Text || cmbAxisType->currentIndex() == ScaleDraw::ColHeader) && !w);
}

void AxisDetails::apply()
{
  if (m_modified && valid())
  {
    Table *w = d_app->table(cmbColName->currentText());

    QString formula = txtFormula->text();
    if (!chkShowFormula->isChecked())
    {
      formula = QString();
    }

    int type = cmbAxisType->currentIndex();
    QString formatInfo = cmbColName->currentText();
    if (type == ScaleDraw::Day || type == ScaleDraw::Month)
    {
      formatInfo = QString::number(cmbFormat->currentIndex());
    }
    else if (type == ScaleDraw::Time || type == ScaleDraw::Date)
    {
      QStringList lst = d_graph->axisFormatInfo(m_mappedaxis).split(";", QString::SkipEmptyParts);
      lst[1] = cmbFormat->currentText();
      formatInfo = lst.join(";");
    }

    d_graph->showAxis(m_mappedaxis, cmbAxisType->currentIndex(), formatInfo, w, chkShowAxis->isChecked(), cmbMajorTicksType->currentIndex(),
      cmbMinorTicksType->currentIndex(), grpShowLabels->isChecked(), cbtnAxisColor->color(), cmbFormat->currentIndex(), spnPrecision->value(), spnAngle->value(), spnBaseline->value(), formula,
      cbtnAxisNumColor->color());
    d_graph->setAxisTitle(m_mappedaxis, txtTitle->text());
    d_graph->setAxisFont(m_mappedaxis, m_scaleFont);
    d_graph->setAxisTitleFont(m_mappedaxis, m_labelFont);
    m_modified = false;
  }
}

void AxisDetails::showAxis()
{
  bool shown = chkShowAxis->isChecked();
  bool labels = grpShowLabels->isChecked();

  grpShowLabels->setEnabled(shown);
  grpAxisDisplay->setEnabled(shown);
  grpTitle->setEnabled(shown);

  if (shown)
  {
    cmbFormat->setEnabled(labels);
    cmbColName->setEnabled(labels);
    chkShowFormula->setEnabled(labels);
    txtFormula->setEnabled(labels);

    //this should so the work of the below IF but on one line and slightly more efficently as i assume setDisabled negates that given to it
    spnAngle->setEnabled((m_mappedaxis == QwtPlot::xBottom || m_mappedaxis == QwtPlot::xTop) && labels);
    /*
    if (m_mappedaxis == QwtPlot::xBottom || m_mappedaxis == QwtPlot::xTop)
    {
    spnAngle->setEnabled(labels);
    }
    else
    {
    spnAngle->setDisabled(true);
    }
    */
    spnPrecision->setEnabled(labels && (cmbAxisType->currentIndex() == ScaleDraw::Numeric) && (cmbFormat->currentIndex() != 0));

    enableFormulaBox();
  }
}

void AxisDetails::setAxisFormatOptions(int type)
{
  cmbFormat->clear();
  cmbFormat->setEditable(false);
  cmbFormat->hide();
  spnPrecision->hide();
  cmbColName->hide();
  lblColumn->hide();
  lblFormat->hide();
  lblPrecision->hide();
  chkShowFormula->hide();
  txtFormula->hide();
  cmbTableName->hide();
  lblTable->hide();

  switch (type)
  {
  case 0:
    lblFormat->show();
    cmbFormat->show();
    cmbFormat->insertItem(tr("Automatic"));
    cmbFormat->insertItem(tr("Decimal: 100.0"));
    cmbFormat->insertItem(tr("Scientific: 1e2"));
    cmbFormat->insertItem(tr("Scientific: 10^2"));
    cmbFormat->setCurrentIndex(d_graph->plotWidget()->axisLabelFormat(m_mappedaxis));

    lblPrecision->show();
    spnPrecision->show();
    spnPrecision->setEnabled(cmbFormat->currentIndex() != 0);
    chkShowFormula->show();
    txtFormula->show();

    enableFormulaBox();
    break;

  case 1:
    lblColumn->show();
    cmbColName->show();
    break;

  case 2:
    {
      int day = (QDate::currentDate()).dayOfWeek();
      lblFormat->show();
      cmbFormat->show();
      cmbFormat->insertItem(QDate::shortDayName(day));
      cmbFormat->insertItem(QDate::longDayName(day));
      cmbFormat->insertItem((QDate::shortDayName(day)).left(1));
      cmbFormat->setCurrentIndex((d_graph->axisFormatInfo(m_mappedaxis)).toInt());
    }
    break;

  case 3:
    {
      int month = (QDate::currentDate()).month();
      lblFormat->show();
      cmbFormat->show();
      cmbFormat->insertItem(QDate::shortMonthName(month));
      cmbFormat->insertItem(QDate::longMonthName(month));
      cmbFormat->insertItem((QDate::shortMonthName(month)).left(1));
      cmbFormat->setCurrentIndex((d_graph->axisFormatInfo(m_mappedaxis)).toInt());
    }
    break;

  case 4:
    {
      lblFormat->show();
      cmbFormat->show();
      cmbFormat->setEditable(true);

      QStringList lst = (d_graph->axisFormatInfo(m_mappedaxis)).split(";",
        QString::KeepEmptyParts);
      if (lst.count() == 2)
      {
        cmbFormat->insertItem(lst[1]);
        cmbFormat->setCurrentText(lst[1]);
      }

      cmbFormat->insertItem("h");
      cmbFormat->insertItem("h ap");
      cmbFormat->insertItem("h AP");
      cmbFormat->insertItem("h:mm");
      cmbFormat->insertItem("h:mm ap");
      cmbFormat->insertItem("hh:mm");
      cmbFormat->insertItem("h:mm:ss");
      cmbFormat->insertItem("h:mm:ss.zzz");
      cmbFormat->insertItem("mm:ss");
      cmbFormat->insertItem("mm:ss.zzz");
      cmbFormat->insertItem("hmm");
      cmbFormat->insertItem("hmmss");
      cmbFormat->insertItem("hhmmss");
    }
    break;

  case 5:
    {
      lblFormat->show();
      cmbFormat->show();
      cmbFormat->setEditable(true);

      QStringList lst = (d_graph->axisFormatInfo(m_mappedaxis)).split(";",
        QString::KeepEmptyParts);
      if (lst.count() == 2)
      {
        cmbFormat->insertItem(lst[1]);
        cmbFormat->setCurrentText(lst[1]);
      }
      cmbFormat->insertItem("yyyy-MM-dd");
      cmbFormat->insertItem("dd.MM.yyyy");
      cmbFormat->insertItem("ddd MMMM d yy");
      cmbFormat->insertItem("dd/MM/yyyy");
      cmbFormat->insertItem("HH:mm:ss");
    }
    break;

  case 6:
    {
      lblTable->show();
      QString tableName = d_graph->axisFormatInfo(m_mappedaxis);
      if (tablesList.contains(tableName))
        cmbTableName->setCurrentText(tableName);
      cmbTableName->show();
    }
    break;
  }
}

void AxisDetails::enableFormulaBox()
{
  if (chkShowFormula->isChecked())
  {
    txtFormula->setEnabled(true);
  }
  else
  {
    txtFormula->setEnabled(false);
  }
}

void AxisDetails::setLabelFont()
{
  bool okF;
  QFont oldFont = d_graph->axisTitleFont(m_mappedaxis);
  QFont fnt = QFontDialog::getFont(&okF, oldFont, this);
  if (okF && fnt != oldFont)
  {
    m_labelFont = fnt;
  }
}

void AxisDetails::setScaleFont()
{
  bool okF;
  QFont oldFont = d_graph->axisFont(m_mappedaxis);
  QFont fnt = QFontDialog::getFont(&okF, oldFont, this);
  if (okF && fnt != oldFont)
  {
    m_scaleFont = fnt;
  }  
}