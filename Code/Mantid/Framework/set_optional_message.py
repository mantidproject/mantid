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

#======================================================================================
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

#======================================================================================
def escape_for_c(rawstring):
    """ Replace \ with \\; " with \" to escape the slash for a c-style string"""
    out = rawstring
    out = out.replace("\\", "\\\\")
    out = out.replace('"', '\\"')
    return out

#======================================================================================
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


#======================================================================================
def get_padding(line):
    """Return a string with the spaces padding the start of the given line."""
    out = ""
    for c in line:
        if c == " ":
            out += " "
        else:
            break
    return out


#======================================================================================
def find_line_number(lines, searchfor, startat=0):
    """Look line-by-line in lines[] for a line that starts with searchfor. Return
    the line number in source where the line was found, and the padding (in spaces) before it"""
    count = 0
    done = False
    padding = ""
    for n in xrange(startat, len(lines)):
        line = lines[n]
        s = line.strip()
        if s.startswith(searchfor):
            # How much padding?
            padding = get_padding(line)
            return (n, padding)

    return (None, None)


#======================================================================================
def insert_lines(lines, insert_lineno, extra, padding):
    """Insert a text, split by lines, inside a list of 'lines', at index 'insert_lineno'
    Adds 'padding' to each line."""
    # split
    extra_lines = extra.split("\n");
    #Pad 
    for n in xrange(len(extra_lines)):
        extra_lines[n] = padding+extra_lines[n]
    return lines[0:insert_lineno] + extra_lines + lines[insert_lineno:]


#======================================================================================
def save_lines_to(lines, fullpath):
    """Save a list of strings to one file"""        
    # Join into one big string
    source = "\n".join(lines)
    # Save to disk
    f = open(fullpath,'w')
    f.write(source)
    f.close()

#======================================================================================
def add_optional_message(name, fullpath, summary, wikified_summary):
    f = open(fullpath)
    source = f.read()
    f.close()


#    regex = "setOptionalMessage\("
#    match1 = re.search(regex, source)
#    if not match1 is None:
#        print name, "------> Optional message already set for", name
        
    # -------- Look for a spot to stick the new function ----------
    searchfor = "DECLARE_ALGORITHM"
    if "DECLARE_LOADALGORITHM" in source: searchfor = "DECLARE_LOADALGORITHM" 
    lines = source.split("\n")
    
    (linenum, padding) = find_line_number(lines,searchfor)
    # Look for the next empty line
    if not linenum is None:
        n = linenum
        line = "!!!"
        while line != "":
            n += 1
            line = lines[n].strip()

        extra = """
/// Sets documentation strings for this algorithm
void %s::initDocs()
{
  this->setWikiSummary("%s");
  this->setOptionalMessage("%s");
}
""" % (name, escape_for_c(wikified_summary), escape_for_c(summary))
        
        # Add the extra (with padding)
        lines = insert_lines(lines, n, extra, padding)
        
        # Save to disk
        save_lines_to(lines, fullpath)
                            
    else:
        print name, "------> DECLARE_ALGORITHM not found"
        
        
#    if 0:
#        # Look for init() and then some padding before { 
#        regex = "::init\(\)([\ ]+)?\n([.\s]+)?{"
#        m = re.search(regex, source)
#        if not m is None:
#            n = m.end(0)
#            padding = m.group(2)
#            if padding is None: padding = ""


#======================================================================================
def find_corresponding_headerpath(cpp_path):
    """Return the path to the header given the .cpp path"""
    (root, filename) = os.path.split(cpp_path)
    (barename, ext) = os.path.splitext(filename)
    (project_path, src) = os.path.split(root)
    (ignore, project_name) = os.path.split(project_path)
    header_path = project_path + "/inc/Mantid%s/%s.h" % (project_name, barename)
    return header_path
    

#======================================================================================
def add_method_to_header(name, cpp_path, method):
    """ Look for a header file from a cpp path.
    Add a method declaration to it."""
    header_path = find_corresponding_headerpath(cpp_path)
    
    source = open(header_path).read()
    lines = source.split("\n");
    
    # Find the start of the private section
    (n, padding) = find_line_number(lines, "private:")
    if n is None:
        (n, padding) = find_line_number(lines, "protected:") 
    if n is None:
        (n, padding) = find_line_number(lines, "public:")
         
    if not n is None:
        # private is back 2 spaces, so add 2 spaces of padding
        padding = padding+"  "
        lines = insert_lines(lines, n+1, method, padding)
        # Now save to disk
        save_lines_to(lines, header_path)
    else:
        print name, "-------> Could not find where to insert method"


#======================================================================================
def find_algorithm_real_name(cpp_path):
    """ Look in the header for the real name of the algorithm. """
    header_path = find_corresponding_headerpath(cpp_path)
    if not os.path.exists(header_path):
        return None
    source = open(header_path).read()
    regex = r'std::string\ name\(\)\ const\ \{\ return\ \"([\w]+)\"'
    m = re.search(regex, source)
    if not m is None:
        return m.group(1) 
    else:
        return None



#======================================================================================
if __name__=="__main__":
    # Find paths to all algorithms
   
    algs = get_all_algos()
    
    for (name, fullpath) in algs:
        # Find the real algo name
        real_name = find_algorithm_real_name(fullpath)
        if real_name is None: real_name = name
          
        # Extract from the wiki
        (summary, wikified_summary) = get_wiki_summary(real_name)
        if summary == "":
            print name, "------> WIKI SUMMARY NOT FOUND at ", real_name
        else:
            add_optional_message(name, fullpath, summary, wikified_summary)
            add_method_to_header(name, fullpath, "/// Sets documentation strings for this algorithm\nvirtual void initDocs();")
    
    