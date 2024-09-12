# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
from PyQt5.QtWidgets import QCommandLinkButton
from qtpy.QtCore import Qt, QSize
from qtpy.QtGui import QPixmap, QIcon, QGuiApplication, QPainter
from qtpy.QtWidgets import (
    QVBoxLayout,
    QHBoxLayout,
    QSpacerItem,
    QSizePolicy,
    QLabel,
    QDialog,
    QGroupBox,
    QFormLayout,
    QComboBox,
    QPushButton,
    QCheckBox,
    QWidget,
    QScrollArea,
)
from mantidqt.widgets import instrumentselector

REFERENCE_HEIGHT = 642
REFERENCE_WIDTH = 745
REFERENCE_ASPECT_RATIO = REFERENCE_WIDTH / REFERENCE_HEIGHT
WIDESCREEN_ASPECT_RATIO = 16 / 9


class AboutView(QDialog):
    """
    Dialog that contains the about widget. If the screen is too small, the dialog will use a scroll area,
    otherwise it will simply contain the about widget.
    """

    def __init__(self, parent, presenter, version_text, date_text=None):
        super(AboutView, self).__init__(parent)
        self.presenter = presenter
        self.about_widget = AboutViewWidget(self, version_text, date_text)

        layout = QVBoxLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        self.setLayout(layout)

        if self.about_widget.is_widget_too_big:
            self.scroll_area = QScrollArea()
            self.scroll_area.setWidget(self.about_widget)
            layout.addWidget(self.scroll_area)
            self.setMaximumSize(self.about_widget.size())

        else:
            layout.addWidget(self.about_widget)
            self.setFixedSize(self.about_widget.size())

    def closeEvent(self, event):
        self.presenter.save_on_closing()
        self.deleteLater()
        super(AboutView, self).closeEvent(event)

    def resizeEvent(self, event):
        """
        Hide the scroll bars if the dialog reaches the size of the about widget.
        """
        super().resizeEvent(event)
        if hasattr(self, "scroll_area"):
            if self.width() < self.about_widget.width() or self.height() < self.about_widget.height():
                self.scroll_area.setHorizontalScrollBarPolicy(Qt.ScrollBarAsNeeded)
                self.scroll_area.setVerticalScrollBarPolicy(Qt.ScrollBarAsNeeded)
            else:
                self.scroll_area.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
                self.scroll_area.setVerticalScrollBarPolicy(Qt.ScrollBarAlwaysOff)


class AboutViewWidget(QWidget):
    def __init__(self, parent, version_text, date_text):
        super(AboutViewWidget, self).__init__(parent)
        self.background_pixmap = QPixmap(":/images/First_use_Background.png")
        self.mantid_pixmap = QPixmap(":/images/mantid_smaller.png")
        self.lbl_version = QLabel()
        self.clb_release_notes = QCommandLinkButton()
        self.clb_sample_datasets = QCommandLinkButton()
        self.clb_mantid_introduction = QCommandLinkButton()
        self.clb_python_introduction = QCommandLinkButton()
        self.clb_python_in_mantid = QCommandLinkButton()
        self.clb_extending_mantid = QCommandLinkButton()
        self.cb_facility = QComboBox()
        self.cb_instrument = instrumentselector.InstrumentSelector()
        self.pb_manage_user_directories = QPushButton()
        self.chk_allow_usage_data = QCheckBox()
        self.lbl_privacy_policy = QLabel()
        self.chk_do_not_show_until_next_release = QCheckBox()
        self.pb_close = QPushButton()
        self.is_widget_too_big = False
        self.setupUI()
        self.customize_layout(version_text, date_text)

    def paintEvent(self, event):
        scaled_background = self.background_pixmap.scaled(
            self.rescale_w(self.background_pixmap.width()),
            self.rescale_h(self.background_pixmap.height()),
            Qt.KeepAspectRatio,
            Qt.SmoothTransformation,
        )

        scaled_mantid = self.mantid_pixmap.scaled(
            self.rescale_w(self.mantid_pixmap.width()),
            self.rescale_h(self.mantid_pixmap.height()),
            Qt.KeepAspectRatio,
            Qt.SmoothTransformation,
        )
        qp = QPainter()
        qp.begin(self)
        qp.drawPixmap(0, 0, scaled_background)
        qp.drawPixmap(self.width() - scaled_mantid.width(), self.height() - scaled_mantid.height(), scaled_mantid)
        qp.end()

    def determine_dialog_dimensions(self):
        width = REFERENCE_WIDTH
        height = REFERENCE_HEIGHT

        screen = None
        try:
            if hasattr(QGuiApplication, "screenAt"):
                screen = QGuiApplication.screenAt(self.parent().parent().geometry().center())
            else:
                # get the screen from the last top level window
                windows = QGuiApplication.topLevelWindows()
                screen = windows[-1].screen()
        except Exception:
            # something failed just take the primary screen
            screen = QGuiApplication.primaryScreen()

        if screen is not None:
            screen_width = screen.availableSize().width()
            screen_height = screen.availableSize().height()

            # the proportion of the whole window size for the about screen
            window_scaling = 0.4
            width = int(screen_width * window_scaling)

            # also calculate the intended width but using the hieght and a standard screen aspect ratio
            width_by_height = int(screen_height * WIDESCREEN_ASPECT_RATIO * window_scaling)
            # take the smaller of the width from the screen width and height
            if width_by_height < width:
                width = width_by_height

            # set a minimum size
            if width < REFERENCE_WIDTH:
                width = REFERENCE_WIDTH

            # calculate height from the width and aspect ratio
            height = int(width / REFERENCE_ASPECT_RATIO)

            if width > screen_width or height > screen_height:
                self.is_widget_too_big = True

        return width, height

    def rescale_w(self, value):
        return int(value * (self.width() / REFERENCE_WIDTH))

    def rescale_h(self, value):
        return int(value * (self.height() / REFERENCE_HEIGHT))

    def setupUI(self):
        width, height = self.determine_dialog_dimensions()

        self.setFixedSize(width, height)
        self.setSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed)
        self.setWindowTitle("About Mantid Workbench")
        self.setStyleSheet(
            f"""QDialog {{
    background-color: rgb(190, 230, 190);
}}
QLabel{{
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
    background-color: rgba(45, 105, 45, 40);
}}"""
        )

        # version label section at th etop
        parent_layout = QVBoxLayout()
        parent_layout.addSpacerItem(QSpacerItem(self.rescale_w(20), self.rescale_h(70), vPolicy=QSizePolicy.Fixed))
        self.lbl_version.setText("version ")
        self.lbl_version.setIndent(self.rescale_w(115))
        self.lbl_version.setStyleSheet(
            f"""color: rgb(215, 215, 215);
font: {self.rescale_w(28)}pt;
font-weight: bold;
font-size: {self.rescale_w(28)}px"""
        )
        parent_layout.addWidget(self.lbl_version)
        parent_layout.addSpacerItem(QSpacerItem(self.rescale_w(20), self.rescale_h(40), vPolicy=QSizePolicy.MinimumExpanding))

        # split into the two columns
        two_box_layout = QHBoxLayout()
        # left side Welcome and Tutorial
        left_layout = QVBoxLayout()
        left_layout.setContentsMargins(self.rescale_w(5), 0, self.rescale_w(10), 0)
        left_layout.setSpacing(0)
        # welcome label
        lbl_welcome = QLabel()
        lbl_welcome.setStyleSheet(f"color: rgb(45, 105, 45); font-size: {self.rescale_w(28)}px;")
        lbl_welcome.setText("Welcome")
        left_layout.addWidget(lbl_welcome)
        # release notes
        self.clb_release_notes.setStyleSheet("QCommandLinkButton { color: black; }")
        self.setup_command_link_button(self.clb_release_notes, "Release Notes", ":/images/Notepad-Bloc-notes-icon-48x48.png")
        left_layout.addWidget(self.clb_release_notes)
        # sample datasets
        self.clb_sample_datasets.setStyleSheet("QCommandLinkButton { color: black; }")
        self.setup_command_link_button(self.clb_sample_datasets, "Sample Datasets", ":/images/download-icon-48x48.png")
        left_layout.addWidget(self.clb_sample_datasets)
        # Tutorials Label
        lbl_tutorials = QLabel()
        lbl_tutorials.setStyleSheet(f"color: rgb(45, 105, 45) ; font-size: {self.rescale_w(28)}px;")
        lbl_tutorials.setText("Tutorials")
        left_layout.addWidget(lbl_tutorials)
        # Mantid Introduction
        self.clb_mantid_introduction.setStyleSheet("QCommandLinkButton { color: black; }")
        self.setup_command_link_button(self.clb_mantid_introduction, "Mantid Introduction", ":/images/Misc-Tutorial-icon-48x48.png")
        left_layout.addWidget(self.clb_mantid_introduction)
        # Introduction to python
        self.clb_python_introduction.setStyleSheet("QCommandLinkButton { color: black; }")
        self.setup_command_link_button(self.clb_python_introduction, "Introduction to Python", ":/images/Python-icon-48x48.png")
        left_layout.addWidget(self.clb_python_introduction)
        # Python in Mantid
        self.clb_python_in_mantid.setStyleSheet("QCommandLinkButton { color: black; }")
        self.setup_command_link_button(self.clb_python_in_mantid, "Python In Mantid", ":/images/Circle_cog_48x48.png")
        left_layout.addWidget(self.clb_python_in_mantid)
        # Extending Mantid with python
        self.clb_extending_mantid.setStyleSheet("QCommandLinkButton { color: black; }")
        self.setup_command_link_button(self.clb_extending_mantid, "Extending Mantid with Python", ":/images/Plugin-Python-icon-48x48.png")
        left_layout.addWidget(self.clb_extending_mantid)

        # right hand side Setup and facility icons
        right_layout = QVBoxLayout()
        right_layout.setSpacing(0)
        # personal setup
        grp_personal_setup = QGroupBox()
        grp_personal_setup.setStyleSheet(
            f"""QGroupBox {{
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
    color: black;
}}
QGroupBox QComboBox QAbstractItemView{{
     background-color: rgb(240, 240, 240);
}}
QGroupBox QPushButton{{
    font: {self.rescale_w(12)}px;
    color: black;
}}
QGroupBox QComboBox::down-arrow{{
     image: url(images/DropDownArrow.png)
     color: black;
}}
font: {self.rescale_w(12)}px;
"""
        )
        grp_personal_setup_layout = QVBoxLayout()
        grp_personal_setup_layout.setContentsMargins(self.rescale_w(9), self.rescale_h(1), self.rescale_w(9), self.rescale_h(9))
        grp_personal_setup_layout.setSpacing(0)
        grp_personal_setup.setLayout(grp_personal_setup_layout)
        lbl_personal_setup = QLabel()
        lbl_personal_setup.setStyleSheet(f"color: rgb(38, 128, 20);\nfont-size: {self.rescale_w(18)}px;")
        lbl_personal_setup.setText("Personal Setup")
        lbl_personal_setup.setAlignment(Qt.AlignHCenter)
        grp_personal_setup_layout.addWidget(lbl_personal_setup)
        personal_setup_form_layout = QFormLayout()
        personal_setup_form_layout.setFieldGrowthPolicy(QFormLayout.AllNonFixedFieldsGrow)
        personal_setup_form_layout.setHorizontalSpacing(self.rescale_w(5))
        personal_setup_form_layout.setVerticalSpacing(self.rescale_h(5))
        # default Facility
        lbl_default_facilty = QLabel()
        lbl_default_facilty.setText("Default Facility")
        personal_setup_form_layout.addRow(lbl_default_facilty, self.cb_facility)
        # default instrument
        lbl_default_instrument = QLabel()
        lbl_default_instrument.setText("Default Instrument")
        personal_setup_form_layout.addRow(lbl_default_instrument, self.cb_instrument)
        # Set Data Directories
        lbl_mud = QLabel()
        lbl_mud.setText("Set data directories")
        self.pb_manage_user_directories.setText("Manage User Directories")
        personal_setup_form_layout.addRow(lbl_mud, self.pb_manage_user_directories)
        # Usage data
        lbl_allow_usage_data = QLabel()
        lbl_allow_usage_data.setText(
            f"<span style='text-align: right; font-size:{self.rescale_h(12)}px;'>Report Usage Data</span><br/>"
            f"<span style='text-align: right; font-size:{self.rescale_h(8)}px;'>Required to use the Error Reporter</span>"
        )
        usagelayout = QHBoxLayout()
        usagelayout.setContentsMargins(0, 0, 0, 0)
        self.chk_allow_usage_data.setChecked(True)
        self.chk_allow_usage_data.setStyleSheet(f"padding: {self.rescale_w(4)}px;")
        usagelayout.addWidget(self.chk_allow_usage_data)
        usagelayout.addSpacerItem(QSpacerItem(self.rescale_w(40), self.rescale_h(20), hPolicy=QSizePolicy.Expanding))
        self.lbl_privacy_policy.setText(
            r"<html><head/><body><p>"
            r'<a href="https://www.mantidproject.org/privacy#details-of-data-retention'
            r'#Usage_Data_recorded_in_Mantid">'
            r'<span style=" text-decoration: underline; color:#0000ff;">'
            r"Privacy Policy</span></a></p></body></html>"
        )
        self.lbl_privacy_policy.setOpenExternalLinks(False)
        usagelayout.addWidget(self.lbl_privacy_policy)
        personal_setup_form_layout.addRow(lbl_allow_usage_data, usagelayout)
        grp_personal_setup_layout.addLayout(personal_setup_form_layout)
        right_layout.addWidget(grp_personal_setup)
        right_layout.addSpacerItem(QSpacerItem(self.rescale_w(20), self.rescale_h(40), vPolicy=QSizePolicy.Expanding))

        # facility icons
        # Row one
        icon_layout_top = QHBoxLayout()
        icon_layout_top.setContentsMargins(0, self.rescale_h(10), 0, 0)
        icon_layout_top.setSpacing(0)
        icon_layout_top.addWidget(self.create_label_with_image(112, 60, ":/images/ISIS_Logo_Transparent_UKRI.png"))
        icon_layout_top.addSpacerItem(QSpacerItem(self.rescale_w(10), self.rescale_h(20), hPolicy=QSizePolicy.Fixed))
        icon_layout_top.addWidget(self.create_label_with_image(94, 50, ":/images/ess_logo_transparent_small.png"))
        icon_layout_top.addSpacerItem(QSpacerItem(self.rescale_w(40), 20, hPolicy=QSizePolicy.Expanding))
        right_layout.addLayout(icon_layout_top)
        # Row two
        icon_layout_middle = QHBoxLayout()
        icon_layout_middle.setContentsMargins(0, self.rescale_h(10), 0, 0)
        icon_layout_middle.setSpacing(0)
        icon_layout_middle.addWidget(self.create_label_with_image(200, 30, ":/images/Ornl_hfir_sns_logo_small.png"))
        icon_layout_middle.addSpacerItem(QSpacerItem(self.rescale_w(40), self.rescale_h(20), hPolicy=QSizePolicy.Expanding))
        right_layout.addLayout(icon_layout_middle)
        # Row three
        icon_layout_bottom = QHBoxLayout()
        icon_layout_bottom.setContentsMargins(0, self.rescale_h(10), 0, 0)
        icon_layout_bottom.setSpacing(0)
        icon_layout_bottom.addWidget(self.create_label_with_image(110, 40, ":/images/Tessella_Logo_Transparent.gif"))
        icon_layout_bottom.addSpacerItem(QSpacerItem(self.rescale_w(10), self.rescale_h(20), hPolicy=QSizePolicy.Fixed))
        icon_layout_bottom.addWidget(self.create_label_with_image(50, 50, ":/images/ILL_logo.png"))
        icon_layout_bottom.addSpacerItem(QSpacerItem(self.rescale_w(10), self.rescale_h(20), hPolicy=QSizePolicy.Fixed))
        icon_layout_bottom.addWidget(self.create_label_with_image(92, 50, ":/images/CSNS_Logo_Short.png"))
        icon_layout_bottom.addSpacerItem(QSpacerItem(self.rescale_w(40), self.rescale_h(20), hPolicy=QSizePolicy.Expanding))
        right_layout.addLayout(icon_layout_bottom)

        # end the two box layout
        two_box_layout.addLayout(left_layout)
        two_box_layout.addLayout(right_layout)
        parent_layout.addLayout(two_box_layout)

        # footer
        footer_layout = QHBoxLayout()
        # do not show again
        do_not_show_layout = QVBoxLayout()
        do_not_show_layout.setContentsMargins(self.rescale_w(15), 0, 0, 0)
        do_not_show_layout.setSpacing(self.rescale_w(2))
        do_not_show_layout.addSpacerItem(QSpacerItem(1, self.rescale_h(1), vPolicy=QSizePolicy.Expanding))
        lbl_update = QLabel()
        lbl_update.setMinimumSize(self.rescale_w(400), 0)
        lbl_update.setStyleSheet("color: rgb(25,125,25);")
        lbl_update.setText('You can revisit this dialog by selecting "About" on the Help menu.')
        lbl_update.setAlignment(Qt.AlignBottom)
        do_not_show_layout.addWidget(lbl_update)

        do_not_show_checkbox_layout = QHBoxLayout()
        self.chk_do_not_show_until_next_release.setChecked(True)
        do_not_show_checkbox_layout.addWidget(self.chk_do_not_show_until_next_release)
        do_not_show_checkbox_layout.addSpacerItem(QSpacerItem(self.rescale_w(10), self.rescale_h(2), hPolicy=QSizePolicy.Fixed))
        lbl_do_not_show = QLabel()
        lbl_do_not_show.setStyleSheet("color: rgb(25,125,25);")
        lbl_do_not_show.setText("Do not show again until next release")
        do_not_show_checkbox_layout.addWidget(lbl_do_not_show)
        do_not_show_checkbox_layout.addSpacerItem(QSpacerItem(self.rescale_w(40), 10, hPolicy=QSizePolicy.Expanding))
        do_not_show_layout.addLayout(do_not_show_checkbox_layout)
        footer_layout.addLayout(do_not_show_layout)

        # Close button
        close_button_layout = QVBoxLayout()
        close_button_layout.addSpacerItem(QSpacerItem(20, self.rescale_h(15), vPolicy=QSizePolicy.Expanding))
        self.pb_close.setStyleSheet("color: black;")
        self.pb_close.setText("Close")
        self.pb_close.setDefault(True)
        close_button_layout.addWidget(self.pb_close)
        footer_layout.addLayout(close_button_layout)
        footer_layout.addSpacerItem(QSpacerItem(self.rescale_w(100), self.rescale_h(20), hPolicy=QSizePolicy.Fixed))
        parent_layout.addLayout(footer_layout)
        self.setLayout(parent_layout)

        self.setAttribute(Qt.WA_DeleteOnClose, True)

    def setup_command_link_button(self, link_button, text, image_location, width=40, height=40):
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
        self.setWindowTitle(self.windowTitle() + " " + version_text)
        version_label = version_text
        # add a date if it is an official release
        if date_text and len(version_text) < 10:
            # strip off the first few characters that will be "day, "
            version_label += " ({0})".format(date_text[5:])
        self.lbl_version.setText(self.lbl_version.text() + version_label)
