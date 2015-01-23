import re


class ReflTestHarness:
    '''
    Test harness does common work associated with setting up the GUI environment for testing.
    '''
    
    def __init__(self):
        pass

    def setup_application(self):
        startApplication("MantidPlot_exe")
        
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
    
    def filter_out_workspaces(self, run_number):
        filtered = list()
        workspaces = self.list_from_workspace_list()
        for ws in workspaces:
            if re.search(str(run_number), str(ws)):
                filtered.append(str(ws))
        return filtered


def do_test_minimal(test_harness):
    '''Enter a single run number into the table and process it.'''

    run_number = 13460
    row_index = 0
    column_index = 0
    
    # Fetch the table
    tbl = waitForObject(":splitterList.tableMain_QTableWidget")
    
    # Set the run number
    tbl.item(row_index, column_index).setText(str(run_number))
    
    # Process everything
    test_harness.process_everything()
    
    # Get a handle to the workspace list
    out_workspaces = test_harness.list_from_workspace_list()
    
    out_workspaces_from_run = test_harness.filter_out_workspaces(run_number)
    
    # Check the output workspaces are in the workspace list
    test.verify(str(run_number) + '_IvsQ' in out_workspaces_from_run, '{0}_IvsQ should exist in output'.format(run_number))
    test.verify(str(run_number) +'_IvsLam' in out_workspaces_from_run,'{0}_IvsLam should exist in output'.format(run_number))
    test.verify('TOF' in out_workspaces, 'Should have a TOF group in the output')
    test.compare(len(out_workspaces), len(out_workspaces_from_run)+1, "All workspaces in output list should be accounted for")
    
    btn = waitForObject(':tableMain.Plot_QPushButton')
    test.verify(btn.isEnabled(), 'Should be able to plot the result')
    
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
    # Set the transmission runs
    tbl.item(row_index, 2).setText("{0},{1}".format(transmission_run1, transmission_run2))
    
    # Process everything
    test_harness.process_everything()
    
    # Get a handle to the workspace list
    out_workspaces = test_harness.list_from_workspace_list()
    
    # Check the output workspaces are in the workspace list
    test.verify(str(run_number) + '_IvsQ' in out_workspaces,'{0}_IvsQ should exist in output'.format(run_number))
    test.verify(str(run_number) +'_IvsLam' in out_workspaces,'{0}_IvsLam dshould exist in output'.format(run_number))
    test.verify('TRANS_{0}_{1}'.format(transmission_run1, transmission_run2) in out_workspaces,'Transmission workspace should exist in output')

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
    
    # Set the second run number
    tbl.item(row_index, 5).setText(str(run_number2))
    # Set the transmission runs AS their workspace name
    tbl.item(row_index, 2).setText('TRANS_{0}_{1}'.format(transmission_run1, transmission_run2))
    # Set the transmission runs AS their run numbers. Should be recycled anyway as the transmission workspace already exists.
    tbl.item(row_index, 7).setText('{0},{1}'.format(transmission_run1, transmission_run2))
    
    # Process everything
    test_harness.process_everything()
    
    # Get a handle to the workspace list
    out_workspaces = test_harness.list_from_workspace_list()
    
    # Check the output workspaces are in the workspace list
    test.verify(str(run_number1) + '_IvsQ' in out_workspaces,'{0}_IvsQ should exist in output'.format(run_number1))
    test.verify(str(run_number1) +'_IvsLam' in out_workspaces,'{0}_IvsLam should exist in output'.format(run_number1))
    test.verify(str(run_number2) + '_IvsQ' in out_workspaces,'{0}_IvsQ should exist in output'.format(run_number2))
    test.verify(str(run_number2) +'_IvsLam' in out_workspaces,'{0}_IvsLam should exist in output'.format(run_number2))
    test.verify('TRANS_{0}_{1}'.format(transmission_run1, transmission_run2) in out_workspaces,'Transmission workspace should exist in output')
    
def do_test_stitch_output(test_harness):
    '''Check stitching'''

    run_number1 = 13460
    run_number2 = 13462
    transmission_run1 = 13463
    transmission_run2 = 13464
    row_index = 0
    
    # Fetch the table
    tbl = waitForObject(":splitterList.tableMain_QTableWidget")
    
    # Set the stitched state
    ck_stitched = waitForObject(':tableMain_QCheckBox')
    ck_stitched.setChecked(True)
    
    # Set the first run number
    tbl.item(row_index, 0).setText(str(run_number1))
    
    # Set the second run number
    tbl.item(row_index, 5).setText(str(run_number2))
    # Set the transmission runs AS their workspace name
    tbl.item(row_index, 2).setText('TRANS_{0}_{1}'.format(transmission_run1, transmission_run2))
    # Set the transmission runs AS their run numbers. Should be recycled anyway as the transmission workspace already exists.
    tbl.item(row_index, 7).setText('TRANS_{0}_{1}'.format(transmission_run1, transmission_run2))
    
    # Process everything
    test_harness.process_everything()
    
    # Get a handle to the workspace list
    out_workspaces = test_harness.list_from_workspace_list()
    
    # Check the output workspaces are in the workspace list
    test.verify(str(run_number1) + '_IvsQ' in out_workspaces,'{0}_IvsQ should exist in output'.format(run_number1))
    test.verify(str(run_number1) +'_IvsLam' in out_workspaces,'{0}_IvsLam should exist in output'.format(run_number1))
    test.verify(str(run_number2) + '_IvsQ' in out_workspaces,'{0}_IvsQ should exist in output'.format(run_number2))
    test.verify(str(run_number2) +'_IvsLam' in out_workspaces,'{0}_IvsLam should exist in output'.format(run_number2))
    test.verify('TRANS_{0}_{1}'.format(transmission_run1, transmission_run2) in out_workspaces,'Transmission workspace should exist in output')    
    # Test that the stitched workspace is generated.
    test.verify('13460_62' in out_workspaces,'Stitched workspace should be generated')
    
def do_test_three_runs(test_harness):
    '''Check stitching'''

    run_number1 = 13460
    run_number2 = 13462
    run_number3 = 13463
    
    row_index = 0
    
    # Fetch the table
    tbl = waitForObject(":splitterList.tableMain_QTableWidget")
    
    # Set the first run number
    tbl.item(row_index, 0).setText(str(run_number1))
    
    # Set the second run number
    tbl.item(row_index, 5).setText(str(run_number2))
    
    # Set the thrid run number
    tbl.item(row_index, 10).setText(str(run_number3))
    # Give the third run a theta value
    tbl.item(row_index, 11).setText(str(1.0))
    
    # Process everything
    test_harness.process_everything()
    
    # Get a handle to the workspace list
    out_workspaces = test_harness.list_from_workspace_list()
    
    # Check the output workspaces are in the workspace list
    test.verify(str(run_number1) + '_IvsQ' in out_workspaces,'{0}_IvsQ should exist in output'.format(run_number1))
    test.verify(str(run_number1) +'_IvsLam' in out_workspaces,'{0}_IvsLam should exist in output'.format(run_number1))
    test.verify(str(run_number2) + '_IvsQ' in out_workspaces,'{0}_IvsQ should exist in output'.format(run_number2))
    test.verify(str(run_number2) +'_IvsLam' in out_workspaces,'{0}_IvsLam should exist in output'.format(run_number2))
    test.verify(str(run_number3) + '_IvsQ' in out_workspaces,'{0}_IvsQ should exist in output'.format(run_number3))
    test.verify(str(run_number3) +'_IvsLam' in out_workspaces,'{0}_IvsLam should exist in output'.format(run_number3))
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
    test_app.run_test(do_test_stitch_output)
    test_app.run_test(do_test_three_runs)
    
    # Clean up
    test_app.tear_down_application()
    
    
    
    
