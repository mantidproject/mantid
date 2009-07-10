import os
import buildNotification

print 'Current path == ',os.getcwd()
buildNotification.sendTestBuildStarted("Mantid")
os.system('Build\Winbuildscripts\BuildTests.bat')
buildNotification.sendTestBuildCompleted("Mantid")
buildNotification.sendTestStarted("Mantid")
os.system('Build\Winbuildscripts\ExecTests.bat')
