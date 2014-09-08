source(findFile("scripts", "global_helpers.py"))

import os
def open_file(filename):
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

    # Now, click down the file path. CMake uses Linux slashes on Windows too.
    dirs = dirname.split('/')
    for dir in dirs:
        # Left: Linux, Right: Windows
        if '' == dir or ':' in dir:
            continue
        dir = fix_slashes(dir)
        #test.log("Clicking Dir %s" % dir)
        waitForObjectItem(":Open File:  (open multiple files with <ctrl> key.).Files_QTreeView", dir)
        doubleClickItem(":Open File:  (open multiple files with <ctrl> key.).Files_QTreeView", dir, 27, 11, 0, Qt.LeftButton)

    fileDialog_FileName = waitForObject(":Open File:  (open multiple files with <ctrl> key.).FileName_QLineEdit")
    fileDialog_FileName.setText(filename)
    fileDialog_OkButton = waitForObject(":Open File:  (open multiple files with <ctrl> key.).OK_QPushButton")
    clickButton(fileDialog_OkButton)

def quit_program():
    activateItem(waitForObjectItem(":_QMenuBar", "File"))
    activateItem(waitForObjectItem(":File_QMenu", "Exit"))

def switch_mode(mode):
    clickButton(":%sButton_QPushButton" % mode)

def set_ptw_lineedit_property(value, property, ext=""):
    if ext != "":
        ext = "_" + ext
    object = ":ScrollArea.%s_QLineEdit%s" % (property, ext)
    lineedit = waitForObject(object)
    N = lineedit.text.length()
    for i in range(N):
        lineedit.cursorBackward(True)
        type(lineedit, "<Del>")
    lineedit.text = str(value)
