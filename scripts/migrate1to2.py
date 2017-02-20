"""
Migrates scripts from version 1 of Mantid's Python API
to version 2.
"""
import lib1to2.main as main

import sys

sys.exit(main.main(sys.argv[1:]))
