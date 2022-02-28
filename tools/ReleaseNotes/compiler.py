#!/usr/bin/env python
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import sys

EXCLUDED_FILES = ["Bugfixes.rst", "New_features.rst"]


def getReleaseRoot():
    script_dir = os.path.split(sys.argv[0])[0]
    return os.path.abspath(os.path.join(script_dir, '../../docs/source/release/'))


def createFileLocation(release_num):
    release_root = getReleaseRoot()
    release_root = os.path.join(release_root, release_num)
    return release_root


def getReleaseNoteDirectories(path, directoryList):
    # return nothing if path is a file
    if os.path.isfile(path):
        return ()

    # add dir to directorylist if it contains .rst files
    if len([f for f in os.listdir(path) if f.endswith('.rst')]) > 0:
        directoryList.append(path)

    for d in os.listdir(path):
        new_path = os.path.join(path, d)
        if os.path.isdir(new_path):
            getReleaseNoteDirectories(new_path, directoryList)

    return directoryList


def fixReleaseName(name):
    if name.startswith('v'):
        name = name[1:]

    # make sure that all of the parts can be converted to integers
    try:
        version = [int(i) for i in name.split('.')]
    except ValueError as e:
        raise RuntimeError('expected version number form: major.minor.patch', e)
    if len(version) == 3:
        pass # perfect
    elif len(version) == 2:
        name += '.0'
    elif len(version) == 1:
        name += '.0.0'
    else:
        raise RuntimeError('expected version number form: major.minor.patch')

    return 'v' + name


def checkContainsReleaseNote(path):
    releaseNoteList = []
    getReleaseNoteDirectories(path, releaseNoteList)
    releaseNoteList.remove(path)
    return releaseNoteList


if __name__ == '__main__':
    from argparse import ArgumentParser
    parser = ArgumentParser(description="Compile release notes")
    parser.add_argument('--release', required=True)
    args = parser.parse_args()
    args.release = fixReleaseName(args.release)

    pathToCheck = createFileLocation(args.release)

    listofDirectories = checkContainsReleaseNote(pathToCheck)

    for path in listofDirectories:
        combinedRst = ""
        filesScanned = []
        for file in os.listdir(path):
            if file.endswith(".rst") and file not in EXCLUDED_FILES:
                filesScanned.append(file)
                with open(path + '/' + file) as f:
                    contents = f.read()
                    combinedRst = combinedRst + "\n" + contents
        for file in os.listdir(path):
            if file.endswith(".rst") and file in EXCLUDED_FILES:
                with open(path + '/' + file, 'w') as handle:
                    handle.write(combinedRst)
