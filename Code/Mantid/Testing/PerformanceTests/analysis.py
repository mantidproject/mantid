""" Module containing functions for test
performance analyis, plotting, and saving
to other formats (CSV, PDF) """

import testresult
import os
import sys
import sqlresults
from sqlresults import get_results
import matplotlib
from pylab import *
import numpy as np
import datetime
import random

# This is the date string format as returned by the database
DATE_STR_FORMAT = "%Y-%m-%d %H:%M:%S.%f"

#============================================================================================
def get_orderby_clause(last_num):
    """Returns a order by clause that limits to the last # revisions """
    if last_num > 0:
        return " ORDER BY revision DESC limit %d" % last_num
    else:
        return ''


#============================================================================================
def get_runtime_data(name='', type='', x_field='revision', last_num=-1):
    """Get the test runtime/iteration as a function of an X variable.

    Parameters
    ----------
        name :: full name of the test
        type :: type of test to filter by
        x_field :: name of the field for the X axis.
                e.g. 'revision' (default)
                or 'date' : exact date/time of launch
                or 'index' : using the date, but returning an index of build #
                    instead of the date (better scaling)
        last_num :: only get the last this-many entries from the table, sorted by revision.
                if < 0, then get everything

    Returns
    -------
        x :: list of X values, sorted increasing
        y :: list of runtime/iteration for each x
        """

    results = get_results(name, type, where_clause='', orderby_clause=get_orderby_clause(last_num))

    # Data dict. Key = X variable; Value = (iterations total, runtime total)
    data = {}
    for res in results:
        # Get the x field value
        if x_field == 'index':
            x = res['date']
        else:
            x = res[x_field]

        if data.has_key(x):
            old = data[x]
            iters = old[0] + 1 # Iterations
            runtime = old[1] + res["runtime"]
        else:
            iters = 1
            runtime = res["runtime"]
        # Save the # of iterations and runtime
        data[x] = (iters, runtime)

    # Now make a sorted list of (x, runtime/iteration)
    sorted = [(x, y[1]/y[0]) for (x,y) in data.items()]
    sorted.sort()

    x = [a for (a,b) in sorted]
    # For index, convert into an integer index
    if x_field == 'index':
        x = range( len(x) )
    y = [b for (a,b) in sorted]

    return (x,y)


#============================================================================================
def get_unique_fields(results, field):
    """Given a list of TestResult, return a
    list of all unique values of 'field'"""
    out = set()
    for res in results:
        out.add( res[field] )
    return list(out)

#============================================================================================
def get_results_matching(results, field, value):
    """Given a list of TestResult,
    return a list of TestResult's where 'field' matches 'value'."""
    out = []
    for res in results:
        if res[field] == value:
            out.append(res)
    return out


#============================================================================================
def smart_ticks(index, values):
    """On the current figure, set the ticks at X positions
    given by index, with value given by values (ints).
    But it tries to space them out in a reasonable way.
    """
    if type(values[0]).__name__ == "unicode":
        # Make the array of dates
        dates = []
        for val in values:
            try:
                datetime.datetime.strptime(val, DATE_STR_FORMAT)
                dates.append(val)
            except:
                pass
        if len(dates) == 0: return
        td = dates[-1] - dates[0]
        if (td < datetime.timedelta(hours=1)):
            values_str = [d.strftime("%M:%S") for d in dates]
        elif (td < datetime.timedelta(days=1)):
            values_str = [d.strftime("%H:%M") for d in dates]
        else:
            values_str = [d.strftime("%m-%d, %H:%M") for d in dates]
    else:
        # convert to list of strings
        values_str = [str(val) for val in values]

    if len(values_str) == 0: return

    w = gcf().get_figwidth()*gcf().get_dpi()
    spacing = w/len(index)

    tick_index = []
    tick_strings = []

    space_available = 0
    for i in xrange(len(index)):
        s = str(values_str[i]);
        s_width = (len(s)+1) * 12.0 # About 12 pixels per letter? And add a space
        space_available +=spacing
        if space_available >= s_width:
            space_available = 0
            tick_index.append(i)
            tick_strings.append(s)

    xticks( tick_index, tick_strings )



#============================================================================================
def plot_success_count(type='system', last_num=-1, x_field='revision'):
    """ Plot the count of successful/failed tests vs revision number

    Parameters
    ----------
        type :: 'system', or 'performance'
    """
    results = get_results('', type, where_clause='', orderby_clause=get_orderby_clause(last_num))
    revisions = get_unique_fields(results, x_field)

    # Go through each revision
    success = []
    fail = []
    for revision in revisions:
        these = get_results_matching(results, x_field, revision)
        succeeded = 0
        failed = 0
        for res in these:
            if res["success"]:
                succeeded += 1
            else:
                failed += 1
        # Keep the list of them
        success.append(succeeded)
        fail.append(failed)

    figure()
    revisions = np.array(revisions)
    fail = np.array(fail)
    success = np.array(success)

    index = np.arange(len(revisions))
#    p1 = bar(index, fail, color='r')
#    p2 = bar(index, success, color='g', bottom=fail)
#    legend( (p1[0], p2[0]), ('Failure', 'Success') )

    p1 = fill_between(index, fail, 0, color='r')
    p2 = fill_between(index, success+fail, fail, color='g')
    #legend( (p1, p2), ('Failure', 'Success') )

    smart_ticks( index, revisions)


    ylabel('Success/Fail')
    xlabel(x_field)
    revsare = "all revs"
    if last_num > 0: revsare = "last %d revs" % last_num
    title("Success/Fail History of %s tests (%s)" % (type, revsare))


#============================================================================================
def plot_runtime(*args, **kwargs):
    """ Call get_runtime_data()

    Parameters
    ----------
        - See get_runtime_data() for the full list
    """
    (x,y) = get_runtime_data(*args, **kwargs)

    figure()
    index = np.arange(len(x))
    plot(index,y,'-b.')
    smart_ticks( index, x)
    ylabel('Runtime/iteration (sec)')
    xlabel(kwargs['x_field'])

    last_num =kwargs.get('last_num',-1)
    if last_num > 0:
        title("Runtime History of %s (last %d revs)" % (kwargs['name'], kwargs["last_num"]) )
    else:
        title("Runtime History of %s (all revs)" % kwargs['name'])












# The default HTML header
default_html_header = """<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd"><html>
<head><LINK href="report.css" rel="stylesheet" type="text/css"></head>
"""

default_html_footer =  """</body></html>"""

#============================================================================================
def make_css_file(path):
    """ Make a save the report.css file to be used by all html """
    default_css = """
table
{
border-collapse:collapse;
background-color:FFAAAA;
}
table, th, td
{
border: 1px solid black;
padding: 2px 6px;
}
.failedrow, .failedrow TD, .failedrow TH
{
background-color:#FFAAAA;
color:black;
}
.alternaterow, .alternaterow TD, .alternaterow TH
{
background-color:#FFFFAA;
color:black;
}
.error
{
color:red;
font-weight: bold;
}

    """
    f = open(os.path.join(path, "report.css"), 'w')
    f.write(default_css)
    f.close()

#============================================================================================
def make_environment_html(res):
    """Return a HTML string with details of test environment, taken from the
    'res' TestResult object"""
    html = """<table border=1>
    <tr><th>Host name:</th> <td>%s</td> </tr>
    <tr><th>Environment:</th> <td>%s</td> </tr>
    <tr><th>Type of runner:</th> <td>%s</td> </tr>
    </table>
    """ % (res['host'], res['environment'], res['runner'])
    return html

#============================================================================================
def make_detailed_html_file(basedir, name, fig1, fig2, last_num):
    """ Create a detailed HTML report for the named test """
    html = default_html_header
    html += """<h1>Detailed report for %s</h1><br>""" % (name)
    html += """<img src="%s" alt="runtime vs revision number (latest %d entries)" />\n""" % (fig1, last_num)
    html += """<img src="%s" alt="runtime vs revision number" />\n""" % (fig2)
    html += """<h3>Test Results</h3>"""

    fields = ['revision', 'date', 'commitid', 'compare', 'status', 'runtime', 'cpu_fraction', 'variables']

    table_row_header = "<tr>"
    for field in fields:
        if field == "runtime": field = "Runtime/Iter."
        field = field[0].upper() + field[1:]
        table_row_header += "<th>%s</th>" % field
    table_row_header += "</tr>"

    html += """<table border="1">""" + table_row_header

    table_html = ''
    results = get_results(name, type='', where_clause='')
    sorted = [(res["revision"], res["variables"], res["date"], res) for res in results]
    sorted.sort(reverse=False)
    count = 0
    last_rev = 0
    commitid = ''
    last_commitid = ''
    row_class = ''
    table_rows = []
    for (rev, variable, date, res) in sorted:
        table_row_html = ''
        if (rev != last_rev):
            # Changed SVN revision. Swap row color
            if row_class == '':
                row_class = "class=alternaterow"
            else:
                row_class = ''
            last_rev = rev

        if commitid != last_commitid:
            last_commitid = commitid

        if res["success"]:
            table_row_html += "<tr %s>\n" % row_class
        else:
            table_row_html += "<tr class=failedrow>\n"

        for field in fields:
            val = ''

            if field == 'compare':
                # Comparison to previous commit, if anything can be done
                if (last_commitid != ""):
                    val = """<a href="https://github.com/mantidproject/mantid/compare/%s...%s">diff</a>""" % (last_commitid, commitid)

            else:
                # Normal fields
                val = res[field]

                # Trim the fractional seconds
                if field=="date":
                    val = str(val)[0:19]

                # Add a trac link
                if field=="commitid":
                    commitid = val
                    partial_commitid = val
                    if (len(partial_commitid) > 7): partial_commitid = partial_commitid[0:7];
                    val = """<a href="https://github.com/mantidproject/mantid/commit/%s">%s</a>""" % (commitid, partial_commitid)

                if field=="runtime":
                    val = "%.3f" % (res["runtime"])

            table_row_html += "<td>%s</td>" % val
        table_row_html += "\n</tr>\n"
        table_rows.append(table_row_html)

    # Now print out all the rows in reverse order
    table_rows.reverse()
    for row in table_rows:
        html += row
#        # Add the row header every 30 entries
#        count += 1
#        if count % 30 == 0: html += table_row_header

    # And one more at the end for good measure
    html += table_row_header
    html += "</table>"

    if len(results)> 0:
        html += """<h3>Environment</h3>
        %s""" % make_environment_html(results[0])

    html += default_html_footer

#    last_date = sorted[-1][1]["date"]
#    results = get_results(name, type='', get_log=False, where_clause=" date = '%s'" % last_date)
#    if len(results)>0:
#        html +=

    f = open(os.path.join(basedir, "%s.htm" % name), "w")
    html = html.replace("\n", os.linesep) # Fix line endings for windows
    f.write(html)
    f.close()


#============================================================================================
def how_long_ago(timestr):
    """Returns a string giving how long ago something happened,
    in human-friendly way """
    import time
    now = datetime.datetime.now()
    then = datetime.datetime.strptime(timestr, DATE_STR_FORMAT)
    td = (now-then)
    sec = td.seconds
    min = int(sec / 60)
    hours = int(min / 60)
    days = td.days
    weeks = int(days / 7)
    sec = sec % 60
    min = min % 60
    hours = hours % 24
    days = days % 7

    if weeks > 0:
        return "%dw%dd" % (weeks,days)
    elif days > 0:
        return "%dd%dh" % (days, hours)
    elif hours > 0:
        return "%dh%dm" % (hours, min)
    elif min > 0:
        return "%dm%ds" % (min, sec)
    else:
        return "%ds" % (sec)

    return ""


#============================================================================================
def get_html_summary_table(test_names):
    """Returns a html string summarizing the tests with these names """
    html = """
    <table ><tr>
    <th>Test Name</th>
    <th>Type</th>
    <th>Status</th>
    <th>When?</th>
    <th>Total runtime (s)</th>
    """

    for name in test_names:
        res = sqlresults.get_latest_result(name)
        if not res is None:
            # Calculate how long ago

            if not res["success"]:
                html += """<tr class="failedrow">"""
            else:
                html += """<tr>"""
            html += """<td><a href="%s.htm">%s</a></td>""" % (name, name)
            html += """<td>%s</td>""" % res['type']
            html += """<td>%s</td>""" % res['status']

            # Friendly date
            date = datetime.datetime.strptime(res['date'], DATE_STR_FORMAT)
            html += """<td>%s</td>""" %  date.strftime("%b %d, %H:%M:%S")

            html += """<td>%s</td>""" % res['runtime']
            html += """</tr>"""

    html += """</table>"""
    return html


#============================================================================================
def generate_html_subproject_report(path, last_num, x_field='revision', starts_with=""):
    """ HTML report for a subproject set of tests.

    starts_with : the prefix of the test name

    Returns: (filename saved, HTML for a page with ALL figures in it)
    """
    basedir = os.path.abspath(path)
    if not os.path.exists(basedir):
        os.mkdir(basedir)


    # Detect if you can do figures
    dofigs = True
    try:
        figure()
    except:
        dofigs = False

    # Start the HTML
    overview_html = ""
    html = default_html_header
    html += """<h1>Mantid System Tests: %s</h1>""" % starts_with
    if not dofigs:
        html += """<p class="error">There was an error generating plots. No figures will be present in the report.</p>"""


    # ------ Find the test names of interest ----------------
    # Limit with only those tests that exist in the latest rev
    latest_rev = sqlresults.get_latest_revison()
    temp_names = list(sqlresults.get_all_test_names(" revision = %d" % latest_rev))
    # Filter by their start
    test_names = []
    for name in temp_names:
        if name.startswith(starts_with):
            test_names.append(name)

    test_names.sort()

    # --------- Table with the summary of latest results --------
    html += """<h2>Latest Results Summary</h2>"""
    html += get_html_summary_table(test_names)


    # -------- Report for each test ------------------------
    for name in test_names:
        print "Plotting", name
        html += """<hr><h2>%s</h2>\n""" % name
        overview_html += """<hr><h2>%s</h2>\n""" % name

        # Path to the figures
        fig1 = "%s.runtime.v.revision.png" % name
        fig2 = "%s.runtime.v.revision.ALL.png" % name

        if dofigs:
            # Only the latest X entries
            plot_runtime(name=name,x_field=x_field,last_num=last_num)
            savefig(os.path.join(basedir, fig1))
            close()

            # Plot all svn times
            plot_runtime(name=name,x_field=x_field,last_num=-1)
            savefig(os.path.join(basedir, fig2))
            close()

            #html += """<img src="%s" alt="runtime vs revision number (latest %d entries)" />\n""" % (fig1, last_num)
            html += """<img src="%s" alt="runtime vs revision number" />\n""" % (fig2)
            overview_html +=  """<img src="%s" alt="runtime vs revision number" />\n""" % (fig1)

        make_detailed_html_file(basedir, name, fig1, fig2, last_num)
        detailed_html = """<br><a href="%s.htm">Detailed test report for %s</a>
        <br><br>
        """ % (name, name)
        html += detailed_html
        overview_html +=  detailed_html

    html += default_html_footer

    filename = starts_with + ".htm"

    f = open(os.path.join(basedir,filename), "w")
    html = html.replace("\n", os.linesep) # Fix line endings for windows
    f.write(html)
    f.close()

    return (filename, overview_html)



#============================================================================================
def generate_html_report(path, last_num, x_field='revision'):
    """Make a comprehensive HTML report of runtime history for all tests.
    Parameters
    ----------
        path :: base path to the report folder
        last_num :: in the shorter plot, how many SVN revs to show?
        x_field :: the field to use as the x-axis. 'revision' or 'date' make sense
    """
    basedir = os.path.abspath(path)
    if not os.path.exists(basedir):
        os.mkdir(basedir)

    # Make the CSS file to be used by all HTML
    make_css_file(path)

    # Detect if you can do figures
    dofigs = True
    try:
        figure()
    except:
        dofigs = False

    # --------- Start the HTML --------------
    html = default_html_header
    html += """<h1>Mantid System Tests Auto-Generated Report</h1>"""
    html += """<p><a href="overview_plot.htm">See an overview of performance plots for all tests by clicking here.</a></p> """
    if not dofigs:
        html += """<p class="error">There was an error generating plots. No figures will be present in the report.</p>"""

    html += """<h2>Run Environment</h2>
    %s
    """ % ( make_environment_html(sqlresults.get_latest_result()) )

    overview_html = ""

    # ------ Find the test names of interest ----------------
    # Limit with only those tests that exist in the latest rev
    latest_rev = sqlresults.get_latest_revison()
    test_names = list(sqlresults.get_all_test_names(" revision = %d" % latest_rev))
    test_names.sort()

    # ------ Find a list of subproject names --------
    subprojects = set()
    for name in test_names:
        n = name.find(".")
        if n > 0:
            subprojects.add( name[:n] )
    subprojects = list(subprojects)
    subprojects.sort()
    html += """<h2>Test Subprojects</h2>
    <big>
    <table cellpadding="10">    """

    for subproject in subprojects:
        (filename, this_overview) = generate_html_subproject_report(path, last_num, x_field, subproject)
        overview_html += this_overview
        html += """<tr> <td> <a href="%s">%s</a> </td> </tr>
        """ % (filename, subproject)
    html += """</table></big>"""

    # --------- Table with the summary of latest results --------
    html += """<h2>Overall Results Summary</h2>"""
    html += get_html_summary_table(test_names)

    # -------- Overall success history graphs ------------
    #if dofigs:
    #    # We report the overall success
    #    fig_path = "OverallSuccess.png"
    #    plot_success_count(type='',last_num=last_num, x_field=x_field)
    #    savefig(os.path.join(basedir, fig_path))
    #    close()
    #
    #    fig_path2 = "OverallSuccess.ALL.png"
    #    plot_success_count(type='',last_num=-1, x_field=x_field)
    #    savefig(os.path.join(basedir, fig_path2))
    #    close()
    #
    #    html += """<h2>Overall Success/Failure</h2>
    #    <img src="%s" />
    #    <img src="%s" />
    #    """ % (fig_path, fig_path2)

    html += default_html_footer

    f = open(os.path.join(basedir, "report.htm"), "w")
    html = html.replace("\n", os.linesep) # Fix line endings for windows
    f.write(html)
    f.close()

    # -------- Overview of plots ------------
    f = open(os.path.join(basedir, "overview_plot.htm"), "w")
    overview_html = overview_html.replace("\n", os.linesep) # Fix line endings for windows
    f.write(overview_html)
    f.close()

    print "Report complete!"




#============================================================================================
if __name__ == "__main__":
    sqlresults.set_database_filename("MyFakeData.db")
    # Make up some test data
    if 0:
        if os.path.exists("MyFakeData.db"): os.remove("MyFakeData.db")
        sqlresults.generate_fake_data(300)


    generate_html_report("../Report", 50)

#    plot_runtime(name='MyFakeTest', x_field='revision')
#    plot_runtime(name='MyFakeTest', x_field='date')
#    plot_success_count()
#    show()

