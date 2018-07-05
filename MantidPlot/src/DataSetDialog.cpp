/***************************************************************************
    File                 : DataSetDialog.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Tilman Hoener zu
 Siederdissen
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
    Description          : Multi purpose dialog for choosing a data set

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
#include "DataSetDialog.h"
#include "Graph.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

DataSetDialog::DataSetDialog(const QString &text, ApplicationWindow *app,
                             Graph *g, Qt::WFlags fl)
    : QDialog(g, fl), d_app(app), d_graph(g) {
  setAttribute(Qt::WA_DeleteOnClose);
  setWindowTitle(tr("MantidPlot - Select data set"));

  d_operation = ApplicationWindow::NoAnalysis;
  d_graph = nullptr;

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  QHBoxLayout *bottomLayout = new QHBoxLayout();
  bottomLayout->addStretch();

  groupBox1 = new QGroupBox();
  QHBoxLayout *topLayout = new QHBoxLayout(groupBox1);

  topLayout->addWidget(new QLabel(text));
  boxName = new QComboBox();
  topLayout->addWidget(boxName);

  buttonOk = new QPushButton(tr("&OK"));
  buttonOk->setAutoDefault(true);
  buttonOk->setDefault(true);
  bottomLayout->addWidget(buttonOk);

  buttonCancel = new QPushButton(tr("&Cancel"));
  buttonCancel->setAutoDefault(true);
  bottomLayout->addWidget(buttonCancel);

  mainLayout->addWidget(groupBox1);
  mainLayout->addLayout(bottomLayout);

  connect(buttonOk, SIGNAL(clicked()), this, SLOT(accept()));
  connect(buttonCancel, SIGNAL(clicked()), this, SLOT(reject()));

  setGraph(g);
}

void DataSetDialog::accept() {
  if (d_operation == ApplicationWindow::NoAnalysis)
    emit options(boxName->currentText());
  else if (d_graph) {
    if (d_app)
      d_app->analyzeCurve(d_graph, d_operation, boxName->currentText());
  }
  close();
}

void DataSetDialog::setCurveNames(const QStringList &names) {
  boxName->addItems(names);
}

void DataSetDialog::setCurentDataSet(const QString &s) {
  int row = boxName->findText(s);
  boxName->setCurrentIndex(row);
}

void DataSetDialog::setGraph(Graph *g) {
  if (!g)
    return;

  d_graph = g;
  boxName->addItems(g->analysableCurvesList());
}
