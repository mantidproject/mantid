#!/usr/bin/env python
""" A few tools for validating the wiki at mantidproject.org """
import warnings
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


mantid_initialized = False
cpp_files = []
cpp_files_bare = []
python_files = []
python_files_bare = []



#======================================================================
def remove_wiki_from_header():
    """One-time method to remove *WIKI* tags from all header files """
    parent_dir = os.path.abspath(os.path.join(os.path.split(__file__)[0], os.path.pardir))
    for root, dirnames, filenames in os.walk(parent_dir):
      for filename in fnmatch.filter(filenames, '*.h'):
        fullfile = os.path.join(root, filename)
        f = open(fullfile,'r')
        lines = f.read().split('\n')
        f.close()
        n = 0
        # The output file
        outlines = []
        while n < len(lines) and (not lines[n].startswith("/*WIKI*") and not lines[n].startswith('"""*WIKI*')):
            outlines.append( lines[n] )
            n += 1
        #Skip all the WIKI lines
        while (n < len(lines)) and not lines[n].startswith("*WIKI*"):
            n += 1
        n += 1
        # Copy the rest
        while n < len(lines):
            outlines.append( lines[n] )
            n += 1
        
        f = open(fullfile,'w')
        f.write('\n'.join(outlines))
        f.close()

#======================================================================
def add_wiki_description(algo, wikidesc):
    """One-time use method that adds a wiki description  in the algo's CPP file under comments tag."""
    wikidesc = wikidesc.split('\n')
    source = find_algo_file(algo)
    if source != '':
        if len("".join(wikidesc)) == 0:
            print "No wiki description found to add!!!!"
        
        f = open(source,'r')
        lines = f.read().split('\n')
        f.close()
        
        #What lines are we adding?
        if source.endswith(".py"):
            adding = ['"""*WIKI* ', ''] + wikidesc + ['*WIKI*"""'] 
        else:
            adding = ['/*WIKI* ', ''] + wikidesc + ['*WIKI*/'] 
    
        lines = adding + lines
        
        text = "\n".join(lines)
        f = codecs.open(source, encoding='utf-8', mode='w+')
        f.write(text)
        f.close()
        
        
#======================================================================
def intialize_files():
    """ Get path to every header file """
    global file_matches
    parent_dir = os.path.abspath(os.path.join(os.path.split(__file__)[0], os.path.pardir))
    file_matches = []
    for root, dirnames, filenames in os.walk(parent_dir):
      for filename in fnmatch.filter(filenames, '*.cpp'):
          fullfile = os.path.join(root, filename)
          cpp_files.append(fullfile)
          cpp_files_bare.append( os.path.split(fullfile)[1] )
      for filename in fnmatch.filter(filenames, '*.py'):
          fullfile = os.path.join(root, filename)
          python_files.append(fullfile)
          python_files_bare.append( os.path.split(fullfile)[1] )

#======================================================================
def find_algo_file(algo, version=-1):
    """Find the files for a given algorithm (and version)"""
    source = ''
    filename = algo
    if version > 1:
        filename += '%d' % version
    cpp = filename + ".cpp"
    pyfile = filename + ".py"
    if cpp in cpp_files_bare:
        n = cpp_files_bare.index(cpp, )
        source = cpp_files[n]
    elif pyfile in python_files_bare:
        n = python_files_bare.index(pyfile, )
        source = python_files[n]
    return source

#======================================================================
def initialize_wiki(args):
    global site
    # Init site object
    print "Connecting to site mantidproject.org"
    site = mwclient.Site('www.mantidproject.org', path='/')
    # Login, if specified
    if hasattr(args, 'username'):
        print "Logging in as %s" % args.username
        site.login(args.username, args.password) 
#        if res is None:
#            warnings.warn("Login with username '%s' failed! Check your username and password." % args.username)
    
    
#======================================================================
def initialize_Mantid(mantidpath):
    """ Start mantid framework """
    global mtd
    global mantid_initialized
    if mantid_initialized:   return
    sys.path.append(mantidpath)
    sys.path.append( os.path.join(mantidpath, 'bin') )
    sys.path.append(os.getcwd())
    sys.path.append( os.path.join( os.getcwd(), 'bin') )
    try:
        import MantidFramework
        from MantidFramework import mtd
    except:
        raise Exception("Error importing MantidFramework. Did you specify the --mantidpath option?")
    mtd.initialise()
    mantid_initialized = True

#======================================================================
def get_all_algorithms():
    """REturns a list of all algorithm names"""
    temp = mtd._getRegisteredAlgorithms(True)
    print temp
    algos = [x for (x, version) in temp]
    return algos

#======================================================================
def find_misnamed_algos():
    """Prints out algos that have mismatched class names and algorithm names"""
    algos = get_all_algorithms()
    print "\n--- The following algorithms have misnamed class files."
    print   "--- No file matching their algorithm name could be found.\n"
    for algo in algos:
        source = find_algo_file(algo, -1)
        if source=='':
            print "%s was NOT FOUND" % (algo, )
            
#======================================================================
def find_section_text(lines, section, go_to_end=False, section2=""):
    """ Search WIKI text to find a section text there """
    if len(lines) == 0:
        return ""
    n = 0
    for line in lines:
        line_mod = line.replace(" ", "")
        if line_mod.startswith("==%s" % section) \
            or (section2 != "" and line_mod.startswith("==%s" % section2)):
            # Section started
            n += 1
            doc = ""
            # collect the documents till next section or the end 
            newline = lines[n]
            while (go_to_end or not newline.strip().startswith('==')) \
                  and not newline.strip().startswith('[[Category'):
                doc += newline + '\n'
                n += 1
                if n < len(lines):
                    newline = lines[n]
                else:
                    break
            return doc
        n += 1
            
    return ""
    
    

#======================================================================
def find_property_doc(lines, propname):
    """ Search WIKI text to find a properties' documentation there """
    if len(lines) == 0:
        return ""
    n = 0
    for line in lines:
        if line.strip() == "|" + propname:
            doc = lines[n+4].strip()
            if len(doc)>1:
                doc = doc[1:]
            return doc
        n += 1
            
    return ""
    
#======================================================================
def validate_wiki(args, algos):
    """ Look for any missing wiki pages / inconsistencies """
    missing = []
    
    for algo in algos:
        print "\n================= %s ===================" % algo
        
        # Check wiki page and download contents
        page = site.Pages[algo]
        wiki = page.edit()
        lines = []
        if len(wiki) == 0:
            print "- Algorithm wiki page for %s is MISSING!" % algo
            missing.append(algo)
        else:
            lines = wiki.split('\n')
            
        source = find_algo_file(algo)
        if source=='':
            print "- Source file not found."

        # Validate the algorithm itself
        alg = mtd.createAlgorithm(algo)
        summary = alg._ProxyObject__obj.getWikiSummary()
        if len(summary) == 0: 
            print "- Summary is missing (in the code)."
            wikidoc = find_section_text(lines, "Summary", go_to_end=False, section2="")
            if args.show_missing: print wikidoc
            
#        # One-time code to add wiki desc to CPP file
#        desc = find_section_text(lines, "Description", True, "Introduction")
#        # Fallback to the code one
#        if len(desc) == 0: 
#            desc = alg._ProxyObject__obj.getWikiDescription()
#        add_wiki_description(algo, desc)
            
        props = alg._ProxyObject__obj.getProperties()
        for prop in props:
            if len(prop.documentation) == 0:
                print "- Property %s has no documentation/tooltip (in the code)." % prop.name,
                wikidoc = find_property_doc(lines, prop.name)
                if len(wikidoc) > 0:
                    print "Found in wiki"
                    if args.show_missing: print "   \"%s\"" % wikidoc
                else:
                    print "Not found in wiki."
            
    print "\n\n"
    print "Algorithms with missing wiki page:\n", " ".join(missing) 


#======================================================================
def find_orphan_wiki():
    """ Look for pages on the wiki that do NOT correspond to an algorithm """
    category = site.Pages['Category:Algorithms']
    algos = get_all_algorithms()
            
    for page in category:
        algo_name = page.name
        if not (algo_name.startswith("Category:")):
            if not (algo_name in algos) :
                print "Algorithm %s was not found." % algo_name

#======================================================================
if __name__ == "__main__":
   
    # First, get the config for the last settings
    config = ConfigParser.ConfigParser()
    config_filename = os.path.split(__file__)[0] + "/wiki_tools.ini"
    config.read(config_filename)
    defaultmantidpath = ""
    try:
        defaultmantidpath = config.get("mantid", "path")
    except:
        pass
    
    parser = argparse.ArgumentParser(description='Generate the Wiki documentation for one '
                                      'or more algorithms, and updates the mantidproject.org website')
    
    parser.add_argument('algos', metavar='ALGORITHM', type=str, nargs='*',
                        help='Name of the algorithm(s) to generate wiki docs.')
    
    parser.add_argument('--mantidpath', dest='mantidpath', default=defaultmantidpath,
                        help="Full path to the Mantid compiled binary folder. Default: '%s'. This will be saved to an .ini file" % defaultmantidpath)

    parser.add_argument('--validate', dest='validate_wiki', action='store_const',
                        const=True, default=False,
                        help="Validate algorithms' documentation. Validates them all if no algo is specified. Look for missing wiki pages, missing properties documentation, etc. using the list of registered algorithms.")
    
    parser.add_argument('--show-missing', dest='show_missing', action='store_const',
                        const=True, default=False,
                        help="When validating, pull missing in-code documentation from the wiki and show it.")
    
    parser.add_argument('--find-orphans', dest='find_orphans', action='store_const',
                        const=True, default=False,
                        help="Look for 'orphan' wiki pages that are set as Category:Algorithms but do not have a matching Algorithm in Mantid.")
    
    parser.add_argument('--find-misnamed', dest='find_misnamed', action='store_const',
                        const=True, default=False,
                        help="Look for algorithms where the name of the algorithm does not match any filename.")

    args = parser.parse_args()
        
    # Write out config for next time
    config = ConfigParser.ConfigParser()
    config.add_section("mantid")
    config.set("mantid", "path", args.mantidpath)
    f = open(config_filename, 'w')
    config.write(f)
    f.close()
    
    if not args.validate_wiki and not args.find_misnamed and not args.find_orphans and len(args.algos)==0:
        parser.error("You must specify at least one algorithm if not using --validate")

    initialize_Mantid(args.mantidpath)
    intialize_files()
    initialize_wiki(args)

    if args.find_misnamed:
        find_misnamed_algos()
        sys.exit(1)
  
    if args.find_orphans:
        find_orphan_wiki()
        sys.exit(1)

    if args.validate_wiki:
        # Validate a few, or ALL algos
        algos = args.algos
        if len(algos) == 0:
            algos = get_all_algorithms()
        validate_wiki(args, algos)        
        
