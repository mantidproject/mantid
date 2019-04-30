// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//---------------------------
// Includes
//--------------------------

#include "GridDetails.h"
#include "ApplicationWindow.h"
#include <qwt_scale_widget.h>
//#include <qwt_plot.h>
#include "MantidQtWidgets/Common/DoubleSpinBox.h"
#include "MantidQtWidgets/Plotting/Qwt/ScaleEngine.h"
#include "MantidQtWidgets/Plotting/Qwt/qwt_compat.h"
#include "Plot.h"

#include <ColorBox.h>
#include <Grid.h>
#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QWidget>

/** The constructor for a single set of widgets containing parameters for a
 * single direction of gridlines.
 *  @param app :: the containing application window
 *  @param graph :: the graph the dialog is settign the options for
 *  @param mappedaxis :: the QwtPlot::axis value that corresponds to this axis
 *  @param parent :: the QWidget that acts as this widget's parent in the
 * hierarchy
 */
GridDetails::GridDetails(ApplicationWindow *app, Graph *graph, int alignment,
                         QWidget *parent)
    : QWidget(parent) {
  m_app = app;
  m_graph = graph;
  m_initialised = false;
  m_alignment = alignment;
  if (m_alignment != 0 && m_alignment != 1) {
    m_alignment = 0;
  }

  QGridLayout *rightLayout = new QGridLayout(this);

  m_chkMajorGrid = new QCheckBox();
  m_chkMajorGrid->setText(tr("Major Grids"));
  m_chkMajorGrid->setChecked(true);
  rightLayout->addWidget(m_chkMajorGrid, 0, 1);

  m_chkMinorGrid = new QCheckBox();
  m_chkMinorGrid->setText(tr("Minor Grids"));
  m_chkMinorGrid->setChecked(false);
  rightLayout->addWidget(m_chkMinorGrid, 0, 2);

  rightLayout->addWidget(new QLabel(tr("Line Color")), 1, 0);

  m_cboxColorMajor = new ColorBox(nullptr);
  rightLayout->addWidget(m_cboxColorMajor, 1, 1);

  m_cboxColorMinor = new ColorBox(nullptr);
  m_cboxColorMinor->setDisabled(true);
  rightLayout->addWidget(m_cboxColorMinor, 1, 2);

  rightLayout->addWidget(new QLabel(tr("Line Type")), 2, 0);

  m_cmbTypeMajor = new QComboBox();
  m_cmbTypeMajor->addItem("_____");
  m_cmbTypeMajor->addItem("- - -");
  m_cmbTypeMajor->addItem(".....");
  m_cmbTypeMajor->addItem("_._._");
  m_cmbTypeMajor->addItem("_.._..");
  rightLayout->addWidget(m_cmbTypeMajor, 2, 1);

  m_cmbTypeMinor = new QComboBox();
  m_cmbTypeMinor->addItem("_____");
  m_cmbTypeMinor->addItem("- - -");
  m_cmbTypeMinor->addItem(".....");
  m_cmbTypeMinor->addItem("_._._");
  m_cmbTypeMinor->addItem("_.._..");
  m_cmbTypeMinor->setDisabled(true);
  rightLayout->addWidget(m_cmbTypeMinor, 2, 2);

  rightLayout->addWidget(new QLabel(tr("Thickness")), 3, 0);

  m_dspnWidthMajor = new DoubleSpinBox('f');
  m_dspnWidthMajor->setLocale(m_app->locale());
  m_dspnWidthMajor->setSingleStep(0.1);
  m_dspnWidthMajor->setRange(0.1, 20);
  m_dspnWidthMajor->setValue(1);
  rightLayout->addWidget(m_dspnWidthMajor, 3, 1);

  m_dspnWidthMinor = new DoubleSpinBox('f');
  m_dspnWidthMinor->setLocale(m_app->locale());
  m_dspnWidthMinor->setSingleStep(0.1);
  m_dspnWidthMinor->setRange(0.1, 20);
  m_dspnWidthMinor->setValue(1);
  m_dspnWidthMinor->setDisabled(true);
  rightLayout->addWidget(m_dspnWidthMinor, 3, 2);

  rightLayout->addWidget(new QLabel(tr("Axis")), 4, 0);

  m_cmbGridAxis = new QComboBox();
  if (m_alignment == 1) {
    m_cmbGridAxis->addItem(tr("Bottom"));
    m_cmbGridAxis->addItem(tr("Top"));
    m_chkZeroLine = new QCheckBox(tr("X=0"));
  } else {
    m_cmbGridAxis->addItem(tr("Left"));
    m_cmbGridAxis->addItem(tr("Right"));
    m_chkZeroLine = new QCheckBox(tr("Y=0"));
  }
  rightLayout->addWidget(m_cmbGridAxis, 4, 1);

  rightLayout->addWidget(new QLabel(tr("Additional lines")), 5, 0);

  rightLayout->addWidget(m_chkZeroLine, 5, 1);

  rightLayout->setRowStretch(7, 1);
  rightLayout->setColumnStretch(4, 1);

  initWidgets();
}

GridDetails::~GridDetails() {}

/** Initialisation method. Sets up all widgets and variables not done in the
 * constructor.
 *
 */
void GridDetails::initWidgets() {
  if (m_initialised) {
    return;
  } else {
    Grid *grd = dynamic_cast<Grid *>(m_graph->plotWidget()->grid());
    if (!grd) {
      return;
    }

    if (m_alignment == 1) {
      m_chkMajorGrid->setChecked(grd->xEnabled());
      m_chkMinorGrid->setChecked(grd->xMinEnabled());

      QPen majPenX = grd->majPenX();
      m_cmbTypeMajor->setCurrentIndex(majPenX.style() - 1);
      m_cboxColorMajor->setColor(majPenX.color());
      m_dspnWidthMajor->setValue(majPenX.widthF());

      QPen minPenX = grd->minPenX();
      m_cmbTypeMinor->setCurrentIndex(minPenX.style() - 1);
      m_cboxColorMinor->setColor(minPenX.color());
      m_dspnWidthMinor->setValue(minPenX.widthF());

      m_cmbGridAxis->setCurrentIndex(grd->xAxis() - 2);
      m_chkZeroLine->setChecked(grd->xZeroLineEnabled());
    } else {
      m_chkMajorGrid->setChecked(grd->yEnabled());
      m_chkMinorGrid->setChecked(grd->yMinEnabled());

      QPen majPenY = grd->majPenY();
      m_cmbTypeMajor->setCurrentIndex(majPenY.style() - 1);
      m_cboxColorMajor->setColor(majPenY.color());
      m_dspnWidthMajor->setValue(majPenY.widthF());

      QPen minPenY = grd->minPenY();
      m_cmbTypeMinor->setCurrentIndex(minPenY.style() - 1);
      m_cboxColorMinor->setColor(minPenY.color());
      m_dspnWidthMinor->setValue(minPenY.widthF());

      m_cmbGridAxis->setCurrentIndex(grd->yAxis());
      m_chkZeroLine->setChecked(grd->yZeroLineEnabled());
    }

    bool majorOn = m_chkMajorGrid->isChecked();
    majorGridEnabled(majorOn);

    bool minorOn = m_chkMinorGrid->isChecked();
    minorGridEnabled(minorOn);

    connect(m_chkMajorGrid, SIGNAL(toggled(bool)), this,
            SLOT(majorGridEnabled(bool)));
    connect(m_chkMinorGrid, SIGNAL(toggled(bool)), this,
            SLOT(minorGridEnabled(bool)));

    connect(m_chkMajorGrid, SIGNAL(clicked()), this, SLOT(setModified()));
    connect(m_chkMinorGrid, SIGNAL(clicked()), this, SLOT(setModified()));
    connect(m_chkZeroLine, SIGNAL(clicked()), this, SLOT(setModified()));
    connect(m_cboxColorMinor, SIGNAL(currentIndexChanged(int)), this,
            SLOT(setModified()));
    connect(m_cboxColorMajor, SIGNAL(currentIndexChanged(int)), this,
            SLOT(setModified()));
    connect(m_cmbTypeMajor, SIGNAL(currentIndexChanged(int)), this,
            SLOT(setModified()));
    connect(m_cmbTypeMinor, SIGNAL(currentIndexChanged(int)), this,
            SLOT(setModified()));
    connect(m_cmbGridAxis, SIGNAL(currentIndexChanged(int)), this,
            SLOT(setModified()));
    connect(m_dspnWidthMajor, SIGNAL(valueChanged(double)), this,
            SLOT(setModified()));
    connect(m_dspnWidthMinor, SIGNAL(valueChanged(double)), this,
            SLOT(setModified()));

    m_modified = false;
    m_initialised = true;
  }
}

/** Sets the modified flag to true so that the changes may be applied.
 *
 */
void GridDetails::setModified() { m_modified = true; }

/** Applies the grid parameters to the graphs
*
@param grid :: the grid to apply this formatting to
@bool antialias :: apply antialias to this formatting or not
@bool multirun :: this will run multiple times for this dialog, and forces even
if no modified
*/
void GridDetails::apply(Grid *grid, bool antialias, bool multirun) {
  if ((m_modified || multirun) && grid) {
    if (m_alignment == 1) {
      grid->enableX(m_chkMajorGrid->isChecked());
      grid->enableXMin(m_chkMinorGrid->isChecked());
      grid->setXAxis(m_cmbGridAxis->currentIndex() + 2);

      grid->setMajPenX(
          QPen(ColorBox::color(m_cboxColorMajor->currentIndex()),
               m_dspnWidthMajor->value(),
               Graph::getPenStyle(m_cmbTypeMajor->currentIndex())));
      grid->setMinPenX(
          QPen(ColorBox::color(m_cboxColorMinor->currentIndex()),
               m_dspnWidthMinor->value(),
               Graph::getPenStyle(m_cmbTypeMinor->currentIndex())));
      grid->enableZeroLineX(m_chkZeroLine->isChecked());
    } else {
      grid->enableY(m_chkMajorGrid->isChecked());
      grid->enableYMin(m_chkMinorGrid->isChecked());
      grid->setYAxis(m_cmbGridAxis->currentIndex());

      grid->setMajPenY(
          QPen(ColorBox::color(m_cboxColorMajor->currentIndex()),
               m_dspnWidthMajor->value(),
               Graph::getPenStyle(m_cmbTypeMajor->currentIndex())));
      grid->setMinPenY(
          QPen(ColorBox::color(m_cboxColorMinor->currentIndex()),
               m_dspnWidthMinor->value(),
               Graph::getPenStyle(m_cmbTypeMinor->currentIndex())));
      grid->enableZeroLineY(m_chkZeroLine->isChecked());
    }

    grid->setRenderHint(QwtPlotItem::RenderAntialiased, antialias);
    m_modified = false;
  }
}

/** Enables or disables widgets corresponding to the current value of the
 * majorGridEnabled check box.
 *
 */
void GridDetails::majorGridEnabled(bool on) {
  m_cmbTypeMajor->setEnabled(on);
  m_cboxColorMajor->setEnabled(on);
  m_dspnWidthMajor->setEnabled(on);
}

/** Enables or disables widgets corresponding to the current value of the
 * minorGridEnabled check box.
 *
 */
void GridDetails::minorGridEnabled(bool on) {
  m_cmbTypeMinor->setEnabled(on);
  m_cboxColorMinor->setEnabled(on);
  m_dspnWidthMinor->setEnabled(on);
}