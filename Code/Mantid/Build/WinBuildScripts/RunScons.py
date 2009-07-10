import os
import buildNotification

print 'Current path == ',os.getcwd()
buildNotification.sendBuildStarted("Mantid")
os.system('Build\Winbuildscripts\RunScons.bat')
buildNotification.sendBuildCompleted("Mantid")
