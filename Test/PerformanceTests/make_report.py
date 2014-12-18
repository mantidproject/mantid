#!/usr/bin/env python

import argparse
import sys
import os
import subprocess
import sqlite3

#====================================================================================
def getSourceDir():
    """Returns the location of the source code."""
    import os
    import sys
    script = os.path.abspath(sys.argv[0])
    if os.path.islink(script):
        script = os.path.realpath(script)
    return os.path.dirname(script)



def join_databases(dbfiles):
    """Create a single DB joining several ones
    Returns: filename created
    """
    outfile = os.path.join(os.path.dirname(dbfiles[0]), "JoinedDatabases.db")
    all_results = []
    # Get the results of each file
    for dbfile in dbfiles:
        print "Reading", dbfile
        sqlresults.set_database_filename(dbfile)
        these_results = sqlresults.get_results("")
        all_results += these_results
    # Write them into one
    sqlresults.set_database_filename(outfile)
    sqlresults.setup_database()
    reporter = sqlresults.SQLResultReporter()
    for res in all_results:
        reporter.dispatchResults(res)
    # Done!
    return outfile



#====================================================================================
if __name__ == "__main__":
    # Parse the command line
    parser = argparse.ArgumentParser(description='Generates a HTML report using the Mantid System Tests results database')

    parser.add_argument('--path', dest='path',
                        default="./Report",
                        help='Path to the ouput HTML. Default "./Report".' )

    parser.add_argument('--x_field', dest='x_field',
                        default="revision",
                        help="Field to use as the x-axis. Default: 'revision'. Other possibilities: 'date'.")

    parser.add_argument('dbfile', metavar='DBFILE', type=str, nargs='+',
                        default=["./MantidSystemTests.db"],
                        help='Required: Path to the SQL database file(s).')


    args = parser.parse_args()

    # Import the manager definition
    import analysis
    import sqlresults

    if len(args.dbfile) > 1:
        # Several files - join them into one big .db
        dbfile = join_databases(args.dbfile)
    else:
        # Only one file - use it
        dbfile = args.dbfile[0]


    if not os.path.exists(dbfile):
        print "Error! Could not find", dbfile
        sys.exit(1)

    # This is where we look for the DB file
    sqlresults.set_database_filename(dbfile)

    # Make the report
    analysis.generate_html_report(args.path, 100, args.x_field)