#ifndef MANTID_CUSTOMINTERFACES_INDIRECTSPECTRUMSELECTIONPRESENTER_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTSPECTRUMSELECTIONPRESENTER_H_

#include "IndirectSpectrumSelectionView.h"

#include "../General/UserInputValidator.h"

#include <boost/variant.hpp>

#include <unordered_map>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

using Spectra =
    const boost::variant<const std::vector<std::size_t> &,
                         const std::pair<std::size_t, std::size_t> &>;

/*
template <typename RangeFunctor>
using RangeResult = typename std::result_of<RangeFunctor(
    const std::pair<std::size_t, std::size_t> &)>::type;

template <typename RangeFunctor, typename VectorFunctor>
class SpectraVisitor : public boost::static_visitor<RangeResult<RangeFunctor>> {
public:
  SpectraVisitor(RangeFunctor &&rangeFunctor, VectorFunctor &&vectorFunctor)
      : m_rangeFunctor(rangeFunctor), m_vectorFunctor(vectorFunctor) {}

  RangeResult<RangeFunctor>
  operator()(const std::pair<std::size_t, std::size_t> &range) const {
    return m_rangeFunctor(range);
  }

  RangeResult<RangeFunctor>
  operator()(const std::vector<std::size_t> &list) const {
    return m_vectorFunctor(list);
  }

private:
  RangeFunctor m_rangeFunctor;
  VectorFunctor m_vectorFunctor;
};
*/

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
  IndirectSpectrumSelectionPresenter(IndirectSpectrumSelectionView *view);
  ~IndirectSpectrumSelectionPresenter() override;

  std::pair<std::size_t, std::size_t> spectraRange() const;
  Spectra selectedSpectra() const;
  const std::unordered_map<std::size_t, std::string> &binMasks() const;

  void setSpectrumRange(std::size_t minimum, std::size_t maximum);

signals:
  void spectraChanged(const std::vector<std::size_t> &);
  void spectraChanged(const std::pair<std::size_t, std::size_t> &);
  void maskSpectrumChanged(std::size_t);
  void invalidSpectraString(const QString &errorMessage);
  void invalidMaskBinsString(const QString &errorMessage);

private slots:
  void setBinMask(std::size_t index, const std::string &maskString);
  void displayBinMask(std::size_t index);
  void updateSpectraList(const std::string &spectraList);
  void updateSpectraRange(std::size_t minimum, std::size_t maximum);

private:
  Spectra selectedSpectra(SpectrumSelectionMode mode) const;
  void setSpectraList(const std::string &spectraList);
  void setSpectraRange(std::size_t minimum, std::size_t maximum);

  UserInputValidator validateSpectraString();
  UserInputValidator validateMaskBinsString();

  std::size_t m_minimumSpectrum;
  std::size_t m_maximumSpectrum;
  std::vector<std::size_t> m_spectraList;
  std::pair<std::size_t, std::size_t> m_spectraRange;
  std::unordered_map<std::size_t, std::string> m_binMasks;
  std::unique_ptr<IndirectSpectrumSelectionView> m_view;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
