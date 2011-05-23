#include "modecontrolwidget.h"

ModeControlWidget::ModeControlWidget(QWidget *parent)
    : QWidget(parent)
{
	this->ui.setupUi(this);

	QObject::connect(parent->parent(), SIGNAL(enableModeButtons()), this,
			SLOT(enableModeButtons()));
	QObject::connect(this->ui.multiSliceButton, SIGNAL(clicked()),
			this, SLOT(onMultiSliceViewButtonClicked()));
	QObject::connect(this->ui.standardButton, SIGNAL(clicked()),
			this, SLOT(onStandardViewButtonClicked()));
	QObject::connect(this->ui.threeSliceButton, SIGNAL(clicked()),
			this, SLOT(onThreeSliceViewButtonClicked()));
}

ModeControlWidget::~ModeControlWidget()
{

}

void ModeControlWidget::enableModeButtons()
{
	this->ui.multiSliceButton->setEnabled(true);
	this->ui.threeSliceButton->setEnabled(true);
}

void ModeControlWidget::onMultiSliceViewButtonClicked()
{
	emit executeSwitchViews(ModeControlWidget::MULTISLICE);
	this->ui.multiSliceButton->setEnabled(false);
	this->ui.standardButton->setEnabled(true);
	this->ui.threeSliceButton->setEnabled(true);
}

void ModeControlWidget::onStandardViewButtonClicked()
{
	emit executeSwitchViews(ModeControlWidget::STANDARD);
	this->ui.standardButton->setEnabled(false);
	this->ui.multiSliceButton->setEnabled(true);
	this->ui.threeSliceButton->setEnabled(true);
}

void ModeControlWidget::onThreeSliceViewButtonClicked()
{
	emit executeSwitchViews(ModeControlWidget::THREESLICE);
	this->ui.threeSliceButton->setEnabled(false);
	this->ui.multiSliceButton->setEnabled(true);
	this->ui.standardButton->setEnabled(true);
}
