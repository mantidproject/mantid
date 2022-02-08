import os
import sys
import shutil


def getReleaseRoot():
    script_dir = os.path.split(sys.argv[0])[0]
    return os.path.abspath(os.path.join(script_dir, '../../docs/source/release/'))


def createFileLocation():
    release_root = getReleaseRoot()
    release_root = release_root + '/Test'
    return release_root


mainDirectory = createFileLocation()
includeStatement = ".. include:: "


def updateFile(originalFile, fileToAdd, textToReplace):
    mainRST = open(originalFile, 'r')
    rstToRead = mainRST.read()
    mainRST.close()

    releaseNoteFile = open(fileToAdd)
    replaceText = releaseNoteFile.read()
    releaseNoteFile.close()

    newdata = rstToRead.replace(textToReplace, replaceText)

    mainRST = open(originalFile, 'w')
    mainRST.write(newdata)
    mainRST.close()


def createFileName(fileName, mainDirectory, includeStatement):
    newFileName = fileName.replace(includeStatement, "")
    newFileName = newFileName.replace("\n", "")
    newFileName = mainDirectory + '/' + newFileName
    return newFileName


def addReleaseNotesToMain(mainDirectory):
    for file in os.listdir(mainDirectory):
        if file.endswith(".rst"):
            with open(mainDirectory + '/' + file) as fileToEdit:
                for line in fileToEdit:
                    if line.startswith(includeStatement):
                        fileName = createFileName(line, mainDirectory, includeStatement)
                        originalFile = mainDirectory + '/' + file
                        updateFile(originalFile, fileName, line)


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


def moveFiles(mainDirectory):
    listofDirectories = checkContainsReleaseNote(mainDirectory)
    for path in listofDirectories:
        print("the path is ", path)
        os.makedirs(path + '/' + 'Used')
        for file in os.listdir(path):
            print("the file is ", file)
            if file.endswith(".rst"):
                currentFileLocation = path + '/' + file
                newFileLocation = path + '/Used/' + file
                shutil.move(currentFileLocation, newFileLocation)


addReleaseNotesToMain(mainDirectory)
moveFiles(mainDirectory)
