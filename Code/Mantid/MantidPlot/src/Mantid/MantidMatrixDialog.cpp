#include "MantidMatrixDialog.h"
#include "MantidMatrix.h"

#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QLayout>
#include <QGroupBox>
#include <QSpinBox>
#include <QLineEdit>

MantidMatrixDialog::MantidMatrixDialog( QWidget* parent, Qt::WFlags fl )
    : QDialog( parent, fl ),
    d_matrix(0)
{
    setWindowTitle( tr( "MantidPlot - Matrix Properties" ) );

	QGridLayout * topLayout = new QGridLayout();
	QHBoxLayout * bottomLayout = new QHBoxLayout();

	topLayout->addWidget( new QLabel(tr( "Cell Width" )), 0, 0 );
	boxColWidth = new QSpinBox();
	boxColWidth->setRange(0,1000);
	boxColWidth->setSingleStep(10);
	topLayout->addWidget( boxColWidth, 0, 1 );

	topLayout->addWidget( new QLabel(tr( "Data Format" )), 1, 0 );
	
    boxFormat = new QComboBox();
    boxFormat->addItem( tr( "Decimal: 1000" ) );
	boxFormat->addItem( tr( "Scientific: 1E3" ) );
	topLayout->addWidget( boxFormat, 1, 1 );

	topLayout->addWidget( new QLabel( tr( "Numeric Display" )), 2, 0 );
	boxNumericDisplay = new QComboBox();
    boxNumericDisplay->addItem( tr( "Default Decimal Digits" ) );
	boxNumericDisplay->addItem( tr( "Significant Digits=" ) );
	topLayout->addWidget( boxNumericDisplay, 2, 1 );

	boxPrecision = new QSpinBox();
	boxPrecision->setRange(0, 13);
	boxPrecision->setEnabled( false );
	topLayout->addWidget( boxPrecision, 2, 2 );

    topLayout->addWidget( new QLabel( tr( "Set new range" )), 3, 0 );
    editRangeMin = new QLineEdit();
    topLayout->addWidget(editRangeMin, 3, 1 );
    editRangeMax = new QLineEdit();
    topLayout->addWidget(editRangeMax, 3, 2 );

	buttonOk = new QPushButton(tr( "&OK" ));
	buttonOk->setAutoDefault( true );
	buttonOk->setDefault( true );
	bottomLayout->addWidget( buttonOk );

	buttonCancel = new QPushButton(tr( "&Cancel" ));
	buttonCancel->setAutoDefault( true );
	bottomLayout->addWidget( buttonCancel );

	QVBoxLayout * mainLayout = new QVBoxLayout(this);
	mainLayout->addLayout(topLayout);
	mainLayout->addLayout(bottomLayout);

	// signals and slots connections
	connect(buttonOk, SIGNAL(clicked()), this, SLOT(accept()));
	connect(buttonCancel, SIGNAL(clicked()), this, SLOT(close()));
	connect(boxNumericDisplay, SIGNAL(activated(int)), this, SLOT(showPrecisionBox(int)));
}

void MantidMatrixDialog::showPrecisionBox(int item)
{
	if (item)
		boxPrecision->setEnabled(true);
	else {
		boxPrecision->setValue(6);
		boxPrecision->setEnabled(false);
	}
}

void MantidMatrixDialog::apply()
{
    int width = boxColWidth->value();
    if (d_matrix->columnsWidth() != width){
        d_matrix->setColumnsWidth(width,false);
    }

    int prec = boxPrecision->value();
    QChar format = 'f';
    QString fmtName = tr("Decimal");
    if (boxFormat->currentIndex()){
        format = 'e';
        fmtName = tr("Scientific");
    }

   d_matrix->setNumberFormat(format, prec);

    double yMin,yMax;
    yMin = editRangeMin->text().toDouble();
    yMax = editRangeMax->text().toDouble();
    d_matrix->setRange(yMin,yMax);
}

void MantidMatrixDialog::setMatrix(MantidMatrix *m)
{
    if (!m)
        return;

    d_matrix = m;
    boxColWidth->setValue(m->columnsWidth());

    if (QString(m->numberFormat()) == "f")
		boxFormat->setCurrentIndex(0);
	else
		boxFormat->setCurrentIndex(1);

	boxPrecision->setValue(m->precision());
	if (m->precision() != 6){
		boxPrecision->setEnabled( true );
		boxNumericDisplay->setCurrentIndex(1);
	}

    double yMin,yMax;
    m->range(&yMin,&yMax);
    editRangeMin->setText(QString::number(yMin));
    editRangeMax->setText(QString::number(yMax));
    editRangeMin->setValidator(new QDoubleValidator(this));
    editRangeMax->setValidator(new QDoubleValidator(this));
}

void MantidMatrixDialog::accept()
{
	apply();
	close();
}
