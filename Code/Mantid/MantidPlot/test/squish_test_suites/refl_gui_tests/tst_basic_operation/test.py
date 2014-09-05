


class ReflTestHarness:
    '''
    Test harness does common work associated with setting up the GUI environment for testing.
    '''
    
    def __init__(self):
        pass

    def setup_application(self):
        startApplication("MantidPlot")
        
    def tear_down_application(self):
        sendEvent("QCloseEvent", waitForObject(":MantidPlot - untitled_ApplicationWindow_2"))
         
    def __setup(self):
        activateItem(waitForObjectItem(":_QMenuBar", "Interfaces"))
        activateItem(waitForObjectItem(":MantidPlot - untitled.Interfaces_QMenu", "Reflectometry"))
        refl_gui = waitForObjectItem(":Interfaces.Reflectometry_QMenu", "ISIS Reflectometry")
        activateItem(refl_gui)

    def process_everything(self):
        # Hit process
        clickButton(waitForObject(":splitterList.Process_QPushButton"))
        # Agree to process everything
        setWindowState(waitForObject(":_QMessageBox"), WindowState.Normal)
        clickButton(waitForObject(":tableMain.Yes_QPushButton"))
        
    def __teardown(self):
        sendEvent("QCloseEvent", waitForObject(":ISIS Reflectometry_ReflGui"))
        dont_save_name = ":Don't Save_QPushButton"
        if(object.exists(dont_save_name)):
            clickButton(waitForObject(dont_save_name))
        else:
            clickButton(waitForObject(":Discard_QPushButton"))
        
        
        
    def run_test(self,test_func):
        self.__setup()
        test_func(self)
        self.__teardown()
        
    def list_from_workspace_list(self):
        item_names = list()
        workspace_tree = waitForObject(":Workspaces.WorkspaceTree_MantidTreeWidget")
        
        topItem = workspace_tree.topLevelItem(0)
        item = topItem
        i = 0
        while i < workspace_tree.topLevelItemCount:
            item_names.append(item.text(0))
            item = workspace_tree.itemBelow(item)
            i += 1
            
        return item_names


def do_test_minimal(test_harness):
    '''Enter a single run number into the table and process it.'''

    run_number = 13460
    row_index = 0
    column_index = 0
    
    # Fetch the table
    tbl = waitForObject(":splitterList.tableMain_QTableWidget")
    
    # Set the run number
    tbl.item(row_index, column_index).setText(str(run_number))
    
    tbl.item(row_index, column_index+1).setText(str(0.1)) # HACK!!!!!!!!
    
    # Process everything
    test_harness.process_everything()
    
    # Get a handle to the workspace list
    out_workspaces = test_harness.list_from_workspace_list()
    
    # Check the output workspaces are in the workspace list
    test.verify(str(run_number) + '_IvsQ' in out_workspaces,'{run_number}_IvsQ does not exist in output')
    test.verify(str(run_number) +'_IvsLam' in out_workspaces,'{run_number}_IvsLam does not exist in output')
    
def do_test_transmission(test_harness):
    '''Enter a single run with transmission correction applied.'''
    run_number = 13460
    transmission_run1 = 13463
    transmission_run2 = 13464
    row_index = 0
    
    # Fetch the table
    tbl = waitForObject(":splitterList.tableMain_QTableWidget")
    
    # Set the run number
    tbl.item(row_index, 0).setText(str(run_number))
    tbl.item(row_index, 1).setText(str(0.1)) # HACK!!!!!!!!
    # Set the transmission runs
    tbl.item(row_index, 2).setText("{0},{1}".format(transmission_run1, transmission_run2))
    
    # Process everything
    test_harness.process_everything()
    
    # Get a handle to the workspace list
    out_workspaces = test_harness.list_from_workspace_list()
    
    # Check the output workspaces are in the workspace list
    test.verify(str(run_number) + '_IvsQ' in out_workspaces,'{run_number}_IvsQ does not exist in output')
    test.verify(str(run_number) +'_IvsLam' in out_workspaces,'{run_number}_IvsLam does not exist in output')
    test.verify('TRANS_{0}_{1}'.format(transmission_run1, transmission_run2) in out_workspaces,'Transmission workspace does not exist in output')

def do_test_reuse_transmission(test_harness):
    '''Enter two runs and recycle the transmission runs in two different ways. 
    Depends upon the output from the previous test which created the transmission runs'''

    run_number1 = 13460
    run_number2 = 13462
    transmission_run1 = 13463
    transmission_run2 = 13464
    row_index = 0
    
    # Fetch the table
    tbl = waitForObject(":splitterList.tableMain_QTableWidget")
    
    # Set the first run number
    tbl.item(row_index, 0).setText(str(run_number1))
    
    tbl.item(row_index, 1).setText(str(0.1)) # HACK!!!!!!!!
    # Set the second run number
    tbl.item(row_index, 5).setText(str(run_number2))
    tbl.item(row_index, 6).setText(str(0.1)) # HACK!!!!!!!!
    # Set the transmission runs AS their workspace name
    tbl.item(row_index, 2).setText('TRANS_{0}_{1}'.format(transmission_run1, transmission_run2))
    # Set the transmission runs AS their run numbers. Should be recycled anyway as the transmission workspace already exists.
    tbl.item(row_index, 7).setText('{0},{1}'.format(transmission_run1, transmission_run2))
    
    # Process everything
    test_harness.process_everything()
    
    # Get a handle to the workspace list
    out_workspaces = test_harness.list_from_workspace_list()
    
    # Check the output workspaces are in the workspace list
    test.verify(str(run_number1) + '_IvsQ' in out_workspaces,'{run_number}_IvsQ does not exist in output')
    test.verify(str(run_number1) +'_IvsLam' in out_workspaces,'{run_number}_IvsLam does not exist in output')
    test.verify(str(run_number2) + '_IvsQ' in out_workspaces,'{run_number}_IvsQ does not exist in output')
    test.verify(str(run_number2) +'_IvsLam' in out_workspaces,'{run_number}_IvsLam does not exist in output')
    test.verify('TRANS_{0}_{1}'.format(transmission_run1, transmission_run2) in out_workspaces,'Transmission workspace does not exist in output')
   

def main():
    # Create the test harness
    test_app = ReflTestHarness()
    # Get the application ready
    test_app.setup_application()
    
    '''
    Run tests
    '''
    test_app.run_test(do_test_minimal)
    test_app.run_test(do_test_transmission)
    test_app.run_test(do_test_reuse_transmission)
    
    # Clean up
    test_app.tear_down_application()
    
    
    
    
