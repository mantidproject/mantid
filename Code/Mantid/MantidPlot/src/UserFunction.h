#ifndef USERFUNCTION_H
#define USERFUNCTION_H

#include <qwt3d_function.h>
#include <QString>
using namespace Qwt3D;

class UserHelperFunction
{
public:
    virtual ~UserHelperFunction() {};
    virtual double operator()(double x, double y)=0;
    virtual double getMinPositiveValue()const = 0;
};


//! Class for user defined surfaces
class UserFunction : public Function
{
public:
    UserFunction(const QString& s, SurfacePlot& pw);

    double operator()(double x, double y);
	QString function(){return formula;};

	unsigned int rows(){return d_rows;};
	unsigned int columns(){return d_columns;};
	void setMesh (unsigned int columns, unsigned int rows);
    void setHlpFun(UserHelperFunction* hlp){m_hlpFun = hlp;}//Mantid
    UserHelperFunction* hlpFun(){return m_hlpFun;}

private:
	  QString formula;
	  unsigned int d_rows, d_columns;
      UserHelperFunction* m_hlpFun;//Mantid
};

#endif
