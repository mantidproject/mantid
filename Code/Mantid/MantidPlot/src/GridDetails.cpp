//---------------------------
// Includes
//--------------------------

#include "GridDetails.h"
#include "ApplicationWindow.h"
#include <qwt_scale_widget.h>
//#include <qwt_plot.h>
#include "qwt_compat.h"
#include "Plot.h"
#include "plot2D/ScaleEngine.h"
#include "DoubleSpinBox.h"

#include <QWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QGridLayout>
#include <ColorBox.h>
#include <Grid.h>

GridDetails::GridDetails(ApplicationWindow* app, Graph* graph, int alignment, QWidget *parent) : QWidget(parent)
{
  d_app = app;
  d_graph = graph;
  m_initialised = false;
  m_alignment = alignment;

  QGridLayout * rightLayout = new QGridLayout(this);

  boxMajorGrid = new QCheckBox();
  boxMajorGrid->setText(tr("Major Grids"));
  boxMajorGrid->setChecked(true);
  rightLayout->addWidget(boxMajorGrid, 0, 1);

  boxMinorGrid = new QCheckBox();
  boxMinorGrid->setText(tr("Minor Grids"));
  boxMinorGrid->setChecked(false);
  rightLayout->addWidget(boxMinorGrid, 0, 2);

  rightLayout->addWidget(new QLabel(tr("Line Color")), 1, 0);

  boxColorMajor = new ColorBox(0);
  rightLayout->addWidget(boxColorMajor, 1, 1);

  boxColorMinor = new ColorBox(0);
  boxColorMinor->setDisabled(true);
  rightLayout->addWidget(boxColorMinor, 1, 2);

  rightLayout->addWidget(new QLabel(tr("Line Type")), 2, 0);

  boxTypeMajor = new QComboBox();
  boxTypeMajor->addItem("_____");
  boxTypeMajor->addItem("- - -");
  boxTypeMajor->addItem(".....");
  boxTypeMajor->addItem("_._._");
  boxTypeMajor->addItem("_.._..");
  rightLayout->addWidget(boxTypeMajor, 2, 1);

  boxTypeMinor = new QComboBox();
  boxTypeMinor->addItem("_____");
  boxTypeMinor->addItem("- - -");
  boxTypeMinor->addItem(".....");
  boxTypeMinor->addItem("_._._");
  boxTypeMinor->addItem("_.._..");
  boxTypeMinor->setDisabled(true);
  rightLayout->addWidget(boxTypeMinor, 2, 2);

  rightLayout->addWidget(new QLabel(tr("Thickness")), 3, 0);

  boxWidthMajor = new DoubleSpinBox('f');
  boxWidthMajor->setLocale(d_app->locale());
  boxWidthMajor->setSingleStep(0.1);
  boxWidthMajor->setRange(0.1, 20);
  boxWidthMajor->setValue(1);
  rightLayout->addWidget(boxWidthMajor, 3, 1);

  boxWidthMinor = new DoubleSpinBox('f');
  boxWidthMinor->setLocale(d_app->locale());
  boxWidthMinor->setSingleStep(0.1);
  boxWidthMinor->setRange(0.1, 20);
  boxWidthMinor->setValue(1);
  boxWidthMinor->setDisabled(true);
  rightLayout->addWidget(boxWidthMinor, 3, 2);

  rightLayout->addWidget(new QLabel(tr("Axis")), 4, 0);

  boxGridAxis = new QComboBox();
  if (m_alignment == 1)
  {
    boxGridAxis->insertItem(tr("Bottom"));
    boxGridAxis->insertItem(tr("Top"));
    boxZeroLine = new QCheckBox(tr("X=0"));
  }
  else
  {
    boxGridAxis->insertItem(tr("Left"));
    boxGridAxis->insertItem(tr("Right"));
    boxZeroLine = new QCheckBox(tr("Y=0"));
  }
  rightLayout->addWidget(boxGridAxis, 4, 1);

  rightLayout->addWidget(new QLabel(tr("Additional lines")), 5, 0);

  rightLayout->addWidget(boxZeroLine, 5, 1);

  rightLayout->setRowStretch(7, 1);
  rightLayout->setColumnStretch(4, 1);

  initWidgets();
}

GridDetails::~GridDetails()
{

}

void GridDetails::initWidgets()
{
  if (m_initialised)
  {
    return;
  }
  else
  {
    Plot *p = d_graph->plotWidget();
    Grid *grd = dynamic_cast<Grid *>(d_graph->plotWidget()->grid());
    if (!grd)
    {
      return;
    }

    if (m_alignment == 1)
    {
      boxMajorGrid->setChecked(grd->xEnabled());
      boxMinorGrid->setChecked(grd->xMinEnabled());

      QPen majPenX = grd->majPenX();
      boxTypeMajor->setCurrentIndex(majPenX.style() - 1);
      boxColorMajor->setColor(majPenX.color());
      boxWidthMajor->setValue(majPenX.widthF());

      QPen minPenX = grd->minPenX();
      boxTypeMinor->setCurrentItem(minPenX.style() - 1);
      boxColorMinor->setColor(minPenX.color());
      boxWidthMinor->setValue(minPenX.widthF());

      boxGridAxis->setCurrentIndex(grd->xAxis() - 2);
      boxZeroLine->setChecked(grd->xZeroLineEnabled());
    }
    else
    {
      boxMajorGrid->setChecked(grd->yEnabled());
      boxMinorGrid->setChecked(grd->yMinEnabled());

      QPen majPenY = grd->majPenY();
      boxTypeMajor->setCurrentIndex(majPenY.style() - 1);
      boxColorMajor->setColor(majPenY.color());
      boxWidthMajor->setValue(majPenY.widthF());

      QPen minPenY = grd->minPenY();
      boxTypeMinor->setCurrentItem(minPenY.style() - 1);
      boxColorMinor->setColor(minPenY.color());
      boxWidthMinor->setValue(minPenY.widthF());

      boxGridAxis->setCurrentIndex(grd->yAxis());
      boxZeroLine->setChecked(grd->yZeroLineEnabled());
    }

    bool majorOn = boxMajorGrid->isChecked();
    majorGridEnabled(majorOn);

    bool minorOn = boxMinorGrid->isChecked();
    minorGridEnabled(minorOn);

    connect(boxMajorGrid, SIGNAL(toggled(bool)), this, SLOT(majorGridEnabled(bool)));
    connect(boxMinorGrid, SIGNAL(toggled(bool)), this, SLOT(minorGridEnabled(bool)));

    connect(boxMajorGrid, SIGNAL(clicked()), this, SLOT(setModified()));
    connect(boxMinorGrid, SIGNAL(clicked()), this, SLOT(setModified()));  
    connect(boxZeroLine, SIGNAL(clicked()), this, SLOT(setModified()));
    connect(boxColorMinor, SIGNAL(currentIndexChanged(int)), this, SLOT(setModified()));
    connect(boxColorMajor, SIGNAL(currentIndexChanged(int)), this, SLOT(setModified()));
    connect(boxTypeMajor, SIGNAL(currentIndexChanged(int)), this, SLOT(setModified()));  
    connect(boxTypeMinor, SIGNAL(currentIndexChanged(int)), this, SLOT(setModified())); 
    connect(boxGridAxis, SIGNAL(currentIndexChanged(int)), this, SLOT(setModified()));
    connect(boxWidthMajor, SIGNAL(valueChanged(double)), this, SLOT(setModified())); 
    connect(boxWidthMinor, SIGNAL(valueChanged(double)), this, SLOT(setModified()));

    m_modified = false;
    m_initialised = true;
  }
}

void GridDetails::setModified()
{
  m_modified = true;
}

void GridDetails::apply(Grid *grid, bool antialias)
{
  if (m_modified && grid)
  {
    if (m_alignment == 1)
    {
      grid->enableX(boxMajorGrid->isChecked());
      grid->enableXMin(boxMinorGrid->isChecked());
      grid->setXAxis(boxGridAxis->currentIndex() + 2);

      grid->setMajPenX(QPen(ColorBox::color(boxColorMajor->currentIndex()), boxWidthMajor->value(), Graph::getPenStyle(boxTypeMajor->currentIndex())));
      grid->setMinPenX(QPen(ColorBox::color(boxColorMinor->currentIndex()), boxWidthMinor->value(), Graph::getPenStyle(boxTypeMinor->currentIndex())));
      grid->enableZeroLineX(boxZeroLine->isChecked());
    }
    else
    {
      grid->enableY(boxMajorGrid->isChecked());
      grid->enableYMin(boxMinorGrid->isChecked());
      grid->setYAxis(boxGridAxis->currentIndex());

      grid->setMajPenY(QPen(ColorBox::color(boxColorMajor->currentIndex()), boxWidthMajor->value(), Graph::getPenStyle(boxTypeMajor->currentIndex())));
      grid->setMinPenY(QPen(ColorBox::color(boxColorMinor->currentIndex()), boxWidthMinor->value(), Graph::getPenStyle(boxTypeMinor->currentIndex())));
      grid->enableZeroLineY(boxZeroLine->isChecked());
    }

    grid->setRenderHint(QwtPlotItem::RenderAntialiased, antialias);
    m_modified = false;
  }
}

void GridDetails::majorGridEnabled(bool on)
{
  boxTypeMajor->setEnabled(on);
  boxColorMajor->setEnabled(on);
  boxWidthMajor->setEnabled(on);
}

void GridDetails::minorGridEnabled(bool on)
{
  boxTypeMinor->setEnabled(on);
  boxColorMinor->setEnabled(on);
  boxWidthMinor->setEnabled(on);
}