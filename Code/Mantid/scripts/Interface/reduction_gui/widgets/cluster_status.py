from PyQt4 import QtGui, uic, QtCore
import os
from reduction_gui.settings.application_settings import GeneralSettings
from reduction_gui.widgets.base_widget import BaseWidget
from reduction_gui.widgets import util
import ui.ui_cluster_status

import mantid.simpleapi as api
from mantid.kernel import ConfigService
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
        headers = ["Job ID", "Title", "Status", "Start", "End"]
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
        
        alg = AlgorithmManager.create("QueryAllRemoteJobs")
        alg.initialize()
        alg.setProperty("ComputeResource", str(self._settings.compute_resource))
        alg.setProperty("UserName", str(self._settings.cluster_user))
        alg.setProperty("Password", str(self._settings.cluster_pass))
        alg.execute()
        job_id = alg.getProperty("JobId").value
        job_status = alg.getProperty("JobStatusString").value
        job_name = alg.getProperty("JobName").value
        job_start = alg.getProperty("JobStartTime").value
        job_end = alg.getProperty("JobCompletionTime").value
                
        job_list = zip(*(job_id, job_status, job_name, job_start, job_end))
        
        self._clear_table()
        self._content.job_table.setSortingEnabled(False)
        self._content.job_table.setRowCount(len(job_list))

        for i in range(len(job_list)):
            # Job ID
            item = QtGui.QTableWidgetItem(str(job_list[i][0]))
            item.setFlags(QtCore.Qt.ItemIsSelectable |QtCore.Qt.ItemIsEnabled )
            self._content.job_table.setItem(i, 0, item)
          
            # Title
            item = QtGui.QTableWidgetItem(str(job_list[i][2]))
            item.setFlags(QtCore.Qt.ItemIsSelectable |QtCore.Qt.ItemIsEnabled )
            self._content.job_table.setItem(i, 1, item)
          
            # Status
            item = QtGui.QTableWidgetItem(str(job_list[i][1]))
            item.setFlags(QtCore.Qt.ItemIsSelectable |QtCore.Qt.ItemIsEnabled )
            self._content.job_table.setItem(i, 2, item)
            
            # Start time
            item = QtGui.QTableWidgetItem(str(job_list[i][3]))
            item.setFlags(QtCore.Qt.ItemIsSelectable |QtCore.Qt.ItemIsEnabled )
            self._content.job_table.setItem(i, 3, item)
            
            # Completion time
            item = QtGui.QTableWidgetItem(str(job_list[i][4]))
            item.setFlags(QtCore.Qt.ItemIsSelectable |QtCore.Qt.ItemIsEnabled )
            self._content.job_table.setItem(i, 4, item)
          
        self._content.job_table.setSortingEnabled(True)
        self._content.job_table.sortItems(0)
    
    def get_state(self):
        return RemoteJobs()