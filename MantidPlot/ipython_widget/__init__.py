# Make sure we pick up out local IPython, before any system-installed one
import sys
from os import path
sys.path.insert(0,path.split(path.dirname(__file__))[0])

from mantid_ipython_widget import *
