"""
    When run within Mantiplot, this script should print SUCCESS
"""
from TestClass import *

if TestClass.CLASS_DATA_MEMBER==1:
    print "SUCCESS"
else: 
    print "Complete failure"
