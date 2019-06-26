// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_INDIRECTSPECTRUMSELECTIONVIEW_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTSPECTRUMSELECTIONVIEW_H_

#include "ui_IndirectSpectrumSelector.h"
#include "IndexTypes.h"

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

  virtual WorkspaceIndex minimumSpectrum() const;
  virtual WorkspaceIndex maximumSpectrum() const;

  virtual std::string spectraString() const;
  virtual std::string maskString() const;

  virtual void displaySpectra(const std::string &spectraString);
  virtual void displaySpectra(std::pair<WorkspaceIndex, WorkspaceIndex> minmax);

  virtual void setSpectraRange(WorkspaceIndex minimum, WorkspaceIndex maximum);

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
  virtual void setMinimumSpectrum(WorkspaceIndex spectrum);
  virtual void setMaximumSpectrum(WorkspaceIndex spectrum);
  void setMaskSpectrum(WorkspaceIndex spectrum);

  virtual void setSpectraString(const std::string &spectraString);
  virtual void setMaskString(const std::string &maskString);
  void setMaskSpectraList(const std::vector<WorkspaceIndex> &maskSpectra);

  void hideSpectrumSelector();
  void showSpectrumSelector();
  void hideMaskSpectrumSelector();
  void showMaskSpectrumSelector();

  void clearMaskString();

signals:
  void selectedSpectraChanged(const std::string &);
  void selectedSpectraChanged(WorkspaceIndex, WorkspaceIndex);
  void spectraSelectionWidgetChanged(int);
  void maskSpectrumChanged(WorkspaceIndex);
  void maskChanged(const std::string &);

private slots:
  void emitMaskChanged();
  void emitMaskSpectrumChanged(const QString &spectrum);
  void emitMaskSpectrumChanged(int spectrum);
  void emitSpectraChanged(int modeIndex);
  void emitSpectraStringChanged();
  void emitSpectraRangeChanged();
  void setSpectraRangeMiniMax(int value);
  void setSpectraRangeMaxiMin(int value);
  void enableMaskLineEdit(int doEnable);

private:
  void setSpectraRangeMinimum(WorkspaceIndex minimum);
  void setSpectraRangeMaximum(WorkspaceIndex maximum);
  void displaySpectraList();
  QValidator *createValidator(const QString &regex);

  std::unique_ptr<Ui::IndirectSpectrumSelector> m_selector;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
