#ifndef USERFUNCTION_H
#define USERFUNCTION_H

#include "MantidGeometry/Rendering/OpenGL_Headers.h"

#include <QObject>
#include <QString>
#include <qwt3d_function.h>

/**
 * @brief The Function2D class is the base class for 2D functions to be used
 *  in MantidPlot graphics.
 *
 * Concrete classes must implement virtual functions:
 *
 *      double operator()(double x, double y);
 *         which must return the function value at point (x,y)
 *
 *      double getMinPositiveValue()const ;
 *         which must return the smallest positive value a user of this
 *         function can get.
 *
 *      QString saveToString() const ;
 *          which must record any parameters needed to re-create the function.
 *
 * Concrete classes can implement virtual functions:
 *
 *    void connectToViewer(QObject* viewer);
 *          to connect to a viewer object (eg Graph3D)
 */
class Function2D : public Qwt3D::Function {
public:
  Function2D();
  /// Get minimum positive value. It is needed for logarithmic scales.
  virtual double getMinPositiveValue() const = 0;
  /// Save function parameters to a string.
  virtual QString saveToString() const = 0;
  /// Connect to a viewer object
  virtual void connectToViewer(QObject *) {}

  size_t rows() const { return d_rows; }
  size_t columns() const { return d_columns; }
  void setMesh(size_t columns, size_t rows);

private:
  size_t d_rows, d_columns;
};

/**
 * @brief The UserFunction2D class implements a user defined 2D function.
 *
 * The function is defined as a muParser-style string expression. It must use
 * x and y for the function arguments.
 *
 */
class UserFunction2D : public Function2D {
public:
  explicit UserFunction2D(const QString &s);
  /// Get function value
  double operator()(double x, double y) override;
  /// Get minimum positive value.
  double getMinPositiveValue() const override;
  /// Save function parameters to a string.
  QString saveToString() const override;

  QString formula() const { return d_formula; }

private:
  QString d_formula;
};

#endif
