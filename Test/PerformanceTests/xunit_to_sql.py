#!/usr/bin/env python
""" Module to convert XUnit XML to SQL database of test results of the same type used
by python system tests """

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
import glob

# Global SQL result reporter
sql_reporter = None
# Variables string for all tests
variables = ""
revision = 0
commitid = ''

def handle_testcase(case, suite_name):
    """ Handle one test case and save it to DB"""
    # Build the full name (Project.Suite.Case)
    name = case.getAttribute("classname") + "." + case.getAttribute("name")
    try:
        time = float(case.getAttribute("time"))
    except:
        time = 0.0
    try:
        total_time = float(case.getAttribute("totalTime"))
    except:
        total_time = 0.0
    try:
        cpu_fraction = float(case.getAttribute("CPUFraction"))
    except:
        cpu_fraction = 0.0


    tr = TestResult(date = datetime.datetime.now(),
                 name=name,
                 type="performance",
                 host=platform.uname()[1],
                 environment=envAsString(),
                 runner="ctest",
                 revision=revision,
                 commitid=commitid,
                 runtime=time,
                 cpu_fraction=cpu_fraction,
                 success=True,
                 status="",
                 log_contents="",
                 variables=variables)
    #print tr.data
    # Now report it to SQL
    sql_reporter.dispatchResults(tr)

def handle_suite(suite):
    """ Handle all the test cases in a suite """
    suite_name = suite.getAttribute("name")
    cases = suite.getElementsByTagName("testcase")
    for case in cases:
        handle_testcase(case, suite_name)


def convert_xml(filename):
    """Convert a single XML file to SQL db"""
    # Parse the xml
    print "Reading", filename
    doc = parse(filename)
    suites = doc.getElementsByTagName("testsuite")
    for suite in suites:
        handle_suite(suite)


#====================================================================================
if __name__ == "__main__":
    # Parse the command line
    parser = argparse.ArgumentParser(description='Add the contents of Xunit-style XML test result files to a SQL database.')

    parser.add_argument('--db', dest='db',
                        default="./MantidPerformanceTests.db",
                        help='Full path to the SQLite database holding the results (default "./MantidPerformanceTests.db"). The database will be created if it does not exist.')

    parser.add_argument('--variables', dest='variables',
                        default="",
                        help='Optional string of comma-separated "VAR1NAME=VALUE,VAR2NAME=VALUE2" giving some parameters used, e.g. while building.')

    parser.add_argument('--commit', dest='commitid',
                        default="",
                        help='Commit ID of the current build (a 40-character SHA string).')

    parser.add_argument('xmlpath', metavar='XMLPATH', type=str, nargs='+',
                        default="",
                        help='Required: Path to the Xunit XML files.')

    args = parser.parse_args()

    # Setup the SQL database but only if it does not exist
    sqlresults.set_database_filename(args.db)
    if not os.path.exists(args.db):
        sqlresults.setup_database()
    # Set up the reporter
    sql_reporter = sqlresults.SQLResultReporter()

    variables = args.variables
    # Add a new revision and get the "revision" number
    revision = sqlresults.add_revision()
    # Save the commitid
    commitid = args.commitid

    # If a directory has been provided, look there for all of the XML files
    if os.path.isdir(args.xmlpath[0]):
        xmldir = args.xmlpath[0]
        if not os.path.isabs(xmldir):
            xmldir = os.path.abspath(xmldir)
        xmlfiles = glob.glob(os.path.join(xmldir, '*.xml'))
    else:
        xmlfiles = args.xmlpath

    # Convert each file
    for file in xmlfiles:
        convert_xml(file)


