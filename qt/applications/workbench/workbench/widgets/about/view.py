# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
from PyQt5.QtWidgets import QCommandLinkButton
from qtpy.QtCore import Qt, QSize
from qtpy.QtGui import QPixmap, QTextFormat, QIcon
from qtpy.QtWidgets import qApp, QVBoxLayout, QHBoxLayout, QSpacerItem, QSizePolicy, QLabel, QDialog, \
    QGroupBox, QFormLayout, QComboBox, QPushButton, QCheckBox


class AboutView(QDialog):
    def __init__(self, parent, presenter, version_text, date_text = None):
        super(AboutView, self).__init__(parent)
        self.setupUI()
        self.customize_layout(version_text, date_text)
        self.presenter = presenter

    def setupUI(self):
        self.setMinimumSize(745,642)
        self.setWindowTitle("About Mantid Workbench")
        self.setStyleSheet("""QDialog {
    background-color: rgb(190, 230, 190);
    background-image: url(:/images/First_use_Background.png);
    background-repeat: no-repeat;
    image: url(:/images/mantid_smaller.png);
    image-position: right bottom
}
QLabel{
    font: 14px;
}
QCheckbox{
    font: 16px;
}
QPushButton{
    font: 14px;
}
QCommandLinkButton{
    font: 22px;
}""")

        stlyeName = qApp.style().metaObject().className()
        if stlyeName in ["QCDEStyle", "QMotifStyle"]:
            # add stylesheet formatting for other environments
            ss = self.styleSheet()
            ss += "\n"
            "QCommandLinkButton {"
            " background-color: rgba(255, 255, 255, 0);"
            "  border-radius: 15px;"
            "}"
            "\n"
            "QCommandLinkButton:hover {"
            "  background-color: rgba(255, 255, 255, 128);"
            "}"
            self.setStyleSheet(ss)

        # version label section at th etop
        self.parentLayout = QVBoxLayout()
        self.topSpacer = QSpacerItem(20,70,vPolicy = QSizePolicy.Fixed)
        self.parentLayout.addSpacerItem(self.topSpacer)
        self.lblVersion = QLabel(self)
        self.lblVersion.setText("version ")
        self.lblVersion.setIndent(115)
        self.lblVersion.setStyleSheet("""color: rgb(215, 215, 215);
font: 28pt;
font-weight: bold;
font-size: 28px""")
        self.parentLayout.addWidget(self.lblVersion)
        self.parentLayout.addSpacerItem(QSpacerItem(20,40,vPolicy = QSizePolicy.MinimumExpanding))

        # split into the two columns
        self.twoBoxLayout = QHBoxLayout()
        # left side Welcome and Tutorial
        leftLayout = QVBoxLayout()
        leftLayout.setContentsMargins(5, 0, 10, 0)
        leftLayout.setSpacing(0)
        # welcome label
        lblWelcome = QLabel()
        lblWelcome.setStyleSheet("color: rgb(45, 105, 45); font-size: 28px;")
        lblWelcome.setText("Welcome")
        leftLayout.addWidget(lblWelcome)
        # release notes
        self.clbReleaseNotes = self.create_command_link_button("Release Notes",
                                                               ':/images/Notepad-Bloc-notes-icon-48x48.png')
        leftLayout.addWidget(self.clbReleaseNotes)
        # sample datasets
        self.clbSampleDatasets = self.create_command_link_button("Sample Datasets",
                                                                 ':/images/download-icon-48x48.png')
        leftLayout.addWidget(self.clbSampleDatasets)
        # Tutorials Label
        lblTutorials = QLabel()
        lblTutorials.setStyleSheet("color: rgb(45, 105, 45); font-size: 28px;")
        lblTutorials.setText("Tutorials")
        leftLayout.addWidget(lblTutorials)
        # Mantid Introduction
        self.clbMantidIntroduction = self.create_command_link_button("Mantid Introduction",
                                                                     ':/images/Misc-Tutorial-icon-48x48.png')
        leftLayout.addWidget(self.clbMantidIntroduction)
        # Introduction to python
        self.clbPythonIntroduction = self.create_command_link_button("Introduction to Python",
                                                                     ':/images/Python-icon-48x48.png')
        leftLayout.addWidget(self.clbPythonIntroduction)
        # Python in Mantid
        self.clbPythonInMantid = self.create_command_link_button("Python In Mantid",
                                                                 ':/images/Circle_cog_48x48.png')
        leftLayout.addWidget(self.clbPythonInMantid)
        # Extending Mantid with python
        self.clbExtendingMantid = self.create_command_link_button("Extending Mantid with Python",
                                                                  ':/images/Plugin-Python-icon-48x48.png')
        leftLayout.addWidget(self.clbExtendingMantid)

        # right hand side Setup and facility icons
        rightLayout = QVBoxLayout()
        rightLayout.setSpacing(0)
        # personal setup
        grpPersonalSetup = QGroupBox()
        grpPersonalSetup.setStyleSheet("""QGroupBox {
     border: 3px solid  rgb(38, 128, 20);;
     border-radius: 10px;
     background-color: rgb(240, 240, 240);
}
QGroupBox QLabel{
    font: 12px;
    color: rgb(121, 121, 121);
}

font: 12px;
""")
        grpPersonalSetupLayout = QVBoxLayout()
        grpPersonalSetupLayout.setContentsMargins(9,1,9,9)
        grpPersonalSetupLayout.setSpacing(0)
        grpPersonalSetup.setLayout(grpPersonalSetupLayout)
        lblPersonalSetup = QLabel()
        lblPersonalSetup.setStyleSheet("color: rgb(38, 128, 20);\nfont-size: 18px;")
        lblPersonalSetup.setText("Personal Setup")
        lblPersonalSetup.setAlignment(Qt.AlignHCenter)
        grpPersonalSetupLayout.addWidget(lblPersonalSetup)
        personalSetupFormLayout = QFormLayout()
        personalSetupFormLayout.setHorizontalSpacing(5)
        personalSetupFormLayout.setVerticalSpacing(5)
        personalSetupFormLayout.setLabelAlignment(Qt.AlignRight)
        # default Facility
        lblDefaultFacilty = QLabel()
        lblDefaultFacilty.setText("Default Facility")
        self.cbFacility = QComboBox()
        personalSetupFormLayout.addRow(lblDefaultFacilty,self.cbFacility)
        # default instrument
        lblDefaultInstrument = QLabel()
        lblDefaultInstrument.setText("Default Instrument")
        self.cbInstrument = QComboBox()
        personalSetupFormLayout.addRow(lblDefaultInstrument,self.cbInstrument)
        # Set Data Directories
        lblMUD = QLabel()
        lblMUD.setText("Set data directories")
        self.pbMUD = QPushButton()
        self.pbMUD.setText("Manage User Directories")
        personalSetupFormLayout.addRow(lblMUD,self.pbMUD)
        # Usage data
        lblAllowUsageData = QLabel()
        lblAllowUsageData.setText("Report Usage Data")
        usagelayout = QHBoxLayout()
        usagelayout.setContentsMargins(0,0,0,0)
        self.chkAllowUsageData = QCheckBox()
        self.chkAllowUsageData.setChecked(True)
        self.chkAllowUsageData.setStyleSheet("padding: 4px;")
        usagelayout.addWidget(self.chkAllowUsageData)
        usagelayout.addSpacerItem(QSpacerItem(40,20,hPolicy = QSizePolicy.Expanding))
        self.lblPrivacyPolicy = QLabel()
        self.lblPrivacyPolicy.setText(r'<html><head/><body><p><a href="https://www.mantidproject.org/MantidProject:Privacy_policy#Usage_Data_recorded_in_Mantid"><span style=" text-decoration: underline; color:#0000ff;">Privacy Policy</span></a></p></body></html>')
        #self.lblPrivacyPolicy.textFormat(QTextFormat.RichText)
        self.lblPrivacyPolicy.setOpenExternalLinks(False)
        usagelayout.addWidget(self.lblPrivacyPolicy)
        personalSetupFormLayout.addRow(lblAllowUsageData,usagelayout)
        grpPersonalSetupLayout.addLayout(personalSetupFormLayout)
        rightLayout.addWidget(grpPersonalSetup)
        rightLayout.addSpacerItem(QSpacerItem(20,40,vPolicy = QSizePolicy.Expanding))

        # facility icons
        # Row one
        iconlayout1 = QHBoxLayout()
        iconlayout1.setContentsMargins(0,10,0,0)
        iconlayout1.setSpacing(0)
        iconlayout1.addWidget(self.create_label_with_image(100, 40, ':/images/ISIS_Logo_Transparent.gif'))
        iconlayout1.addSpacerItem(QSpacerItem(10,20,hPolicy = QSizePolicy.Fixed))
        iconlayout1.addWidget(self.create_label_with_image(94, 50, ':/images/ess_logo_transparent_small.png'))
        iconlayout1.addSpacerItem(QSpacerItem(40,20,hPolicy = QSizePolicy.Expanding))
        rightLayout.addLayout(iconlayout1)
        # Row two
        iconlayout2 = QHBoxLayout()
        iconlayout2.setContentsMargins(0,10,0,0)
        iconlayout2.setSpacing(0)
        iconlayout2.addWidget(self.create_label_with_image(200, 30, ':/images/Ornl_hfir_sns_logo_small.png'))
        iconlayout2.addSpacerItem(QSpacerItem(40,20,hPolicy = QSizePolicy.Expanding))
        rightLayout.addLayout(iconlayout2)
        # Row three
        iconlayout3 = QHBoxLayout()
        iconlayout3.setContentsMargins(0,10,0,0)
        iconlayout3.setSpacing(0)
        iconlayout3.addWidget(self.create_label_with_image(110, 40, ':/images/Tessella_Logo_Transparent.gif'))
        iconlayout3.addSpacerItem(QSpacerItem(10,20,hPolicy = QSizePolicy.Fixed))
        iconlayout3.addWidget(self.create_label_with_image(50, 50, ':/images/ILL_logo.png'))
        iconlayout3.addSpacerItem(QSpacerItem(10,20,hPolicy = QSizePolicy.Fixed))
        iconlayout3.addWidget(self.create_label_with_image(92, 50, ':/images/CSNS_Logo_Short.png'))
        iconlayout3.addSpacerItem(QSpacerItem(40,20,hPolicy = QSizePolicy.Expanding))
        rightLayout.addLayout(iconlayout3)

        # end the two box layout
        self.twoBoxLayout.addLayout(leftLayout)
        self.twoBoxLayout.addLayout(rightLayout)
        self.parentLayout.addLayout(self.twoBoxLayout)

        # footer
        footerLayout = QHBoxLayout()
        # do not show again
        doNotShowLayout = QVBoxLayout()
        doNotShowLayout.setContentsMargins(15,0,0,0)
        doNotShowLayout.setSpacing(2)
        lblUpdate = QLabel()
        lblUpdate.setMinimumSize(400,0)
        lblUpdate.setStyleSheet("font: 12px; color: rgb(25,125,25);")
        lblUpdate.setText('You can revisit this dialog by selecting "About" on the Help menu.')
        lblUpdate.setAlignment(Qt.AlignBottom)
        doNotShowLayout.addWidget(lblUpdate)

        doNotShowCheckboxLayout = QHBoxLayout()
        doNotShowCheckboxLayout.setSpacing(7)
        self.chkDoNotShowUntilNextRelease = QCheckBox()
        self.chkDoNotShowUntilNextRelease.setChecked(True)
        doNotShowCheckboxLayout.addWidget(self.chkDoNotShowUntilNextRelease)
        doNotShowCheckboxLayout.addSpacerItem(QSpacerItem(3,2,hPolicy = QSizePolicy.Fixed))
        lblDoNotShow = QLabel()
        lblDoNotShow.setStyleSheet("font: 12px; color: rgb(25,125,25);")
        lblDoNotShow.setText('Do not show again until next release')
        doNotShowCheckboxLayout.addWidget(lblDoNotShow)
        doNotShowCheckboxLayout.addSpacerItem(QSpacerItem(40,20,hPolicy = QSizePolicy.Expanding))
        doNotShowLayout.addLayout(doNotShowCheckboxLayout)
        footerLayout.addLayout(doNotShowLayout)

        # Close button
        closeButtonLayout = QVBoxLayout()
        closeButtonLayout.addSpacerItem(QSpacerItem(20,15,vPolicy = QSizePolicy.Expanding))
        self.pbClose = QPushButton()
        self.pbClose.setText("Close")
        self.pbClose.setDefault(True)
        closeButtonLayout.addWidget(self.pbClose)
        footerLayout.addLayout(closeButtonLayout)
        footerLayout.addSpacerItem(QSpacerItem(100,20,hPolicy = QSizePolicy.Fixed))
        self.parentLayout.addLayout(footerLayout)
        self.setLayout(self.parentLayout)

        self.setAttribute(Qt.WA_DeleteOnClose, True)

    def create_command_link_button(self, text, image_location, width = 40, height = 40):
        link_button = QCommandLinkButton()
        link_button.setText(text)
        link_button.setIconSize(QSize(width, height))
        link_button.setIcon(QIcon(QPixmap(image_location)))
        return link_button

    def create_label_with_image(self, width, height, image_location):
        label_with_image = QLabel()
        label_with_image.setMaximumSize(width, height)
        label_with_image.setPixmap(QPixmap(image_location))
        label_with_image.setScaledContents(True)
        return label_with_image

    def customize_layout(self, version_text, date_text):
        self.setWindowTitle(self.windowTitle() + " "
                            + version_text)
        version_label = version_text
        # add a date if it is an official release
        if date_text and len(version_text) < 10:
            # strip off the first few characters that will be "day, "
            version_label += " ({0})".format(date_text[5:])
        self.lblVersion.setText(self.lblVersion.text() + version_label)

    def closeEvent(self, event):
        self.presenter.action_close()
        self.deleteLater()
        super(AboutView, self).closeEvent(event)
