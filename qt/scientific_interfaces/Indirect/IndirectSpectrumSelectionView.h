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

#include "MantidKernel/System.h"
#include "MantidQtWidgets/Common/MantidWidget.h"

#include <cstddef>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

enum class SpectrumSelectionMode { RANGE, STRING };

/** IndirectSpectrumSelectionView
 */
class DLLExport IndirectSpectrumSelectionView : public API::MantidWidget {
  Q_OBJECT
public:
  IndirectSpectrumSelectionView(QWidget *parent = nullptr);
  ~IndirectSpectrumSelectionView() override;

  SpectrumSelectionMode selectionMode() const;

  std::size_t minimumSpectrum() const;
  std::size_t maximumSpectrum() const;

  std::string spectraString() const;
  std::string maskString() const;

  void displaySpectra(const std::string &spectraString);
  void displaySpectra(int minimum, int maximum);

  void setSpectraRange(int minimum, int maximum);

  void setSpectraRegex(const std::string &regex);
  void setMaskBinsRegex(const std::string &regex);

  UserInputValidator &validateSpectraString(UserInputValidator &uiv) const;
  UserInputValidator &validateMaskBinsString(UserInputValidator &uiv) const;

  void showSpectraErrorLabel();
  void showMaskBinErrorLabel();
  void hideSpectraErrorLabel();
  void hideMaskBinErrorLabel();

  void setMaskSelectionEnabled(bool enabled);
  void clear();

public slots:
  void setMinimumSpectrum(std::size_t spectrum);
  void setMaximumSpectrum(std::size_t spectrum);
  void setMaskSpectrum(std::size_t spectrum);

  void setSpectraString(const std::string &spectraString);
  void setMaskString(const std::string &maskString);
  void setMaskSpectraList(const std::vector<std::size_t> &maskSpectra);

  void hideSpectrumSelector();
  void showSpectrumSelector();
  void hideMaskSpectrumSelector();
  void showMaskSpectrumSelector();

  void clearMaskString();

signals:
  void selectedSpectraChanged(const std::string &);
  void selectedSpectraChanged(std::size_t, std::size_t);
  void maskSpectrumChanged(int);
  void maskChanged(const std::string &);

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
  QValidator *createValidator(const QString &regex);

  std::unique_ptr<Ui::IndirectSpectrumSelector> m_selector;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
