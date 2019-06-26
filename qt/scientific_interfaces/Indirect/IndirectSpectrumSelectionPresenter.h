// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_INDIRECTSPECTRUMSELECTIONPRESENTER_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTSPECTRUMSELECTIONPRESENTER_H_

#include "IndirectFittingModel.h"
#include "IndirectSpectrumSelectionView.h"

#include "DllConfig.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include <unordered_map>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INDIRECT_DLL IndirectSpectrumSelectionPresenter
    : public QObject {
  Q_OBJECT
public:
  IndirectSpectrumSelectionPresenter(IndirectFittingModel *model,
                                     IndirectSpectrumSelectionView *view);
  ~IndirectSpectrumSelectionPresenter() override;
  UserInputValidator &validate(UserInputValidator &validator);

signals:
  void spectraChanged(DatasetIndex /*_t1*/);
  void maskChanged(std::string const & /*_t1*/);
  void invalidSpectraString(QString const &errorMessage);
  void invalidMaskBinsString(QString const &errorMessage);

public slots:
  void setActiveModelIndex(DatasetIndex index);
  void setActiveIndexToZero();
  void updateSpectra();
  void displayBinMask();
  void disableView();
  void enableView();

private slots:
  void setBinMask(std::string const &maskString);
  void setMaskSpectraList(std::string const &spectraList);
  void updateSpectraList(std::string const &spectraList);
  void updateSpectraRange(WorkspaceIndex minimum, WorkspaceIndex maximum);
  void displaySpectraList(std::string const &spectra);
  void setMaskIndex(WorkspaceIndex index);

private:
  void setSpectraRange(WorkspaceIndex minimum, WorkspaceIndex maximum);
  void setModelSpectra(Spectra const &spectra);

  UserInputValidator validateSpectraString();
  UserInputValidator &validateSpectraString(UserInputValidator &validator);
  UserInputValidator validateMaskBinsString();

  IndirectFittingModel *m_model;
  std::unique_ptr<IndirectSpectrumSelectionView> m_view;
  DatasetIndex m_activeIndex;
  WorkspaceIndex m_maskIndex;
  std::string m_spectraError;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
