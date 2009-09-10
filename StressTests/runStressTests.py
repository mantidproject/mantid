#!/usr/bin/env python

# Define some necessary paths
stressmodule_dir = '../Code/StressTestFramework'
mtdpy_header_dir = '../Code/Mantid/Bin/Shared'
tests_dir = '../StressTests/MantidScript'

# Import the stress manager definition
import sys
sys.path.append(stressmodule_dir)
import stresstesting

# Execute the tests, telling it where Mantid is
mgr = stresstesting.TestManager(tests_dir, mtdpy_header_dir)
mgr.executeAllTests()
