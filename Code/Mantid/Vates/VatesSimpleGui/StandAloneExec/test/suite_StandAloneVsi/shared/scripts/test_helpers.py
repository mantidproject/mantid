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
    #mouseClick(lineedit, 1, 1, 0, Qt.LeftButton)
    #lineedit.text = str(value)
    lineedit.clear()
    type(lineedit, value)
    #lineedit.setText(str(value))

def apply_ptw_settings():
    clickButton(waitForObject(":objectInspector.Apply_QPushButton"))

def make_slice(axisScaleName, coordinate):
    axisScale = waitForObject(":splitter_2.%s_Mantid::Vates::SimpleGui::AxisInteractor" % axisScaleName)
    ext = None
    if axisScaleName[0] == "x":
        ext = ""
    if axisScaleName[0] == "y":
        ext = "_2"
    if axisScaleName[0] == "z":
        ext = "_3"
        
    scaleWidget = waitForObject(":splitter_2_QwtScaleWidget%s" % ext)
        
    sp = axisScale.scalePosition
    min = axisScale.getMinimum
    max = axisScale.getMaximum
    delta = max - min
    width = -1
    height = -1
    if sp in (0, 1):
        width = scaleWidget.width
        height = axisScale.height
    else:
        width = scaleWidget.height
        height = axisScale.width

    scaleFactor = delta/height
    #test.log("%s: %d, %d" % (axisScaleName, width, height))
    y = height / 2
    
    if sp in (0, 2):
        x = 1
    else:
        x = width - 1
    
    if sp in (0, 1):
        mouseClick(scaleWidget, x, y, 0, Qt.LeftButton)
    else:
        mouseClick(scaleWidget, y, x, 0, Qt.LeftButton)
