// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/InstrumentView/BaseCustomInstrumentModel.h"
#include "MantidQtWidgets/InstrumentView/BaseCustomInstrumentPresenter.h"

#include "ALFCustomInstrumentView.h"
#include "DllConfig.h"
#include "MantidQtWidgets/Common/ObserverPattern.h"

#include <string>

namespace MantidQt {

namespace CustomInterfaces {

class MANTIDQT_DIRECT_DLL ALFCustomInstrumentPresenter : public MantidWidgets::BaseCustomInstrumentPresenter {
  Q_OBJECT

public:
  ALFCustomInstrumentPresenter(IALFCustomInstrumentView *view, MantidWidgets::IBaseCustomInstrumentModel *model);
  ~ALFCustomInstrumentPresenter() {
    delete m_extractSingleTubeObserver;
    delete m_averageTubeObserver;
  };

  void addInstrument() override;

  std::pair<instrumentSetUp, instrumentObserverOptions> setupALFInstrument();

  void averageTube();

private:
  IALFCustomInstrumentView *m_view;
  MantidWidgets::IBaseCustomInstrumentModel *m_model;
  VoidObserver *m_extractSingleTubeObserver;
  VoidObserver *m_averageTubeObserver;
};
} // namespace CustomInterfaces
} // namespace MantidQt
