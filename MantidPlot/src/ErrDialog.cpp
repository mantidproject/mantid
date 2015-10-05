/***************************************************************************
    File                 : ErrDialog.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Tilman Hoener zu Siederdissen
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
    Description          : Add error bars dialog

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
#include "ErrDialog.h"
#include "Table.h"
#include "ApplicationWindow.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QList>
#include <QLabel>
#include <QComboBox>
#include <QRadioButton>
#include <QLineEdit>
#include <QPushButton>
#include <QGroupBox>
#include <QButtonGroup>
#include <QList>
#include <QWidget>
#include <QCheckBox>


ErrDialog::ErrDialog( ApplicationWindow* parent, Qt::WFlags fl )
  : QDialog( parent, fl )
{
  setFocusPolicy( Qt::StrongFocus );
  setSizeGripEnabled( true );

  QVBoxLayout *vbox1 = new QVBoxLayout();
  vbox1->setSpacing (5);

  QHBoxLayout *hbox1 = new QHBoxLayout();
  vbox1->addLayout(hbox1);

  textLabel1 = new QLabel();
  hbox1->addWidget(textLabel1);

  nameLabel = new QComboBox();
  hbox1->addWidget(nameLabel);

  groupBox1 = new QGroupBox(QString(tr("Source of errors")));
  QGridLayout * gridLayout = new QGridLayout(groupBox1);
  vbox1->addWidget(groupBox1);

  buttonGroup1 = new QButtonGroup();
  buttonGroup1->setExclusive( true );

  // Add option to use Mantid workspace errors
  mantidBox = new QRadioButton();
  // Since Mantid is only in English, just set this here instead of in languageChange()
  mantidBox->setText("Mantid Workspace");
  mantidBox->setChecked( true );
  buttonGroup1->addButton(mantidBox);
  gridLayout->addWidget(mantidBox, 0, 0);

  drawAllErrors = new QCheckBox(this);
  drawAllErrors->setText("Draw all errors");
  drawAllErrors->setChecked( parent->drawAllErrors );
  gridLayout->addWidget(drawAllErrors, 0, 1);

  columnBox = new QRadioButton();
  //columnBox->setChecked( true );  // Mantid button now takes this
  buttonGroup1->addButton(columnBox);
  gridLayout->addWidget(columnBox, 1, 0 );

  colNamesBox = new QComboBox();
  tableNamesBox = new QComboBox();
  // Disable initially because Mantid button will be selected
  colNamesBox->setEnabled(false);
  tableNamesBox->setEnabled(false);

  QHBoxLayout * comboBoxes = new QHBoxLayout();
  comboBoxes->addWidget(tableNamesBox);
  comboBoxes->addWidget(colNamesBox);

  gridLayout->addLayout(comboBoxes, 1, 1);

  percentBox = new QRadioButton();
  buttonGroup1->addButton(percentBox);
  gridLayout->addWidget(percentBox, 2, 0 );

  valueBox = new QLineEdit();
  valueBox->setText("5");
  valueBox->setAlignment( Qt::AlignHCenter );
  valueBox->setEnabled(false);
  gridLayout->addWidget(valueBox, 2, 1);

  standardBox = new QRadioButton();
  buttonGroup1->addButton(standardBox);
  gridLayout->addWidget(standardBox, 3, 0 );

  groupBox3 = new QGroupBox(QString());
  vbox1->addWidget(groupBox3);
  QHBoxLayout * hbox2 = new QHBoxLayout(groupBox3);

  buttonGroup2 = new QButtonGroup();
  buttonGroup2->setExclusive( true );

  xErrBox = new QRadioButton();
  // Disable initially because Mantid button will be selected
  xErrBox->setEnabled(false);
  buttonGroup2->addButton(xErrBox);
  hbox2->addWidget(xErrBox );

  yErrBox = new QRadioButton();
  buttonGroup2->addButton(yErrBox);
  hbox2->addWidget(yErrBox );
  yErrBox->setChecked( true );

  QVBoxLayout * vbox2 = new QVBoxLayout();
  buttonAdd = new QPushButton();
  buttonAdd->setDefault( true );
  vbox2->addWidget(buttonAdd);

  buttonCancel = new QPushButton();
  vbox2->addWidget(buttonCancel);

  vbox2->addStretch(1);

  QHBoxLayout * hlayout1 = new QHBoxLayout(this);
  hlayout1->addLayout(vbox1);
  hlayout1->addLayout(vbox2);

  languageChange();

  // signals and slots connections
  connect( buttonAdd, SIGNAL( clicked() ), this, SLOT( add() ) );
  connect( buttonCancel, SIGNAL( clicked() ), this, SLOT( reject() ) );
  connect( percentBox, SIGNAL( toggled(bool) ), valueBox, SLOT( setEnabled(bool) ) );
  connect( columnBox, SIGNAL( toggled(bool) ), tableNamesBox, SLOT( setEnabled(bool) ) );
  connect( columnBox, SIGNAL( toggled(bool) ), colNamesBox, SLOT( setEnabled(bool) ) );
  connect( tableNamesBox, SIGNAL( activated(int) ), this, SLOT( selectSrcTable(int) ));
  // Don't allow X errors to be asked for if choosing Mantid errors
  connect( mantidBox, SIGNAL( toggled(bool) ), xErrBox, SLOT( setDisabled(bool) ) );
}

void ErrDialog::setCurveNames(const QStringList& names)
{
  nameLabel->addItems(names);
}

void ErrDialog::setSrcTables(QList<MdiSubWindow *> tables)
{
  if (tables.isEmpty())
    return;

  srcTables = tables;
  tableNamesBox->clear();

  foreach(MdiSubWindow *w, tables)
    tableNamesBox->insertItem(w->objectName());

  if (!nameLabel->currentText().contains("="))
    tableNamesBox->setCurrentIndex(tableNamesBox->findText(nameLabel->currentText().split("_", QString::SkipEmptyParts)[0]));
  if (tableNamesBox->currentIndex() != -1) selectSrcTable(tableNamesBox->currentIndex());
}

void ErrDialog::selectSrcTable(int tabnr)
{
  auto table = dynamic_cast<Table*>(srcTables.at(tabnr));
  if (table) {
    colNamesBox->clear();
    colNamesBox->addItems(table->colNames());
  }
}

void ErrDialog::add()
{
  int direction=-1;
  if (xErrBox->isChecked())
    direction = 0;
  else
    direction = 1;

  if (columnBox->isChecked())
    emit options(nameLabel->currentText(), tableNamesBox->currentText()+"_"+colNamesBox->currentText(), direction);
  else
  {
    int type;
    if (percentBox->isChecked())
      type = 0;
    else if (standardBox->isChecked())
      type = 1;
    else // Use this if the Mantid box is checked
      type = 2;

    emit options(nameLabel->currentText(),type, valueBox->text(), direction,drawAllErrors->isChecked());
  }

  // If there's only one curve, close the dialog now
  if ( nameLabel->count() == 1 ) reject();
}

void ErrDialog::languageChange()
{
  setWindowTitle( tr( "MantidPlot - Error Bars" ) );
  xErrBox->setText( tr( "&X Error Bars" ) );
  buttonAdd->setText( tr( "&Add" ) );
  textLabel1->setText( tr( "Add Error Bars to" ) );
  groupBox1->setTitle( tr( "Source of errors" ) );
  percentBox->setText( tr( "Percent of data (%)" ) );
  standardBox->setText( tr( "Standard Deviation of Data" ) );
  yErrBox->setText( tr( "&Y Error Bars" ) );
  buttonCancel->setText( tr( "&Close" ) );
  columnBox->setText(tr("Existing column"));
}
