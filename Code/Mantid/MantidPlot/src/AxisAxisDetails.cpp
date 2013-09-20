//---------------------------
// Includes
//--------------------------

#include "AxisAxisDetails.h"
#include "ApplicationWindow.h"
#include "DoubleSpinBox.h"
#include <qwt_scale_widget.h>
//#include <qwt_plot.h>
#include "qwt_compat.h"
#include "Plot.h"
#include "plot2D/ScaleEngine.h"

#include <QWidget>
#include <QSpinBox>
#include <QCheckBox>
#include <QRadioButton>
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

AxisAxisDetails::AxisAxisDetails(ApplicationWindow* app, Graph* graph, int mappedaxis, QWidget *parent) : QWidget(parent)
{
  d_app = app;
  d_graph = graph;
  tablesList = d_app->tableNames();
  m_mappedaxis = mappedaxis;

  QHBoxLayout * topLayout = new QHBoxLayout();

  chkShowAxis = new QCheckBox(tr("Show"));
  chkShowAxis->setChecked(true);
  topLayout->addWidget(chkShowAxis);

  grpLabel = new QGroupBox(tr("Title"));
  topLayout->addWidget(grpLabel);

  QVBoxLayout *labelBoxLayout = new QVBoxLayout(grpLabel);
  labelBoxLayout->setSpacing(2);

  txtTitle = new QTextEdit();
  txtTitle->setTextFormat(Qt::PlainText);
  QFontMetrics metrics(this->font());
  txtTitle->setMaximumHeight(3 * metrics.height());
  labelBoxLayout->addWidget(txtTitle);

  QHBoxLayout *hl = new QHBoxLayout();
  hl->setMargin(0);
  hl->setSpacing(2);
  btnLabelFont = new QPushButton(tr("&Font"));
  hl->addWidget(btnLabelFont);

  formatButtons = new TextFormatButtons(txtTitle, TextFormatButtons::AxisLabel);
  hl->addWidget(formatButtons);
  hl->addStretch();

  txtTitle->setMaximumWidth(btnLabelFont->width() + formatButtons->width());
  labelBoxLayout->addLayout(hl);

  QHBoxLayout * bottomLayout = new QHBoxLayout();

  QGroupBox *leftBox = new QGroupBox(QString());
  bottomLayout->addWidget(leftBox);
  QGridLayout * leftBoxLayout = new QGridLayout(leftBox);

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

  label1 = new QLabel(tr("Column"));
  rightBoxLayout->addWidget(label1, 0, 0);

  cmbColName = new QComboBox();
  rightBoxLayout->addWidget(cmbColName, 0, 1);

  labelTable = new QLabel(tr("Table"));
  rightBoxLayout->addWidget(labelTable, 1, 0);

  cmbTableName = new QComboBox();
  cmbTableName->insertStringList(tablesList);
  cmbColName->insertStringList(d_app->columnsList(Table::All));
  rightBoxLayout->addWidget(cmbTableName, 1, 1);

  label2 = new QLabel(tr("Format"));
  rightBoxLayout->addWidget(label2, 2, 0);

  cmbFormat = new QComboBox();
  cmbFormat->setDuplicatesEnabled(false);
  rightBoxLayout->addWidget(cmbFormat, 2, 1);

  label3 = new QLabel(tr("Precision"));
  rightBoxLayout->addWidget(label3, 3, 0);
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

AxisAxisDetails::~AxisAxisDetails()
{

}

void AxisAxisDetails::initWidgets()
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

  //change this, txtFormula should always be visible if it's numeric, but disabled if the checkbox is out and shouldn't clear
  if (!formula.isEmpty())
  {
    chkShowFormula->setChecked(true);
    txtFormula->setEnabled(true);
    //txtFormula->show();
    txtFormula->setText(formula);
  }
  else
  {
    chkShowFormula->setChecked(false);
    txtFormula->setEnabled(false);
    //txtFormula->clear();
    //txtFormula->hide();
  }
  showAxis();
  /*
  bool ok = chkShowAxis->isChecked();
  txtTitle->setEnabled(ok);
  cbtnAxisColor->setEnabled(ok);
  cbtnAxisNumColor->setEnabled(ok);
  btnAxesFont->setEnabled(ok);
  grpShowLabels->setEnabled(ok);
  cmbMajorTicksType->setEnabled(ok);
  cmbMinorTicksType->setEnabled(ok);
  cmbAxisType->setEnabled(ok);
  spnBaseline->setEnabled(ok);
  grpLabel->setEnabled(ok);
  */
}

void AxisAxisDetails::setLabelFont()
{
  bool okF;
  QFont oldFont = d_graph->axisTitleFont(m_mappedaxis);
  QFont fnt = QFontDialog::getFont(&okF, oldFont, this);
  if (okF && fnt != oldFont)
  {
    m_labelFont = fnt;
  }
  //apply bypass
  //if (okF && fnt != oldFont)
  //{
  //  d_graph->setAxisTitleFont(m_mappedaxis, fnt);
  //}
}

void AxisAxisDetails::setAxisFormatOptions(int format)
{
  cmbFormat->clear();
  cmbFormat->setEditable(false);
  cmbFormat->hide();
  spnPrecision->hide();
  cmbColName->hide();
  label1->hide();
  label2->hide();
  label3->hide();
  chkShowFormula->hide();
  txtFormula->hide();
  cmbTableName->hide();
  labelTable->hide();

  switch (format)
  {
  case 0:
    label2->show();
    cmbFormat->show();
    cmbFormat->insertItem(tr("Automatic"));
    cmbFormat->insertItem(tr("Decimal: 100.0"));
    cmbFormat->insertItem(tr("Scientific: 1e2"));
    cmbFormat->insertItem(tr("Scientific: 10^2"));
    cmbFormat->setCurrentIndex(d_graph->plotWidget()->axisLabelFormat(m_mappedaxis));

    label3->show();
    spnPrecision->show();
    chkShowFormula->show();
    txtFormula->show();

    enableFormulaBox();
    break;

  case 1:
    label1->show();
    cmbColName->show();
    break;

  case 2:
    {
      int day = (QDate::currentDate()).dayOfWeek();
      label2->show();
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
      label2->show();
      cmbFormat->show();
      cmbFormat->insertItem(QDate::shortMonthName(month));
      cmbFormat->insertItem(QDate::longMonthName(month));
      cmbFormat->insertItem((QDate::shortMonthName(month)).left(1));
      cmbFormat->setCurrentIndex((d_graph->axisFormatInfo(m_mappedaxis)).toInt());
    }
    break;

  case 4:
    {
      label2->show();
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
      label2->show();
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
      labelTable->show();
      QString tableName = d_graph->axisFormatInfo(m_mappedaxis);
      if (tablesList.contains(tableName))
        cmbTableName->setCurrentText(tableName);
      cmbTableName->show();
    }
    break;
  }
}

void AxisAxisDetails::showAxis()
{
  //this is needed
  bool shown = chkShowAxis->isChecked();
  bool labels = grpShowLabels->isChecked();

  //this needs reducing and the parenting feature can take care of most of this
  txtTitle->setEnabled(shown);
  cbtnAxisColor->setEnabled(shown);
  cbtnAxisNumColor->setEnabled(shown);
  btnAxesFont->setEnabled(shown);
  grpShowLabels->setEnabled(shown);
  cmbMajorTicksType->setEnabled(shown);
  cmbMinorTicksType->setEnabled(shown);
  cmbAxisType->setEnabled(shown);
  spnBaseline->setEnabled(shown);
  grpLabel->setEnabled(shown);

  if (shown)
  {
    //all of these previously relied on shown
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

    /*QString formula = txtFormula->text();
    if (!chkShowFormula->isChecked())
    {
    formula = QString();
    }*/
    if (chkShowFormula->isChecked())
    {
      txtFormula->setEnabled(true);
      //txtFormula->show();
    }
    else
    {
      txtFormula->setEnabled(false);
      //txtFormula->hide();
    }
  }
}

void AxisAxisDetails::enableFormulaBox()
{
  if (chkShowFormula->isChecked())
  {
    txtFormula->setEnabled(true);
    //txtFormula->show();
  }
  else
  {
    txtFormula->setEnabled(false);
    //txtFormula->hide();
  }
}

void AxisAxisDetails::setScaleFont()
{
  bool okF;
  QFont oldFont = d_graph->axisFont(m_mappedaxis);
  QFont fnt = QFontDialog::getFont(&okF, oldFont, this);
  if (okF && fnt != oldFont)
  {
    m_scaleFont = fnt;
  }
  //apply bypass
  //d_graph->setAxisFont(axis, fnt);
}

/*
void AxisAxisDetails::updateTickLabelsList(bool on)
{
//this is duplicated code

if (m_mappedaxis == QwtPlot::xBottom || m_mappedaxis == QwtPlot::xTop)
spnAngle->setEnabled(on);

bool userFormat = true;
if (cmbFormat->currentIndex() == 0)
userFormat = false;
spnPrecision->setEnabled(on && userFormat);

//

//this shouldn't matter now as this si muti axis

if (tickLabelsOn[m_mappedaxis] == QString::number(on))
return;
tickLabelsOn[m_mappedaxis] = QString::number(on);

//

//this is to go into apply()
QString formatInfo = QString::null;
int type = cmbAxisType->currentIndex();

if (type == ScaleDraw::Day || type == ScaleDraw::Month)
formatInfo = QString::number(cmbFormat->currentIndex());
else if (type == ScaleDraw::Time || type == ScaleDraw::Date)
{
QStringList lst = (d_graph->axisFormatInfo(m_mappedaxis)).split(";",
QString::SkipEmptyParts);
lst[1] = cmbFormat->currentText();
formatInfo = lst.join(";");
}
else
formatInfo = cmbColName->currentText();

QString formula = txtFormula->text();
if (!chkShowFormula->isChecked())
formula = QString();


//this ins't supposed to be here this is an apply bypass

apply(axis, type, formatInfo, chkShowAxis->isChecked(),
cmbMajorTicksType->currentIndex(), cmbMinorTicksType->currentIndex(),
grpShowLabels->isChecked(), cbtnAxisColor->color(), format, prec,
spnAngle->value(), spnBaseline->value(), formula,
cbtnAxisNumColor->color());

}
*/
/*
void AxisAxisDetails::showAxisFormula()
{
QString formula = d_graph->axisFormula(m_mappedaxis);
if (!formula.isEmpty())
{
chkShowFormula->setChecked(true);
//txtFormula->show();
txtFormula->setText(formula);

}
else
{
chkShowFormula->setChecked(false);
//txtFormula->clear();
//txtFormula->hide();
}
}
*/
//to be merged into an apply method
bool AxisAxisDetails::valid()
{
  return true;
}
void AxisAxisDetails::apply(int axis, int type, const QString& labelsColName, bool axisOn, int majTicksType, int minTicksType, bool labelsOn, const QColor& c, int format, int prec, int rotation, int baselineDist, const QString& formula, const QColor& labelsColor)
{
}
void AxisAxisDetails::apply()
{
  if (!(d_app && d_graph && valid()))
    return;

 // if (!d_graph)
  //  return;
  Table *w = d_app->table(cmbColName->currentText());
  if ((cmbAxisType->currentIndex() == ScaleDraw::Text || cmbAxisType->currentIndex() == ScaleDraw::ColHeader) && !w)
    return;

  QString formula = txtFormula->text();
  if (!chkShowFormula->isChecked())
  {
    formula = QString();
  }

  d_graph->showAxis(m_mappedaxis, cmbAxisType->currentIndex(), cmbColName->currentText(), w, chkShowAxis->isChecked(), cmbMajorTicksType->currentIndex(),
    cmbMinorTicksType->currentIndex(), grpShowLabels->isChecked(), cbtnAxisColor->color(), cmbFormat->currentIndex(), spnPrecision->value(), spnAngle->value(), spnBaseline->value(), formula,
    cbtnAxisNumColor->color());
}

//This isn't actually used
/*
void AxisAxisDetails::updateAxisType(int)
{
int a = mapToQwtAxisId();
cmbAxisType->setCurrentIndex(a);
}
*/

//void AxisAxisDetails::updateLabelsFormat(int)
//{
//  if (cmbAxisType->currentIndex() != ScaleDraw::Numeric)
//    return;
//
//  int mappedaxis = mapToQwtAxisId();
//  int format = d_graph->plotWidget()->axisLabelFormat(mappedaxis);
//  cmbFormat->setCurrentIndex(format);
//  spnPrecision->setValue(d_graph->plotWidget()->axisLabelPrecision(mappedaxis));
//
//  if (format == 0)
//    spnPrecision->setEnabled(false);
//  else
//    spnPrecision->setEnabled(true);
//
//  QString formula = d_graph->axisFormula(mappedaxis);
//  if (!formula.isEmpty())
//  {
//    chkShowFormula->setChecked(true);
//    txtFormula->show();
//    txtFormula->setText(formula);
//  }
//  else
//  {
//    chkShowFormula->setChecked(false);
//    txtFormula->clear();
//    txtFormula->hide();
//  }
//}
//
//void AxisAxisDetails::setBaselineDist(int)
//{
//  spnBaseline->setValue(axesBaseline[mapToQwtAxisId()]);
//}
//
//void AxisAxisDetails::setAxisType(int)
//{
//  int mappedaxis = mapToQwtAxisId();
//  int style = (int) d_graph->axisType(mappedaxis);
//  cmbAxisType->setCurrentIndex(style);
//  setAxisFormatOptions(style);
//
//  if (style == 1)
//    cmbColName->setCurrentText(d_graph->axisFormatInfo(mappedaxis));
//}
//
//void AxisAxisDetails::updateTitleBox(int axis)
//{
//  int axisId = mapToQwtAxis(axis);
//  txtTitle->setText(d_graph->axisTitle(axisId));
//}
//
//void AxisAxisDetails::setTicksType(int)
//{
//  int mappedaxis = mapToQwtAxisId();
//  cmbMajorTicksType->setCurrentIndex(majTicks[mappedaxis]);
//  cmbMinorTicksType->setCurrentIndex(minTicks[mappedaxis]);
//}
//
//void AxisAxisDetails::updateShowBox(int axis)
//{
//  switch (axis)
//  {
//  case 0:
//    {
//      chkShowAxis->setChecked(xAxisOn);
//      int labelsOn = tickLabelsOn[2].toInt();
//      grpShowLabels->setChecked(labelsOn);
//      spnAngle->setEnabled(labelsOn && xAxisOn);
//      cmbFormat->setEnabled(labelsOn && xAxisOn);
//      spnAngle->setValue(xBottomLabelsRotation);
//      break;
//    }
//  case 1:
//    {
//      chkShowAxis->setChecked(yAxisOn);
//      int labelsOn = tickLabelsOn[0].toInt();
//      grpShowLabels->setChecked(labelsOn);
//      cmbFormat->setEnabled(labelsOn && yAxisOn);
//      spnAngle->setEnabled(false);
//      spnAngle->setValue(0);
//      break;
//    }
//  case 2:
//    {
//      chkShowAxis->setChecked(topAxisOn);
//
//      int labelsOn = tickLabelsOn[3].toInt();
//      grpShowLabels->setChecked(labelsOn);
//      cmbFormat->setEnabled(labelsOn && topAxisOn);
//      spnAngle->setEnabled(labelsOn && topAxisOn);
//      spnAngle->setValue(xTopLabelsRotation);
//      break;
//    }
//  case 3:
//    {
//      chkShowAxis->setChecked(rightAxisOn);
//      int labelsOn = tickLabelsOn[1].toInt();
//      grpShowLabels->setChecked(labelsOn);
//      cmbFormat->setEnabled(labelsOn && rightAxisOn);
//      spnAngle->setEnabled(false);
//      spnAngle->setValue(0);
//      break;
//    }
//  }
//
//  bool ok = chkShowAxis->isChecked();
//  txtTitle->setEnabled(ok);
//  cbtnAxisColor->setEnabled(ok);
//  cbtnAxisNumColor->setEnabled(ok);
//  btnAxesFont->setEnabled(ok);
//  grpShowLabels->setEnabled(ok);
//  cmbMajorTicksType->setEnabled(ok);
//  cmbMinorTicksType->setEnabled(ok);
//  cmbAxisType->setEnabled(ok);
//  spnBaseline->setEnabled(ok);
//  grpLabel->setEnabled(ok);
//}
//
//void AxisAxisDetails::updateAxisColor(int)
//{
//  int mappedaxis = mapToQwtAxisId();
//  cbtnAxisColor->blockSignals(true);
//  cbtnAxisColor->setColor(d_graph->axisColor(mappedaxis));
//  cbtnAxisColor->blockSignals(false);
//
//  cbtnAxisNumColor->blockSignals(true);
//  cbtnAxisNumColor->setColor(d_graph->axisLabelsColor(mappedaxis));
//  cbtnAxisNumColor->blockSignals(false);
//}
///
//does nothing bar act as an apply bypass
//void AxisAxisDetails::changeBaselineDist(int baseline)
//{
//
//  axesBaseline[m_mappedaxis] = baseline;
//
//  if (d_graph->axisTitle(m_mappedaxis) != txtTitle->text())
//    d_graph->setAxisTitle(m_mappedaxis, txtTitle->text());
//
//  //this ins't supposed to be here this is an apply bypass
//  /*
//  QString formula = txtFormula->text();
//  if (!chkShowFormula->isChecked())
//    formula = QString();
//
//
//  apply(axis, cmbAxisType->currentIndex(), d_graph->axisFormatInfo(axis),
//  chkShowAxis->isChecked(), cmbMajorTicksType->currentIndex(),
//  cmbMinorTicksType->currentIndex(), grpShowLabels->isChecked(),
//  cbtnAxisColor->color(), cmbFormat->currentIndex(), spnPrecision->value(),
//  spnAngle->value(), baseline, formula, cbtnAxisNumColor->color());
//  */
//}

//These do jack shit apart form acting as an apply bypass, and were the same character for character bar the third line whcih does the same
//void AxisAxisDetails::pickAxisColor()
//{
//  QString formula = txtFormula->text();
//  if (!chkShowFormula->isChecked())
//    formula = QString();
//
//  //this ins't supposed to be here this is an apply bypass
//  /*
//  apply(axis, cmbAxisType->currentIndex(), d_graph->axisFormatInfo(axis),
//  chkShowAxis->isChecked(), cmbMajorTicksType->currentIndex(),
//  cmbMinorTicksType->currentIndex(), grpShowLabels->isChecked(),
//  cbtnAxisColor->color(), cmbFormat->currentIndex(), spnPrecision->value(),
//  spnAngle->value(), spnBaseline->value(), formula,
//  cbtnAxisNumColor->color());
//  */
//}
//
//void AxisAxisDetails::pickAxisNumColor()
//{
//  QString formula = txtFormula->text();
//  if (!chkShowFormula->isChecked())
//    formula = QString::null;
//
//  //this ins't supposed to be here this is an apply bypass
//  /*
//  apply(axis, cmbAxisType->currentIndex(), d_graph->axisFormatInfo(axis),
//  chkShowAxis->isChecked(), cmbMajorTicksType->currentIndex(),
//  cmbMinorTicksType->currentIndex(), grpShowLabels->isChecked(),
//  cbtnAxisColor->color(), cmbFormat->currentIndex(), spnPrecision->value(),
//  spnAngle->value(), spnBaseline->value(), formula,
//  cbtnAxisNumColor->color());
//  */
//}

//these do nothing and are apply bypasses
//void AxisAxisDetails::updateMajTicksType(int)
//{
//  int type = cmbMajorTicksType->currentIndex();
//  if (majTicks[m_mappedaxis] == type)
//    return;
//
//  majTicks[m_mappedaxis] = type;
//  QString formula = txtFormula->text();
//  if (!chkShowFormula->isChecked())
//    formula = QString();
//
//  //this ins't supposed to be here this is an apply bypass
//  /*
//  apply(axis, type, formatInfo, chkShowAxis->isChecked(),
//  cmbMajorTicksType->currentIndex(), cmbMinorTicksType->currentIndex(),
//  grpShowLabels->isChecked(), cbtnAxisColor->color(), format, prec,
//  spnAngle->value(), spnBaseline->value(), formula,
//  cbtnAxisNumColor->color());
//  */
//}
//
//void AxisAxisDetails::updateMinTicksType(int)
//{
//  int type = cmbMinorTicksType->currentIndex();
//  if (minTicks[m_mappedaxis] == type)
//    return;
//
//  minTicks[m_mappedaxis] = type;
//  QString formula = txtFormula->text();
//  if (!chkShowFormula->isChecked())
//    formula = QString();
//
//  //this ins't supposed to be here this is an apply bypass
//  /*
//  apply(axis, type, formatInfo, chkShowAxis->isChecked(),
//  cmbMajorTicksType->currentIndex(), cmbMinorTicksType->currentIndex(),
//  grpShowLabels->isChecked(), cbtnAxisColor->color(), format, prec,
//  spnAngle->value(), spnBaseline->value(), formula,
//  cbtnAxisNumColor->color());
//  */
//}

///this is dealt wiht by another function now
//void AxisAxisDetails::setLabelsNumericFormat(int)
//{
//  int type = cmbAxisType->currentIndex();
//  int prec = spnPrecision->value();
//  int format = cmbFormat->currentIndex();
//
//  Plot *plot = d_graph->plotWidget();
//
//  //QString formatInfo = QString::null;
//  if (type == ScaleDraw::Numeric)
//  {
//    //if (plot->axisLabelFormat(m_mappedaxis) == format
//    //  && plot->axisLabelPrecision(m_mappedaxis) == prec)
//    //  return;
//
//    if (format == 0)
//      spnPrecision->setEnabled(false);
//    else
//      spnPrecision->setEnabled(true);
//  }
//
//  //this bit isn't relevant it's for the apply below
//  /*
//  else if (type == ScaleDraw::Day || type == ScaleDraw::Month)
//  formatInfo = QString::number(format);
//  else if (type == ScaleDraw::Time || type == ScaleDraw::Date)
//  {
//  QStringList lst = d_graph->axisFormatInfo(m_mappedaxis).split(";",
//  QString::SkipEmptyParts);
//  lst[1] = cmbFormat->currentText();
//  formatInfo = lst.join(";");
//  }
//  else
//  formatInfo = cmbColName->currentText();
//
//  QString formula = txtFormula->text();
//  if (!chkShowFormula->isChecked())
//  formula = QString();
//  */
//
//
//  //this ins't supposed to be here this is an apply bypass
//  /*
//  apply(axis, type, formatInfo, chkShowAxis->isChecked(),
//  cmbMajorTicksType->currentIndex(), cmbMinorTicksType->currentIndex(),
//  grpShowLabels->isChecked(), cbtnAxisColor->color(), format, prec,
//  spnAngle->value(), spnBaseline->value(), formula,
//  cbtnAxisNumColor->color());
//  */
//}