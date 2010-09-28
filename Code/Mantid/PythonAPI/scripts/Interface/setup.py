#!/usr/bin/env python
import os

from distutils.core import setup

# Compile resource file
try:
    os.system("pyrcc4 -o reduction_gui/settings/qrc_resources.py reduction_gui/settings/resources.qrc")
except:
    print "Could not compile resource file"
    
setup(name='SANSReduction',
      version='1.0',
      description='SANS Reduction for Mantid',
      author='Mathieu Doucet',
      author_email='doucetm@ornl.gov',
      url='http://www.mantidproject.org',
      packages=['reduction_gui', 'reduction_gui.widgets', 'reduction_gui.instruments', 
                'reduction_gui.reduction', 'reduction_gui.settings'],
      package_dir={'reduction_gui' : '',
                   'reduction_gui.widgets' : 'reduction_gui/widgets',
                   'reduction_gui.instruments' : 'reduction_gui/instruments',
                   'reduction_gui.settings' : 'reduction_gui/settings',
                   'reduction_gui.reduction' : 'reduction_gui/reduction'}
     )
