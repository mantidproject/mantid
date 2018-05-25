#ifndef MANTID_CUSTOMINTERFACES_INDIRECTSPECTRUMSELECTIONPRESENTER_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTSPECTRUMSELECTIONPRESENTER_H_

#include "IndirectFittingModel.h"
#include "IndirectSpectrumSelectionView.h"

#include "../General/UserInputValidator.h"

#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include <unordered_map>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

/** IndirectSpectrumSelectionPresenter

    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
    National Laboratory & European Spallation Source

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
class DLLExport IndirectSpectrumSelectionPresenter : public QObject {
  Q_OBJECT
public:
  IndirectSpectrumSelectionPresenter(IndirectFittingModel *model,
                                     IndirectSpectrumSelectionView *view);
  ~IndirectSpectrumSelectionPresenter() override;
  UserInputValidator &validate(UserInputValidator &validator);

  void disableView();
  void enableView();

signals:
  void spectraChanged(std::size_t);
  void maskChanged(const std::string &);
  void invalidSpectraString(const QString &errorMessage);
  void invalidMaskBinsString(const QString &errorMessage);

public slots:
  void setActiveModelIndex(std::size_t index);
  void setActiveIndexToZero();
  void displayBinMask();

private slots:
  void setBinMask(const std::string &maskString);
  void setMaskSpectraList(const std::string &spectraList);
  void updateSpectraList(const std::string &spectraList);
  void updateSpectraRange(std::size_t minimum, std::size_t maximum);
  void setMaskIndex(int index);

private:
  void setSpectraRange(std::size_t minimum, std::size_t maximum);
  void setModelSpectra(const Spectra &spectra);

  UserInputValidator validateSpectraString();
  UserInputValidator &validateSpectraString(UserInputValidator &validator);
  UserInputValidator validateMaskBinsString();

  IndirectFittingModel *m_model;
  std::unique_ptr<IndirectSpectrumSelectionView> m_view;
  std::size_t m_activeIndex;
  std::size_t m_maskIndex;
  std::string m_spectraError;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
