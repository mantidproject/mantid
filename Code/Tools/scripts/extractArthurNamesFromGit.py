# This python script is run after running extractAlgorithmNames.py, where
# the output of that script is saved to a *.txt
#
# This script then query github for arthur of each algorithm. This script
# was original used in connection with creating usage examples for algorithms
# and creating an algorithm list for each developer to have a go at

import string, os, re

import subprocess


project = 'PyWorkflowAlg'
algorithmList = 'allAlgorithmsIn'+project+'.txt'

allAlgs = [line.strip() for line in open(algorithmList)]
#os.chdir(project+"/src")
os.chdir('PythonInterface/plugins/algorithms/WorkflowAlgorithms')
for line in allAlgs:
    #print line
    fullline = line    
    args = [
        'git', 'log',
        '--pretty=short',
        '--format="%aN"',
        '--follow',
        line
    ]

    authors = subprocess.check_output(args).replace('"', '').split('\n')
    authors = set(authors)
    authors = authors - set([''])
    
    for author in authors:
        #print author
        fullline = fullline + ", " + author
	
    print fullline

#line = 'GroupWorkspaces.cpp'
#fullline = line
#args = [
#        'git', 'log',
#        '--pretty=short',
#        '--format="%aN"',
#        '--follow',
#        line
#]

#authors = subprocess.check_output(args).replace('"', '').split('\n')
#authors = set(authors)
#authors = authors - set([''])

#for author in authors:
    #print author
#    fullline = fullline + ", " + author
	
#print fullline
