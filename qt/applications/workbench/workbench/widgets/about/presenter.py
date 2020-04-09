# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
from qtpy.QtCore import Qt, QSettings

from mantid.kernel import ConfigService, logger, version_str, release_notes_url, release_date
from mantidqt.interfacemanager import InterfaceManager
from workbench.widgets.about.view import AboutView
from workbench.widgets.about.usage_verification_view import UsageReportingVerificationView
from mantidqt.widgets import manageuserdirectories  # noqa


class AboutPresenter(object):

    INSTRUMENT = "default.instrument"
    USAGE_REPORTING = "usagereports.enabled"
    DO_NOT_SHOW_GROUP = "Mantid/FirstUse"
    DO_NOT_SHOW = "DoNotShowUntilNextRelease"
    LAST_VERSION = "LastVersion"
    FACILITY = "default.facility"
    INSTRUMENT = "default.instrument"

    def __init__(self, parent, view=None, usage_reporting_verification_view = None):
        self.view = view if view else AboutView(parent, self, version_str(), release_date().strip())
        self.usage_reporting_verification_view = usage_reporting_verification_view \
            if usage_reporting_verification_view  else UsageReportingVerificationView(parent, self)
        self.parent = parent
        self.view.clbReleaseNotes.clicked.connect(self.action_open_release_notes)
        self.view.clbSampleDatasets.clicked.connect(self.action_open_download_website)
        self.view.clbMantidIntroduction.clicked.connect(self.action_open_mantid_introduction)
        self.view.clbPythonIntroduction.clicked.connect(self.action_open_python_introduction)
        self.view.clbPythonInMantid.clicked.connect(self.action_open_python_in_mantid)
        self.view.clbExtendingMantid.clicked.connect(self.action_open_extending_mantid)
        self.view.lblPrivacyPolicy.linkActivated.connect(self.action_open_external_link)
        self.view.pbMUD.clicked.connect(self.action_manage_user_directories)
        self.view.pbClose.clicked.connect(self.action_close)
        self.setup_facilities_group()

        # set chkAllowUsageData
        isUsageReportEnabled = ConfigService.getString(self.USAGE_REPORTING, True)
        if isUsageReportEnabled:
            self.view.chkAllowUsageData.setChecked(False)
        self.view.chkAllowUsageData.stateChanged.connect(self.action_usage_data_changed)

        # set do not show
        qSettings = QSettings()
        qSettings.beginGroup(self.DO_NOT_SHOW_GROUP)
        doNotShowUntilNextRelease = int(qSettings.value(self.DO_NOT_SHOW, '0'))
        qSettings.endGroup()
        self.view.chkDoNotShowUntilNextRelease.setChecked(doNotShowUntilNextRelease)
        self.view.chkDoNotShowUntilNextRelease.stateChanged.connect(self.action_do_not_show_until_next_release)

    @staticmethod
    def should_show_on_startup():
        """ Determines if the first time dialog should be shown
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
                logger.information("Default facility '{0}', instrument '{1}'\n".format(facilityInfo.name(),
                                                                                       instrumentInfo.name()))
            except RuntimeError:
                # failed to find the facility or instrument
                logger.error("Could not find your default facility '{0}' or instrument '{1}' in facilities.xml, " +
                             "showing please select again.\n".format(facility, instrument))
                return True

        settings = QSettings()
        settings.beginGroup(AboutPresenter.DO_NOT_SHOW_GROUP)
        doNotShowUntilNextRelease =int(settings.value(AboutPresenter.DO_NOT_SHOW, '0'))
        lastVersion = settings.value(AboutPresenter.LAST_VERSION, "")
        settings.endGroup()

        if not doNotShowUntilNextRelease:
            return True

        # Now check if the version has changed since last time
        version = release_notes_url()
        return version != lastVersion

    def setup_facilities_group(self):
        facilities = ConfigService.getFacilityNames()
        if not facilities:
            return
        self.view.cbFacility.addItems(facilities)

        try:
            default_facility = ConfigService.getFacility().name()
        except RuntimeError:
            default_facility = facilities[0]
        self.view.cbFacility.setCurrentIndex(self.view.cbFacility.findText(default_facility))
        self.action_facility_changed(default_facility)
        self.view.cbFacility.currentTextChanged.connect(self.action_facility_changed)

        try:
            default_instrument = ConfigService.getInstrument().name()
            self.view.cbInstrument.setCurrentIndex(self.view.cbInstrument.findText(default_instrument))
        except RuntimeError:
            default_instrument = self.view.cbInstrument.itemText(0)
        self.action_instrument_changed(default_instrument)
        self.view.cbInstrument.currentTextChanged.connect(self.action_instrument_changed)

    def action_facility_changed(self, new_facility):
        """
        When the facility is changed, refreshes all available instruments that can be selected in the dropdown.
        :param new_facility: The name of the new facility that was selected
        """
        current_value = ConfigService.getFacility().name()
        if new_facility != current_value:
            ConfigService.setFacility(new_facility)
        # refresh the instrument selection to contain instruments about the selected facility only
        self.view.cbInstrument.clear()
        self.view.cbInstrument.addItems(
            [instr.name() for instr in ConfigService.getFacility(new_facility).instruments()])

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
        InterfaceManager().showWebPage('http://download.mantidproject.org')

    def action_open_mantid_introduction(self):
        InterfaceManager().showWebPage('http://www.mantidproject.org/Mantid_Basic_Course')

    def action_open_python_introduction(self):
        InterfaceManager().showWebPage('http://www.mantidproject.org/Introduction_To_Python')

    def action_open_python_in_mantid(self):
        InterfaceManager().showWebPage('http://www.mantidproject.org/Python_In_Mantid')

    def action_open_extending_mantid(self):
        InterfaceManager().showWebPage('http://www.mantidproject.org/Extending_Mantid_With_Python')

    def action_open_external_link(self, url):
        InterfaceManager().showWebPage(url)

    def action_usage_data_changed(self, checkedState):
        if not checkedState:
            response = self.usage_reporting_verification_view.exec()

            if not response:
                # No was clicked, or no button was clicked
                # set the checkbox back to checked
                self.view.chkAllowUsageData.setCheckState(Qt.Checked)

    def action_do_not_show_until_next_release(self, checkedState):
        settings = QSettings()
        settings.beginGroup(self.DO_NOT_SHOW_GROUP)
        settings.setValue(self.DO_NOT_SHOW, int(checkedState))
        settings.endGroup()

    def action_close(self):
        # make sure the Last Version is updated on closing
        settings = QSettings()
        settings.beginGroup(self.DO_NOT_SHOW_GROUP)
        settings.setValue(self.LAST_VERSION, release_notes_url())
        settings.endGroup()

        self.view.close()
