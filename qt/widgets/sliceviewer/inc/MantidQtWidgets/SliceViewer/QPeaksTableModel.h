// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "DllOption.h"
#include "boost/bind.hpp"
#include "boost/function.hpp"
#include "boost/tuple/tuple.hpp"
#include <QAbstractTableModel>
#include <boost/shared_ptr.hpp>
#include <map>
#include <vector>

// Forward declarations
namespace Mantid {
namespace Geometry {
class IPeak;
}

namespace API {
class IPeaksWorkspace;
}
} // namespace Mantid

namespace MantidQt {
namespace SliceViewer {

/** @class QPeaksTableModel

QAbstractTableModel for serving up PeaksWorkspaces.

@author Owen Arnold
@date 07/01/2013
*/
class EXPORT_OPT_MANTIDQT_SLICEVIEWER QPeaksTableModel
    : public QAbstractTableModel {
  Q_OBJECT
public:
  QPeaksTableModel(
      boost::shared_ptr<const Mantid::API::IPeaksWorkspace> peaksWS);
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role) const override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  int numCharacters(const int column) const;
  std::vector<int> defaultHideCols();
  ~QPeaksTableModel() override;
  void setPeaksWorkspace(
      boost::shared_ptr<const Mantid::API::IPeaksWorkspace> peaksWS);
signals:
  void peaksSorted(const std::string & /*_t1*/, const bool /*_t2*/);

private:
  using ColumnNameType = QString;
  using ColumnValueType = QString;
  using ColumnNameSortableMap = std::map<ColumnNameType, bool>;
  using ColumnIndexNameMap = std::map<int, ColumnNameType>;

public:
  /// Label for run number column
  static const QString RUNNUMBER;
  /// Label for detector id column
  static const QString DETID;
  /// Label for h column
  static const QString H;
  /// Label for k column
  static const QString K;
  /// Label for l column
  static const QString L;
  /// Label for wavelength column
  static const QString WAVELENGTH;
  /// Label for change in energy column
  static const QString ENERGY_TRANSFER;
  /// Label for initial energy column
  static const QString INITIAL_ENERGY;
  /// Label for final energy column
  static const QString FINAL_ENERGY;
  /// Label for time-of-flight column
  static const QString TOF;
  /// Label for d-spacing column
  static const QString DSPACING;
  /// Label for integrated intensity column
  static const QString INT;
  /// Label for uncertainty in integrated intensity column
  static const QString SIGMINT;
  /// Label for ratio of intensity/uncertainty
  static const QString INT_SIGINT;
  /// Label for bin count column
  static const QString BINCOUNT;
  /// Label for bank name column
  static const QString BANKNAME;
  /// Label for detector row column
  static const QString ROW;
  /// Label for detector column column
  static const QString COL;
  /// Label for Q-vector in the lab frame column
  static const QString QLAB;
  /// Label for Q-vector in the sample column
  static const QString QSAMPLE;
  /// Label for Q-vector in the peak number column
  static const QString PEAKNUM;

private:
  /// Find the correct column name for this column index
  QString findColumnName(const int colIndex) const;

  /// Index for run number column
  static const int COL_RUNNUMBER;
  /// Index for detector id column
  static const int COL_DETID;
  /// Index for h column
  static const int COL_H;
  /// Index for k column
  static const int COL_K;
  /// Index for l column
  static const int COL_L;
  /// Index for wavelength column
  static const int COL_WAVELENGTH;
  /// Index for change in energy column
  static const int COL_ENERGY_TRANSFER;
  /// Index for initial energy column
  static const int COL_INITIAL_ENERGY;
  /// Index for final energy column
  static const int COL_FINAL_ENERGY;
  /// Index for time-of-flight column
  static const int COL_TOF;
  /// Index for d-spacing column
  static const int COL_DSPACING;
  /// Index for integrated intensity column
  static const int COL_INT;
  /// Index for uncertainty in integrated intensity column
  static const int COL_SIGMINT;
  /// Label for ratio of intensity/uncertainty
  static const int COL_INT_SIGINT;
  /// Index for bin count column
  static const int COL_BINCOUNT;
  /// Index for bank name column
  static const int COL_BANKNAME;
  /// Index for detector row column
  static const int COL_ROW;
  /// Index for detector column column
  static const int COL_COL;
  /// Index for Q-vector in the lab frame column
  static const int COL_QLAB;
  /// Index for Q-vector in the sample column
  static const int COL_QSAMPLE;
  /// Unique peak number
  static const int COL_PEAKNUM;
  /// The number of digits past the decimal to display in the table
  int m_hklPrec;
  /// Map from column index to raw peak data
  std::vector<QVariant (*)(const Mantid::Geometry::IPeak &)> m_dataLookup;
  /// Map from column index to formatted peak data
  std::vector<QString (*)(const Mantid::Geometry::IPeak &)>
      m_formattedValueLookup;
  /// Collection of data for viewing.
  boost::shared_ptr<const Mantid::API::IPeaksWorkspace> m_peaksWS;
  /// Map of column indexes to names
  ColumnIndexNameMap m_columnNameMap;
};
} // namespace SliceViewer
} // namespace MantidQt
