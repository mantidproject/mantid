# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
from PyQt5.QtWidgets import QCommandLinkButton
from qtpy.QtCore import Qt, QSize, QPoint
from qtpy.QtGui import QPixmap, QIcon, QGuiApplication, QPainter
from qtpy.QtWidgets import qApp, QVBoxLayout, QHBoxLayout, QSpacerItem, QSizePolicy, QLabel, QDialog, \
    QGroupBox, QFormLayout, QComboBox, QPushButton, QCheckBox

REFERENCE_HEIGHT = 642
REFERENCE_WIDTH = 745


class AboutView(QDialog):
    def __init__(self, parent, presenter, version_text, date_text = None):
        super(AboutView, self).__init__(parent)
        self.background_pixmap = QPixmap(':/images/First_use_Background.png')
        self.mantid_pixmap = QPixmap(':/images/mantid_smaller.png')
        self.setupUI()
        self.customize_layout(version_text, date_text)
        self.presenter = presenter

    def paintEvent(self, event):
        scaled_background = self.background_pixmap.scaled(self.rescale_w(self.background_pixmap.width()),
                                                          self.rescale_h(self.background_pixmap.height()),
                                                          Qt.KeepAspectRatio,
                                                          Qt.SmoothTransformation)

        scaled_mantid = self.mantid_pixmap.scaled(self.rescale_w(self.mantid_pixmap.width()),
                                                  self.rescale_h(self.mantid_pixmap.height()),
                                                  Qt.KeepAspectRatio,
                                                  Qt.SmoothTransformation)
        qp = QPainter()
        qp.begin(self)
        qp.drawPixmap(0,0,scaled_background)
        qp.drawPixmap(self.width() - scaled_mantid.width(), self.height()-scaled_mantid.height(), scaled_mantid)
        qp.end()

    def rescale_w(self, value):
        return int(value * (self.width() / REFERENCE_WIDTH))
    def rescale_h(self, value):
        return int(value * (self.height() / REFERENCE_HEIGHT))

    def setupUI(self):
        width = REFERENCE_WIDTH
        height = REFERENCE_HEIGHT

        # gets the width of the screen where the main window is
        screen = QGuiApplication.screenAt(self.parent().geometry().center())
        if screen is not None:
            screen_width = screen.size().width()
            # the proportion of the whole window size for the about screen
            window_scaling = 0.4
            width = int(screen_width * window_scaling) if screen_width * window_scaling > REFERENCE_WIDTH else REFERENCE_WIDTH
            height = int(REFERENCE_HEIGHT * (width/REFERENCE_WIDTH))

        self.setMinimumSize(width, height)
        self.setSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed)
        self.setWindowTitle("About Mantid Workbench")
        self.setStyleSheet(f"""QDialog {{
    background-color: rgb(190, 230, 190);
}}
QLabel{{
    font: {self.rescale_w(14)}px;
}}
QCheckbox{{
    font: {self.rescale_w(16)}px;
}}
QCombobox{{
    font: {self.rescale_w(14)}px;
}}
QPushButton{{
    font: {self.rescale_w(14)}px;
}}
QCommandLinkButton{{
    font: {self.rescale_w(22)}px;
    background-color: rgba(255, 255, 255, 0);
    border-radius: {self.rescale_w(15)}px;
}}
QCommandLinkButton:hover {{
    background-color: rgba(200, 200, 200, 40);
}}""")

        # version label section at th etop
        self.parentLayout = QVBoxLayout()
        self.topSpacer = QSpacerItem(self.rescale_w(20),self.rescale_h(70),vPolicy = QSizePolicy.Fixed)
        self.parentLayout.addSpacerItem(self.topSpacer)
        self.lblVersion = QLabel(self)
        self.lblVersion.setText("version ")
        self.lblVersion.setIndent(self.rescale_w(115))
        self.lblVersion.setStyleSheet(f"""color: rgb(215, 215, 215);
font: {self.rescale_w(28)}pt;
font-weight: bold;
font-size: {self.rescale_w(28)}px""")
        self.parentLayout.addWidget(self.lblVersion)
        self.parentLayout.addSpacerItem(QSpacerItem(self.rescale_w(20),self.rescale_h(40),vPolicy = QSizePolicy.MinimumExpanding))

        # split into the two columns
        self.twoBoxLayout = QHBoxLayout()
        # left side Welcome and Tutorial
        leftLayout = QVBoxLayout()
        leftLayout.setContentsMargins(self.rescale_w(5), 0, self.rescale_w(10), 0)
        leftLayout.setSpacing(0)
        # welcome label
        lblWelcome = QLabel()
        lblWelcome.setStyleSheet(f"color: rgb(45, 105, 45); font-size: {self.rescale_w(28)}px;")
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
        lblTutorials.setStyleSheet(f"color: rgb(45, 105, 45); font-size: {self.rescale_w(28)}px;")
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
        grpPersonalSetup.setStyleSheet(f"""QGroupBox {{
     border: {self.rescale_w(3)}px solid  rgb(38, 128, 20);;
     border-radius: {self.rescale_w(10)}px;
     background-color: rgb(240, 240, 240);
}}
QGroupBox QLabel{{
    font: {self.rescale_w(12)}px;
    color: rgb(121, 121, 121);
}}
QGroupBox QComboBox{{
    font: {self.rescale_w(12)}px;
    color: rgb(121, 121, 121);
}}
font: {self.rescale_w(12)}px;
""")
        grpPersonalSetupLayout = QVBoxLayout()
        grpPersonalSetupLayout.setContentsMargins(self.rescale_w(9),
                                                  self.rescale_h(1),
                                                  self.rescale_w(9),
                                                  self.rescale_h(9))
        grpPersonalSetupLayout.setSpacing(0)
        grpPersonalSetup.setLayout(grpPersonalSetupLayout)
        lblPersonalSetup = QLabel()
        lblPersonalSetup.setStyleSheet(f"color: rgb(38, 128, 20);\nfont-size: {self.rescale_w(18)}px;")
        lblPersonalSetup.setText("Personal Setup")
        lblPersonalSetup.setAlignment(Qt.AlignHCenter)
        grpPersonalSetupLayout.addWidget(lblPersonalSetup)
        personalSetupFormLayout = QFormLayout()
        personalSetupFormLayout.setHorizontalSpacing(self.rescale_w(5))
        personalSetupFormLayout.setVerticalSpacing(self.rescale_h(5))
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
        self.chkAllowUsageData.setStyleSheet(f"padding: {self.rescale_w(4)}px;")
        usagelayout.addWidget(self.chkAllowUsageData)
        usagelayout.addSpacerItem(QSpacerItem(self.rescale_w(40),self.rescale_h(20),hPolicy = QSizePolicy.Expanding))
        self.lblPrivacyPolicy = QLabel()
        self.lblPrivacyPolicy.setText(r'<html><head/><body><p><a href="https://www.mantidproject.org/MantidProject:Privacy_policy#Usage_Data_recorded_in_Mantid"><span style=" text-decoration: underline; color:#0000ff;">Privacy Policy</span></a></p></body></html>')
        #self.lblPrivacyPolicy.textFormat(QTextFormat.RichText)
        self.lblPrivacyPolicy.setOpenExternalLinks(False)
        usagelayout.addWidget(self.lblPrivacyPolicy)
        personalSetupFormLayout.addRow(lblAllowUsageData,usagelayout)
        grpPersonalSetupLayout.addLayout(personalSetupFormLayout)
        rightLayout.addWidget(grpPersonalSetup)
        rightLayout.addSpacerItem(QSpacerItem(self.rescale_w(20),self.rescale_h(40),vPolicy = QSizePolicy.Expanding))

        # facility icons
        # Row one
        iconlayout1 = QHBoxLayout()
        iconlayout1.setContentsMargins(0,self.rescale_h(10),0,0)
        iconlayout1.setSpacing(0)
        iconlayout1.addWidget(self.create_label_with_image(112, 50, ':/images/ISIS_Logo_Transparent.gif'))
        iconlayout1.addSpacerItem(QSpacerItem(self.rescale_w(10), self.rescale_h(20), hPolicy = QSizePolicy.Fixed))
        iconlayout1.addWidget(self.create_label_with_image(94, 50, ':/images/ess_logo_transparent_small.png'))
        iconlayout1.addSpacerItem(QSpacerItem(self.rescale_w(40),20,hPolicy = QSizePolicy.Expanding))
        rightLayout.addLayout(iconlayout1)
        # Row two
        iconlayout2 = QHBoxLayout()
        iconlayout2.setContentsMargins(0,self.rescale_h(10),0,0)
        iconlayout2.setSpacing(0)
        iconlayout2.addWidget(self.create_label_with_image(200, 30, ':/images/Ornl_hfir_sns_logo_small.png'))
        iconlayout2.addSpacerItem(QSpacerItem(self.rescale_w(40), self.rescale_h(20), hPolicy = QSizePolicy.Expanding))
        rightLayout.addLayout(iconlayout2)
        # Row three
        iconlayout3 = QHBoxLayout()
        iconlayout3.setContentsMargins(0,self.rescale_h(10),0,0)
        iconlayout3.setSpacing(0)
        iconlayout3.addWidget(self.create_label_with_image(110, 40, ':/images/Tessella_Logo_Transparent.gif'))
        iconlayout3.addSpacerItem(QSpacerItem(self.rescale_w(10), self.rescale_h(20), hPolicy = QSizePolicy.Fixed))
        iconlayout3.addWidget(self.create_label_with_image(50, 50, ':/images/ILL_logo.png'))
        iconlayout3.addSpacerItem(QSpacerItem(self.rescale_w(10), self.rescale_h(20), hPolicy = QSizePolicy.Fixed))
        iconlayout3.addWidget(self.create_label_with_image(92, 50, ':/images/CSNS_Logo_Short.png'))
        iconlayout3.addSpacerItem(QSpacerItem(self.rescale_w(40), self.rescale_h(20), hPolicy = QSizePolicy.Expanding))
        rightLayout.addLayout(iconlayout3)

        # end the two box layout
        self.twoBoxLayout.addLayout(leftLayout)
        self.twoBoxLayout.addLayout(rightLayout)
        self.parentLayout.addLayout(self.twoBoxLayout)

        # footer
        footerLayout = QHBoxLayout()
        # do not show again
        doNotShowLayout = QVBoxLayout()
        doNotShowLayout.setContentsMargins(self.rescale_w(15),0,0,0)
        doNotShowLayout.setSpacing(2)
        lblUpdate = QLabel()
        lblUpdate.setMinimumSize(self.rescale_w(400),0)
        lblUpdate.setStyleSheet("color: rgb(25,125,25);")
        lblUpdate.setText('You can revisit this dialog by selecting "About" on the Help menu.')
        lblUpdate.setAlignment(Qt.AlignBottom)
        doNotShowLayout.addWidget(lblUpdate)

        doNotShowCheckboxLayout = QHBoxLayout()
        doNotShowCheckboxLayout.setSpacing(self.rescale_w(7))
        self.chkDoNotShowUntilNextRelease = QCheckBox()
        self.chkDoNotShowUntilNextRelease.setChecked(True)
        doNotShowCheckboxLayout.addWidget(self.chkDoNotShowUntilNextRelease)
        doNotShowCheckboxLayout.addSpacerItem(QSpacerItem(self.rescale_w(3), self.rescale_h(2), hPolicy = QSizePolicy.Fixed))
        lblDoNotShow = QLabel()
        lblDoNotShow.setStyleSheet("color: rgb(25,125,25);")
        lblDoNotShow.setText('Do not show again until next release')
        doNotShowCheckboxLayout.addWidget(lblDoNotShow)
        doNotShowCheckboxLayout.addSpacerItem(QSpacerItem(self.rescale_w(40), self.rescale_h(20), hPolicy = QSizePolicy.Expanding))
        doNotShowLayout.addLayout(doNotShowCheckboxLayout)
        footerLayout.addLayout(doNotShowLayout)

        # Close button
        closeButtonLayout = QVBoxLayout()
        closeButtonLayout.addSpacerItem(QSpacerItem(20,self.rescale_h(15),vPolicy = QSizePolicy.Expanding))
        self.pbClose = QPushButton()
        self.pbClose.setText("Close")
        self.pbClose.setDefault(True)
        closeButtonLayout.addWidget(self.pbClose)
        footerLayout.addLayout(closeButtonLayout)
        footerLayout.addSpacerItem(QSpacerItem(self.rescale_w(100), self.rescale_h(20), hPolicy = QSizePolicy.Fixed))
        self.parentLayout.addLayout(footerLayout)
        self.setLayout(self.parentLayout)

        self.setAttribute(Qt.WA_DeleteOnClose, True)

    def create_command_link_button(self, text, image_location, width = 40, height = 40):
        link_button = QCommandLinkButton()
        link_button.setText(text)
        link_button.setIconSize(QSize(self.rescale_w(width), self.rescale_h(height)))
        link_button.setIcon(QIcon(QPixmap(image_location)))
        return link_button

    def create_label_with_image(self, width, height, image_location):
        label_with_image = QLabel()
        label_with_image.setMinimumSize(self.rescale_w(width), self.rescale_h(height))
        label_with_image.setMaximumSize(self.rescale_w(width), self.rescale_h(height))
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
