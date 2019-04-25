// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_INDIRECTSPECTRUMSELECTIONVIEW_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTSPECTRUMSELECTIONVIEW_H_

#include "ui_IndirectSpectrumSelector.h"

#include "DllConfig.h"
#include "MantidKernel/System.h"
#include "MantidQtWidgets/Common/MantidWidget.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include <cstddef>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

enum class SpectrumSelectionMode { RANGE, STRING };

class MANTIDQT_INDIRECT_DLL IndirectSpectrumSelectionView
    : public API::MantidWidget {
  Q_OBJECT

public:
  IndirectSpectrumSelectionView(QWidget *parent = nullptr);
  virtual ~IndirectSpectrumSelectionView() override;

  SpectrumSelectionMode selectionMode() const;

  virtual std::size_t minimumSpectrum() const;
  virtual std::size_t maximumSpectrum() const;

  virtual std::string spectraString() const;
  virtual std::string maskString() const;

  virtual void displaySpectra(const std::string &spectraString);
  virtual void displaySpectra(int minimum, int maximum);

  virtual void setSpectraRange(int minimum, int maximum);

  void setSpectraRegex(const std::string &regex);
  void setMaskBinsRegex(const std::string &regex);

  UserInputValidator &validateSpectraString(UserInputValidator &uiv) const;
  UserInputValidator &validateMaskBinsString(UserInputValidator &uiv) const;

  virtual void showSpectraErrorLabel();
  void showMaskBinErrorLabel();
  virtual void hideSpectraErrorLabel();
  void hideMaskBinErrorLabel();

  virtual void setMaskSelectionEnabled(bool enabled);
  virtual void clear();

public slots:
  virtual void setMinimumSpectrum(std::size_t spectrum);
  virtual void setMaximumSpectrum(std::size_t spectrum);
  void setMaskSpectrum(std::size_t spectrum);

  virtual void setSpectraString(const std::string &spectraString);
  virtual void setMaskString(const std::string &maskString);
  void setMaskSpectraList(const std::vector<std::size_t> &maskSpectra);

  void hideSpectrumSelector();
  void showSpectrumSelector();
  void hideMaskSpectrumSelector();
  void showMaskSpectrumSelector();

  void clearMaskString();

signals:
  void selectedSpectraChanged(const std::string & /*_t1*/);
  void selectedSpectraChanged(std::size_t /*_t1*/, std::size_t /*_t2*/);
  void maskSpectrumChanged(int /*_t1*/);
  void maskChanged(const std::string & /*_t1*/);

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
