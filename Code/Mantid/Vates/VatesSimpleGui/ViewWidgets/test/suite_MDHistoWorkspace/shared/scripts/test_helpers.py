import os
def run_script(sname):
    ctx = currentApplicationContext()
    dirname = ctx.environmentVariable("SCRIPTS_DIR")
    script_name = os.path.join(dirname, sname)
    
    activateItem(waitForObjectItem(":MantidPlot - untitled_QMenuBar", "View"))
    activateItem(waitForObjectItem(":MantidPlot - untitled.View_QMenu", "Script Window"))
    activateItem(waitForObjectItem(":MantidPlot: Python Window_QMenuBar", "File"))
    activateItem(waitForObjectItem(":MantidPlot: Python Window.File_QMenu", "Open"))
 
    fileDialog_FileName = waitForObject(":fileNameEdit_QLineEdit")
    fileDialog_FileName.setText(script_name)
 
    clickButton(waitForObject(":MantidPlot - Open a script from a file.Open_QPushButton"))
    activateItem(waitForObjectItem(":MantidPlot: Python Window_QMenuBar", "Execute"))
    activateItem(waitForObjectItem(":MantidPlot: Python Window.Execute_QMenu", "Execute"))
    waitForObject(":MantidPlot: Python Window.Script Output - Status: Stopped_ScriptOutputDock")
    
# This function is here to "fix" the workspace names for Squish's kind of silly convention.
def fix_slashes(ws):
    return ws.replace('_', '\\_')

def get_workspace(workspace_name):
    workspace_tree = waitForObject(":Workspaces.WorkspaceTree_MantidTreeWidget")
    openItemContextMenu(workspace_tree, fix_slashes(workspace_name), 10, 10, 0)

def get_action(widget, text):
    actions = widget.actions()
    for i in range(actions.count()):
        action = actions.at(i)
        if action.text == text:
            return action

def get_vsi():
    return waitForObjectItem(":MantidPlot - untitled.WorkspaceContextMenu_QMenu", 
                             "Show Vates Simple Interface")

def activate_vsi():
    vsi = get_vsi();
    activateItem(vsi)
    
def switch_mode(mode):
    clickButton(waitForObject(":Vates Simple Interface.%sButton_QPushButton" % mode))
    
def close_vsi():
    sendEvent("QCloseEvent", waitForObject(":Vates Simple Interface_QMdiSubWindow"))
    
def activate_sv(inumber):
    if inumber > 1:
        snum = "_%d" % inumber
    else:
        snum = ""
    openContextMenu(waitForObject(":_QGraphicsItem%s" % snum), 5, 5, 0)
    activateItem(waitForObjectItem(":_QMenu", "Show in SliceView"))
    
def quit_program():
    sendEvent("QCloseEvent", waitForObject(":MantidPlot - untitled_ApplicationWindow"))
