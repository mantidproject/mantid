// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETMODELMOCK_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETMODELMOCK_H_

#include "../EnggDiffraction/IEnggDiffMultiRunFittingWidgetModel.h"
#include "MantidKernel/WarningSuppressions.h"

#include <gmock/gmock.h>

using namespace MantidQt::CustomInterfaces;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockEnggDiffMultiRunFittingWidgetModel
    : public IEnggDiffMultiRunFittingWidgetModel {

public:
  MOCK_METHOD2(addFittedPeaks,
               void(const RunLabel &runLabel,
                    const Mantid::API::MatrixWorkspace_sptr ws));

  MOCK_METHOD2(addFocusedRun, void(const RunLabel &runLabel,
                                   const Mantid::API::MatrixWorkspace_sptr ws));

  MOCK_CONST_METHOD0(getAllWorkspaceLabels, std::vector<RunLabel>());

  MOCK_CONST_METHOD1(getFittedPeaks,
                     boost::optional<Mantid::API::MatrixWorkspace_sptr>(
                         const RunLabel &runLabel));

  MOCK_CONST_METHOD1(getFocusedRun,
                     boost::optional<Mantid::API::MatrixWorkspace_sptr>(
                         const RunLabel &runLabel));

  MOCK_CONST_METHOD1(hasFittedPeaksForRun, bool(const RunLabel &runLabel));

  MOCK_METHOD1(removeRun, void(const RunLabel &runLabel));
};

GNU_DIAG_ON_SUGGEST_OVERRIDE

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFMULTIRUNFITTINGWIDGETMODELMOCK_H_
