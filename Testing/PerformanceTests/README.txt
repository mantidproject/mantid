Performance Tests
-----------------

This folder contains python scripts used in interpreting C++
performance tests.

The main scripts of interest are:

- xunit_to_sql.py : interprets XUnit .xml files and places them in a SQL
                    performance history database.
- make_report.py  : generate a HTML report with figures, using a
                    SQL database.
- check_performance.py : compare the performance of the latest test runs
                         to their historical averages and generates warnings
                         as needed.

See each script's help (script.py --help) for details.

The other scripts are support modules.


@author Janik Zikovsky
@date October 6th, 2011
