#ifndef MANTIDQTCUSTOMINTERFACES_CREATEMDWORKSPACE_H_
#define MANTIDQTCUSTOMINTERFACES_CREATEMDWORKSPACE_H_

//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/QtWorkspaceMementoModel.h"
#include "ui_CreateMDWorkspace.h"
#include "MantidQtAPI/UserSubWindow.h"

#include <Poco/NObserver.h>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include "MantidKernel/ConfigService.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {
    //Forward declaration
    class CreateMDWorkspace : public MantidQt::API::UserSubWindow
    {
      Q_OBJECT

    public:
      /// The name of the interface as registered into the factory
      static std::string name() { return "Create MD Workspace"; }
      /// Default Constructor
      CreateMDWorkspace(QWidget *parent = 0);
      /// Destructor
      ~CreateMDWorkspace();

    private:
      /// Initialize the layout
      virtual void initLayout();
      /// init python-dependent sections
      virtual void initLocalPython();
      /// Run a confirmation dialog.
      int runConfirmation(const std::string& message);

    private slots:

    private:
      Ui::CreateMDWorkspace  m_uiForm;
    };
  }
}
#endif /* MANTIDQTCUSTOMINTERFACES_INDIRECTANALYSIS_H_ */
