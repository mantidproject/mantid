/***************************************************************************
        File                 : MatrixModel.h
        Project              : QtiPlot
--------------------------------------------------------------------
        Copyright            : (C) 2007 by Ion Vasilief
        Email (use @ for *)  : ion_vasilief*yahoo.fr
        Description          : QtiPlot's matrix model

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/

#ifndef MATRIXMODEL_H
#define MATRIXMODEL_H

#include <QAbstractTableModel>
#include <QLocale>
#include <QMessageBox>
#include <QSize>
#include <QVector>

#include <gsl/gsl_matrix.h>
#include <gsl/gsl_permutation.h>

class Matrix;

class MatrixModel : public QAbstractTableModel {
  Q_OBJECT

protected:
  explicit MatrixModel(QObject *parent);

public:
  MatrixModel(int rows = 32, int cols = 32, QObject *parent = nullptr);
  MatrixModel(const QImage &image, QObject *parent);
  ~MatrixModel() override;

  Matrix *matrix() { return d_matrix; };

  Qt::ItemFlags flags(const QModelIndex &index) const override;

  virtual bool canResize(int rows, int cols);
  virtual void setDimensions(int rows, int cols);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  virtual void setRowCount(int rows);

  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  virtual void setColumnCount(int cols);

  bool removeRows(int row, int count,
                  const QModelIndex &parent = QModelIndex()) override;
  bool insertRows(int row, int count,
                  const QModelIndex &parent = QModelIndex()) override;

  bool removeColumns(int column, int count,
                     const QModelIndex &parent = QModelIndex()) override;
  bool insertColumns(int column, int count,
                     const QModelIndex &parent = QModelIndex()) override;

  virtual double x(int col) const;
  virtual double y(int row) const;

  virtual double cell(int row, int col) const;
  virtual void setCell(int row, int col, double val);

  virtual QString text(int row, int col);
  virtual void setText(int row, int col, const QString &);

  std::string saveToProject();
  virtual QImage renderImage();

  virtual double data(int row, int col) const;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  bool setData(const QModelIndex &index, const QVariant &value,
               int role) override;

  virtual double *dataVector() { return d_data; };
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;

  virtual void setImage(const QImage &image);

  virtual bool importASCII(const QString &fname, const QString &sep,
                           int ignoredLines, bool stripSpaces,
                           bool simplifySpaces, const QString &commentString,
                           int importAs, const QLocale &locale,
                           int endLineChar = 0, int maxRows = -1);

  void setLocale(const QLocale &locale) { d_locale = locale; };
  void setNumericFormat(char f, int prec);

  virtual bool initWorkspace();
  virtual void invert();
  virtual void transpose();
  virtual void flipVertically();
  virtual void flipHorizontally();
  virtual void rotate90(bool clockwise);
  virtual void fft(bool inverse);
  virtual void clear(int startRow = 0, int endRow = -1, int startCol = 0,
                     int endCol = -1);
  virtual bool calculate(int startRow = 0, int endRow = -1, int startCol = 0,
                         int endCol = -1);
  virtual bool muParserCalculate(int startRow = 0, int endRow = -1,
                                 int startCol = 0, int endCol = -1);
  virtual double *dataCopy(int startRow = 0, int endRow = -1, int startCol = 0,
                           int endCol = -1);
  virtual void pasteData(double *clipboardBuffer, int topRow, int leftCol,
                         int rows, int cols);

protected:
  Matrix *d_matrix;
  int d_rows, d_cols;

  // private:
  void init();
  double *d_data;
  //! Format code for displaying numbers
  char d_txt_format;
  //! Number of significant digits
  int d_num_precision;
  //! Locale used to display data
  QLocale d_locale;

  //! Pointers to GSL matrices used during inversion operations
  gsl_matrix *d_direct_matrix, *d_inv_matrix;
  //! Pointer to a GSL permutation used during inversion operations
  gsl_permutation *d_inv_perm;
  QSize d_data_block_size;
};

#endif
