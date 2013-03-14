#!/usr/bin/env python
""" Utility to automatically generate and submit algorithm Wiki pages
to the mantidproject.org"""
from pdb import set_trace as trace
import optparse
import os
import ConfigParser
import sys
import wiki_tools
from wiki_tools import *
from wiki_report import WikiReporter
import difflib
import platform

# Junit report generator.
reporter = WikiReporter()

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
def do_algorithm(args, algo, version):
    """ Do the wiki page
    @param algo :: the name of the algorithm, and it's version as a tuple"""
    is_latest_version = True
    # Find the latest version        
    latest_version = find_latest_alg_version(algo)
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
                        help='Algorithm version to create the wiki for.')
    
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

    # Command-line overrides anything for finding Mantid
    if args.mantidpath is not None:
        os.environ['MANTIDPATH'] = args.mantidpath        
    elif not "MANTIDPATH" in os.environ:
        raise RuntimeError("Cannot find Mantid. MANTIDPATH environment variable not set & --mantidpath option not given")
    else:
        pass
    # Make sure the internal module use can find Mantid 
    sys.path.append(os.environ['MANTIDPATH'])

        # Check if python_d must be used to call into Mantid
    if platform.system() == 'Windows':
        flag_if_build_is_debug(os.environ['MANTIDPATH'])
        module_dir = os.path.dirname(__file__)
        os.environ["PYTHONPATH"] = os.environ.get("PYTHONPATH","") + ";" + module_dir + ";" + os.environ['MANTIDPATH']
    
    initialize_wiki(args)
  
    if len(algos) == 1 and algos[0] == "ALL":
        print "Documenting ALL Algorithms"
        alg_to_vers = get_algorithm_to_version_lookup()
        for name, versions in alg_to_vers.iteritems():
            for version in versions:
                do_algorithm(args, name, version)
    else:
        for algo in algos:
            do_algorithm(args, algo, int(args.algversion))
            
    if args.wikimakerreport:
        junit_file = open('WikiMakerReport.xml', 'w')
        junit_file.write(reporter.getResults())
        junit_file.close()
    
