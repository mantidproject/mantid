from PyQt4 import QtGui, uic, QtCore
import os
from reduction_gui.settings.application_settings import GeneralSettings
from reduction_gui.widgets.base_widget import BaseWidget
from reduction_gui.widgets import util
import ui.ui_cluster_status

import mantid.simpleapi as api
from mantid.kernel import ConfigService

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

        # Update the table
        #self._update_content(False)
        
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
        
    def _update_content(self):
        if self._settings.compute_resource is not None:
            for i in range(self._content.resource_combo.count()):
                if self._content.resource_combo.itemText(i)==self._settings.compute_resource:
                    self._content.resource_combo.setCurrentIndex(i)
                    break

        if self._settings.cluster_user is not None \
            and self._settings.cluster_pass is not None:
            util.set_valid(self._content.username_edit, True)
            util.set_valid(self._content.password_edit, True)
            self._content.username_edit.setText(self._settings.cluster_user)
            self._content.password_edit.setText("password")
            self._content.login_status_edit.setText("Credential ready")
        else:
            user = str(self._content.username_edit.text())
            pwd = str(self._content.password_edit.text())
            if len(user)==0 or len(pwd)==0:
                util.set_valid(self._content.username_edit, False)
                util.set_valid(self._content.password_edit, False)
                self._content.login_status_edit.setText("Enter credentials")
                return
            else:
                self._settings.cluster_user = user
                self._settings.cluster_pass = pwd
                self._content.login_status_edit.setText("Credential ready")
        
        try:
            job_info = api.QueryAllRemoteJobs(ComputeResource=self._settings.compute_resource,
                                              UserName=self._settings.cluster_user,
                                              Password=self._settings.cluster_pass)
        except:
            # TEST
            job_info =  (['901', '912', '905', '903', '914', '904', '906', '913'],
                         ['Complete', 'Complete', 'Complete', 'Complete', 'Complete', 'Complete', 'Complete', 'Complete'],
                         [0, 0, 0, 0, 0, 0, 0, 0],
                         ['STDIN', 'STDIN', 'STDIN', 'STDIN', 'STDIN', 'STDIN', 'STDIN', 'STDIN'])
            
        job_list = zip(*(job_info[0], job_info[1], job_info[3]))
        
        self._content.job_table.clear()
        self._content.job_table.setSortingEnabled(False)
        self._content.job_table.setRowCount(len(job_list))
        headers = ["Job ID", "Title", "Status", "Start", "End"]
        self._content.job_table.setColumnCount(len(headers))
        self._content.job_table.setHorizontalHeaderLabels(headers)

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
          
        self._content.job_table.setSortingEnabled(True)
        self._content.job_table.sortItems(0)
        # Stretch the columns evenly
        h = self._content.job_table.horizontalHeader()
        h.setStretchLastSection(True)
        h.setResizeMode(1)
    
    def get_state(self):
        return Catalog()