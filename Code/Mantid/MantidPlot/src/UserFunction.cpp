#ifdef _WIN32
#pragma warning(disable: 4251)
#endif

#include "MyParser.h"
#include "UserFunction.h"
#include <QMessageBox>

UserFunction2D::UserFunction2D(const QString& s, Qwt3D::SurfacePlot& pw)
: Function(pw), d_formula(s), d_rows(0), d_columns(0)
{}

double UserFunction2D::operator()(double x, double y)
{
    if (d_formula.isEmpty())
		return 0.0;

	MyParser parser;
	double result=0.0;
	try
	{
		parser.DefineVar("x", &x);
		parser.DefineVar("y", &y);

        parser.SetExpr((const std::string)d_formula.ascii());
		result=parser.Eval();
	}
	catch(mu::ParserError &e)
	{
		QMessageBox::critical(0,"MantidPlot - Input function error",QString::fromStdString(e.GetMsg()));
	}
    return result;
}

/**
 * @return The smallest positive value this function ever to return.
 */
double UserFunction2D::getMinPositiveValue() const
{
}

void UserFunction2D::setMesh (unsigned int columns, unsigned int rows)
{
	Function::setMesh (columns, rows);
	d_columns = columns;
	d_rows = rows;
}

