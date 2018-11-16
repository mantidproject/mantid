// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_IINDIRECTSPECTRUMSELECTIONVIEW_H_
#define MANTIDQT_IINDIRECTSPECTRUMSELECTIONVIEW_H_

#include "../General/UserInputValidator.h"
#include "DllConfig.h"
#include "MantidKernel/System.h"
#include "MantidQtWidgets/Common/MantidWidget.h"

#include <QObject>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

enum class SpectrumSelectionMode { RANGE, STRING };

class MANTIDQT_INDIRECT_DLL IIndirectSpectrumSelectionView
    : public API::MantidWidget {
  Q_OBJECT

public:
  IIndirectSpectrumSelectionView(QWidget *parent = nullptr)
      : API::MantidWidget(parent){};

  virtual SpectrumSelectionMode selectionMode() const = 0;

  virtual std::size_t minimumSpectrum() const = 0;
  virtual std::size_t maximumSpectrum() const = 0;

  virtual std::string spectraString() const = 0;
  virtual std::string maskString() const = 0;

  virtual void displaySpectra(std::string const &spectraString) = 0;
  virtual void displaySpectra(int minimum, int maximum) = 0;

  virtual void setSpectraRange(int minimum, int maximum) = 0;

  virtual void setSpectraRegex(std::string const &regex) = 0;
  virtual void setMaskBinsRegex(std::string const &regex) = 0;

  virtual UserInputValidator &
  validateSpectraString(UserInputValidator &uiv) const = 0;
  virtual UserInputValidator &
  validateMaskBinsString(UserInputValidator &uiv) const = 0;

  virtual void showSpectraErrorLabel() = 0;
  virtual void showMaskBinErrorLabel() = 0;
  virtual void hideSpectraErrorLabel() = 0;
  virtual void hideMaskBinErrorLabel() = 0;

  virtual void setMaskSelectionEnabled(bool enabled) = 0;
  virtual void clear() = 0;

public slots:
  virtual void setMinimumSpectrum(std::size_t spectrum) = 0;
  virtual void setMaximumSpectrum(std::size_t spectrum) = 0;
  virtual void setMaskSpectrum(std::size_t spectrum) = 0;

  virtual void setSpectraString(const std::string &spectraString) = 0;
  virtual void setMaskString(const std::string &maskString) = 0;
  virtual void
  setMaskSpectraList(const std::vector<std::size_t> &maskSpectra) = 0;

  virtual void hideSpectrumSelector() = 0;
  virtual void showSpectrumSelector() = 0;
  virtual void hideMaskSpectrumSelector() = 0;
  virtual void showMaskSpectrumSelector() = 0;

  virtual void clearMaskString() = 0;

signals:
  void selectedSpectraChanged(const std::string &);
  void selectedSpectraChanged(std::size_t, std::size_t);
  void maskSpectrumChanged(int);
  void maskChanged(const std::string &);
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQT_IINDIRECTSPECTRUMSELECTIONVIEW_H_ */
