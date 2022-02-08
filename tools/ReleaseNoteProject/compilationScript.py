import os
import sys


def getReleaseRoot():
    script_dir = os.path.split(sys.argv[0])[0]
    return os.path.abspath(os.path.join(script_dir, '../../docs/source/release/'))


def createFileLocation():
    release_root = getReleaseRoot()
    release_root = release_root + '/Test'
    return release_root


def getReleaseNoteDirectories(path, directoryList):
    #return nothing if path is a file
    if os.path.isfile(path):
        return ()

    #add dir to directorylist if it contains .rst files
    if len([f for f in os.listdir(path) if f.endswith('.rst')])>0:
        directoryList.append(path)

    for d in os.listdir(path):
        new_path = os.path.join(path, d)
        if os.path.isdir(new_path):
            getReleaseNoteDirectories(new_path, directoryList)

    return directoryList


def checkContainsReleaseNote(path):
    releaseNoteList = []
    getReleaseNoteDirectories(path, releaseNoteList)
    releaseNoteList.remove(path)
    return releaseNoteList


pathToCheck = createFileLocation()

listofDirectories = checkContainsReleaseNote(pathToCheck)

excludedFileList = ["Bugfixes.rst", "New_features.rst", "Improvements.rst"]

for path in listofDirectories:
    combinedRst = ""
    filesScanned = []
    for file in os.listdir(path):
        if file.endswith(".rst") and file not in excludedFileList:
            filesScanned.append(file)
            with open(path + '/' + file) as f:
                contents = f.read()
                combinedRst = combinedRst + "\n" + contents
    for file in os.listdir(path):
        if file.endswith(".rst") and file in excludedFileList:
            with open(path + '/' + file, 'w') as handle:
                handle.write(combinedRst)
