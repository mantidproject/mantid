#ifdef _WIN32
#pragma warning(disable : 4251)
#endif

#include "UserFunction.h"
#include "MyParser.h"
#include <QMessageBox>

Function2D::Function2D() : Qwt3D::Function(), d_rows(0), d_columns(0) {}

void Function2D::setMesh(size_t columns, size_t rows) {
  Function::setMesh(static_cast<unsigned int>(columns),
                    static_cast<unsigned int>(rows));
  d_columns = columns;
  d_rows = rows;
}

UserFunction2D::UserFunction2D(const QString &s) : Function2D(), d_formula(s) {}

double UserFunction2D::operator()(double x, double y) {
  if (d_formula.isEmpty())
    return 0.0;

  MyParser parser;
  double result = 0.0;
  try {
    parser.DefineVar("x", &x);
    parser.DefineVar("y", &y);

    parser.SetExpr((const std::string)d_formula.toAscii().constData());
    result = parser.Eval();
  } catch (mu::ParserError &e) {
    QMessageBox::critical(nullptr, "MantidPlot - Input function error",
                          QString::fromStdString(e.GetMsg()));
  }
  return result;
}

/**
 * @return The smallest positive value this function ever to return.
 */
double UserFunction2D::getMinPositiveValue() const { return 0.0001; }

/**
 * @brief UserFunction2D::saveToString
 *
 * @return :: Initialization string.
 */
QString UserFunction2D::saveToString() const {
  return formula() + ";" + QString::number(columns()) + ";" +
         QString::number(rows());
}
