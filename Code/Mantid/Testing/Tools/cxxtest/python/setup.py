"""
Script to generate the installer for cxxtest.
"""

classifiers = """\
Development Status :: 4 - Beta
Intended Audience :: End Users/Desktop
License :: OSI Approved :: LGPL License
Natural Language :: English
Operating System :: Microsoft :: Windows
Operating System :: Unix
Programming Language :: Python
Topic :: Software Development :: Libraries :: Python Modules
"""

import cxxtest
import glob
import os

def _find_packages(path):
    """
    Generate a list of nested packages
    """
    pkg_list=[]
    if not os.path.exists(path):
        return []
    if not os.path.exists(path+os.sep+"__init__.py"):
        return []
    else:
        pkg_list.append(path)
    for root, dirs, files in os.walk(path, topdown=True):
      if root in pkg_list and "__init__.py" in files:
         for name in dirs:
           if os.path.exists(root+os.sep+name+os.sep+"__init__.py"):
              pkg_list.append(root+os.sep+name)
    return map(lambda x:x.replace(os.sep,"."), pkg_list)

try:
    from setuptools import setup
except ImportError:
    from distutils.core import setup
packages = _find_packages('cxxtest')

scripts = glob.glob("scripts/*")
doclines = cxxtest.__doc__.split("\n")

setup(name="cxxtest",
      version=cxxtest.__version__,
      maintainer=cxxtest.__maintainer__,
      maintainer_email=cxxtest.__maintainer_email__,
      url = cxxtest.__url__,
      license = cxxtest.__license__,
      platforms = ["any"],
      description = doclines[0],
      classifiers = filter(None, classifiers.split("\n")),
      long_description = "\n".join(doclines[2:]),
      packages=packages,
      keywords=['utility'],
      scripts=scripts
      )

