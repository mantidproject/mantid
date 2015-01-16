#include "MantidVatesSimpleGuiViewWidgets/RebinManager.h"
#include "MantidVatesSimpleGuiQtWidgets/RebinDialog.h"
#include "MantidVatesAPI/ADSWorkspaceProvider.h"
#include <QStringList>
#include <vector>

#include <pqActiveObjects.h>
#include <pqPipelineSource.h>
#include <vtkSMPropertyHelper.h>

// Have to deal with ParaView warnings and Intel compiler the hard way.
#if defined(__INTEL_COMPILER)
  #pragma warning disable 1170
#endif

namespace Mantid
{
  namespace Vates
  {
    namespace SimpleGui
    {
      RebinManager::RebinManager(QObject* parent) : QObject(parent)
      {
      }

      RebinManager::~RebinManager()
      {
      }

      void RebinManager::sendUpdate()
      {
        // Get the workspace information for the active source
        pqPipelineSource* activeSource = pqActiveObjects::instance().activeSource();

        std::string workspaceName(vtkSMPropertyHelper((activeSource)->getProxy(),
                                                      "WorkspaceName", true).GetAsString());

        std::vector<int> bins;
        bins.push_back(50);
        bins.push_back(50);
        bins.push_back(50);

        std::vector<QString> binNames;
        binNames.push_back(QString("Bin1"));
        binNames.push_back(QString("Bin2"));
        binNames.push_back(QString("Bin3"));

        QStringList list;
        list.append("Alg1");
        list.append("Alg2");

        emit this->udpateDialog(list, binNames, bins);
      }

      void RebinManager::onPerformRebinning(QString algorithm,std::vector<QString> binNames, std::vector<int> bins)
      {
        int a = 1;
      }

      void RebinManager::connectDialog(RebinDialog* rebinDialog)
      {
         // Establish connection between Rebinmanager and RebinDialog
        QObject::connect(this, SIGNAL(udpateDialog(QStringList, std::vector<QString>, std::vector<int>)),
          rebinDialog, SLOT(onUpdateDialog(QStringList,std::vector<QString>,  std::vector<int>)), Qt::UniqueConnection);

        QObject::connect(rebinDialog, SIGNAL(performRebinning(QString, std::vector<QString>,  std::vector<int>)),
                         this, SLOT(onPerformRebinning(QString, std::vector<QString> , std::vector<int>)), Qt::UniqueConnection);
      }
    } // SimpleGui
  } // Vates
} // Mantid
