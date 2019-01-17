// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_INDIRECTFITOUTPUTOPTIONSPRESENTER_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTFITOUTPUTOPTIONSPRESENTER_H_

#include "IndirectFitOutputOptionsModel.h"
#include "IndirectFitOutputOptionsView.h"

#include "DllConfig.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INDIRECT_DLL IndirectFitOutputOptionsPresenter : public QObject {
  Q_OBJECT
public:
  IndirectFitOutputOptionsPresenter(IndirectFitOutputOptionsModel *model,
                                    IndirectFitOutputOptionsView *view);
  ~IndirectFitOutputOptionsPresenter() override;

private:
  std::unique_ptr<IndirectFitOutputOptionsModel> m_model;
  std::unique_ptr<IndirectFitOutputOptionsView> m_view;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
