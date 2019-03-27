// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTMANTIDWIDGETS_QDATAPROCESSORONELEVELTREEMODEL_H_
#define MANTIDQTMANTIDWIDGETS_QDATAPROCESSORONELEVELTREEMODEL_H_

#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidQtWidgets/Common/DataProcessorUI/AbstractTreeModel.h"
#include "MantidQtWidgets/Common/DataProcessorUI/WhiteList.h"
#include <boost/shared_ptr.hpp>
#include <map>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {

class RowData;
using RowData_sptr = std::shared_ptr<RowData>;

/** QOneLevelTreeModel : Provides a QAbstractItemModel for a
DataProcessorUI with no post-processing defined. The first argument to the
constructor is a Mantid ITableWorkspace containing the values to use in the
reduction. Each row in the table corresponds to an independent reduction. The
second argument is a WhiteList containing the header of the model, i.e. the name
of the columns that will be displayed in the tree. The table workspace must have
the same number of columns as the number of items in the WhiteList.
*/
class EXPORT_OPT_MANTIDQT_COMMON QOneLevelTreeModel : public AbstractTreeModel {
  Q_OBJECT
public:
  QOneLevelTreeModel(Mantid::API::ITableWorkspace_sptr tableWorkspace,
                     const WhiteList &whitelist);
  ~QOneLevelTreeModel() override;

  // Functions to read data from the model

  // Get data for a cell
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  // Get header data for the table
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role) const override;
  // Get row metadata
  RowData_sptr rowData(const QModelIndex &index) const override;
  // Row count
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  // Get the index for a given column, row and parent
  QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const override;
  // Get the 'processed' status of a row
  bool isProcessed(int position,
                   const QModelIndex &parent = QModelIndex()) const override;
  // Check wheter reduction failed for a row
  bool
  reductionFailed(int position,
                  const QModelIndex &parent = QModelIndex()) const override;
  // Get the underlying data structure
  Mantid::API::ITableWorkspace_sptr getTableWorkspace() const;

  // Get the parent
  QModelIndex parent(const QModelIndex &index) const override;

  // Functions to edit the model

  // Change or add data to the model
  bool setData(const QModelIndex &index, const QVariant &value,
               int role = Qt::EditRole) override;
  // Add new rows to the model
  bool insertRows(int row, int count,
                  const QModelIndex &parent = QModelIndex()) override;
  // Remove rows from the model
  bool removeRows(int row, int count,
                  const QModelIndex &parent = QModelIndex()) override;
  // Remove all rows from the model
  bool removeAll();
  // Set the 'processed' status of a row
  bool setProcessed(bool processed, int position,
                    const QModelIndex &parent = QModelIndex()) override;
  // Set the error message for a row
  bool setError(const std::string &error, int position,
                const QModelIndex &parent = QModelIndex()) override;
  // Transfer rows into the table
  void transfer(const std::vector<std::map<QString, QString>> &runs) override;
private slots:
  void tableDataUpdated(const QModelIndex & /*unused*/, const QModelIndex & /*unused*/);

private:
  /// Update all cached row data from the table data
  void updateAllRowData();
  /// Insert a row with given values into the table
  void insertRowWithValues(int rowIndex,
                           const std::map<QString, QString> &rowValues);
  /// Check whether a row's cell values are all empty
  bool rowIsEmpty(int row) const;
  /// Vector containing process status for each row
  std::vector<RowData_sptr> m_rows;
};

/// Typedef for a shared pointer to \c QOneLevelTreeModel
using QOneLevelTreeModel_sptr = boost::shared_ptr<QOneLevelTreeModel>;

} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTIDQTMANTIDWIDGETS_QDATAPROCESSORONELEVELTREEMODEL_H_ */
