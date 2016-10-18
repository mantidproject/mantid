#pylint: disable=invalid-name
###########################################################################################
# Extracts code blocks marked as python from mediawiki pages and checks they run
# run with -h for command line arguments
#
##########################################################################################
import os
import re
import sys
import urllib2
import argparse
import json


def readWebPage(url):
    # set your environment HTTP_PROXY to be your proxy
    # for ral HTTP_PROXY=http://wwwcache.rl.ac.uk:8080
    aResp = urllib2.urlopen(url)
    web_pg = aResp.read()
    return web_pg


def getTestablePages(url):
    retList = []
    print "Reading Testable pages from " + url
    output = readWebPage(url)
    jsonObj = json.loads(output)
    for member in jsonObj['query']['categorymembers']:
        retList.append(member['title'])
    return retList


def convertURLToRaw(url):
    return url.replace(" ","%20") + "?action=raw"


def writeTestRst(filepath,mediawikiText,pageName):
    '''
    for a block of wiki text, writes out all tests found to an rst page
    :param mediawikiText: mediawiki text
    :param filepath: the filepath to write out the rst file to, if None then print to stdout
    :return: None
    '''
    try:
        if filepath is not None:
            sys.stdout = open(filepath, 'w')
        print ":orphan:\n"
        findCodeSections(mediawikiText,pageName)
    finally:
        sys.stdout = sys.__stdout__


def findCodeSections(mediawikiText,pageName):
    '''
    Finds code sections that look like this:

    #<div style="border:1pt dashed blue; background:#f9f9f9;padding: 1em 0;">
    # <!--skip-->
    # <!-- setup
    # x=1
    # y=2
    # file.open()
    # -->
    # <source lang="python">
    # x = 'The meaning of life is ... '
    # answer = 42
    # y = x + str(answer)  # This converts the number 5 to a string and joins
    #                      # it with the first string and then assigns y to a
    #                      # new string containing the concatenated string
    # </source>
    # <!-- teardown
    # print y # print statements can be in the teardown if you do not want them visible
    # file.close()
    # -->
    # <!-- result
    # the lazy dog
    # -->
    # </div>
    '''

    wikiTestPattern = re.compile(
        r'''(<!--\s*skip\s*-->)?\s*                             # optional skip section (group 1)
            (<!--\s*testsetup\s*(.*?)\s*-->)?\s*                    # optional setup section (group 2), just setup code (group 3)
            <source.*?lang="python".*?>\s*(.*?)\s*<\/source>\s* # mandatory source section, code (group 4)
            (<!--\s*testcleanup\s*(.*?)\s*-->)?\s*                 # optional teardown section (group 5), code (group 6)
            (<!--\s+testoutput\s*(.*?)\s*-->)?                      # optional result section (group 7), text group (8)
            ''', re.IGNORECASE + re.DOTALL + re.VERBOSE)

    for m in re.finditer(wikiTestPattern, mediawikiText):
        (line,col) = coords_of_str_index(mediawikiText, m.start(0))
        testName = pageName + "[" + str(line) + "]"
        #print the test
        if m.group(1) is not None:
            print ".. Skipping Test ", testName
            print
        else:
            printDirective("testsetup",testName,m.group(3),True)
            printDirective("testcode",testName,m.group(4),False)
            printDirective("testcleanup",testName,m.group(6),True)
            printDirective("testoutput",testName,m.group(8),True, "+ELLIPSIS, +NORMALIZE_WHITESPACE")
        print


def printDirective (directive, name, contents, hideIfNone = False,options = None):
    if not(hideIfNone and contents is None):
        print ".. {}:: {}".format(directive,name)
        if options is not None:
            print "   :options: {}".format(options)
        print
        if contents is not None:
            for line in contents.split("\n"):
                print "   " + line
        print


def coords_of_str_index(string, index):
    """Get (line_number, col) of `index` in `string`."""
    lines = string.splitlines(True)
    curr_pos = 0
    for linenum, line in enumerate(lines):
        if curr_pos + len(line) > index:
            return linenum + 1, index-curr_pos
        curr_pos += len(line)


def ensureDirectoriesExist(path):
    try:
        os.makedirs(path)
    except OSError:
        pass

################################################################################################################

parser = argparse.ArgumentParser(description='Extracts code blocks marked as python from mediawiki pages and tests they run.')
parser.add_argument('-i', '--i',
                    help='The title of a mediawiki page, if not specified it will search the Tested Examples category')
parser.add_argument('-s', '--s',
                    help='The url of a mantid mediawiki site')
parser.add_argument('-o', '--o',
                    help='Provide a path to output to an output directory')

args = parser.parse_args()
urlList = []
baseUrl = "http://www.mantidproject.org/"
if args.s is not None:
    baseUrl = args.s
if args.i is None:
    urlList = getTestablePages(baseUrl +
                               "api.php?action=query&list=categorymembers&format=json&cmlimit=500&cmtitle=Category:Tested%20Examples")
else:
    urlList.append(args.i)

outputDir = None
if args.o is not None:
    ensureDirectoriesExist(args.o)
    if not os.path.isdir(args.o):
        print "Output directory not found", args.o
        print "Output will be to stdout"
    else:
        outputDir = args.o

for url in urlList:
    pageName = "mwTest_" + url.replace(" ","_").replace(":","")
    print "Parsing ", pageName,

    outputFile = None
    if outputDir is not None:
        outputFile = os.path.join(outputDir, pageName+".rst")
    print "->", outputFile

    #run pandoc and get the output in rst
    mediawikiText = readWebPage(convertURLToRaw(baseUrl + url))
    writeTestRst(outputFile,mediawikiText,pageName)
