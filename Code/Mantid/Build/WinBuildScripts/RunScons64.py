import os
import buildNotification

print 'Current path == ',os.getcwd()
buildNotification.sendBuildStarted("Mantid")
os.system('Build\Winbuildscripts\RunScons64.bat')
buildNotification.sendBuildCompleted("Mantid")
