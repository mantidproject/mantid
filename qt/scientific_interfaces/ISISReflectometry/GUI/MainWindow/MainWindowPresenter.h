// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_MAINWINDOWPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_MAINWINDOWPRESENTER_H

#include "Common/DllConfig.h"
#include "GUI/Batch/IBatchPresenter.h"
#include "GUI/Options/IOptionsDialogPresenter.h"
#include "IMainWindowPresenter.h"
#include "IMainWindowView.h"
#include "MantidGeometry/Instrument.h"
#include <memory>

namespace MantidQt {
namespace MantidWidgets {
class ISlitCalculator;
}
namespace CustomInterfaces {
namespace ISISReflectometry {

class IBatchPresenterFactory;
class IMainWindowView;
class IFileHandler;
class IMessageHandler;
class IOptionsDialogView;
class IEncoder;
class IDecoder;

/** @class MainWindowPresenter

MainWindowPresenter is the concrete main window presenter implementing the
functionality defined by the interface IMainWindowPresenter.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL MainWindowPresenter
    : public MainWindowSubscriber,
      public IMainWindowPresenter,
      public OptionsDialogPresenterSubscriber {
public:
  /// Constructor
  MainWindowPresenter(
      IMainWindowView *view, IMessageHandler *messageHandler,
      IFileHandler *fileHandler, std::unique_ptr<IEncoder> encoder,
      std::unique_ptr<IDecoder> decoder,
      std::unique_ptr<MantidWidgets::ISlitCalculator> slitCalculator,
      std::unique_ptr<IOptionsDialogPresenter> optionsDialogPresenter,
      std::unique_ptr<IBatchPresenterFactory> batchPresenterFactory);
  ~MainWindowPresenter();
  MainWindowPresenter(MainWindowPresenter const &) = delete;
  MainWindowPresenter(MainWindowPresenter &&);
  MainWindowPresenter &operator=(MainWindowPresenter const &) = delete;
  MainWindowPresenter &operator=(MainWindowPresenter &&);

  // IMainWindowPresenter overrides
  bool isAnyBatchProcessing() const override;
  bool isAnyBatchAutoreducing() const override;
  bool isCloseEventPrevented() override;
  bool isOverwriteBatchPrevented(int tabIndex) const override;
  void notifyAnyBatchAutoreductionResumed() override;
  void notifyAnyBatchAutoreductionPaused() override;
  void notifyAnyBatchReductionResumed() override;
  void notifyAnyBatchReductionPaused() override;
  void
  notifyChangeInstrumentRequested(std::string const &instrumentName) override;
  void notifyUpdateInstrumentRequested() override;
  Mantid::Geometry::Instrument_const_sptr instrument() const override;
  std::string instrumentName() const override;

  // MainWindowSubscriber overrides
  void notifyHelpPressed() override;
  void notifyNewBatchRequested() override;
  void notifyCloseBatchRequested(int batchIndex) override;
  void notifySaveBatchRequested(int batchIndex) override;
  void notifyLoadBatchRequested(int batchIndex) override;
  void notifyShowOptionsRequested() override;
  void notifyShowSlitCalculatorRequested() override;

  // OptionsDialogPresenterSubscriber overrides
  void notifyOptionsChanged() const override;

protected:
  IMainWindowView *m_view;
  IMessageHandler *m_messageHandler;
  IFileHandler *m_fileHandler;
  std::vector<std::unique_ptr<IBatchPresenter>> m_batchPresenters;
  Mantid::Geometry::Instrument_const_sptr m_instrument;

private:
  std::unique_ptr<IEncoder> m_encoder;
  std::unique_ptr<IDecoder> m_decoder;
  std::unique_ptr<MantidWidgets::ISlitCalculator> m_slitCalculator;
  std::unique_ptr<IOptionsDialogPresenter> m_optionsDialogPresenter;
  std::unique_ptr<IBatchPresenterFactory> m_batchPresenterFactory;
  bool m_isUnsaved;

  bool isWarnDiscardChangesChecked() const override;
  bool isCloseBatchPrevented(int batchIndex) const override;
  bool isBatchUnsaved(int batchIndex) const override;
  bool isAnyBatchUnsaved() override;
  void showHelp();
  void addNewBatch(IBatchView *batchView);
  void initNewBatch(IBatchPresenter *batchPresenter,
                    std::string const &instrument);
  void changeInstrument(std::string const &instrumentName);
  void updateInstrument(const std::string &instrumentName);
  void setDefaultInstrument(const std::string &newInstrument);

  void disableSaveAndLoadBatch();
  void enableSaveAndLoadBatch();

  friend class Encoder;
  friend class Decoder;
  friend class CoderCommonTester;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_MAINWINDOWPRESENTER_H */
