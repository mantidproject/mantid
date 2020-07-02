# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
'''
Data object for a TestResult
'''

import sys
import os
import reporters
import re
import time
import datetime
import platform
import subprocess
import tempfile
import sqlresults
import numpy as np


# Copy of that in systemtesting.py as there is no place to share it easily and its tiny and unlikely to change
def linux_distro_description():
    """Human readable string for linux distro description
    """
    try:
        lsb_descr = subprocess.check_output('lsb_release --description', shell=True,
                                            stderr=subprocess.STDOUT).decode('utf-8')
        return lsb_descr.strip()[len('Description:')+1:].strip()
    except subprocess.CalledProcessError as exc:
        return f'Unknown distribution: lsb_release -d failed {exc}'


def envAsString():
    platform_name = sys.platform
    if platform_name == 'win32':
        system = platform.system().lower()[:3]
        arch = platform.architecture()[0][:2]
        env = system + arch
    elif platform_name == 'darwin':
        env = platform.mac_ver()[0]
    else:
        # assume linux
        env = linux_distro_description()
    return env
    
    
#########################################################################
# A class to store the results of a test 
#########################################################################
class TestResult(object):
    '''
    Stores the results of each test so that they can be reported later.
    '''
    
    def __init__(self, 
                 date = datetime.datetime.now(),
                 name="",
                 type="system",
                 host=platform.uname()[1],
                 environment=envAsString(),
                 runner="",
                 commitid='',
                 revision=0,
                 runtime=0.0,
                 speed_up=0.0,
                 cpu_fraction=0.0,
                 memory_change=0, 
                 iterations=1,
                 success=False,
                 status="",
                 log_contents="",
                 variables=""):
        """ Fill the TestResult object with the contents """
        self.data = {}
        self.data["date"] = date
        self.data["name"] = name
        self.data["type"] = type
        self.data["host"] = host
        self.data["environment"] = environment
        self.data["runner"] = runner
        self.data["revision"] = revision
        self.data["commitid"] = commitid
        self.data["runtime"] = runtime
        self.data["cpu_fraction"] = cpu_fraction
        self.data["memory_change"] = memory_change
        self.data["success"] = success
        self.data["status"] = status
        self.data["log_contents"] = log_contents
        self.data["variables"] = variables
        
    
    def get_logarchive_filename(self):
        "Return a bare filename that will hold the archived log contents"
        s = str(self.data["date"])
        s = s.replace(" ", "_")
        s = s.replace(":", "-")
        return "%s.%s.log" % (s, self.data["name"])
        
    def __getitem__(self, key):
        return self.data[key]
    
    def __setitem__(self, key, value):
        self.data.__setitem__(key, value)
            
    def getData(self):
        ''' Get the map storing the results   '''
        return self.data
    
    def __str__(self):
        return str(self.data)


    
