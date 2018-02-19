#!/usr/bin/env python
""" This module checks a SQL database
to determine whether performance in a particular test has dropped.
If so, it prints out an error message and exits with a return
code, causing the build to fail.
"""

import argparse
import sys
import os
import sqlresults
from testresult import TestResult, envAsString
from xml.dom.minidom import parse, parseString
import re
import time
import datetime
import platform
import subprocess
import tempfile
import sqlresults
import numpy as np
try:
    import analysis
except ImportError:
    import analysis_mpl as analysis
import secureemail


#====================================================================================
def run(args):
    """ Execute the program """
    print
    print "=============== Checking For Performance Loss ====================="
    dbfile = args.db[0]

    if not os.path.exists(dbfile):
        print "Database file %s not found."
        sys.exit(1)

    # Set the database to the one given
    sqlresults.set_database_filename(dbfile);

    # Convert the arguments. Will throw if the user is stupid.
    avg = int(args.avg)
    tol = float(args.tol)
    rev = sqlresults.get_latest_revison()

    print "Comparing the average of the %d revisions before rev. %d. Tolerance of %g %%." % (avg, rev, tol)
    if args.verbose: print

    # For limiting the results
    limit = 50*avg;

    names = sqlresults.get_all_test_names("revision = %d" % rev)
    if len(names) == 0:
        print "Error! No tests found at revision number %d.\n" % rev
        sys.exit(1)

    bad_results = ""
    good_results = ""
    num_same = 0
    num_good = 0
    num_bad = 0
    num_notenoughstats=0
    # The timing resolution is different across platforms and the faster tests
    # can cause more false positives on the lower-resolution clocks. We'll
    # up the tolerance for those taking less time than 10ms.
    timer_resolution_hi = 0.01
    timer_resolution_lo = 0.0011
    
    regression_names = []
    speedup_names = []

    for name in names:
        (r, t) = analysis.get_runtime_data(name, x_field='revision')
        r = np.array(r)
        t = np.array(t)

        # this is the timing of the current revision
        current_time = t[r == rev]
        tolerance = tol
        if current_time < timer_resolution_hi:
            # Increase the tolerance to avoid false positives
            if current_time < timer_resolution_lo:
                # Very fast tests are twitchy
                tolerance = 100.0
            else:
                tolerance = 70.0

        # Cut out any times after or = to the current rev
        t = t[r < rev]

        # Keep the latest "avg" #
        t = t[len(t)-avg:]

        if (len(t) == avg):
            # This is the average
            baseline_time = np.mean(t)

            # % speed change
            pct = ((baseline_time / current_time) - 1) * 100

            # Did we fail (slow down too much)
            if pct < -tolerance:
                regression_names.append(name)
            elif pct > tolerance:
                speedup_names.append(name)
    
    regLinks = ["http://builds.mantidproject.org/job/master_performancetests2/Master_branch_performance_tests/{}.htm".format(name) for name in regression_names]
    speedLinks = ["http://builds.mantidproject.org/job/master_performancetests2/Master_branch_performance_tests/{}.htm".format(name) for name in speedup_names]
    email = secureemail.SendEmailSecure(args.sender, args.pwd, args.recipient, regLinks, speedLinks)
    email.send()

#====================================================================================
if __name__ == "__main__":
    # Parse the command line
    parser = argparse.ArgumentParser(description='Reads the SQL database containing performance test results and checks that performance has not dropped.')

    parser.add_argument('db', metavar='DBFILE', type=str, nargs=1,
                        default="./MantidSystemTests.db",
                        help='Full path to the SQLite database holding the results (default "./MantidSystemTests.db"). ')
                        
    parser.add_argument('sender', type=str, default="mantidproject@gmail.com",
                        help='Gmail email address')

    parser.add_argument('pwd', type=str, help='password for gmail address')
    parser.add_argument('recipient', type=str, help='recipient email address')

    parser.add_argument('--avg', dest='avg', type=int, default="5",
                        help='Average over this many previous revisions to find a baseline. Default 5.')

    parser.add_argument('--tol', dest='tol', type=float, default="20",
                        help='Percentage tolerance; speed loss beyond this %% will give a warning. Default 20%%.')

    parser.add_argument('--verbose', dest='verbose', action='store_const',
                        const=True, default=False,
                        help='For full reporting of each timing.')

    args = parser.parse_args()

    run(args)