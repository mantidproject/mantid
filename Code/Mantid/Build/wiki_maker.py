#!/usr/bin/env python
""" Utility to automatically generate and submit algorithm Wiki pages
to the mantidproject.org"""

import argparse
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
from wiki_tools import *

#======================================================================
def add_wiki_description(algo, wikidesc):
    """Adds a wiki description  in the algo's header file under comments tag."""
    wikidesc = wikidesc.split('\n')
    source = find_algo_file(algo)
    if source != '':
        if len("".join(wikidesc)) == 0:
            print "No wiki description found to add!!!!"
        
        f = open(source,'r')
        lines = f.read().split('\n')
        f.close()
        n = 0
        while  n < len(lines):
            line = lines[n].strip()
            n += 1
            if line.startswith('#define'): break
            
        if n >= len(lines): n = 0
        
        #What lines are we adding?
        if source.endswith(".py"):
            adding = ['"""*WIKI* ', ''] + wikidesc + ['*WIKI*"""'] 
        else:
            adding = ['/*WIKI* ', ''] + wikidesc + ['*WIKI*/'] 
    
        lines = lines[:n] + adding + lines[n:]
        
        text = "\n".join(lines)
        f = codecs.open(source, encoding='utf-8', mode='w+')
        f.write(text)
        f.close()
        
        
#======================================================================
def get_wiki_description(algo):
    source = find_algo_file(algo)
    if source == '': 
        alg = mtd.createAlgorithm(algo)
        return alg.getWikiDescription()
    else:
        f = open(source,'r')
        lines = f.read().split('\n')
        f.close()
        n = 0
        while not lines[n].startswith("/*WIKI*") and not lines[n].startswith('"""*WIKI*'):
            n += 1
        desc = ""
        n += 1
        while not lines[n].startswith("*WIKI*"):
            desc += lines[n] + "\n"
            n += 1
        return desc
    
#======================================================================
def make_property_table_line(propnum, p):
    """ Make one line of property table
    propnum :: number of the prop
    p :: Property object
    """
    out = ""
    # The property number
    out += "|" + str(propnum) + "\n"
    # Name of the property
    out += "|" + p.name + "\n"
    # Direction
    direction_string = ["Input", "Output", "InOut", "None"]
    out += "|" + direction_string[p.direction] + "\n"
    # Type (as string)
    out += "|" + str(p.type) + "\n"
    # Default?
    default = p.getDefault
    defaultstr = str(default)
    # Replace the ugly default values with "optional"
    if (defaultstr == "8.9884656743115785e+307") or \
       (defaultstr == "2147483647"):
        defaultstr = "Optional"
        
    if str(p.type) == "boolean":
        if defaultstr == "1": defaultstr = "True" 
        else: defaultstr = "False"
    
    if (p.isValid == ""): #Nothing was set, but it's still valid = NOT mandatory
      out += "| " + defaultstr + "\n"
    else:
      out += "|Mandatory\n"
    # Documentation
    out += "|" + p.documentation.replace("\n", " ") + "\n"
    # End of table line
    out += "|-\n"
    return out
    
    
        
#======================================================================
def make_wiki(algo_name):
    """ Return wiki text for a given algorithm """ 
    
    # Deprecated algorithms: Simply returnd the deprecation message
    deprec = mtd.algorithmDeprecationMessage(algo_name)
    if len(deprec) != 0:
        out = deprec
        out = out.replace(". Use ", ". Use [[")
        out = out.replace(" instead.", "]] instead.")
        return out
    
    alg = mtd.createAlgorithm(algo_name)
    
    out = ""
    out += "== Summary ==\n\n"
    out += alg._ProxyObject__obj.getWikiSummary().replace("\n", " ") + "\n\n"
    out += "== Properties ==\n\n"
    
    out += """{| border="1" cellpadding="5" cellspacing="0" 
!Order\n!Name\n!Direction\n!Type\n!Default\n!Description
|-\n"""

    # Do all the properties
    props = alg._ProxyObject__obj.getProperties()
    propnum = 1
    for prop in props:
        out += make_property_table_line(propnum, prop)
        propnum += 1
        
        
    # Close the table
    out += "|}\n\n"


    out += "== Description ==\n"
    out += "\n"
    desc = get_wiki_description(algo_name)
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
      out += "[[Category:" + categ + "]]\n"

    out +=  "{{AlgorithmLinks|" + algo_name + "}}\n"

    return out






    
    
#======================================================================
def do_algorithm(args, algo):
    print "Generating wiki page for %s" % (algo)
    site = wiki_tools.site
    contents = make_wiki(algo)
    
    print "Saving page to http://www.mantidproject.org/%s" % algo
    #Open the page with the name of the algo
    page = site.Pages[algo]
    text = page.edit()
    #print 'Text in page:', text.encode('utf-8')
    page.save(contents, summary = 'Bot: replaced contents using the auto_wiki.py script.' )

#======================================================================
if __name__ == "__main__":
    # First, get the config for the last settings
    config = ConfigParser.ConfigParser()
    config_filename = os.path.split(__file__)[0] + "/wiki_maker.ini"
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
    
    parser = argparse.ArgumentParser(description='Generate the Wiki documentation for one '
                                      'or more algorithms, and updates the mantidproject.org website')
    
    parser.add_argument('algos', metavar='ALGORITHM', type=str, nargs='*',
                        help='Name of the algorithm(s) to generate wiki docs.')
    
    parser.add_argument('--user', dest='username', default=defaultuser,
                        help="User name, to log into the www.mantidproject.org wiki. Default: '%s'. This value is saved to a .ini file so that you don't need to specify it after." % defaultuser)

    parser.add_argument('--password', dest='password', default=defaultpassword,
                        help="Password, to log into the www.mantidproject.org wiki. Default: '%s'. Note this is saved plain text to a .ini file!" % defaultpassword)
    
    parser.add_argument('--mantidpath', dest='mantidpath', default=defaultmantidpath,
                        help="Full path to the Mantid compiled binary folder. Default: '%s'. This will be saved to an .ini file" % defaultmantidpath)

    args = parser.parse_args()
    
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

    if len(args.algos)==0:
        parser.error("You must specify at least one algorithm.")

    initialize_Mantid(args.mantidpath)
    intialize_files()
    initialize_wiki(args)
  
    for algo in args.algos:
        do_algorithm(args, algo)
    