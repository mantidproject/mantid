#ifndef MANTID_CUSTOMINTERFACES_INDIRECTSPECTRUMSELECTIONVIEW_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTSPECTRUMSELECTIONVIEW_H_

#include "ui_IndirectSpectrumSelector.h"

#include "../General/UserInputValidator.h"

#include "MantidKernel/System.h"

#include <cstddef>

#include <QObject>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

enum SpectrumSelectionMode { RANGE, STRING };

/** IndirectSpectrumSelectionView

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
class DLLExport IndirectSpectrumSelectionView : public QObject {
  Q_OBJECT
public:
  IndirectSpectrumSelectionView(Ui::IndirectSpectrumSelector *selector);
  ~IndirectSpectrumSelectionView() override;

  SpectrumSelectionMode selectionMode() const;

  std::size_t minimumSpectrum() const;
  std::size_t maximumSpectrum() const;
  std::size_t selectedMaskSpectrum() const;

  std::string spectraString() const;
  std::string maskString() const;

  void setSpectrumRange(std::size_t minimum, std::size_t maximum);
  void setMaskSpectrumRange(std::size_t minimum, std::size_t maximum);

  void setSpectraRegex(const std::string &regex);
  void setMaskBinsRegex(const std::string &regex);

  UserInputValidator &validateSpectraString(UserInputValidator &uiv) const;
  UserInputValidator &validateMaskBinsString(UserInputValidator &uiv) const;

  void showSpectraErrorLabel();
  void showMaskBinErrorLabel();

public slots:
  void setMinimumSpectrum(std::size_t spectrum);
  void setMaximumSpectrum(std::size_t spectrum);
  void setMaskSpectrum(std::size_t spectrum);

  void setSpectraString(const std::string &spectraString);
  void setMaskString(const std::string &maskString);

signals:
  void selectedSpectraChanged(const std::string &);
  void selectedSpectraChanged(std::size_t, std::size_t);
  void maskSpectrumChanged(std::size_t);
  void maskChanged(std::size_t, const std::string &);

private slots:
  void emitMaskSpectrumChanged(int spectrum);
  void emitMaskChanged(const QString &mask);
  void emitSpectraStringChanged();
  void emitSpectraRangeChanged();

private:
  QValidator *createValidator(const QString &regex);

  Ui::IndirectSpectrumSelector *m_selector;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
