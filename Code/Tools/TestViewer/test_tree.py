""" An AbstractTreeItem implementation for a QTreeView
that uses the results from test runners. """

from PyQt4 import QtGui, QtCore
import PyQt4.QtCore
from PyQt4.QtCore import *
import PyQt4.QtGui
from PyQt4.QtGui import *

import test_info
from test_info import TestSuite, TestSingle, TestProject, MultipleProjects
import random

HORIZONTAL_HEADERS = ("Test", "Status", "Time (sec)")
  
  
  
# =======================================================================================    
def format_time(seconds):
    """Return a string for the # of seconds """
    if seconds < 1e-3:
        return "<1 ms"
    else:
        return "%.3f s" % (seconds) 
    
    if seconds < 1e-3:
        return "%d us" % (seconds*1000000) 
    elif seconds < 0.1:
        return "%.1f ms" % (seconds*1000) 
    elif seconds < 10:
        return "%.2f s" % (seconds) 
    else:
        return "%.1f s" % (seconds) 
    
# =======================================================================================    
class MyColors:
    """ Some custom colors I want """
    veryLightGray = QColor(220, 220, 220)
    lightGreen = QColor( 128, 255, 128)
    darkishGreen = QColor( 64, 128, 64)
    lightRed = QColor( 255, 200, 200)
    mediumRed = QColor( 255, 150, 150)
    
def desaturate(color, amount):
    """Desaturate a color by pct.
    Parameters:
        amount: fraction between 0.0 and 1.0. 1.0 = fully saturated; 0.0 = desaturated """
    red = color.red() 
    green = color.green() 
    blue = color.blue() 
    gray = 0.3 * red + 0.4 * green + 0.3 * blue
    return QColor( gray * amount + red * (1 - amount),
                   gray * amount + green * (1 - amount),
                   gray * amount + blue * (1 - amount) )
    
# =======================================================================================    
def get_background_color(state):
    """Return the background color for this test result.
    Parameters
        state :: TestResult """
    if not isinstance(state, test_info.TestResult):
        return QVariant()
    
    col = QColor(Qt.lightGray)
    if state == test_info.TestResult.NOT_RUN:
        col = MyColors.veryLightGray
        
    elif state == test_info.TestResult.ALL_PASSED:
        col = MyColors.lightGreen
        if state.old: col = MyColors.darkishGreen
        
    elif state == test_info.TestResult.ALL_FAILED or (state == test_info.TestResult.SOME_FAILED) \
         or state == test_info.TestResult.SEGMENTATION_FAULT:
        col = QColor(Qt.red)
        if state.old: col = QColor( 200, 50, 50 )
        
    elif state == test_info.TestResult.BUILD_ERROR:
        col = QColor(Qt.magenta)
        if state.old: col = desaturate(col, 0.5)
        
#    elif state == test_info.TestResult.SOME_FAILED:
#        col = MyColors.lightRed
#        if state.old: col = MyColors.lightRed
        
    else:
        return QVariant()
    
    # Copy constructor
    col = QColor( col )
        
    return col 
                    


# =======================================================================================    
class TreeItemBase(object):
    """ Base class for common tree items with a ".contents" member."""
    def __init__(self, contents, parentItem):
        # The contents: TestSingle, TestSuite, TestProject for this tree item
        self.contents = contents
        # The parent TreeItem
        self.parentItem = parentItem
        self.childItems = []

    def appendChild(self, item):
        self.childItems.append(item)

    def child(self, row):
        return self.childItems[row]

    def childCount(self):
        return len(self.childItems)

    #-----------------------------------------------------------------------------------            
    def parent(self):
        return self.parentItem
    
    def row(self):
        if self.parentItem:
            return self.parentItem.childItems.index(self)
        return 0
    
    def background_color(self):
        """Return the background color for this item"""
        return get_background_color(self.contents.state)
    

# =======================================================================================    
class TreeItemProject(TreeItemBase):
    '''
    A python object used to return row/column data, and keep note of
    it's parents and/or children.
    
    This one represents a TestProject
    '''
    def __init__(self, project, parentItem):
        TreeItemBase.__init__(self, project, parentItem)

    def columnCount(self):
        return 3

    #-----------------------------------------------------------------------------------            
    def is_checked(self):
        return [Qt.Unchecked, Qt.Checked][self.contents.selected] 
    
    def set_checked(self, value):
        self.contents.selected = value

    #-----------------------------------------------------------------------------------            
    def data(self, column):
        if self.contents is None:
            if column == 0:
                return QtCore.QVariant(self.header)
            if column >= 1:
                return QtCore.QVariant("")                
        else:
            if column == 0:
                return QtCore.QVariant(self.contents.name)
            if column == 1:
                return QtCore.QVariant(self.contents.get_state_str())
            if column == 2:
                return QtCore.QVariant( format_time( self.contents.get_runtime() ))
        return QtCore.QVariant()

      

# =======================================================================================    
class TreeItemSuite(TreeItemBase):
    '''
    A python object used to return row/column data, and keep note of
    it's parents and/or children.
    
    This one represents a TestSuite
    '''
    def __init__(self, suite, parentItem):
        # The test suite for this tree item
        TreeItemBase.__init__(self, suite, parentItem)

    def columnCount(self):
        """Column count of the children?"""
        return 3
            
    #-----------------------------------------------------------------------------------            
    def is_checked(self):
        return [Qt.Unchecked, Qt.Checked][self.contents.get_selected()] 
    
    def set_checked(self, value):
        self.contents.set_selected( value )

    #-----------------------------------------------------------------------------------            
    def data(self, column):
        if self.contents is None:
            if column == 0:
                return QtCore.QVariant(self.header)
            if column >= 1:
                return QtCore.QVariant("")                
        else:
            if column == 0:
                return QtCore.QVariant(self.contents.name)
            if column == 1:
                return QtCore.QVariant(self.contents.get_state_str())
            if column == 2:
                return QtCore.QVariant( format_time( self.contents.get_runtime() ))
        return QtCore.QVariant()



# =======================================================================================    
class TreeItemSingle(TreeItemBase):
    '''
    A python object used to return row/column data, and keep note of
    it's parents and/or children.
    
    This one represents a TestSingle
    '''
    def __init__(self, test, parentItem):
        # The test single for this tree item
        TreeItemBase.__init__(self, test, parentItem)

    def columnCount(self):
        return 3
    
    def is_checked(self):
        """ Can't check at the single test level """
        return QVariant()
    
    def set_checked(self, value):
        """ Can't check at the single test level """
        pass
    
    def data(self, column):
        if self.contents is None:
            if column == 0:
                return QtCore.QVariant(self.header)
            if column >= 1:
                return QtCore.QVariant("")                
        else:
            if column == 0:
                return QtCore.QVariant(self.contents.name)
            if column == 1:
                return QtCore.QVariant(self.contents.get_state_str())
            if column == 2:
                return QtCore.QVariant( format_time( self.contents.runtime ) )
        return QtCore.QVariant()






# =======================================================================================    
class TestTreeModel(QtCore.QAbstractItemModel):
    '''
    A tree model that displays a hierarchy of TestProject.TestSuite.TestSingle
    '''
    #----------------------------------------------------------------------------------
    def __init__(self, parent=None):
        super(TestTreeModel, self).__init__(parent)
        # Make a root tree item
        self.rootItem = TreeItemProject(None, None)
        # Dictionary with key = project name; value = the TreeItemProject 
        self.projects = {}
        
        self.setupModelData()
        self.setup_checks()
        
        # Now set up the checkability
        
    #----------------------------------------------------------------------------------
    def flags(self, index):
        flag = Qt.ItemIsSelectable | Qt.ItemIsEnabled
        if index.column() == 0:
            flag |= Qt.ItemIsUserCheckable
        # Return the flags for the item: It is checkable, selectable and enabled.
        return  flag


    #----------------------------------------------------------------------------------
    def columnCount(self, parent=None):
        if parent and parent.isValid():
            return parent.internalPointer().columnCount()
        else:
            return len(HORIZONTAL_HEADERS)

    #----------------------------------------------------------------------------------
    def data(self, index, role):
        """ Request some data from the model. The role determines what you want
        """
        if not index.isValid():
            return QtCore.QVariant()

        item = index.internalPointer()
        # Return the data to display in the item
        if role == QtCore.Qt.DisplayRole:
            return item.data(index.column())
        
        # Return the checked state 
        if role == QtCore.Qt.CheckStateRole:
            if index.column() == 0:
                return item.is_checked()
            else:
                return QVariant() 
        
        # What background color?    
        if role == Qt.BackgroundRole:
            return item.background_color();
            
        #User role is used when directly querying the contents of the item
        if role == Qt.UserRole:
            if not item is None:
                return item.contents

        return QtCore.QVariant()


    #----------------------------------------------------------------------------------
    def setData(self, index, value, role):
        """ Sets some data (from the user)?
        The role determines what you want
        """
        if not index.isValid():
            return False
        item = index.internalPointer()
        if item is None:
            return False
        
        # Return the checked state
        if role == QtCore.Qt.CheckStateRole:
            item.set_checked( value==Qt.Checked )
            if isinstance(item, TreeItemProject):
                # self.modelReset()
                self.update_project(item.project.name)
                
            return True
        
        return False
#            if index.column() == 0:
#                return Qt.Checked
#            else:
#                return QVariant()

    #----------------------------------------------------------------------------------
    def get_project(self, project_name):
        """Return the QModelIndex, TreeItemProject of the project named."""
        for row in xrange(self.rootItem.childCount()):
            project_item = self.rootItem.child(row)
            if project_item.contents.name == project_name:
                # Matching the project - select all the stuff in it and update
                project_indx = self.index(row, 0, QModelIndex())
                return (project_indx, project_item)
        return (QModelIndex(), None)
            
    #----------------------------------------------------------------------------------
    def update_project(self, project_name):
        """ Updates the data displayed in a specific project"""
        # Matching the project - select all the stuff in it and update
        (project_indx, project_item) = self.get_project(project_name)
        topLeft = self.index(0, 0, project_indx)
        bottomRight = self.index( project_item.childCount()-1, 0, project_indx)
        self.emit( QtCore.SIGNAL("dataChanged(const QModelIndex &, const QModelIndex &)"), topLeft, bottomRight)

    #----------------------------------------------------------------------------------
    def update_all(self):
        """Updates all data in the tree (not for adding/removing tests """
        topLeft = self.index(0, 0, QModelIndex())
        bottomRight = self.index( self.rootItem.childCount()-1, 0, QModelIndex())
        self.emit( QtCore.SIGNAL("dataChanged(const QModelIndex &, const QModelIndex &)"), topLeft, bottomRight)
             
    #----------------------------------------------------------------------------------
    def update_suite(self, suite):
        """Updates the data for only the given suite. """
        # Index of the project
        (project_indx, project_item) = self.get_project( suite.get_parent().name )
        num_suites = project_item.childCount()
        for i in xrange(num_suites):
            if project_item.child(i).contents.name == suite.name:
                # Matching suite name in the same project name
                topLeft = self.index(i, 0, project_indx)
                bottomRight = self.index( i, 2, project_indx)
                self.emit( QtCore.SIGNAL("dataChanged(const QModelIndex &, const QModelIndex &)"), topLeft, bottomRight)
                # Only update one place
                return


    #----------------------------------------------------------------------------------
    def headerData(self, column, orientation, role):
        if (orientation == QtCore.Qt.Horizontal and
        role == QtCore.Qt.DisplayRole):
            try:
                return QtCore.QVariant(HORIZONTAL_HEADERS[column])
            except IndexError:
                pass

        return QtCore.QVariant()

    #----------------------------------------------------------------------------------
    def index(self, row, column, parent):
        if not self.hasIndex(row, column, parent):
            return QtCore.QModelIndex()

        if not parent.isValid():
            parentItem = self.rootItem
        else:
            parentItem = parent.internalPointer()

        childItem = parentItem.child(row)
        if childItem:
            return self.createIndex(row, column, childItem)
        else:
            return QtCore.QModelIndex()

    #-----------------------------------------------------------------------------------            
    def parent(self, index):
        if not index.isValid():
            return QtCore.QModelIndex()

        childItem = index.internalPointer()
        if not childItem:
            return QtCore.QModelIndex()
        
        parentItem = childItem.parent()

        if parentItem == self.rootItem:
            return QtCore.QModelIndex()

        return self.createIndex(parentItem.row(), 0, parentItem)

    #-----------------------------------------------------------------------------------            
    def rowCount(self, parent=QtCore.QModelIndex()):
        if parent.column() > 0:
            return 0
        if not parent.isValid():
            p_Item = self.rootItem
        else:
            p_Item = parent.internalPointer()
        return p_Item.childCount()
    
    #-----------------------------------------------------------------------------------            
    def setupModelData(self):
        for pj in test_info.all_tests.projects:
            # What's the corresponding TreeItemProject
            pj_item = None
            if self.projects.has_key( pj.name ):
                pj_item = self.projects[pj.name]
            else:
                # Make a new one
                pj_item = TreeItemProject(pj, self.rootItem)
                self.rootItem.appendChild(pj_item)
        
            # Now fill the suites
            if not pj_item is None:
                for suite in pj.suites:
                    if not suite is None:
                        suite_item = TreeItemSuite(suite, pj_item)
                        pj_item.appendChild(suite_item)
                        
                        # Now lets fill the TestSingle's
                        for test in suite.tests:
                            test_item = TreeItemSingle(test, suite_item)
                            suite_item.appendChild(test_item)
                            
        
    #-----------------------------------------------------------------------------------            
    def setup_checks(self):
        # Number of root projects
        rowcount = self.rowCount( QModelIndex() )
        for i in xrange(rowcount):
            # Expand each project-level one
            pj_indx = self.index(i, 0, QModelIndex() )
            # And go through the suites and collapse those
            num_suites = self.rowCount( pj_indx)
            for j in xrange(num_suites):
                suite_indx = self.index(j, 0, pj_indx )
                num_tests = self.rowCount( suite_indx )
                for k in xrange(num_tests):
                    test_indx = self.index(k, 0, suite_indx)
                    # Sets it as checked.
                    self.setData(test_indx, QtCore.Qt.Checked, QtCore.Qt.CheckStateRole);
        
                
#        
#    def searchModel(self, person):
#        '''
#        get the modelIndex for a given appointment
#        '''
#        def searchNode(node):
#            '''
#            a function called recursively, looking at all nodes beneath node
#            '''
#            for child in node.childItems:
#                if person == child.person:
#                    index = self.createIndex(child.row(), 0, child)
#                    return index
#                    
#                if child.childCount() > 0:
#                    result = searchNode(child)
#                    if result:
#                        return result
#        
#        retarg = searchNode(self.parents[0])
#        print retarg
#        return retarg
#            
#    def find_GivenName(self, fname):
#        app = None
#        for person in self.people:
#            if person.fname == fname:
#                app = person
#                break
#        if app != None:
#            index = self.searchModel(app)
#            return (True, index)            
#        return (False, None)



class TreeFilterProxyModel(QSortFilterProxyModel):
    #----------------------------------------------------------------------------------
    def __init__(self, parent=None):
        super(TreeFilterProxyModel, self).__init__(parent)
        self.failed_only = False
        self.selected_only = False
        
    #----------------------------------------------------------------------------------
    def set_filter_failed_only(self, value):
        """ Do we filter to show only the failed tests? """
        self.failed_only = value
        self.invalidateFilter()
        
    def set_filter_selected_only(self, value):
        """ Do we filter to show only the selected tests? """
        self.selected_only = value
        self.invalidateFilter()
        
    #----------------------------------------------------------------------------------
    def filterAcceptsColumn(self, source_col, parent_index):
        """ Always show all columns"""
        return True

    #----------------------------------------------------------------------------------
    def filterAcceptsRow(self, source_row, parent_index):
        """Return true if the row is accepted by the filter """

        # If we're not filtering by anything, return True 
        if not self.failed_only and not self.selected_only:
            return True
        
        source = self.sourceModel()
        # Get the index in the source model of this particular row
        index = source.index(source_row, 0, parent_index)
        # Return that particular item
        item = source.data(index, Qt.UserRole)
        if not item is None:
            if self.failed_only and (item.failed <= 0):
                return False
            if self.selected_only:
                if isinstance(item, TestProject) or  isinstance(item, TestSuite) :
                    return item.selected
                else:
                    # Don't filter out TestSingles based on selection (since they're all selected)
                    return True
            # Gets here if it passes through both filters 
            return True 
            
        return True
        

if __name__ == "__main__":    

    app = QtGui.QApplication([])
    
    test_info.all_tests.discover_CXX_projects("/home/8oz/Code/Mantid/Code/Mantid/bin/", "/home/8oz/Code/Mantid/Code/Mantid/Framework/")
    test_info.all_tests.make_fake_results()

    dialog = QtGui.QDialog()

    dialog.setMinimumSize(700,750)
    layout = QtGui.QVBoxLayout(dialog)

    tv = QtGui.QTreeView(dialog)


    
    tv.setAlternatingRowColors(True)
    layout.addWidget(tv)
    
    model = TestTreeModel()
    proxy = TreeFilterProxyModel()
    proxy.setDynamicSortFilter(True)
    proxy.setSourceModel(model)
    proxy.set_filter_failed_only(False)
    #proxy.setFilterWildcard("*G*")
    tv.setModel(proxy)
    
    dialog.show()
    app.exec_()
    #dialog.exec_()

    app.closeAllWindows()
