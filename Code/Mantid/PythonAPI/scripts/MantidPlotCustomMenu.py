###########################################################
# Import settings for custom menu entries in MantidPlot
# using the QSettings class in PyQt to hide the differences
# in OS behaviour
#
# Usage: MantidPlotCustomMenu menu-title script1 script2 script3 ...
# 
################   Module imports  ###################################
import sys
import os.path
# QSettings is defined here but this doesn't seem to work in windows
if os.name == 'nt':
  import _winreg as pywinreg
else:
  from PyQt4 import QtCore
  # This defines some extra types for the object
  from PyQt4 import QtGui

################  Function Definitions   ##############################

def usage(scriptname):
  print "Import a set of scripts into a custom menu within MantidPlot.\n"\
  "Usage:\n"\
  "\t[1] " + scriptname + " menu_title [script1 script2 ...]\n"\
  "\t[2] " + scriptname + " directory\n\n"\
  "In form 1 the listed scripts will be imported into the menu with the given name and "\
  "in form 2 all of the .py files within the given directory will be imported into a menu of the same name.\n"
  
def logMessage(msg):
  print msg

def WindowsImport(menu_name, entries):
  custompath = "Software\\" + organization + "\\" + application + "\\" + customgroup + "\\"
  scriptskey = pywinreg.CreateKey(pywinreg.HKEY_CURRENT_USER,custompath + menu_name)
  for name,value in entries.iteritems():
    pywinreg.SetValueEx(scriptskey, name, 0, pywinreg.REG_SZ, value)
  scriptskey.Close()

def UnixImport(menu_name, entries):
  # The QSettings object needs to know the organization name and application name
  settings = QtCore.QSettings(organization, application)
  settings.beginGroup(customgroup + '/' + menu_name)
  for name, value in entries.iteritems():
    settings.setValue(name, QtCore.QVariant(value))
  settings.endGroup()
  
 
################ Script Entry Point  ##############################
# Variables concerning the location of the MantidPlot settings 
organization = "ISIS"
application = "MantidPlot"
customgroup = "CustomScripts"

scriptname = os.path.basename(sys.argv[0])

# Hack off script name from the arguments list
args = sys.argv[1:]
# Help message
if len(args) < 1 or args[0] == "--help":
  usage(scriptname)
  exit(0)

menuName = args[0]
args = args[1:]
fileList = []
if len(args) == 0 and os.path.isdir(menuName):
  dirfiles = os.listdir(menuName)
  for i in dirfiles:
    fileList.append(menuName + '/' + i)
else:
  fileList = args;

# Create dictionary of name-path pairs
entries = {}
for file in fileList:
  if not os.path.exists(os.path.abspath(file)):
    logMessage(file + " does not exist, skipping.")
    continue
  if os.path.isdir(file):
    logMessage(file + " is a directory, skipping.")
    continue
  path_pieces = os.path.basename(file).split('.')
  if path_pieces[1].lower() != 'py':
    logMessage(file + " does not have a .py extension, skipping.")
    continue
  name = path_pieces[0]
  entries[name] = os.path.abspath(file)
  logMessage("Storing script file " + os.path.abspath(file))
 
if os.name == 'nt':
   WindowsImport(menuName, entries)
else:
   UnixImport(menuName, entries)
