// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_INDIRECTSPECTRUMSELECTIONPRESENTERLEGACY_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTSPECTRUMSELECTIONPRESENTERLEGACY_H_

#include "IndirectFittingModelLegacy.h"
#include "IndirectSpectrumSelectionViewLegacy.h"

#include "DllConfig.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include <unordered_map>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INDIRECT_DLL IndirectSpectrumSelectionPresenterLegacy
    : public QObject {
  Q_OBJECT
public:
  IndirectSpectrumSelectionPresenterLegacy(
      IndirectFittingModelLegacy *model,
      IndirectSpectrumSelectionViewLegacy *view);
  ~IndirectSpectrumSelectionPresenterLegacy() override;
  UserInputValidator &validate(UserInputValidator &validator);

signals:
  void spectraChanged(std::size_t /*_t1*/);
  void maskChanged(std::string const & /*_t1*/);
  void invalidSpectraString(QString const &errorMessage);
  void invalidMaskBinsString(QString const &errorMessage);

public slots:
  void setActiveModelIndex(std::size_t index);
  void setActiveIndexToZero();
  void updateSpectra();
  void displayBinMask();
  void disableView();
  void enableView();

private slots:
  void setBinMask(std::string const &maskString);
  void setMaskSpectraList(std::string const &spectraList);
  void updateSpectraList(std::string const &spectraList);
  void updateSpectraRange(std::size_t minimum, std::size_t maximum);
  void displaySpectraList(std::string const &spectra);
  void setMaskIndex(int index);

private:
  void setSpectraRange(std::size_t minimum, std::size_t maximum);
  void setModelSpectra(SpectraLegacy const &spectra);

  UserInputValidator validateSpectraString();
  UserInputValidator &validateSpectraString(UserInputValidator &validator);
  UserInputValidator validateMaskBinsString();

  IndirectFittingModelLegacy *m_model;
  std::unique_ptr<IndirectSpectrumSelectionViewLegacy> m_view;
  std::size_t m_activeIndex;
  std::size_t m_maskIndex;
  std::string m_spectraError;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
