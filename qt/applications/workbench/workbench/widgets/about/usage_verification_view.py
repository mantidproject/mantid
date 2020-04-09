# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
from qtpy.QtCore import Qt, QSize
from qtpy.QtWidgets import QHBoxLayout, QLabel, QPushButton, QSpacerItem, QStyle, QVBoxLayout, QDialog, QSizePolicy


class UsageReportingVerificationView(QDialog):
    def __init__(self, parent, presenter):
        super(UsageReportingVerificationView, self).__init__(parent)
        self.presenter = presenter

        self.setWindowTitle("Mantid: Report Usage Data ")
        parentLayout = QHBoxLayout()

        # left side
        iconLayout = QVBoxLayout()
        style = self.style()
        icon = style.standardIcon(QStyle.SP_MessageBoxQuestion)
        labelButActuallyAnIcon = QLabel(self)
        labelButActuallyAnIcon.setPixmap(icon.pixmap(32, 32))
        iconLayout.addWidget(labelButActuallyAnIcon)
        vspacer = QSpacerItem(1, 1, QSizePolicy.Minimum, QSizePolicy.Expanding)
        iconLayout.addSpacerItem(vspacer)
        parentLayout.addLayout(iconLayout)

        # right side
        textLayout = QVBoxLayout()
        labelQuestion = QLabel(self)
        labelQuestion.setTextFormat(Qt.RichText)
        labelQuestion.setText("""Are you sure you want to disable reporting of <a 
                        href='https://reports.mantidproject.org'>usage data</a>?
                        (full details in our <a 
                        href='https://www.mantidproject.org/MantidProject:Privacy_policy#Usage_Data_recorded_in_Mantid'
                        >Privacy Policy</a>)""")
        textLayout.addWidget(labelQuestion)
        labelQuestion.setOpenExternalLinks(False)
        labelQuestion.linkActivated.connect(presenter.action_open_external_link)

        labelInformation = QLabel(self)
        labelInformation.setText("All usage data is anonymous and untraceable.\n" +
            "We use the usage data to inform the future development of Mantid.\n" +
            "If you click \"Yes\" aspects you need risk being deprecated in \n" +
            "future versions if we think they are not used.\n\n" +
            "Are you sure you still want to disable reporting usage data?\n" +
            "Please click \"No\".")
        textLayout.addWidget(labelInformation)

        buttonLayout = QHBoxLayout()
        leftSpacer = QSpacerItem(1, 1, QSizePolicy.Expanding, QSizePolicy.Minimum)
        buttonLayout.addSpacerItem(leftSpacer)

        noBtn = QPushButton("No", self)
        buttonLayout.addWidget(noBtn)
        noBtn.clicked.connect(self.reject)

        yesBtn = QPushButton("Yes", self)
        buttonLayout.addWidget(yesBtn)
        yesBtn.clicked.connect(self.accept)

        rightSpacer = QSpacerItem(1, 1, QSizePolicy.Expanding, QSizePolicy.Minimum)
        buttonLayout.addSpacerItem(rightSpacer)

        textLayout.addLayout(buttonLayout)
        parentLayout.addLayout(textLayout)


        self.setLayout(parentLayout)
        self.setMinimumSize(498,161)
