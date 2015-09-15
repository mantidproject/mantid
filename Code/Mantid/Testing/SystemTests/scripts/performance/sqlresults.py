try:
    import sqlite3
    has_sql = True
except ImportError:
    has_sql = False
    print "Error importing sqlite3. SQL will not work"
    
import reporters
import datetime
import testresult
import os
import shutil
import math
import random

#====================================================================================
def getSourceDir():
    """Returns the location of the source code."""
    import os
    import sys
    script = os.path.abspath(sys.argv[0])
    if os.path.islink(script):
        script = os.path.realpath(script)
    return os.path.dirname(script)


#=====================================================================
# These are the table fields, in order
TABLE_FIELDS = ['date', 'name', 'type', 'host', 'environment', 'runner',
                 'revision', 'commitid', 'runtime', 'cpu_fraction', 
                 'memory_change', 'success',
                 'status', 'logarchive', 'variables']

#=====================================================================
# The default path to the database file
database_file = os.path.join(getSourceDir(), "MantidSystemTests.db")     

#=====================================================================
def get_database_filename():
    """Return the path to the database to use """
    return database_file

#=====================================================================
def set_database_filename(value):
    """Override the default database location"""
    global database_file
    database_file = value

#=====================================================================
def SQLgetConnection():
    """ Get a connection to the SQL database """
    # These are automatic conversion factors
    return sqlite3.connect(get_database_filename())


#=====================================================================
def get_TestResult_from_row(row):     
    """Return a filled TestResult object from a "row"
    obtained by selecting * from the TestRuns table
    Returns
    -------
        result :: TestResult object, with an extra
            .testID member containing the ID into the table (testID field) 
    """
    res = testresult.TestResult()
    res.testID = row[0]
    # ------ Get each entry in the table ---------
    for i in xrange(len(TABLE_FIELDS)):
        res[TABLE_FIELDS[i]] = row[i+1]
        
    return (res)

    
#=====================================================================
def get_latest_result(name=''):
    """Returns a TestResult object corresponding to the 
    last result in the table
    Parameters
    ----------
        name :: optional, test name to filter by"""
    db = SQLgetConnection()
    c = db.cursor()
    where = ""
    if name != "": where = " WHERE name='%s'" % name
    query = """SELECT * FROM TestRuns %s ORDER BY testID DESC LIMIT 1;""" % where
    c.execute(query)
    # Get all rows - there should be only one
    rows = c.fetchall()
    c.close()
    
    if len(rows) > 0:
        res = get_TestResult_from_row(rows[0])
        return res
    else:
        return None

#=====================================================================
def get_results(name, type="", where_clause='', orderby_clause=''):
    """Return a list of testresult.TestResult objects
    generated from looking up in the table 
    Parameters:
        name: test name to search for. Empty string = don't limit by name
        type: limit by type; default empty string means = don't limit by type.
        get_log : set to True to retrieve the log_contents too.
        where_clause : an additional SQL "where" clause to further limit the search.
            Do not include the WHERE keyword!
            e.g "date > 2010 AND environment='mac'". 
        orderby_clause : a clause to order and/or limit the results.
            e.g. "ORDER BY revision DESC limit 100" to get only the latest 100 revisions.            
        """
    out = []
    
    db = SQLgetConnection()
    c = db.cursor()
    
    query = "SELECT * FROM TestRuns "
    
    # Build up the where clause
    where_clauses = []
    if name != "":
        where_clauses.append(" name = '%s'" % name)
    if (type != ""):
        where_clauses.append(" type = '%s'" % type)
    if (where_clause != ""):
        where_clauses.append(" (" + where_clause + ")")
    # Add it to the query
    if len(where_clauses) > 0:
        query += "WHERE " + " AND ".join(where_clauses)
    # Now the ordering clause
    query += " " + orderby_clause
    
    c.execute(query)
    
    # Get all rows
    rows = c.fetchall()
    
    for row in rows:
        # Turn the row into TestResult
        res = get_TestResult_from_row(row)
        
        out.append(res)
    c.close()
    return out
      
      
#=====================================================================
def get_all_field_values(field_name, where_clause=""):
    """Return a list of every entry of the given
    field (e.g. 'name' or 'environment').  
    Parameters:
        field_name: field/column name to search for.
        where_clause : an additional SQL "where" clause to further limit the search.
            Do not include the WHERE keyword!
            e.g "date > 2010 AND environment='mac'". 
            
        """
    db = SQLgetConnection()
    c = db.cursor()
    
    query = "SELECT (%s) FROM TestRuns " % field_name
    if (where_clause != ""):
        query += "WHERE " + where_clause
        
    c.execute(query)
    
    # Get all rows
    rows = c.fetchall()
    
    out = [x for (x,) in rows] 
        
    return out
      
#=====================================================================
def get_latest_revison():
    """ Return the latest revision number """
    # Now get the latest revision
    db = SQLgetConnection()
    c = db.cursor()
    query = "SELECT (revision) FROM Revisions ORDER BY revision DESC LIMIT 1;"  
    c.execute(query)
    rows = c.fetchall()
    if (len(rows)>0):
        return int(rows[0][0])
    else:
        return 0
    
#=====================================================================
def add_revision():
    """ Adds an entry with the current date/time to the table. 
    Retrieve the index of that entry = the "revision".
    Returns the current revision"""
    db = SQLgetConnection()
    c = db.cursor()
    query = "INSERT INTO Revisions VALUES(NULL, '%s');" % str(datetime.datetime.now())
    c.execute(query)
    db.commit()
    return get_latest_revison()

            
#=====================================================================
def get_all_test_names(where_clause=""):
    """Returns a set containing all the UNIQUE test names in the database.
    ----
    where_clause: Do not include the WHERE keyword! """
    return set(get_all_field_values('name', where_clause))


#=====================================================================
def setup_database():
    """ Routine to set up the mysql database the first time.
    WARNING: THIS DELETES ANY TABLES ALREADY THERE 
    """
    print "Setting up SQL database at",get_database_filename()
    if os.path.exists(get_database_filename()):
        print "Creating a backup at", get_database_filename()+".bak"
        shutil.copyfile(get_database_filename(), get_database_filename()+".bak")
        
    db = SQLgetConnection()

    c = db.cursor()
    try:
        c.execute("DROP TABLE TestRuns;")
        c.execute("DROP TABLE Revisions;")
    except:
        print "Error dropping tables. Perhaps one does not exist (this is normal on first run)."
        
    c.execute("""CREATE TABLE TestRuns (
    testID INTEGER PRIMARY KEY,
    date DATETIME, name VARCHAR(60), type VARCHAR(20), 
    host VARCHAR(30), environment VARCHAR(50), runner VARCHAR(20), 
    revision INT, commitid VARCHAR(45), 
    runtime DOUBLE, cpu_fraction DOUBLE, memory_change INT,
    success BOOL,
    status VARCHAR(50), logarchive VARCHAR(80),
    variables VARCHAR(200)
    ); """)
    
    # Now a table that is just one entry per run (a fake "revision")
            
    c.execute("""CREATE TABLE Revisions (
    revision INTEGER PRIMARY KEY,
    date DATETIME
    ); """)

        
###########################################################################
# A class to report the results of stress tests to the Mantid Test database
# (requires sqlite3 module)
###########################################################################
class SQLResultReporter(reporters.ResultReporter):
    '''
    Send the test results to the Mantid test results database
    '''

    def __init__(self):
        pass
       

    def dispatchResults(self, result):
        '''
        Construct the SQL commands and send them to the databse
        '''
        dbcxn = SQLgetConnection()
        cur = dbcxn.cursor()
        #last_id = dbcxn.insert_id()
        
        # Create the field for the log archive name
        result["logarchive"] = result.get_logarchive_filename()
               
        valuessql = "INSERT INTO TestRuns VALUES(NULL, "

        # Insert the test results in the order of the table
        for field in TABLE_FIELDS:
            val = result[field]
            # Make into a string
            val_str = str(val)
            
            # Booleans must be 0 or 1
            if type(val).__name__ == "bool":
                val_str = ["0", "1"][val] 
                
            valuessql += "'" + val_str + "',"
            
        valuessql = valuessql.rstrip(',')
        valuessql += ');'
        cur.execute(valuessql)
        # Save test id for iteration table
        test_id = cur.lastrowid
        
        # Commit and close the connection
        dbcxn.commit()
        cur.close()
        dbcxn.close()


#============================================================================================
def generate_fake_data(num_extra = 0):
    """ Make up some data for a database """
    print "Generating fake data..."
    setup_database()
    rep = SQLResultReporter()
    for timer in [9400, 9410,9411, 9412] + range(9420,9440) + [9450, 9466] + range(9450, 9450+num_extra):
        rev = add_revision()
        for name in ["Project1.MyFakeTest", "Project1.AnotherFakeTest", "Project2.FakeTest", "Project2.OldTest"]:
            if (name != "Project2.OldTest"):
                result = testresult.TestResult()
                result["name"] = name
                result["date"] = datetime.datetime.now() + datetime.timedelta(days=timer, minutes=timer)
                result["log_contents"] = "Revision %d" % rev
                result["runtime"] = timer/10.0 + random.randrange(-2,2)
                result["commitid"] = rev #'926bf82e36b4c90c95efc3f1151725696273de5a'
                result["success"] = (random.randint(0,10) > 0)
                result["status"] = ["failed","success"][result["success"]]
                result["revision"] = rev
                rep.dispatchResults(result)
    print "... Fake data made."


#=====================================================================
if __name__ == "__main__":
    set_database_filename("SqlResults.test.db")
    generate_fake_data()
    
    res = get_latest_result()
    print res
    
    
