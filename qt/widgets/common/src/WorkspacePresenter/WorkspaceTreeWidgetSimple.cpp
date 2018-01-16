#include "MantidQtWidgets/Common/WorkspacePresenter/WorkspaceTreeWidgetSimple.h"
#include <MantidQtWidgets/Common/MantidTreeWidget.h>
#include <MantidQtWidgets/Common/MantidTreeWidgetItem.h>

#include <MantidAPI/AlgorithmManager.h>
#include <MantidAPI/FileProperty.h>
#include <MantidAPI/MatrixWorkspace.h>
#include <MantidAPI/WorkspaceGroup.h>
#include <MantidAPI/ITableWorkspace.h>

#include <QMenu>
#include <QSignalMapper>

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace MantidQt {
	namespace MantidWidgets {

		WorkspaceTreeWidgetSimple::WorkspaceTreeWidgetSimple(MantidDisplayBase *mdb,
			QWidget *parent)
			: WorkspaceTreeWidget(mdb, parent){}

		WorkspaceTreeWidgetSimple::~WorkspaceTreeWidgetSimple() {};

		void WorkspaceTreeWidgetSimple::popupContextMenu() {
			QTreeWidgetItem *treeItem = m_tree->itemAt(m_menuPosition);
			selectedWsName = "";
			if (treeItem)
				selectedWsName = treeItem->text(0);
			else
				m_tree->selectionModel()->clear();

			QMenu *menu(nullptr);

			// If no workspace is here then have load raw and dae
			if (selectedWsName.isEmpty())
				menu = m_loadMenu;
			else { // else show instrument, sample logs and delete
				   // Fresh menu
				menu = new QMenu(this);
				menu->setObjectName("WorkspaceContextMenu");
				auto mantidTreeItem = dynamic_cast<MantidTreeWidgetItem *>(treeItem);
				auto ws = mantidTreeItem->data(0, Qt::UserRole)
					.value<Mantid::API::Workspace_sptr>();

				// Add the items that are appropriate for the type
				if (auto matrixWS =
					boost::dynamic_pointer_cast<const Mantid::API::MatrixWorkspace>(
						ws)) {
					addMatrixWorkspaceMenuItems(menu, matrixWS);
				}
				else if (auto groupWS =
					boost::dynamic_pointer_cast<const WorkspaceGroup>(ws)) {
					addWorkspaceGroupMenuItems(menu);
				}
				else if (boost::dynamic_pointer_cast<const Mantid::API::ITableWorkspace>(
					ws)) {
					addTableWorkspaceMenuItems(menu);
				}
				else {
					// None of the above? -> not a workspace
					return;
				}

				// Get the names of the programs for the send to option
				std::vector<std::string> programNames =
					(Mantid::Kernel::ConfigService::Instance().getKeys(
						"workspace.sendto.name"));
				bool firstPass(true);
				// Check to see if any options aren't visible
				for (auto &programName : programNames) {
					std::string visible = Mantid::Kernel::ConfigService::Instance().getString(
						"workspace.sendto." + programName + ".visible");
					std::string target = Mantid::Kernel::ConfigService::Instance().getString(
						"workspace.sendto." + programName + ".target");
					if (Mantid::Kernel::ConfigService::Instance().isExecutable(target) &&
						visible == "Yes") {
						bool compatible(true);
						std::string saveUsing(
							Mantid::Kernel::ConfigService::Instance().getString(
								"workspace.sendto." + programName + ".saveusing"));
						try {
							Mantid::API::IAlgorithm_sptr alg =
								Mantid::API::AlgorithmManager::Instance().create(saveUsing);
							alg->setPropertyValue("InputWorkspace", selectedWsName.toStdString());
						}
						catch (std::exception &) {
							compatible = false;
						}
						if (compatible) {
							if (firstPass) {
								m_saveToProgram = new QMenu(tr("Send to"), this);
								menu->addMenu(m_saveToProgram);

								// Sub-menu for program list
								m_programMapper = new QSignalMapper(this);
							}
							QString name = QString::fromStdString(programName);
							// Setup new menu option for the program
							m_program = new QAction(name, this);
							connect(m_program, SIGNAL(triggered()), m_programMapper, SLOT(map()));
							// Send name of program when clicked
							m_programMapper->setMapping(m_program, name);
							m_saveToProgram->addAction(m_program);

							// Set first pass to false so that it doesn't set up another menu
							// entry for all programs.
							firstPass = false;
						}
					}
				}

				// Tell the button what to listen for and what to do once clicked (if there
				// is anything to connect it will be set to false)
				if (!firstPass)
					connect(m_programMapper, SIGNAL(mapped(const QString &)), this,
						SLOT(onClickSaveToProgram(const QString &)));

				// Rename is valid for all workspace types
				menu->addAction(m_rename);
				// separate delete
				menu->addSeparator();
				menu->addAction(m_delete);
			}

			// Show the menu at the cursor's current position
			menu->popup(QCursor::pos());
		}

		/**
		* Add the actions that are appropriate for a MatrixWorkspace
		* @param menu :: The menu to store the items
		* @param matrixWS :: The workspace related to the menu
		*/
		void WorkspaceTreeWidgetSimple::addMatrixWorkspaceMenuItems(
			QMenu *menu,
			const Mantid::API::MatrixWorkspace_const_sptr &matrixWS) const {
			menu->addAction(m_saveNexus);
		}

		/**
		* Add the actions that are appropriate for a WorkspaceGroup
		* @param menu :: The menu to store the items
		*/
		void WorkspaceTreeWidgetSimple::addWorkspaceGroupMenuItems(QMenu *menu) const {
			menu->addAction(m_saveNexus);
		}

		/**
		* Add the actions that are appropriate for a MatrixWorkspace
		* @param menu :: The menu to store the items
		*/
		void WorkspaceTreeWidgetSimple::addTableWorkspaceMenuItems(QMenu *menu) const {
			menu->addAction(m_saveNexus);
		}

	} // namespace MantidWidgets
} // namespace MantidQt
