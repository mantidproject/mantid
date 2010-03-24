# 
# A simple python script to print the path to the correct site-packages directory
#
import sys
import os
import platform

sitepackages = os.path.join(sys.prefix,'lib')
if platform.architecture()[0] == "64bit":
    sitepackages += '64'
sitepackages = os.path.join(sitepackages,'python' + sys.version[:3],'site-packages')
print sitepackages
