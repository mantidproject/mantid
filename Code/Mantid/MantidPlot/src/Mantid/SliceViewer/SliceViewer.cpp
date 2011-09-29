#include "SliceViewer.h"
#include <qwt_plot_spectrogram.h>

SliceViewer::SliceViewer(QWidget *parent)
    : QWidget(parent)
{
	ui.setupUi(this);

	// Create the plot
  m_spectLayout = new QVBoxLayout(ui.frmPlot);
	m_plot = new QwtPlot();
  m_spectLayout->addWidget(m_plot);

	// Add a spectrograph
	m_spect = new QwtPlotSpectrogram();
	m_spect->attach(m_plot);

}

SliceViewer::~SliceViewer()
{

}


void SliceViewer::initLayout()
{
}
