#include "MantidVatesSimpleGuiViewWidgets/RebinManager.h"
#include "MantidVatesAPI/ADSWorkspaceProvider.h"
#include "MantidAPI/Algorithm.h"
#include "MantidQtAPI/InterfaceManager.h"
#include "MantidQtMantidWidgets/SlicingAlgorithmDialog.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "boost/shared_ptr.hpp"
#include "MantidKernel/Logger.h"

#include <string>
#include <vector>

#include <QString>
#include <QVariant>
#include <QHash>

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

    namespace
    {
      Mantid::Kernel::Logger g_log("RebinManager");
    }

      RebinManager::RebinManager(QWidget* parent) : QWidget(parent), 
                                                    m_binMdVersion(1),
                                                    m_binMdName("BinMD"),
                                                    m_lblInputWorkspace("InputWorkspace"),
                                                    m_lblOutputWorkspace("OutputWorkspace"),
                                                    m_binCutOffValue(50)
      {
      }

      RebinManager::~RebinManager()
      {
      }

      /**
       * Show a Bin MD dialog for rebinning in the VSI
       * @param inputWorkspace The name of the input workspace.
       * @param outputWorkspace The name of the output workspace.
       */
      void RebinManager::showDialog(std::string inputWorkspace, std::string outputWorkspace)
      {
        if (inputWorkspace.empty() || outputWorkspace.empty())
        {
          return;
        }

        // Create the algorithm
        Mantid::API::IAlgorithm_sptr algorithm = createAlgorithm(m_binMdName, m_binMdVersion);

        if (!algorithm)
        {
          return;
        }

        MantidQt::API::AlgorithmDialog* rebinDialog = createDialog(algorithm, inputWorkspace, outputWorkspace);

        rebinDialog->show();
        rebinDialog->raise();
        rebinDialog->activateWindow();
      }

      /**
       * Gets the event workspace
       * @param workspaceName The name of the input workspace.
       * @returns A pointer to the current event workspace
       */
      Mantid::API::IMDEventWorkspace_sptr RebinManager::getWorkspace(std::string workspaceName)
      {
        Mantid::API::IMDEventWorkspace_sptr eventWorkspace;


        if (!m_adsWorkspaceProvider.canProvideWorkspace(workspaceName))
        {
          return eventWorkspace;
        }

        Mantid::API::Workspace_sptr workspace = m_adsWorkspaceProvider.fetchWorkspace(workspaceName);

        // Make sure it is a and MDEvent
        eventWorkspace = boost::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(workspace);

        return eventWorkspace;
      }

      /**
       * Creates an algorithm
       * @param algorithmName The name of the algorithm.
       * @param version The version of the algorithm
       * @returns A pointer to the newly created algorithm.
       */
      Mantid::API::IAlgorithm_sptr RebinManager::createAlgorithm(const QString& algorithmName, int version)
      {
        Mantid::API::IAlgorithm_sptr alg;
        try
        {
          alg = Mantid::API::AlgorithmManager::Instance().create(algorithmName.toStdString(),version);
        }
        catch(...)
        {
          g_log.warning() << "Error: " << algorithmName.toStdString() << " was not created. Version number is " << version;
        }
        return alg;
      }

      /**
       * Creates the dialog for the algorithm (see InterfaceManager).
       * @param algorithm The algorithm which is to be used.
       * @param inputWorkspace The name of the input workspace.
       * @param outputWorkspace The name of the output workspace.
       * @returns The algorithm dialog
       */
      MantidQt::API::AlgorithmDialog* RebinManager::createDialog( Mantid::API::IAlgorithm_sptr algorithm, std::string inputWorkspace, std::string outputWorkspace)
      {
         QHash<QString, QString> presets;
         getPresetsForBinMD(inputWorkspace, outputWorkspace, presets);

        //Check if a workspace is selected in the dock and set this as a preference for the input workspace
        //This is an optional message displayed at the top of the GUI.
        QString optional_msg(algorithm->summary().c_str());


        MantidQt::MantidWidgets::BinMDDialog* dialog = new MantidQt::MantidWidgets::BinMDDialog(this);
                // The parent so that the dialog appears on top of it
        dialog->setParent(this);
        dialog->setAttribute(Qt::WA_DeleteOnClose, true);

        // Set the QDialog window flags to ensure the dialog ends up on top
        Qt::WindowFlags flags = 0;
        flags |= Qt::Dialog;
        flags |= Qt::WindowContextHelpButtonHint;
        dialog->setWindowFlags(flags);

        // Set the content
        dialog->setAlgorithm(algorithm);
        dialog->setPresetValues(presets);
        dialog->setOptionalMessage(QString(algorithm->summary().c_str()));

        // Setup the layout
        dialog->initializeLayout();
        dialog->customiseLayoutForVsi(inputWorkspace);

        // Setup the values of the axis dimensions
        setAxisDimensions(dialog, inputWorkspace);

        return dialog;
      }

      /**
       * Determine the preset values 
       * @param inputWorkspace The name of the input workspace.
       * @param outputWorkspace The name of the output workspace.
       * @param presets A container for the preset values.
       */
      void RebinManager::getPresetsForBinMD(std::string inputWorkspace, std::string outputWorkspace, QHash<QString, QString>& presets)
      {
        // Set the input workspace
        presets.insert(QString(m_lblInputWorkspace),QString::fromStdString(inputWorkspace));

        // Set the output workspace
        presets.insert(QString(m_lblOutputWorkspace),QString::fromStdString(outputWorkspace));
      }

      /**
       * Resets the aligned dimensions properties
       * @param dialog A pointer to the BinMD dialog
       * @param inputWorkspace The name of the input workspace.
       */
      void RebinManager::setAxisDimensions(MantidQt::MantidWidgets::BinMDDialog* dialog, std::string inputWorkspace)
      {
        Mantid::API::IMDEventWorkspace_sptr  eventWorkspace = getWorkspace(inputWorkspace);

        size_t nDimensions = eventWorkspace->getNumDims();

        for (size_t index = 0; index < nDimensions; ++index)
        {
          Mantid::Geometry::IMDDimension_const_sptr dim = eventWorkspace->getDimension(index);

          std::string name = dim->getName();
          std::string dimensionId = dim->getDimensionId();
          coord_t minimum = dim->getMinimum();
          coord_t maximum = dim->getMaximum();
          size_t numberOfBins = dim->getNBins();

          // Check the bins size
          QString newNumberOfBins;
          if (numberOfBins < m_binCutOffValue)
          {
            newNumberOfBins = QString::number(static_cast<unsigned long long>(m_binCutOffValue));
          }
          else
          {
            newNumberOfBins = QString::number(static_cast<unsigned long long>(numberOfBins));
          }

          // If Id exists use it as the identifier otherwise use the name 
          std::string identifier;
          if (dimensionId.empty())
          {
            identifier = name;
          }
          else
          {
            identifier = dimensionId;
          }

          // Check here if the set bins are OK
          QString propertyValue = QString::fromStdString(identifier) + ","
                                + QString::number(static_cast<float>(minimum)) + ","
                                + QString::number(static_cast<float>(maximum)) + ","
                                + newNumberOfBins;

          dialog->resestAlignedDimProperty(index, propertyValue);
        }
      }
    }
  }
}
