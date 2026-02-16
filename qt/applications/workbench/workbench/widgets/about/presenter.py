# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
from qtpy.QtCore import Qt, QSettings

from mantid.kernel import ConfigService, logger, release_notes_url, release_date, version
from mantidqt.interfacemanager import InterfaceManager
from workbench.widgets.about.view import AboutView
from workbench.widgets.about.usage_verification_view import UsageReportingVerificationView
from mantidqt.widgets import manageuserdirectories


class AboutPresenter(object):
    USAGE_REPORTING = "usagereports.enabled"
    DO_NOT_SHOW_GROUP = "Mantid/FirstUse"
    DO_NOT_SHOW = "DoNotShowUntilNextRelease"
    PREVIOUS_VERSION = "PreviousVersion"
    FACILITY = "default.facility"
    INSTRUMENT = "default.instrument"

    def __init__(self, parent, view=None, usage_reporting_verification_view=None):
        self.view = view or AboutView(parent, self, str(version()), release_date().strip())
        self.usage_reporting_verification_view = usage_reporting_verification_view or UsageReportingVerificationView(parent, self)
        self.parent = parent

        about_widget = self.view.about_widget
        about_widget.clb_release_notes.clicked.connect(self.action_open_release_notes)
        about_widget.clb_sample_datasets.clicked.connect(self.action_open_download_website)
        about_widget.clb_mantid_introduction.clicked.connect(self.action_open_mantid_introduction)
        about_widget.clb_python_introduction.clicked.connect(self.action_open_python_introduction)
        about_widget.clb_python_in_mantid.clicked.connect(self.action_open_python_in_mantid)
        about_widget.clb_extending_mantid.clicked.connect(self.action_open_extending_mantid)
        about_widget.lbl_privacy_policy.linkActivated.connect(self.action_open_external_link)
        about_widget.pb_manage_user_directories.clicked.connect(self.action_manage_user_directories)
        about_widget.pb_close.clicked.connect(self.action_close)

        self.setup_facilities_group()

        # set chk_allow_usage_data
        isUsageReportEnabled = ConfigService.getString(self.USAGE_REPORTING, True)
        if isUsageReportEnabled == "0":
            about_widget.chk_allow_usage_data.setChecked(False)
        about_widget.chk_allow_usage_data.stateChanged.connect(self.action_usage_data_changed)

        # set do not show
        qSettings = QSettings()
        qSettings.beginGroup(self.DO_NOT_SHOW_GROUP)
        doNotShowUntilNextRelease = qSettings.value(self.DO_NOT_SHOW, 0, type=int)
        qSettings.endGroup()
        about_widget.chk_do_not_show_until_next_release.setChecked(doNotShowUntilNextRelease)
        about_widget.chk_do_not_show_until_next_release.stateChanged.connect(self.action_do_not_show_until_next_release)

    @staticmethod
    def should_show_on_startup():
        """Determines if the first time dialog should be shown
        :return: True if the dialog should be shown
        """
        # first check the facility and instrument
        facility = ConfigService.getString(AboutPresenter.FACILITY)
        instrument = ConfigService.getString(AboutPresenter.INSTRUMENT)
        if not facility:
            return True
        else:
            # check we can get the facility and instrument
            try:
                facilityInfo = ConfigService.getFacility(facility)
                instrumentInfo = ConfigService.getInstrument(instrument)
                logger.information("Default facility '{0}', instrument '{1}'\n".format(facilityInfo.name(), instrumentInfo.name()))
            except RuntimeError:
                # failed to find the facility or instrument
                logger.error(
                    f"Could not find your default facility '{facility}' or instrument '{instrument}' in facilities.xml, "
                    + "showing please select again.\n"
                )
                return True

        settings = QSettings()
        settings.beginGroup(AboutPresenter.DO_NOT_SHOW_GROUP)
        doNotShowUntilNextRelease = settings.value(AboutPresenter.DO_NOT_SHOW, 0, type=int)
        lastVersion = settings.value(AboutPresenter.PREVIOUS_VERSION, "", type=str)
        current_version = version().major + "." + version().minor
        settings.endGroup()

        if not doNotShowUntilNextRelease:
            return True

        # Now check if the version has changed since last time
        return current_version != lastVersion

    def setup_facilities_group(self):
        facilities = sorted(ConfigService.getFacilityNames())
        if not facilities:
            return
        self.view.about_widget.cb_facility.addItems(facilities)

        current_facility = self._get_current_facility()
        default_facility = current_facility if current_facility is not None else facilities[0]

        self.view.about_widget.cb_facility.setCurrentIndex(self.view.about_widget.cb_facility.findText(default_facility))
        self.action_facility_changed(default_facility)
        self.view.about_widget.cb_facility.currentTextChanged.connect(self.action_facility_changed)

    def action_facility_changed(self, new_facility):
        """
        When the facility is changed, refreshes all available instruments that can be selected in the dropdown.
        :param new_facility: The name of the new facility that was selected
        """
        current_facility = self._get_current_facility()
        self.store_facility(new_facility)
        # refresh the instrument selection to contain instruments about the selected facility only
        self.view.about_widget.cb_instrument.facility = new_facility
        if new_facility != current_facility:
            self.view.about_widget.cb_instrument.setCurrentIndex(0)

    def store_facility(self, new_facility):
        current_facility = self._get_current_facility()
        if new_facility != current_facility:
            ConfigService.setFacility(new_facility)
            return True
        return False

    @staticmethod
    def _get_current_facility() -> str:
        try:
            return ConfigService.getFacility().name()
        except RuntimeError:
            return None

    def action_instrument_changed(self, new_instrument):
        current_value = ConfigService.getString(self.INSTRUMENT)
        if new_instrument != current_value:
            ConfigService.setString(self.INSTRUMENT, new_instrument)

    def action_manage_user_directories(self):
        manageuserdirectories.ManageUserDirectories.openManageUserDirectories()

    def show(self, modal=True):
        if modal:
            self.view.setWindowModality(Qt.WindowModal)
        self.view.show()

    def action_open_release_notes(self):
        InterfaceManager().showHelpPage(release_notes_url())

    def action_open_download_website(self):
        InterfaceManager().showWebPage("https://www.mantidproject.org/installation/index#sample-data")

    def action_open_mantid_introduction(self):
        InterfaceManager().showWebPage("https://docs.mantidproject.org/nightly/tutorials/mantid_basic_course")

    def action_open_python_introduction(self):
        InterfaceManager().showWebPage("https://docs.mantidproject.org/nightly/tutorials/introduction_to_python")

    def action_open_python_in_mantid(self):
        InterfaceManager().showWebPage("https://docs.mantidproject.org/nightly/tutorials/python_in_mantid")

    def action_open_extending_mantid(self):
        InterfaceManager().showWebPage("https://docs.mantidproject.org/nightly/tutorials/extending_mantid_with_python")

    def action_open_external_link(self, url):
        InterfaceManager().showWebPage(url)

    def action_usage_data_changed(self, checkedState):
        if not checkedState:
            response = self.usage_reporting_verification_view.exec()

            if not response:
                # No was clicked, or no button was clicked
                # set the checkbox back to checked
                checkedState = Qt.Checked
                self.view.about_widget.chk_allow_usage_data.setCheckState(checkedState)

        ConfigService.setString(self.USAGE_REPORTING, "1" if checkedState == Qt.Checked else "0")

    def action_do_not_show_until_next_release(self, checkedState):
        settings = QSettings()
        settings.beginGroup(self.DO_NOT_SHOW_GROUP)
        settings.setValue(self.DO_NOT_SHOW, int(checkedState))
        settings.endGroup()

    def action_close(self):
        self.view.close()

    def save_on_closing(self):
        # make sure the Last Version is updated on closing
        settings = QSettings()
        settings.beginGroup(self.DO_NOT_SHOW_GROUP)
        settings.setValue(self.PREVIOUS_VERSION, version().major + "." + version().minor)
        settings.endGroup()
        self.store_facility(self.view.about_widget.cb_facility.currentText())
        self.action_instrument_changed(self.view.about_widget.cb_instrument.currentText())
        ConfigService.saveConfig(ConfigService.getUserFilename())
        self.parent.config_updated()
