'''
Data object for a TestResult

Copyright &copy; 2009 STFC Rutherford Appleton Laboratories

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>.
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




#########################################################################
#########################################################################
def envAsString():
    """ Return the environment as a string """
    if os.name == 'nt':
        system = platform.system().lower()[:3]
        arch = platform.architecture()[0][:2]
        env = system + arch
    elif os.name == 'mac':
        env = platform.mac_ver()[0]
    else:
        env = " ".join(platform.dist())
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


    
