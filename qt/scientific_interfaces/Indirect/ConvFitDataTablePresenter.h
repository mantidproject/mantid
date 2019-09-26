// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_CONVFITDATATABLEPRESENTER_H_
#define MANTIDQTCUSTOMINTERFACES_CONVFITDATATABLEPRESENTER_H_

#include "ConvFitModel.h"
#include "IndirectDataTablePresenterLegacy.h"

#include <QTableWidget>

#include <cstddef>
#include <unordered_map>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

/**
  Presenter for a table of convolution fitting data.
*/
class DLLExport ConvFitDataTablePresenter : public IndirectDataTablePresenterLegacy {
  Q_OBJECT
public:
  ConvFitDataTablePresenter(ConvFitModel *model, QTableWidget *dataTable);

protected:
  void addTableEntry(std::size_t dataIndex, std::size_t spectrum,
                     int row) override;
  void updateTableEntry(std::size_t dataIndex, std::size_t spectrum,
                        int row) override;

private:
  int workspaceIndexColumn() const override;
  int startXColumn() const override;
  int endXColumn() const override;
  int excludeColumn() const override;
  std::string getResolutionName(int row) const;

  ConvFitModel *m_convFitModel;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACES_CONVFITDATATABLEPRESENTER_H_ */
