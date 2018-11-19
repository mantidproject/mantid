// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_INDIRECTSPECTRUMSELECTIONVIEW_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTSPECTRUMSELECTIONVIEW_H_

#include "ui_IndirectSpectrumSelector.h"

#include "../General/UserInputValidator.h"
#include "DllConfig.h"
#include "IIndirectSpectrumSelectionView.h"
#include "MantidKernel/System.h"
#include "MantidQtWidgets/Common/MantidWidget.h"

#include <cstddef>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INDIRECT_DLL IndirectSpectrumSelectionView
    : public API::MantidWidget, public IIndirectSpectrumSelectionView {
  Q_OBJECT

public:
  IndirectSpectrumSelectionView(QWidget *parent = nullptr);
  ~IndirectSpectrumSelectionView() override;

  SpectrumSelectionMode selectionMode() const override;

  std::size_t minimumSpectrum() const override;
  std::size_t maximumSpectrum() const override;

  std::string spectraString() const override;
  std::string maskString() const override;

  void displaySpectra(const std::string &spectraString) override;
  void displaySpectra(int minimum, int maximum) override;

  void setSpectraRange(int minimum, int maximum) override;

  void setSpectraRegex(const std::string &regex) override;
  void setMaskBinsRegex(const std::string &regex) override;

  UserInputValidator &
  validateSpectraString(UserInputValidator &uiv) const;
  UserInputValidator &
  validateMaskBinsString(UserInputValidator &uiv) const;

  void showSpectraErrorLabel() override;
  void showMaskBinErrorLabel() override;
  void hideSpectraErrorLabel() override;
  void hideMaskBinErrorLabel() override;

  void setMaskSelectionEnabled(bool enabled) override;
  void clear() override;

public slots:
  void setMinimumSpectrum(std::size_t spectrum) override;
  void setMaximumSpectrum(std::size_t spectrum) override;
  void setMaskSpectrum(std::size_t spectrum) override;

  void setSpectraString(const std::string &spectraString) override;
  void setMaskString(const std::string &maskString) override;
  void setMaskSpectraList(const std::vector<std::size_t> &maskSpectra) override;

  void hideSpectrumSelector() override;
  void showSpectrumSelector() override;
  void hideMaskSpectrumSelector() override;
  void showMaskSpectrumSelector() override;

  void clearMaskString() override;

signals:
  void selectedSpectraChanged(const std::string &) override;
  void selectedSpectraChanged(std::size_t, std::size_t) override;
  void maskSpectrumChanged(int) override;
  void maskChanged(const std::string &) override;

private slots:
  void emitMaskChanged();
  void emitMaskSpectrumChanged(const QString &spectrum);
  void emitSpectraChanged(int modeIndex);
  void emitSpectraStringChanged();
  void emitSpectraRangeChanged();
  void setSpectraRangeMiniMax(int value);
  void setSpectraRangeMaxiMin(int value);
  void enableMaskLineEdit(int doEnable);

private:
  void setSpectraRangeMinimum(int minimum);
  void setSpectraRangeMaximum(int maximum);
  void displaySpectraList();
  QValidator *createValidator(const QString &regex);

  std::unique_ptr<Ui::IndirectSpectrumSelector> m_selector;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
