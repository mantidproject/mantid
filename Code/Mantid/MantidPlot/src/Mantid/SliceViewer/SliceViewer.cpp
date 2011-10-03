#include "SliceViewer.h"
#include <qwt_plot_spectrogram.h>
#include "QwtRasterDataTester.h"
#include <qwt_color_map.h>
#include <qwt_plot.h>


SliceViewer::SliceViewer(QWidget *parent)
    : QWidget(parent)
{
	ui.setupUi(this);

	// Create the plot
  m_spectLayout = new QVBoxLayout(ui.frmPlot);
	m_plot = new QwtPlot();
  m_plot->autoRefresh();
  m_spectLayout->addWidget(m_plot, 1, 0);

	// Add a spectrograph
	m_spect = new QwtPlotSpectrogram();
	m_spect->attach(m_plot);

	m_data = new QwtRasterDataTester();
	m_spect->setData(*m_data);
	m_spect->setColorMap(QwtLinearColorMap(Qt::black, Qt::white));
	m_spect->itemChanged();
  m_plot->autoRefresh();
}

SliceViewer::~SliceViewer()
{

}


void SliceViewer::initLayout()
{
}
