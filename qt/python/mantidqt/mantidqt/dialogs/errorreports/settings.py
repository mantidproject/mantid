# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy.QtCore import QSettings

ERROR_REPORTER_APPLICATION = "mantid-error-reporter"

CONTACT_INFO = "ContactInfo"
NAME = "Name"
EMAIL = "Email"


def _workbench_organization() -> str:
    # Importing workbench.config constructs CONF, so defer this until settings are actually needed.
    from workbench.config import ORGANIZATION

    return ORGANIZATION


def _workbench_application() -> str:
    # Importing workbench.config constructs CONF, so defer this until settings are actually needed.
    from workbench.config import APPNAME

    return APPNAME


def create_error_reporter_settings() -> QSettings:
    """Return the dedicated user settings store for the error reporter."""
    return QSettings(QSettings.IniFormat, QSettings.UserScope, _workbench_organization(), ERROR_REPORTER_APPLICATION)


def create_legacy_workbench_settings() -> QSettings:
    """Return the Workbench store used only to read legacy contact information."""
    return QSettings(QSettings.IniFormat, QSettings.UserScope, _workbench_organization(), _workbench_application())


def read_contact_information() -> tuple[str, str]:
    """Read dedicated contact values, falling back individually to the legacy Workbench store."""
    reporter_settings = create_error_reporter_settings()
    reporter_settings.beginGroup(CONTACT_INFO)
    name_is_set = reporter_settings.contains(NAME)
    email_is_set = reporter_settings.contains(EMAIL)
    name = reporter_settings.value(NAME, "", type=str) if name_is_set else ""
    email = reporter_settings.value(EMAIL, "", type=str) if email_is_set else ""
    reporter_settings.endGroup()

    if name_is_set and email_is_set:
        return name, email

    legacy_settings = create_legacy_workbench_settings()
    legacy_settings.beginGroup(CONTACT_INFO)
    if not name_is_set:
        name = legacy_settings.value(NAME, "", type=str)
    if not email_is_set:
        email = legacy_settings.value(EMAIL, "", type=str)
    legacy_settings.endGroup()
    return name, email
