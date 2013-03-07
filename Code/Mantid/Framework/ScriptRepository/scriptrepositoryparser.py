#! /usr/bin/env python
# -*- coding: utf-8 -*-
"""
The ScriptRepository parser is responsible for parsing the whole repository
in order to update the information that is available at the repository.json 
file. It belongs to the Script Repository Web Service. 

It goes through the repository tree, looking into each file to produce 
the entries of the repository.json. 

The repository.json is formed with the following structure: 
    
    path of the directory/file
        pub_date: the date when the file was last modified
        directory: flag that indicates if the entry is a directory of a file
        description: the description of the file or directory. 

This structure, can be created by the json module, using a python dictionary
whose value is a dictionary. 

.. code-block: python

    {'path':{'description':'file description','pub_date':'today'}}

        
The :func:`parse_repository` function is responsible for iterating over all the 
files inside the repository. Than, it ask the make_entry to form the 
entry for the python structure.

In order to get the description of the entries, :func:`make_entry` depends on the
:func:`get_description` function. 

The ScriptRepository design defined that the documentation of a directory
is the same as the documentation of its direct child __init__.py or README
file. So, these functions passes the information if the description of 
the current entry could be used to describe its parent directory. 

Created on Wed Mar  6 09:05:08 2013

@author: gesner
"""

import json
import os
from os.path import join
import time
import compiler
import sys



def extract_readme_doc(path):
    """
    Extract the description of README file.    
    
    The description of the README files is its entire content. 
    So, this function only copies the content of the README file.
    
    
    :param path: system path to access the file entry    
    :return : The README file description (content)

    .. note:
        It does not check for exception related to opening and reading files.    
    """
    return (open(path,'r').read())




def extract_python_doc(path):
    """
    Extract the description of Python Scripts.
    
    The description of python files may come from two different sources.
    The prefered end encouraged one, it the docstring from the python module.
    But, many files inside the script repository uses an old fashion way of 
    self descrition, thorugh a commented session at the head of the script. 
    
    So, this function will first try to extract the docstring documentation, 
    if it fails, it will try to get the commented header. If both fail, it
    will return an empty string
    
    
    :param path: system path to access the file entry    
    :return : The python module description or an empty string
    
    .. note:
        It does not check for exception related to opening and reading files, nor 
        with exception related to malformed python scripts (returned by compiler module)
    
    """
    
    ## first of all, try to get the docstring from the compiler 
    ## parserFile function, if the doc is not an empty string,
    ## return this documentation
    doc = compiler.parseFile(path).doc
    if doc : return doc

    ## extracting the comments at the header of the file
    ## it will iterate over the lines of the script
    ## till find a documentation section.
    
    comment_found = False
    comment_counts = 0
    doc = ''    
    for line in open(path,'r'):
        ## loop through the head of the file, up to the definition of the
        ## first function or class
        if line.startswith('class') or line.startswith('def'):
            # if class or function is find, this means, 
            # that the section for the header documentation 
            # has finished. break the loop
            break

        #jump the first line that may contain #! /usr/bin/env python                
        if line.startswith('#!'): continue

        # look for the comment mark        
        if line.startswith('#'):
            
            if '-*- coding:' in line: 
                #ignore the coding definition
                continue            

            if not comment_found:
                #this is the first time, we found the '#' from the header
                #count the number of '#'
                i = 0
                for char in line:
                    if char == '#' or char == ' ': 
                        i+= 1
                    else:
                        break
                comment_counts = i
                comment_found = True
            
            ## remove the first characters that are used for the documentation
            ## and add the line break
            doc += line[comment_counts:] + '\n'
        
        elif comment_found: 
            #the documentation session was already parsed, stop the loop
            break
    return doc


def get_description(path):
    """
    Extract the description of the repository entry. 
    
    It deals only with files. 
    
    From the path, it identifies which parser to use to extract the 
    information from the path (current, there are two available 
    :func:`extract_python_doc` and :func:`extract_readme_doc`). 
    
    It also identifies if the file description should be used to describe 
    its parent directory as well. README files are used to describe their
    parents directories as well as __init__.py scripts. 
    
    :param path: the system path to access the file.

    :rtype : tuple of description, directory_documentation_flag
    :return : The description of the file and a flag that indicates if this 
    documentation can be used to describe the parent directory as well. 

    This function wrappes all the exceptions that may occurr, and silently, 
    return the default value for the description ('',False) which means, empty
    description and this description should not be used to describe the
    directory.    
    """    
    try:
        if path.endswith('.py'):
            #extract python modules documentation
            return (extract_python_doc(path), path.endswith('__init__.py'))
        elif '/README' in path:
            #extract readme file documentation
            return (extract_readme_doc(path),True)            
    except:
        pass
    #default value: no description, do not use this for the directory
    return ("",False)



def make_entry(path, first_root):
    """
    Creates the python dictionary and key values necessary to describe
    each entry (file/folder) of the repository. 
    
    
    Each entry of the repository requires the following information:
        - pub_date: date of the last modification written in this format => %Y-%m-%d %H:%M:%S
        - directory: flag (true/false) to indicate if this entry is a directory or a normal file
        - description: the description of the file/folder.
    
    For each path given, it will extract the required information and it will return a tuple 
    of the entry key, entry value and the flag indicating that the description of this entry 
    should be used to the directory as well. 
    
    :param path: the path of the system to access the directory or file.
    :param first_root: The key should be related to the repository, and not its location in the specific machine,
    in order to be able to remove the location of the specific machine, the first_root, or repository root folder must
    be given.
    
    :rtype : tuple with 
    :return : path related to repository, repository entry information, flag to indicate that it contains the directory documentation
    
    .. code-block: python
    
        path_key, rep_value, dir_doc = make_entry('/var/www/scripts/README.md','/var/www/scripts/')
        # path_key == README.md
        # rep_value == {pub_date:,description:,directory:}
        # dir_doc == True    
    
    """
    try:
        timeformat="%Y-%b-%d %H:%M:%S"
        stat = os.stat(path)
        pub_date = time.strftime(timeformat,time.gmtime(int(stat.st_mtime)))
        directory = os.path.isdir(path)
        if directory:
            return (path.replace(first_root,''),{'pub_date':pub_date, 'directory':directory, 'description':""}, False)
                
        description,has_dir_desc = get_description(path)    
        return (path.replace(first_root,''),{'pub_date':pub_date, 'directory':directory, 'description':description}, has_dir_desc)
    except:
        return ("",{"description":"failed"},False)



def parse_repository(repository_path, out_directory):
    """
    Parse through all the entries (file/folders) of the repository and creates
    the repository.json file. 
    
    It walks through the entries and require :func:`make_entry` to create
    the information and compose the python dictionary that can be passed to
    :mod:`json` in order to produce the repository.json file. 
    
    This module require the right to override the repository.json file 
    presented at the out_directory.
    
    :param repository_path: the system path that access the repository folder.
    :out_directory: folder where the repository.json file will be created (override)
    """    
    fdb = dict()
    #change the system path to unix like
    first_root = repository_path.replace('\\','/')
    #directory requires the last slash
    if first_root.endswith('/'):
        first_root += '/'
    ## iterating through the folders/files
    for root, dirs, files in os.walk(first_root):
        #ignoring the git related entries
        if '.git/' in root: continue
    
        #create the entry for the current folder
        key, value, d = make_entry(root, first_root)

        #check that the path is not a .git related folder        
        if key and '.git' not in key:
            #insert at the dictionary
            fdb[key] = value
        # set the directory_key to allow to update its description
        directory_key = key
    
        #iterating through all the files in this directory    
        for name in files:
            key,value,dir_doc = make_entry(join(root,name), first_root)
            #check the entry is not .git specific
            if key and '.git' not in key:
                fdb[key] = value
            #check if the current value has the description of the parent directory
            if dir_doc:
                fdb[directory_key]['description'] = value['description']
    
    #after looping through all the entries inside the repository, 
    # it is time to create the json file
    try:
        out_path = out_directory+'/'+'repository.json'
        print 'creating out_path',out_path
        json.dump(fdb,open(out_path,'w'), sort_keys=True,
           indent=2, separators=(',', ': '))
    except: 
        print "PARSE REPOSITORY: Failed to create the repository.json file!"
        print sys.exc_info()


if __name__ == "__main__":
    parse_repository('/var/www/scripts/','/var/www')
