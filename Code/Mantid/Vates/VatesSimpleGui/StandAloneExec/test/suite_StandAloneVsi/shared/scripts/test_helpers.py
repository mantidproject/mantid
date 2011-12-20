import os
def openFile(filename):
    activateItem(waitForObjectItem(":_QMenuBar", "File"))
    activateItem(waitForObjectItem(":File_QMenu", "Open"))
    ctx = currentApplicationContext()
    dirname = ctx.environmentVariable("MANTID_AUTOTEST_DATA")
    #test.log("Data Dir: %s" % dirname)
    fileDialog = waitForObject(":Open File:  (open multiple files with <ctrl> key.)_pqFileDialog")

    fileDialog_NavigateUp = waitForObject(":Open File:  (open multiple files with <ctrl> key.).NavigateUp_QToolButton")
    fileDialog_Files = waitForObject(":Open File:  (open multiple files with <ctrl> key.).Files_QTreeView")
    fileDialog_Parents = waitForObject(":Open File:  (open multiple files with <ctrl> key.).Parents_QComboBox")
    # Reset to top-level directory
    count = fileDialog_Parents.count
    for i in range(count):
        clickButton(fileDialog_NavigateUp)
    
    # Now, click down the file path
    dirs = dirname.split(os.path.sep)
    for dir in dirs:
        if '' == dir:
            continue
        #test.log("Clicking Dir %s" % dir)
        waitForObjectItem(":Open File:  (open multiple files with <ctrl> key.).Files_QTreeView", dir)
        doubleClickItem(":Open File:  (open multiple files with <ctrl> key.).Files_QTreeView", dir, 27, 11, 0, Qt.LeftButton)
    
    fileDialog_FileName = waitForObject(":Open File:  (open multiple files with <ctrl> key.).FileName_QLineEdit")
    fileDialog_FileName.setText(filename)
    fileDialog_OkButton = waitForObject(":Open File:  (open multiple files with <ctrl> key.).OK_QPushButton")
    clickButton(fileDialog_OkButton)

def quitProgram():
    activateItem(waitForObjectItem(":_QMenuBar", "File"))
    activateItem(waitForObjectItem(":File_QMenu", "Exit"))
    
def switch_mode(mode):
    clickButton(waitForObject(":%sButton_QPushButton" % mode))

def set_ptw_lineedit_property(object, value):
    lineedit = waitForObject(object)
    lineedit.setText(str(value))

def apply_ptw_settings():
    clickButton(waitForObject(":objectInspector.Apply_QPushButton"))
    
def pause(seconds=1):
    import time
    time.sleep(seconds)
