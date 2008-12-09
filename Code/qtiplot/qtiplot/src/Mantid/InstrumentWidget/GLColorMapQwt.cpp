#include "GLColorMapQwt.h"
#include "qwt_double_interval.h"
#include "GLColor.h"

GLColorMapQwt::GLColorMapQwt():GLColorMap(),QwtColorMap(QwtColorMap::Indexed)
{
}

GLColorMapQwt::~GLColorMapQwt()
{
}

QwtColorMap* GLColorMapQwt::copy()const
{
	GLColorMapQwt* tmp=new GLColorMapQwt();
	*tmp=*this;
	return dynamic_cast<QwtColorMap*>(tmp);
}


QRgb GLColorMapQwt::rgb(const QwtDoubleInterval& interval,double value) const
{
	int count=((GLColorMap*)this)->getNumberOfColors();
	double delta=(value-interval.minValue())/((interval.maxValue()-interval.minValue())/count);
	int index;
	if(delta<0)index=0;
	else if(delta>(count-1))index=(count-1);
	else index=int(delta);
	boost::shared_ptr<GLColor> col=((GLColorMap*)this)->getColor(index);
	float r,g,b,a;
	col->get(r,g,b,a);
	return qRgb(r*255,g*255,b*255);
}

unsigned char GLColorMapQwt::colorIndex(const QwtDoubleInterval &interval,double value)const
{
	int count=((GLColorMap*)this)->getNumberOfColors();
	double delta=(value-interval.minValue())/((interval.maxValue()-interval.minValue())/count);
	int index;
	if(delta<0)index=0;
	else if(delta>(count-1))index=count-1;
	else index=int(delta);
	return index;
}

QVector<QRgb> GLColorMapQwt::colorTable(const QwtDoubleInterval &interval)const
{
	int count=((GLColorMap*)this)->getNumberOfColors();
	QVector<QRgb>  table(count);

	if ( interval.isValid() )
	{
		const double step = interval.width() / (table.size() - 1);
		for ( int i = 0; i < (int) table.size(); i++ )
			table[i] = rgb(interval, interval.minValue() + step * i);
	}

	return table;
}