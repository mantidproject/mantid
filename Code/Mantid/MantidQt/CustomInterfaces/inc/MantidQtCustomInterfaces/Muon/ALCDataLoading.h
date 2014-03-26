#ifndef MANTID_CUSTOMINTERFACES_ALCDATALOADING_H_
#define MANTID_CUSTOMINTERFACES_ALCDATALOADING_H_

#include <QObject>

#include "MantidKernel/System.h"
#include "MantidAPI/MatrixWorkspace.h"

#include "MantidQtCustomInterfaces/DllConfig.h"

namespace MantidQt
{
namespace CustomInterfaces
{
  using namespace Mantid::API;

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

  public slots:
    /// Updates the data displayed by the view
    virtual void setData(MatrixWorkspace_const_sptr data) = 0;

  signals:
    /// Request to load data
    void loadData();
  };

  /**
   * Presenter
   */
  class MANTIDQT_CUSTOMINTERFACES_DLL ALCDataLoading : public QObject
  {
    Q_OBJECT

  public:
    ALCDataLoading(IALCDataLoadingView* view);
    virtual ~ALCDataLoading();

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

} // namespace CustomInterfaces
} // namespace Mantid

#endif  /* MANTID_CUSTOMINTERFACES_ALCDATALOADING_H_ */
