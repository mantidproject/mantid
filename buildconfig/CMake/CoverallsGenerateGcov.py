import json
import os
import sys
import getopt
import subprocess
import re
import copy
import hashlib


def getBranchName(directory):
    """Returns the name of the current git branch"""
    return subprocess.check_output(["git", "rev-parse", "--abbrev-ref", "HEAD"], cwd=directory).strip()


def getRemotes(directory):
    """Returns list of remote git repositories"""
    gitRemoteOutput = subprocess.check_output(['git', 'remote', '-v'], cwd=directory)
    remotes = []
    for line in gitRemoteOutput.splitlines():
        if '(fetch)' in line:
            splitLine = line.split()
            remotes.append({'name': splitLine[0].strip(), 'url': splitLine[1].strip()})
    return remotes


def gitLogValue(format, directory):
    """Returns git log value specified by format"""
    return subprocess.check_output(["git", "log", "-1", "--pretty=format:%" + format], cwd=directory).strip()


def getAllFilesWithExtension(directory, extension):
    """Recursively return a list of all files in directory with specified extension"""
    filesWithExtension = []
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file.endswith(extension):
                filesWithExtension.append(os.path.realpath(os.path.join(root, file)))
    return filesWithExtension


def getSourcePathFromGcovFile(gcovFilename):
    """Return the source path corresponding to a .gcov file"""
    gcovPath, gcovFilenameWithExtension = os.path.split(gcovFilename)
    srcFilename = re.sub(".gcov$", "", gcovFilenameWithExtension)
    return re.sub("#", "/", srcFilename)


def main(argv):
    arguments = ['COVERAGE_SRCS_FILE=', 'COVERALLS_OUTPUT_FILE=', 'COV_PATH=', 'PROJECT_ROOT=']
    COVERAGE_SRCS_FILE = None
    COVERALLS_OUTPUT_FILE = None
    COV_PATH = None
    PROJECT_ROOT = None
    optlist, args = getopt.getopt(argv, '', arguments)

    for o, a in optlist:
        if o == "--COVERAGE_SRCS_FILE":
            COVERAGE_SRCS_FILE = a
        elif o == "--COVERALLS_OUTPUT_FILE":
            COVERALLS_OUTPUT_FILE = a
        elif o == "--COV_PATH":
            COV_PATH = a
        elif o == "--PROJECT_ROOT":
            PROJECT_ROOT = a
        else:
            assert False, "unhandled option"

    if COVERAGE_SRCS_FILE == None:
        assert False, "COVERAGE_SRCS_FILE is not defined"
    if COVERALLS_OUTPUT_FILE == None:
        assert False, "COVERALLS_OUTPUT_FILE is not defined"
    if COV_PATH == None:
        assert False, "COV_PATH is not defined"
    if PROJECT_ROOT == None:
        assert False, "PROJECT_ROOT is not defined"

    gcdaAllFiles = getAllFilesWithExtension(COV_PATH, ".gcda")
    for gcdaFile in gcdaAllFiles:
        gcdaDirectory = os.path.dirname(gcdaFile)
        subprocess.check_call(["gcov", "-p", "-o", gcdaDirectory, gcdaFile], cwd=COV_PATH)

    gcovAllFiles = getAllFilesWithExtension(COV_PATH, ".gcov")

    sourcesToCheck = [line.strip() for line in open(COVERAGE_SRCS_FILE, 'r')]

    gcovCheckedFiles = []
    uncheckedSources = sourcesToCheck
    for gcovFile in gcovAllFiles:
        sourceWithPath = getSourcePathFromGcovFile(gcovFile)
        if sourceWithPath in sourcesToCheck:
            print "YES: ", sourceWithPath.strip(), " WAS FOUND"
            gcovCheckedFiles.append(gcovFile)
            uncheckedSources.remove(sourceWithPath)
        else:
            print "NO: ", sourceWithPath.strip(), " WAS NOT FOUND"

    coverageList = []
    for gcovFilename in gcovCheckedFiles:
        fileCoverage = {}
        #get name for json file
        sourceWithPath = getSourcePathFromGcovFile(gcovFilename)
        fileCoverage['name'] = os.path.relpath(sourceWithPath, PROJECT_ROOT)
        print "Generating JSON file for " + fileCoverage['name']
        fileCoverage['source_digest'] = hashlib.md5(open(sourceWithPath, 'rb').read()).hexdigest()
        lineCoverage = []
        gcovFile = open(gcovFilename, 'r')
        for line in gcovFile:
            line = [i.strip() for i in line.split(':')]
            lineNumber = int(line[1])
            if lineNumber != 0:
                if line[0] == '#####':
                    lineCoverage.append(0)
                elif line[0] == '-':
                    lineCoverage.append(None)
                else:
                    lineCoverage.append(int(line[0]))
                if lineNumber != len(lineCoverage):
                    raise RuntimeError['line_number does not match len(array)']
        gcovFile.close()
        fileCoverage['coverage'] = lineCoverage
        coverageList.append(copy.deepcopy(fileCoverage))

    for uncheckedFilename in uncheckedSources:
        fileCoverage = {}
        fileCoverage['name'] = os.path.relpath(uncheckedFilename, PROJECT_ROOT)
        fileCoverage['source_digest'] = hashlib.md5(open(uncheckedFilename, 'rb').read()).hexdigest()
        lineCoverage = []
        uncheckedFile = open(uncheckedFilename, 'r')
        for line in uncheckedFile:
            lineCoverage.append(0)
        uncheckedFile.close()
        fileCoverage['coverage'] = lineCoverage
        coverageList.append(copy.deepcopy(fileCoverage))

    coverallsOutput = {}
    coverallsOutput['repo_token'] = os.environ.get('COVERALLS_REPO_TOKEN')
    coverallsOutput['source_files'] = coverageList

    head = {'id':gitLogValue('H',PROJECT_ROOT),'author_name':gitLogValue('an',PROJECT_ROOT), \
            'author_email':gitLogValue('ae',PROJECT_ROOT),'committer_name':gitLogValue('cn',PROJECT_ROOT), \
            'committer_email':gitLogValue('ce',PROJECT_ROOT), 'message':gitLogValue('B',PROJECT_ROOT)}

    gitDict = {'head': head, 'branch': getBranchName(PROJECT_ROOT), 'remotes': getRemotes(COV_PATH)}
    coverallsOutput['git'] = gitDict

    with open(COVERALLS_OUTPUT_FILE, 'w') as outfile:
        json.dump(coverallsOutput, outfile, indent=4)


if __name__ == "__main__":
    main(sys.argv[1:])
