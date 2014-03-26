#ifndef MANTID_CUSTOMINTERFACES_ALCDATALOADING_H_
#define MANTID_CUSTOMINTERFACES_ALCDATALOADING_H_

#include <QObject>

#include "MantidKernel/System.h"
#include "MantidQtCustomInterfaces/DllConfig.h"

namespace Mantid
{
namespace CustomInterfaces
{

  /**
   * View interface
   */
  class MANTIDQT_CUSTOMINTERFACES_DLL IALCDataLoadingView : public QObject
  {
    Q_OBJECT

  public:
    virtual std::string firstRun() = 0;

  public slots:
    virtual void setData() = 0;

  signals:
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
    IALCDataLoadingView* const m_view;
  };

} // namespace CustomInterfaces
} // namespace Mantid

#endif  /* MANTID_CUSTOMINTERFACES_ALCDATALOADING_H_ */
