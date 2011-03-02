#!/usr/bin/env python
import os
import sys
import glob
import urllib
import re

s = 'const std::string name() const { return "'

def get_all_algos():
    """ Get all the algorithms .cpp files """
    fileList = []
    rootdir = "."
    for root, subFolders, files in os.walk(rootdir):
        for file in files:
            (name, ext) = os.path.splitext(file)
            if ext == ".cpp" and not file.endswith("Test.cpp")  and not file.endswith("_runner.cpp") :
                fullpath = os.path.join(root,file)
                #headerfile = os.path.join(root, ".." 
                
                source = open(fullpath).read()
                regex = "DECLARE_ALGORITHM\(([\w]+)\)"
                res = re.search(regex, source)
                if not res is None:
                    # The first match = the algo name
                    name = res.group(1)
                    fileList.append( (name, fullpath) )
    return fileList

def de_wikify(input):
    """ Remove wiki mark-up """
    line = input
    # Match for [[BLA|bla bla]]
    regex = r"\[\[[\w\ ]+\|([\w\s]+)\]\]"
    # replace just with the plain text one
    replace = r"\1"
    line = re.sub(regex, replace, line)
    line = line.replace("[[", "")
    line = line.replace('"', "'")
    line = line.replace("]]", "")
    line = line.replace("'''", "'")
    line = line.replace("''", "'")
    return line


def get_wiki_summary(name):
    """ Get the wiki summary from the algorithm name """
    url = "http://www.mantidproject.org/index.php?title=%s&action=raw&section=1" % name
    
    webFile = urllib.urlopen(url)
    wiki = webFile.read()
    webFile.close()
    out = ""
    wikified = ""
    for line in wiki.split("\n"):
        line = line.strip()
        if (line != "== Summary ==") and (line != "==Summary=="):
            # Keep all markup in here
            wikified += line + " "
            # And strip it out for this one
            out += de_wikify(line) + " "
            
        if line.startswith("{{Binary"):
            # Template in binary ops. Skip the rest
            break
        
    out = out.strip()
    return (out, wikified)

def get_padding(line):
    """Return a string with the spaces padding the start of the given line."""
    out = ""
    for c in line:
        if c == " ":
            out += " "
        else:
            break
    return out


def add_optional_message(name, fullpath, summary, wikified_summary):
    f = open(fullpath)
    source = f.read()
    f.close()

    regex = "setOptionalMessage\("
    match1 = re.search(regex, source)
    if not match1 is None:
        print name, "------> Optional message already set for", name
        
        
        
    # -------- Look for a spot to stick the new function ----------
    searchfor = "DECLARE_ALGORITHM"
    if "DECLARE_LOADALGORITHM" in source: searchfor = "DECLARE_LOADALGORITHM" 
    lines = source.split("\n")
    count = 0
    done = False
    padding = ""
    for n in xrange(len(lines)):
        line = lines[n]
        s = line.strip()
        if s.startswith(searchfor):
            # How much padding?
            padding = get_padding(line)
            # Look for the first blank line
            while line.strip() != "":
                count += len(line) + 1
                n += 1
                line = lines[n]
                done = True
                break
            
        if done: break
        # Keep a running count of the character
        count += len(line) + 1

    extra = """
/// Sets documentation strings for this Algorithm
%svoid %s::initDocs()
%s{
%s  this->setWikiSummary("%s");
%s  this->setOptionalMessage("%s");
%s}
""" % padding
    
    source = source[0:count]+extra+source[count:]
        
    print
    print source[count-100:count+200]
    print
        
    if 0:
        # Look for init() and then some padding before { 
        regex = "::init\(\)([\ ]+)?\n([.\s]+)?{"
        m = re.search(regex, source)
        if not m is None:
            n = m.end(0)
            padding = m.group(2)
            if padding is None: padding = ""
            extraline = '\n%s  this->setWikiSummary("%s");\n%s  this->setOptionalMessage("%s");\n' % (padding, wikified_summary, padding, summary)
            source = source[0:n] + extraline + source[n:]
            if 0:
                f = open(fullpath,'w')
                f.write(source)
                f.close()
        else:
            print name, "------> Could not find init()"

if __name__=="__main__":
    # Find paths to all algorithms
   
    algs = get_all_algos()
    
    for (name, fullpath) in algs:
        (summary, wikified_summary) = get_wiki_summary(name)
        if summary == "":
            print name, "------> WIKI SUMMARY NOT FOUND"
        else:
            add_optional_message(name, fullpath, summary, wikified_summary)
    
    