##############################################################
# Utilitly functions for custom script menus in MantidPlot 
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
# Variables concerning the location of the MantidPlot settings 
ORGANIZATION = "ISIS"
APPLICATION = "MantidPlot"
CUSTOMGROUP = "CustomScripts"

###
 # Add the Python scripts in the given directory to a menu in MantidPlot
 # dirpath - The path of the directory. Can be relative or absolute
 # menu_name - The name of the menu (optional). If not given, the directory name is used
###
def addScriptsDirToMenu(dirpath, menu_name=""):
  # Check function arguments
  if not os.path.isdir(dirpath):
    logMessage("Error: " + dirpath + " is not a directory.")
    return
  
  if menu_name == "":
    menu_name = dirpath

  # Construct a file list with paths that are correct
  fileList = []
  for i in os.listdir(dirpath):
    fileList.append(dirpath + '/' + i)
    
  addScriptsToMenu(fileList, menu_name)

###
 # Add a list of scripts a menu in MantidPlot
 # fileList - A list of paths to script files that can be either relative or absolute
 # menu_name - The name of the menu 
###
def addScriptsToMenu(fileList, menu_name):
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
    
  if os.name == 'nt':
    WIN_scriptsInstall(entries, menu_name)
  else:
    UNIX_scriptsInstall(entries, menu_name)
    
###
 # Remove scripts from a menu
 # fileList - A list menu entries to remove
 # menu_name - The menu to remove them from
###
def removeScriptsFromMenu(fileList, menu_name):
  if os.name == 'nt':
    WIN_scriptsRemove(fileList, menu_name)
  else:
    logMessage("Unix remove not implemented yet!!!!")

###
 # Remove all scripts in a directory from a menu
 # dirpath - The directory
 # menu_name - The name of the menu (optional)
###
def removeScriptsDirFromMenu(dirpath, menu_name = ""):
  # Check function arguments
  if not os.path.isdir(dirpath):
    logMessage("Error: " + dirpath + " is not a directory.")
    return

  if menu_name == "":
    menu_name = dirpath

    # Construct a file list 
  fileList = []
  for file in os.listdir(dirpath):
    if os.path.isdir(file):
      logMessage(file + " is a directory, skipping removal.")
      continue
    path_pieces = os.path.basename(file).split('.')
    if path_pieces[1].lower() != 'py':
      logMessage(file + " does not have a .py extension, skipping removal.")
      continue
    fileList.append(path_pieces[0])
    
  if os.name == 'nt':
    WIN_scriptsRemove(fileList, menu_name)
  else:
    logMessage("Unix remove not implemented yet!!!!")
    

################### Utility functions #################################
###
 # Output a log message
###
def logMessage(msg):
  print msg

###
 # The usage message
### 
def usage(scriptname):
  print "Import a set of scripts into a custom menu within MantidPlot.\n"\
  "Usage:\n"\
  "\t[1] " + scriptname + " menu_title [script1 script2 ...]\n"\
  "\t[2] " + scriptname + " directory\n\n"\
  "In form 1 the listed scripts will be imported into the menu with the given name and "\
  "in form 2 all of the .py files within the given directory will be imported into a menu of the same name.\n"
  
###
 # Import files on Windows
 # entries - A dictionary of (name:path) pairs 
 # menu_name - The name of the menu
###
def WIN_scriptsInstall(entries, menu_name):
  custompath = "Software\\" + ORGANIZATION + "\\" + APPLICATION + "\\" + CUSTOMGROUP + "\\"
  scriptskey = pywinreg.CreateKey(pywinreg.HKEY_CURRENT_USER,custompath + menu_name)
  for name,value in entries.iteritems():
    logMessage("Adding script file " + value + " to " + menu_name + " menu")
    pywinreg.SetValueEx(scriptskey, name, 0, pywinreg.REG_SZ, value)
  scriptskey.Close()
  
###
 # Remove script entries on Windows
 # entries - A list of menu entries to remove
 # menu_name - The name of the menu
###
def WIN_scriptsRemove(entries, menu_name):
  custompath = "Software\\" + ORGANIZATION + "\\" + APPLICATION + "\\" + CUSTOMGROUP
  try:
    scriptskey = pywinreg.OpenKey(pywinreg.HKEY_CURRENT_USER,custompath + "\\" + menu_name, 0, pywinreg.KEY_SET_VALUE)
  except EnvironmentError:
    logMessage("Error: Trying to remove an item from the " + menu_name + " menu, but the menu does not exist")
    return
      
  for name in entries:
    logMessage("Removing " + name + " item from  " + menu_name + " menu")
    pywinreg.DeleteValue(scriptskey, name)
  scriptskey.Close()
    
  # If there are no values left, remove the key
  menukey = pywinreg.OpenKey(pywinreg.HKEY_CURRENT_USER, custompath)
  subkeys, values, modified = pywinreg.QueryInfoKey(menukey)
  if values == 0:
    logMessage("The menu has no entries left, removing menu.")
    pywinreg.DeleteKey(menukey, menu_name);

###
 # Import files on Unix
 # entries - A dictionary of (name:path) pairs
 # menu_name - The name of the menu
 ###
def UNIX_scriptsInstall(entries, menu_name):
  # The QSettings object needs to know the ORGANIZATION name and APPLICATION name
  settings = QtCore.QSettings(ORGANIZATION, APPLICATION)
  settings.beginGroup(CUSTOMGROUP + '/' + menu_name)
  for name, value in entries.iteritems():
    logMessage("Adding script file " + value + " to " + menu_name + " menu")
    settings.setValue(name, QtCore.QVariant(value))
  settings.endGroup()
