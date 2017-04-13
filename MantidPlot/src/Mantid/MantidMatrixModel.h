#ifndef MANTIDPLOT_MANTIDMATRIXMODEL_H
#define MANTIDPLOT_MANTIDMATRIXMODEL_H

#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <QAbstractTableModel>
#include <QColor>
#include <QLocale>
#include <QObject>
#include <QSet>

/**
MantidMatrixModel is an implementation of QAbstractTableModel which is an
interface between the data (workspace) and the widget displaying it
(QTableView).
It presents spectrum data (Type Y), bin boundaries (Type X), and errors (Type E)
as a table.
*/
class MantidMatrixModel : public QAbstractTableModel {
  Q_OBJECT
public:
  typedef enum { Y, X, E, DX } Type;
  MantidMatrixModel(QObject *parent, const Mantid::API::MatrixWorkspace *ws,
                    int rows, int cols, int start, Type type);

  /// Call this function if the workspace has changed
  void setup(const Mantid::API::MatrixWorkspace *ws, int rows, int cols,
             int start);

  /// Implementation of QAbstractTableModel::rowCount() -- number of rows
  /// (spectra) that can be shown
  int rowCount(const QModelIndex &parent = QModelIndex()) const override {
    (void)parent; // To avoid compiler warning
    return m_rows;
  }

  /// Implementation of QAbstractTableModel::columnCount() -- number of columns.
  /// If type is X it is
  /// the number of bin boundaries. If the type is DX it is the number of bin
  /// boundaries as well.
  /// If type is Y or E it is the number of data values.
  int columnCount(const QModelIndex &parent = QModelIndex()) const override {
    (void)parent; // To avoid compiler warning
    int columnCount = 0;
    if (m_type == X || m_type == DX) {
      columnCount = m_cols + m_colNumCorr;
    } else {
      columnCount = m_cols;
    }
    return columnCount;
  }

  double data(int row, int col) const;

  /// Implementation of QAbstractTableModel::data(...). QTableView uses this
  /// function
  /// to retrieve data for displaying.
  QVariant data(const QModelIndex &index, int role) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;

  Qt::ItemFlags flags(const QModelIndex &index) const override;

  // Set format and precision of displayed numbers.
  void setFormat(const QChar &f, int prec);
  QChar format() { return m_format; }
  int precision() { return m_prec; }
public slots:
  /// Signals QTableView that the data have changed.
  void resetData() { reset(); }

private:
  bool checkMonitorCache(
      int row) const; // check the monitor cache and add to it if neccessary

  bool checkMaskedCache(
      int row) const; // check the masked cache and add to it if neccessary

  bool checkMaskedBinCache(int row, int bin) const;

  const Mantid::API::MatrixWorkspace *m_workspace;
  int m_startRow;     ///< starting workspace index to display
  int m_endRow;       ///< ending workspace index to display
  int m_rows, m_cols; ///< numbers of rows and columns
  int m_colNumCorr;   ///< == 1 for histograms and == 0 for point data
  QLocale m_locale;
  Type m_type; ///< The type: X for bin boundaries, Y for the spectrum data, E
  /// for errors, DX for x errors
  char m_format; //  Format of numbers returned by data(): 'f' - fixed, 'e' -
                 //  scientific.
  int m_prec;    //  Number precision
  QColor m_mon_color;            // Monitor Specific background color
  mutable QSet<int> m_monCache;  // monitor flag cache
  QColor m_mask_color;           // Masked Detector Specific background color
  mutable QSet<int> m_maskCache; // masked flag cache
  mutable QHash<int, QSet<int>> m_maskBinCache; // cache for masked bins
};

#endif
