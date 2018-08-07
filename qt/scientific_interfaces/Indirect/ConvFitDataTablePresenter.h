#ifndef MANTIDQTCUSTOMINTERFACES_CONVFITDATATABLEPRESENTER_H_
#define MANTIDQTCUSTOMINTERFACES_CONVFITDATATABLEPRESENTER_H_

#include "ConvFitModel.h"
#include "IndirectDataTablePresenter.h"

#include <QTableWidget>

#include <cstddef>
#include <unordered_map>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

/**
  Presenter for a table of convolution fitting data.
  Copyright &copy; 2015-2016 ISIS Rutherford Appleton Laboratory, NScD
  Oak Ridge National Laboratory & European Spallation Source
  This file is part of Mantid.
  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.
  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport ConvFitDataTablePresenter : public IndirectDataTablePresenter {
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
