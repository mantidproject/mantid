#include "MantidQtMantidWidgets/TrackedAction.h"
#include "MantidKernel/UsageService.h"
#include <QCoreApplication>

namespace MantidQt {
namespace MantidWidgets {

/** Constructor
*   @param parent The parent of this action
**/
TrackedAction::TrackedAction(QObject *parent)
    : QAction(parent), m_isTracking(true), m_trackingName() {
  setupTracking();
}

/** Constructor
*   @param text The text for the action
*   @param parent The parent of this action
**/
TrackedAction::TrackedAction(const QString &text, QObject *parent)
    : QAction(text, parent), m_isTracking(true), m_trackingName() {
  setupTracking();
}

/** Constructor
*   @param icon The icon for the action
*   @param text The text for the action
*   @param parent The parent of this action
**/
TrackedAction::TrackedAction(const QIcon &icon, const QString &text,
                             QObject *parent)
    : QAction(icon, text, parent), m_isTracking(true), m_trackingName() {
  setupTracking();
}

/** Sets the tracking name for this action
*   @param name the tracking name for this action
**/
void TrackedAction::setTrackingName(const std::string &name) {
  m_trackingName = name;
}

/** Gets the tracking name for this action
*   If the tacking name is not set a default name will be generated using
*   generateTrackingName
*   @returns The tracking name for this action
**/
std::string TrackedAction::getTrackingName() const {
  if (m_trackingName.empty()) {
    m_trackingName = generateTrackingName();
  }
  return m_trackingName;
}

/** Sets whether this action is tracking usage
*   @param enableTracking True if the action should tracking usage
**/
void TrackedAction::setIsTracking(const bool enableTracking) {
  m_isTracking = enableTracking;
}

/** Gets whether this action is tracking usage
*   @returns True if the action is tracking usage
**/
bool TrackedAction::getIsTracking() const { return m_isTracking; }

/** Sets up tracking for the class
**/
void TrackedAction::setupTracking() {
  connect(this, SIGNAL(triggered(bool)), this, SLOT(trackActivation(bool)));
}

/** Creates a tracking name from the action text
*   @returns A generated name using ApplicationName->ActionText
**/
std::string TrackedAction::generateTrackingName() const {
  return QCoreApplication::applicationName().toStdString() + "->" +
         QAction::text().remove("&").remove(" ").toStdString();
}

/** Registers the feature usage if usage is enabled
*   @param checked Whether the QAction is checked
**/
void TrackedAction::trackActivation(const bool checked) {
  UNUSED_ARG(checked);
  if (m_isTracking) {
    // do tracking
    registerUsage(getTrackingName());
  }
}

/** Registers the feature usage with the usage service
*   @param name The name to use when registering usage
**/
void TrackedAction::registerUsage(const std::string &name) {
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage("Feature", name,
                                                                false);
}
} // namespace MantidWidgets
} // namespace Mantid
