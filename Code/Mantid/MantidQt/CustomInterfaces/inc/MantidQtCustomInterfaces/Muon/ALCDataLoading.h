#ifndef MANTID_CUSTOMINTERFACES_ALCDATALOADING_H_
#define MANTID_CUSTOMINTERFACES_ALCDATALOADING_H_

#include <QObject>

#include "MantidKernel/System.h"
#include "MantidAPI/MatrixWorkspace.h"

#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtAPI/UserSubWindow.h"

#include "ui_ALCDataLoading.h"

using namespace Mantid::API;

namespace MantidQt
{
  using namespace API;

namespace CustomInterfaces
{

  /**
   * View interface
   */
  class MANTIDQT_CUSTOMINTERFACES_DLL IALCDataLoadingView : public QObject
  {
    Q_OBJECT

  public:
    /// Returns a path to the first run file
    virtual std::string firstRun() = 0;
    /// Returns a path to the last run file
    virtual std::string lastRun() = 0;
    /// Returns the name of the log to use
    virtual std::string log() = 0;

  public slots:
    /// Updates the data displayed by the view
    virtual void displayData(MatrixWorkspace_const_sptr data) = 0;

  signals:
    /// Request to load data
    void loadData();
  };

  /**
   * Presenter
   */
  class MANTIDQT_CUSTOMINTERFACES_DLL ALCDataLoadingPresenter : public QObject
  {
    Q_OBJECT

  public:
    ALCDataLoadingPresenter(IALCDataLoadingView* view);
    virtual ~ALCDataLoadingPresenter();

    void initialize();

  private:
    /// Begin listening to the view signals
    void connectView();

    /// View which the object works with
    IALCDataLoadingView* const m_view;

  private slots:
    /// Load new data and update the view accordingly
    void loadData();
  };

  /**
   * Widget ALC Data Loading view implementation
   */
  class MANTIDQT_CUSTOMINTERFACES_DLL ALCDataLoadingView : public IALCDataLoadingView
  {
  public:
    ALCDataLoadingView(QWidget* widget);

    std::string firstRun();
    std::string lastRun();
    std::string log();

    void displayData(MatrixWorkspace_const_sptr data);

  private:
    ALCDataLoadingPresenter m_dataLoading;
    Ui::ALCDataLoading m_ui;
  };

  class MANTIDQT_CUSTOMINTERFACES_DLL ALCInterface : public UserSubWindow
  {
  public:
    static std::string name() { return "ALC Data Loading"; }
    static QString categoryInfo() { return "Muon"; }
  protected:
    void initLayout();
  };

} // namespace CustomInterfaces
} // namespace Mantid

#endif  /* MANTID_CUSTOMINTERFACES_ALCDATALOADING_H_ */
