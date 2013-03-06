#!/usr/bin/env python
""" Utility to automatically generate and submit algorithm Wiki pages
to the mantidproject.org"""
from pdb import set_trace as trace
import optparse
import os
import mwclient
import ConfigParser
import string
import time
import datetime
import subprocess
import commands
import sys
import codecs
import fnmatch
import wiki_tools
from wiki_tools import *
from wiki_report import WikiReporter
import difflib
import platform

# Junit report generator.
reporter = WikiReporter()
# no version identier
noversion = -1

#======================================================================
def get_wiki_description(algo, version):
    """ Extract the text between the *WIKI* tags in the .cpp file
    
    @param algo :: name of the algorithm
    @param version :: version, -1 for latest 
    """
    global mtd
    source = find_algo_file(algo, version)
    if source == '':
        alg = mtd.createAlgorithm(algo, version)
        print "Getting algorithm description from binaries."
        return alg.getWikiDescription()
    else:
        f = open(source,'r')
        lines = f.read().split('\n')
        print lines
        f.close()
        n = 0
        print algo
        while not lines[n].lstrip().startswith("/*WIKI*") and not lines[n].lstrip().startswith('"""*WIKI*'):
            n += 1
        desc = ""
        n += 1
        while not lines[n].lstrip().startswith("*WIKI*"):
            desc += lines[n] + "\n"
            n += 1
        print "Getting algorithm description from source."
        return desc
    
    
#======================================================================
def make_group_header_line(group):
    """ Make a group header line for the property table
    
     Args:
        group :: name of the group
    Returns:
        string to add to the wiki
    """
    if group=="":
        return "|colspan=6 align=center|   \n|-\n"
    else:
        return "|colspan=6 align=center|'''%s'''\n|-\n" % group

#======================================================================  
def create_property_default_string(prop):
    """ Create a default string 
    
     Args:
        default. The property default value.
    Returns:
        string to add to the wiki property table default section.
    """
    # Convert to int, then float, then any string
    
    default = prop.getDefault
    defaultstr = ""
    try:
        val = int(default)
        if (val >= 2147483647):
            defaultstr = "Optional"
        else:
            defaultstr = str(val)
    except:
        try:
            val = float(default)
            if (val >= 1e+307):
                defaultstr = "Optional"
            else:
                defaultstr = str(val)
        except:
            # Fall-back default for anything
            defaultstr = str(default)
            
    # Replace the ugly default values with "optional"
    if (defaultstr == "8.9884656743115785e+307") or \
       (defaultstr == "1.7976931348623157e+308") or \
       (defaultstr == "2147483647"):
        defaultstr = "Optional"
        
    if str(prop.type) == "boolean":
        if defaultstr == "1": defaultstr = "True" 
        else: defaultstr = "False"
    return defaultstr

#======================================================================
def make_property_table_line(propnum, p):
    """ Make one line of the property table
    
    Args:
        propnum :: number of the prop
        p :: Property object
    Returns:
        string to add to the wiki
    """
    
    out = ""
    # The property number
    out += "|" + str(propnum) + "\n"
    # Name of the property
    out += "|" + p.name + "\n"
    # Direction
    InputDirection = "Intput"
    OutputDirection = "Output"
    InOutDirection = "InOut"
    NoDirection = "None"
    direction_string = [InputDirection, OutputDirection, InOutDirection, NoDirection]
    out += "|" + direction_string[p.direction] + "\n"
    # Type (as string) wrap an IWorkspaceProperty in a link.
    if isinstance(p, IWorkspaceProperty): 
        out += "|[[" + str(p.type) + "]]\n"
    else:
        out += "|" + str(p.type) + "\n"
       
    if (direction_string[p.direction] == OutputDirection) and (not isinstance(p, IWorkspaceProperty)):
      out += "|\n" # Nothing to show under the default section for an output properties that are not workspace properties.
    elif (p.isValid == ""): #Nothing was set, but it's still valid = NOT  mandatory
      defaultstr = create_property_default_string(p)
      out += "| " + defaultstr + "\n"
    else:
      out += "|Mandatory\n"
      
    # Documentation
    out += "|" + p.documentation.replace("\n", "<br />") + "\n"
    # End of table line
    out += "|-\n"
    return out
    
    
        
#======================================================================
def make_wiki(algo_name, version, latest_version):
    """ Return wiki text for a given algorithm
    @param algo_name :: name of the algorithm (bare)
    @param version :: version requested
    @param latest_version :: the latest algorithm 
    """ 
    out = ""
    # Deprecated algorithms: Simply returnd the deprecation message
    print "Creating... ", algo_name, version
    deprec = mtd.algorithmDeprecationMessage(algo_name,version)
    if len(deprec) != 0:
        out = "== Deprecated ==\n\n"
        deprecstr = deprec
        deprecstr = deprecstr.replace(". Use ", ". Use [[")
        deprecstr = deprecstr.replace(" instead.", "]] instead.")
        out += deprecstr 
        out += "\n\n"
    
    alg = mtd.createAlgorithm(algo_name, version)
    
    if (latest_version > 1):
        if (version < latest_version):
            out += "Note: This page refers to version %d of %s. The latest version is %d - see [[%s v.%d]].\n\n" % (version, algo_name, latest_version, algo_name, latest_version)
        else:
            out += "Note: This page refers to version %d of %s. "% (version, algo_name)
            if latest_version > 2:
                out += "The documentation for older versions is available at: "
            else:
                out += "The documentation for the older version is available at: "
            for v in xrange(1,latest_version):
                out += "[[%s v.%d]] " % (algo_name, v)
            out += "\n\n"
        
    
    out += "== Summary ==\n\n"
    out += alg._ProxyObject__obj.getWikiSummary().replace("\n", " ") + "\n\n"
    out += "== Python Signature ==\n\n"
    out += " " + create_function_signature(algo_name) + "\n\n" 
    out += "== Properties ==\n\n"
    
    out += """{| border="1" cellpadding="5" cellspacing="0" 
!Order\n!Name\n!Direction\n!Type\n!Default\n!Description
|-\n"""

    # Do all the properties
    props = alg._ProxyObject__obj.getProperties()
    propnum = 1
    last_group = ""
    for prop in props:
        group = prop.getGroup
        if (group != last_group):
            out += make_group_header_line(group)
            last_group = group
        out += make_property_table_line(propnum, prop)
        propnum += 1
        
        
    # Close the table
    out += "|}\n\n"


    out += "== Description ==\n"
    out += "\n"
    desc = ""
    try:
        desc = get_wiki_description(algo_name,version)
    except IndexError:
        pass
    if (desc == ""):
      out += "INSERT FULL DESCRIPTION HERE\n"
      print "Warning: missing wiki description for %s! Placeholder inserted instead." % algo_name
    else:
      out += desc + "\n"
    out += "\n"
    out += "[[Category:Algorithms]]\n"
    
    # All other categories
    categories = alg.categories()
    for categ in categories:
        n = categ.find("\\")
        if (n>0):
            # Category is "first\second"
            first = categ[0:n]
            second = categ[n+1:]
            out += "[[Category:" + first + "]]\n"
            out += "[[Category:" + second + "]]\n"
        else:
            out += "[[Category:" + categ + "]]\n"

    # Point to the right source ffiles
    if version > 1:
        out +=  "{{AlgorithmLinks|%s%d}}\n" % (algo_name, version)
    else:
        out +=  "{{AlgorithmLinks|%s}}\n" % (algo_name)

    return out





#======================================================================
def confirm(prompt=None, resp=False):
    """prompts for yes or no response from the user. Returns True for yes and
    False for no.

    'resp' should be set to the default value assumed by the caller when
    user simply types ENTER.

    >>> confirm(prompt='Create Directory?', resp=True)
    Create Directory? [y]|n: 
    True
    >>> confirm(prompt='Create Directory?', resp=False)
    Create Directory? [n]|y: 
    False
    >>> confirm(prompt='Create Directory?', resp=False)
    Create Directory? [n]|y: y
    True

    """
    
    if prompt is None:
        prompt = 'Confirm'

    if resp:
        prompt = '%s [%s]|%s: ' % (prompt, 'y', 'n')
    else:
        prompt = '%s [%s]|%s: ' % (prompt, 'n', 'y')
        
    while True:
        ans = raw_input(prompt)
        if not ans:
            return resp
        if ans not in ['y', 'Y', 'n', 'N']:
            print 'please enter y or n.'
            continue
        if ans == 'y' or ans == 'Y':
            return True
        if ans == 'n' or ans == 'N':
            return False
    
    
#======================================================================
def make_redirect(from_page, to_page):
    """Make a redirect from_page to to_page"""
    print "Making a redirect from %s to %s" % (from_page, to_page)
    site = wiki_tools.site
    page = site.Pages[from_page]
    contents = "#REDIRECT [[%s]]" % to_page
    page.save(contents, summary = 'Bot: created redirect to the latest version.' )
    
#======================================================================
def page_exists(page):
    # Determine if the wikipage exists or not.
    revisions = page.revisions()
    for rev in revisions:
        return True
    return False

#======================================================================   
def last_page_editor(page):
    #Get the last editor of the page.
    revisions = page.revisions()
    for rev in revisions:
        return rev['user']
     
#======================================================================   
def wiki_maker_page(page):
    """
    returns True if the wikimaker was the last editor.
    """
    return ("WikiMaker" == last_page_editor(page))

#======================================================================
def create_function_signature(algo):
    """
    create the function signature for the algorithm.
    """
    from mantid.simpleapi import _get_function_spec
    import mantid.simpleapi
    _alg = getattr(mantid.simpleapi, algo)
    prototype =  algo + _get_function_spec(_alg)
    
    # Replace every nth column with a newline.
    nth = 4
    commacount = 0
    prototype_reformated = ""
    for char in prototype:
        if char == ',':
            commacount += 1
            if (commacount % nth == 0):
                prototype_reformated += ",\n  "
            else:
                prototype_reformated += char
        else: 
           prototype_reformated += char
    prototype_reformated = prototype_reformated.replace(",[Version]", "")
    return prototype_reformated
    
#======================================================================
def do_algorithm(args, algo, version):
    """ Do the wiki page
    @param algo :: the name of the algorithm, and it's version as a tuple"""
    global mtd
    is_latest_version = True
    # Find the latest version        
    latest_version = mtd.createAlgorithm(algo, noversion).version()
    if (version == noversion): 
        version = latest_version

    print "Latest version of %s is %d. You are making version %d." % (algo, latest_version, version)
    # What should the name on the wiki page be?
    wiki_page_name = algo
    if latest_version > 1:
        wiki_page_name = algo + " v." + str(version)
        # Make sure there is a redirect to latest version
        if not args.dryrun:
            make_redirect(algo, algo + " v." + str(latest_version))
        
    
    print "Generating wiki page for %s at http://www.mantidproject.org/%s" % (algo, wiki_page_name)
    site = wiki_tools.site
    new_contents = make_wiki(algo, version, latest_version) 
    
    #Open the page with the name of the algo
    page = site.Pages[wiki_page_name]
    if not page_exists(page):
        print "Error: Wiki Page wiki_page_name %s does not exist on the wiki." % wiki_page_name
        reporter.addFailureNoPage(algo, wiki_page_name)
        return
    
    old_contents = page.edit() + "\n"
    
    if old_contents == new_contents:
        print "Generated wiki page is identical to that on the website."
        # Report a success test case.
        reporter.addSuccessTestCase(algo)
    else:
        print "Generated wiki page is DIFFERENT than that on the website."
        print
        print "Printing out diff:"
        print
        # Perform a diff of the new vs old contents
        diff = difflib.context_diff(old_contents.splitlines(True), new_contents.splitlines(True), fromfile='website', tofile='new')
        diff_list = []
        for line in diff:
            sys.stdout.write(line) 
            diff_list.append(line)
        print
        
        wiki_maker_edited_last = wiki_maker_page(page)
        
        if not wiki_maker_edited_last:
            print "The last editor was NOT the WIKIMAKER"
            last_modifier = last_page_editor(page);
            print "The last page editor was ", last_modifier
            if not last_modifier == None:
                # Report a failure test case
                reporter.addFailureTestCase(algo, version, last_modifier, ''.join(diff_list))
        
        if args.dryrun:
            print "Dry run of saving page to http://www.mantidproject.org/%s" % wiki_page_name
        elif wiki_maker_edited_last or args.force or confirm("Do you want to replace the website wiki page?", True):
            print "Saving page to http://www.mantidproject.org/%s" % wiki_page_name
            page.save(new_contents, summary = 'Bot: replaced contents using the wiki_maker.py script.' )
            
    saved_text = open(wiki_page_name+'.txt', 'w')
    saved_text.write(new_contents)
    saved_text.close()
    
    
#======================================================================
if __name__ == "__main__":
    
    reload(sys).setdefaultencoding('utf8')
    # First, get the config for the last settings
    config = ConfigParser.ConfigParser()
    localpath = os.path.split(__file__)[0]
    if not localpath: localpath = '.'
    config_filename = localpath + "/wiki_maker.ini"
    config.read(config_filename)
    defaultuser = ""
    defaultpassword = ""
    defaultmantidpath = ""
    try:
        defaultuser = config.get("login", "username")
        defaultpassword = config.get("login", "password")
        defaultmantidpath = config.get("mantid", "path")
    except:
        pass
    
    parser = optparse.OptionParser(description='Generate the Wiki documentation for one '
                                      'or more algorithms, and updates the mantidproject.org website')
   
    parser.add_option('--user', dest='username', default=defaultuser,
                        help="User name, to log into the www.mantidproject.org wiki. Default: '%s'. This value is saved to a .ini file so that you don't need to specify it after." % defaultuser)

    parser.add_option('--password', dest='password', default=defaultpassword,
                        help="Password, to log into the www.mantidproject.org wiki. Default: '%s'. Note this is saved plain text to a .ini file!" % defaultpassword)
    
    parser.add_option('--mantidpath', dest='mantidpath', default=defaultmantidpath,
                        help="Full path to the Mantid compiled binary folder. Default: '%s'. This will be saved to an .ini file" % defaultmantidpath)

    parser.add_option('--force', dest='force', action='store_const',
                        const=True, default=False,
                        help="Force overwriting the wiki page on the website if different (don't ask the user)")

    parser.add_option('--alg-version', dest='algversion', default=noversion, 
                        help='Algorithm version to create the wiki for. Latest version if absent.')
    
    parser.add_option('--report', dest='wikimakerreport', default=False, action='store_const', const=True,
                        help="Record authors and corresponding algorithm wiki-pages that have not been generated with the wiki-maker")
    
    parser.add_option('--cache-config', dest='cacheconfig', default=False, action='store_const', const=True,
                        help="If true, the creditials of the executor will be cached for the next run.")
    
    parser.add_option('--dry-run', dest='dryrun', default=False, action='store_const', const=True,
                        help="If false, then the utility will work exactly the same, but no changes will actually be pushed to the wiki.")
    

    (args, algos) = parser.parse_args()
    
    if args.cacheconfig:
        # Write out config for next time
        config = ConfigParser.ConfigParser()
        config.add_section("login")
        config.set("login", "username", args.username)
        config.set("login", "password", args.password)
        config.add_section("mantid")
        config.set("mantid", "path", args.mantidpath)
        f = open(config_filename, 'w')
        config.write(f)
        f.close()

    if len(algos)==0:
        parser.error("You must specify at least one algorithm.")
    
    if platform.system() == 'Windows':
        os.environ['MANTIDPATH'] = args.mantidpath
    
    initialize_Mantid(args.mantidpath)
    global mtd
    from MantidFramework import IWorkspaceProperty
    mtd = wiki_tools.mtd
    intialize_files()
    initialize_wiki(args)
  
    if len(algos) == 1 and algos[0] == "ALL":
        print "Documenting All Algorithms"
        allAlgorithms = get_all_algorithms_tuples()
        for algo_tuple in allAlgorithms:
            do_algorithm(args, algo_tuple[0], algo_tuple[1][0])
    else:
        for algo in algos:
            do_algorithm(args, algo, int(args.algversion))
            
    if args.wikimakerreport:
        junit_file = open('WikiMakerReport.xml', 'w')
        junit_file.write(reporter.getResults())
        junit_file.close()
    
