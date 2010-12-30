#include "MyParser.h"
#include "UserFunction.h"
#include <QMessageBox>

UserFunction::UserFunction(const QString& s, SurfacePlot& pw)
: Function(pw), formula(s), 
m_hlpFun(0)//Mantid
{}

double UserFunction::operator()(double x, double y)
{
    //Mantid
    if (m_hlpFun) 
        return (*m_hlpFun)(x,y);

	if (formula.isEmpty())
		return 0.0;

	MyParser parser;
	double result=0.0;
	try
	{
		parser.DefineVar("x", &x);
		parser.DefineVar("y", &y);

		parser.SetExpr((const std::string)formula.ascii());
		result=parser.Eval();
	}
	catch(mu::ParserError &e)
	{
		QMessageBox::critical(0,"MantidPlot - Input function error",QString::fromStdString(e.GetMsg()));
	}
	return result;
}

void UserFunction::setMesh (unsigned int columns, unsigned int rows)
{
	Function::setMesh (columns, rows);
	d_columns = columns;
	d_rows = rows;
}

