#!/usr/bin/env python
import argparse
import os
import mwclient
import ConfigParser
import string
import time
import datetime
import subprocess
import commands

#
## Edit page
#page = site.Pages['Commons:Sandbox']
#text = page.edit()
#print 'Text in sandbox:', text.encode('utf-8')
#page.save(text + u'\nExtra data', summary = 'Test edit')
#
## Printing imageusage
#image = site.Images['Example.jpg']
#print 'Image', image.name.encode('utf-8'), 'usage:'
#for page in image.imageusage():
#    print 'Used:', page.name.encode('utf-8'), '; namespace', page.namespace
#    print 'Image info:', image.imageinfo
#
## Uploading a file
#site.upload(open('file.jpg'), 'destination.jpg', 'Image description')

#======================================================================
def initialize(args):
    global site
    # Init site object
    print "Connecting to site mantidproject.org"
    site = mwclient.Site('www.mantidproject.org', path='/')
    print "Logging in."
    site.login(args.username, args.password) # Optional
    

#======================================================================
def do_algorithm(args, algo):
    print "Generating wiki page for %s" % (algo)
    global site
    #retcode = subprocess.Popen(["bin/WikiMaker", args.algo, "wiki.txt"])
    #os.system()
    output = commands.getoutput("bin/WikiMaker " + algo + " wiki.txt");
    
    f = open('wiki.txt', 'r')
    contents = f.read()
    f.close()
    
    print "Saving page to www.mantidproject.org/%s" % algo
    #Open the page with the name of the algo
    page = site.Pages[algo]
    text = page.edit()
    #print 'Text in page:', text.encode('utf-8')
    page.save(contents, summary = 'Bot: replace contents by auto_wiki.py script, using output from WikiMaker.' )
    
    #print contents

#======================================================================
if __name__ == "__main__":
    # First, get the config for the last settings
    config = ConfigParser.ConfigParser()
    config_filename = os.path.split(__file__)[0] + "/auto_wiki.ini"
    config.read(config_filename)
    defaultuser = ""
    defaultpassword = ""
    try:
        defaultuser = config.get("login", "username")
        defaultpassword = config.get("login", "password")
    except:
        pass
    
    parser = argparse.ArgumentParser(description='Generate the Wiki documentation for one '
                                      'or more algorithms, and updates the mantidproject.org website')
    
    parser.add_argument('algo', metavar='ALGORITHM', type=str, nargs='+',
                        help='Name of the algorithm(s) to generate wiki docs.')
    
    parser.add_argument('--user', dest='username', default=defaultuser,
                        help="User name, to log into the www.mantidproject.org wiki. Default: '%s'. This value is saved to a .ini file so that you don't need to specify it after." % defaultuser)

    parser.add_argument('--password', dest='password', default=defaultpassword,
                        help="Password, to log into the www.mantidproject.org wiki. Default: '%s'. Note this is saved plain text to a .ini file!" % defaultpassword)

    args = parser.parse_args()
    
    # Write out for next time
    config = ConfigParser.ConfigParser()
    config.add_section("login")
    config.set("login", "username", args.username)
    config.set("login", "password", args.password)
    f = open(config_filename, 'w')
    config.write(f)
    f.close()
    
    # Open the site
    initialize(args)
    
    for algo in args.algo:
        do_algorithm(args, algo)
        