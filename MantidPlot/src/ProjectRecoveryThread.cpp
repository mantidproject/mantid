#include "ProjectRecoveryThread.h"

#include "ApplicationWindow.h"
#include "globals.h"
#include "Folder.h"
#include "ProjectSerialiser.h"

#include "MantidAPI/FileProperty.h"

#include <chrono>
#include <string>
#include <thread>

namespace {
	const std::chrono::seconds TIME_BETWEEN_SAVING = std::chrono::seconds(30);

	std::string getOutputPath() {
		std::string homePath = Mantid::API::FileProperty::getHomePath();
		std::string filename = "test";
		std::string fullBasePath = homePath + '/' + homePath;
		return fullBasePath;
	}
}

namespace MantidQt {
namespace API {

ProjectRecoveryThread::ProjectRecoveryThread(ApplicationWindow *windowHandle)
    : m_windowPtr(windowHandle), m_backgroundSavingThread(), m_runProjectSaving(true) {
	startProjectSaving();
}

ProjectRecoveryThread::~ProjectRecoveryThread() {
	stopProjectSaving();
}

std::thread ProjectRecoveryThread::createBackgroundThread() {
	return std::thread([this] {projectSavingThread(m_runProjectSaving); });
}

void ProjectRecoveryThread::startProjectSaving() {
	m_runProjectSaving = true;

	// Close the existing thread first
	if (m_backgroundSavingThread.joinable()) {
		m_backgroundSavingThread.join();
	}

	// Attempt to spin up a new thread
	m_backgroundSavingThread = createBackgroundThread();
}

void ProjectRecoveryThread::stopProjectSaving() {
	m_runProjectSaving = false;

	if (m_backgroundSavingThread.joinable()) {
		m_backgroundSavingThread.join();
	}
}



void ProjectRecoveryThread::projectSavingThread(bool &runThread) {
	while (runThread) {
		// Generate output paths
		const auto basePath = getOutputPath();
		const std::string historyDest = basePath + ".history";
		const std::string projectDest = basePath + ".project";

		// Trigger main saving routines for the ADS and GUI
		saveWsHistories(historyDest);
		//saveOpenWindows(projectDest);
		std::this_thread::sleep_for(TIME_BETWEEN_SAVING);
	}
}

void ProjectRecoveryThread::saveOpenWindows(std::string projectFilepath) {
  const bool isRecovery = true;
  ProjectSerialiser projectWriter(m_windowPtr, isRecovery);
  projectWriter.save(QString::fromStdString(projectFilepath));
}

void ProjectRecoveryThread::saveWsHistories(std::string historyFilePath) {
	// TODO
	int foo = 2;
}

void ProjectRecoveryThread::loadOpenWindows(std::string projectFilePath) {
  const bool isRecovery = true;
  ProjectSerialiser projectWriter(m_windowPtr, isRecovery);

  // Use this version of Mantid as the current version field - as recovery
  // across major versions is not an intended use case
  const int fileVersion = 100 * maj_version + 10 * min_version + patch_version;

  projectWriter.load(projectFilePath, fileVersion);
}

} // namespace API
} // namespace MantidQt