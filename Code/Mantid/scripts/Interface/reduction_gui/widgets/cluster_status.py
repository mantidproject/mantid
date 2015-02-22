#pylint: disable=invalid-name
from PyQt4 import QtGui, uic, QtCore
import os
import sys
import datetime
from functools import partial
from reduction_gui.settings.application_settings import GeneralSettings
from reduction_gui.widgets.base_widget import BaseWidget
from reduction_gui.widgets import util
import ui.ui_cluster_status

import mantid.simpleapi as api
from mantid.kernel import ConfigService, DateAndTime, Logger
from mantid.api import AlgorithmManager

from reduction_gui.reduction.scripter import BaseScriptElement
class RemoteJobs(BaseScriptElement):
    def __init__(self):
        pass

class RemoteJobsWidget(BaseWidget):
    """
        Widget that presents a list of remote jobs for the user
    """
    ## Widget name
    name = "Remote Jobs"

    def __init__(self, parent=None, state=None, settings=None):
        super(RemoteJobsWidget, self).__init__(parent, state, settings)

        class DataFrame(QtGui.QFrame, ui.ui_cluster_status.Ui_Frame):
            def __init__(self, parent=None):
                QtGui.QFrame.__init__(self, parent)
                self.setupUi(self)

        self._content = DataFrame(self)
        self.initialize_content()
        self._layout.addWidget(self._content)

        # General GUI settings
        if settings is None:
            settings = GeneralSettings()
        self._settings = settings

    def initialize_content(self):
        # Add functionality to copy and paste
        self.copyAction = QtGui.QAction("Copy", self)
        self.copyAction.setShortcut("Ctrl+C")
        self.addAction(self.copyAction)
        self.connect(self.copyAction, QtCore.SIGNAL("triggered()"), self.copyCells)
        self._content.job_table.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
        self.connect(self._content.job_table, QtCore.SIGNAL("customContextMenuRequested(QPoint)"), self.tableWidgetContext)

        self.connect(self._content.refresh_button, QtCore.SIGNAL("clicked()"), self._update_content)

        # Set the time of the oldest displayed job to 2 days ago
        self._content.date_time_edit.setDateTime(QtCore.QDateTime().currentDateTime().addDays(-2))

        compute_resources = ConfigService.Instance().getFacility().computeResources()
        self._content.resource_combo.clear()
        for res in compute_resources:
            self._content.resource_combo.addItem(QtGui.QApplication.translate("Dialog", res, None, QtGui.QApplication.UnicodeUTF8))

        self._clear_table()

    def tableWidgetContext(self, point):
        '''Create a menu for the tableWidget and associated actions'''
        tw_menu = QtGui.QMenu("Menu", self)
        tw_menu.addAction(self.copyAction)
        tw_menu.exec_(self.mapToGlobal(point))

    def copyCells(self):
        indices = self._content.job_table.selectedIndexes()
        if len(indices)==0:
            return

        col_count = self._content.job_table.columnCount()
        rows = []
        for r in indices:
            if r.row() not in rows:
                rows.append(r.row())

        selected_text = ""
        for row in rows:
            for i in range(col_count):
                data = self._content.job_table.item(row,i)
                if data is not None:
                    selected_text += str(data.text())
                if i<col_count-1:
                    selected_text += '\t'
            selected_text += '\n'

        QtGui.QApplication.clipboard().setText(selected_text)

    def paintEvent(self, event):
        """
            Catch the paint events and update the credential info.
        """
        super(RemoteJobsWidget, self).paintEvent(event)
        self._fill_in_defaults()

    def _fill_in_defaults(self):
        """
            Fill in the credentials boxes if we have the information
        """
        if self._settings.compute_resource is not None:
            for i in range(self._content.resource_combo.count()):
                if self._content.resource_combo.itemText(i)==self._settings.compute_resource:
                    self._content.resource_combo.setCurrentIndex(i)
                    break

        if self._settings.cluster_user is not None \
            and self._settings.cluster_pass is not None:
            self._content.username_edit.setText(self._settings.cluster_user)
            self._content.password_edit.setText(self._settings.cluster_pass)

    def _clear_table(self):
        """
            Clear the job table and set the headers
        """
        self._content.job_table.clear()
        headers = ["Job ID", "Title", "Status", "Start", "End", "Action"]
        self._content.job_table.setColumnCount(len(headers))
        self._content.job_table.setHorizontalHeaderLabels(headers)
        # Stretch the columns evenly
        h = self._content.job_table.horizontalHeader()
        h.setStretchLastSection(True)
        h.setResizeMode(1)

    def _update_content(self):
        """
            Get the job status from the compute resource and
            update the job table content.
        """
        self._fill_in_defaults()

        user = str(self._content.username_edit.text())
        pwd = str(self._content.password_edit.text())
        if len(user)==0 or len(pwd)==0:
            util.set_valid(self._content.username_edit, False)
            util.set_valid(self._content.password_edit, False)
            return
        else:
            self._settings.cluster_user = user
            self._settings.cluster_pass = pwd
            util.set_valid(self._content.username_edit, True)
            util.set_valid(self._content.password_edit, True)
        alg = AlgorithmManager.create("Authenticate")
        alg.initialize()
        alg.setProperty("ComputeResource", str(self._settings.compute_resource))
        alg.setProperty("UserName", str(self._settings.cluster_user))
        alg.setProperty("Password", str(self._settings.cluster_pass))
        alg.execute()

        alg = AlgorithmManager.create("QueryAllRemoteJobs")
        alg.initialize()
        alg.setProperty("ComputeResource", str(self._settings.compute_resource))
        alg.execute()
        job_id = alg.getProperty("JobId").value
        job_status = alg.getProperty("JobStatusString").value
        job_name = alg.getProperty("JobName").value
        job_trans_id = alg.getProperty("TransID").value

        njobs = len(job_name)
        job_start = alg.getProperty("StartDate").value
        job_end = alg.getProperty("CompletionDate").value

        job_list = zip(*(job_id, job_status, job_name, job_start, job_end, job_trans_id))

        self._clear_table()
        self._content.job_table.setSortingEnabled(False)
        self._content.job_table.setRowCount(len(job_list))
        unavailable = DateAndTime(0)
        unavailable.setToMinimum()

        for i in range(len(job_list)):
            # Make sure that only recent jobs are displayed
            oldest = DateAndTime(str(self._content.date_time_edit.dateTime().toString(QtCore.Qt.ISODate)))
            end_time = job_list[i][4]
            if end_time == '':
                job_end = unavailable
            else:
                job_end = DateAndTime(end_time)
            if job_end>unavailable and job_end<oldest:
                self._content.job_table.setRowHidden(i, True)
                continue
            self._content.job_table.setRowHidden(i, False)

            # Job ID
            item = QtGui.QTableWidgetItem(str(job_list[i][0]))
            item.setFlags(QtCore.Qt.ItemIsSelectable |QtCore.Qt.ItemIsEnabled )
            self._content.job_table.setItem(i, 0, item)
            job_id = str(job_list[i][0])

            # Title
            item = QtGui.QTableWidgetItem(str(job_list[i][2]))
            item.setFlags(QtCore.Qt.ItemIsSelectable |QtCore.Qt.ItemIsEnabled )
            self._content.job_table.setItem(i, 1, item)

            # Status
            item = QtGui.QTableWidgetItem(str(job_list[i][1]))
            item.setFlags(QtCore.Qt.ItemIsSelectable |QtCore.Qt.ItemIsEnabled )
            self._content.job_table.setItem(i, 2, item)
            is_running = str(job_list[i][1]).lower()=='running'

            # Start time
            time_displayed = str(job_list[i][3]).replace('T', ' ')
            if DateAndTime(job_list[i][3]) == unavailable:
                time_displayed = ''
            item = QtGui.QTableWidgetItem(time_displayed)
            item.setFlags(QtCore.Qt.ItemIsSelectable |QtCore.Qt.ItemIsEnabled )
            self._content.job_table.setItem(i, 3, item)

            # Completion time
            time_displayed = end_time.replace('T', ' ')
            if job_end == unavailable:
                time_displayed = ''
            item = QtGui.QTableWidgetItem(time_displayed)
            item.setFlags(QtCore.Qt.ItemIsSelectable |QtCore.Qt.ItemIsEnabled )
            self._content.job_table.setItem(i, 4, item)

            # create an cell widget
            btn = QtGui.QPushButton(self._content.job_table)
            if is_running:
                btn.setText('Abort')
                btn.setToolTip('Cleanly abort this job')
            else:
                btn.setText('Remove')
                btn.setToolTip('Remove this job and its temporary files')
            call_back = partial(self._remove_job, is_running=is_running, job_id=job_id, trans_id=job_list[i][5])
            self.connect(btn, QtCore.SIGNAL("clicked()"), call_back)
            self._content.job_table.setCellWidget(i, 5, btn)


        self._content.job_table.setSortingEnabled(True)
        self._content.job_table.sortItems(3, 1)

    def _remove_job(self, trans_id, job_id=None, is_running=False):
        """
            Abort job and/or stop transaction
            @param trans_id: remote transaction ID
            @param job_id: remote job ID
            @param is_running: True if the job is currently running
        """
        if is_running:
            try:
                # At this point we are authenticated so just purge
                alg = AlgorithmManager.create("AbortRemoteJob")
                alg.initialize()
                alg.setProperty("ComputeResource", str(self._settings.compute_resource))
                alg.setProperty("JobID", job_id)
                alg.execute()
            except:
                Logger("cluster_status").error("Problem aborting job: %s" % sys.exc_value)
        try:
            alg = AlgorithmManager.create("StopRemoteTransaction")
            alg.initialize()
            alg.setProperty("ComputeResource", str(self._settings.compute_resource))
            alg.setProperty("TransactionID", trans_id)
            alg.execute()
        except:
            Logger("cluster_status").error("Project stopping remote transaction: %s" % sys.exc_value)
        self._update_content()

    def get_state(self):
        return RemoteJobs()
