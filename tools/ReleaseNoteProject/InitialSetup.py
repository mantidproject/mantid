import os
import shutil
import sys

level1 = ['/Diffraction', '/Muon', '/Direct_Geometry', '/Framework']
#For upper level folders that will require Bugfixes, Improvements and New features as sub directories
level1Upper = ['/Workbench', '/Reflectometry', '/SANS', '/Indirect']

diffraction = ['/Powder', '/Single_Crystal', '/Engineering']
framework = ['/Algorithms', '/Data_Objects', '/Fit_Functions', '/Python']
workbench = ['/InstrumentViewer', '/SliceViewer']
direct = ['/General', '/CrystalField', '/MSlice']
indirect = ['/Algorithms']
muon = ['/FDA', '/Muon_Analysis', '/MA_FDA', '/ALC', '/Elemental_Analysis', '/Algorithms']

subfolders = ['/Bugfixes', '/New_features', '/Improvements']


def getReleaseRoot():
    script_dir = os.path.split(sys.argv[0])[0]
    return os.path.abspath(os.path.join(script_dir, '../../docs/source/release/'))


def getReleaseTemplateRoot():
    script_dir = os.path.split(sys.argv[0])[0]
    return os.path.abspath(os.path.join(script_dir, '../../docs/source/release/templates'))


def createFileLocation():
    release_root = getReleaseRoot()
    release_root = release_root + '/Test'
    return release_root


HigherLevel = createFileLocation()


def makeReleaseNoteDirectories():
    for directory in level1:
        os.makedirs(HigherLevel+directory, exist_ok=True)
    for directory in level1Upper:
        os.makedirs(HigherLevel + directory, exist_ok=True)
        makeReleaseNoteSubfolders(directory)
    makeSubDirectoriesFromList(diffraction, '/Diffraction')
    makeSubDirectoriesFromList(framework, '/Framework')
    makeSubDirectoriesFromList(workbench, '/Workbench')
    makeSubDirectoriesFromList(direct, '/Direct_Geometry')
    makeSubDirectoriesFromList(indirect, '/Indirect')
    makeSubDirectoriesFromList(muon, '/Muon')


def makeSubDirectoriesFromList(directoryList, upperDirectory):
    for directory in directoryList:
        combinedDirectory = upperDirectory + directory
        os.makedirs(HigherLevel + combinedDirectory, exist_ok=True)
        makeReleaseNoteSubfolders(combinedDirectory)


def makeReleaseNoteSubfolders(directory):
    for folder in subfolders:
        os.makedirs(HigherLevel + directory + folder, exist_ok=True)
        makeEmptyRST(folder, directory)


def makeEmptyRST(folder, directory):
    filename = folder + '.rst'
    fullPath = HigherLevel + directory + folder + filename
    open(fullPath, 'a').close()


def textToAppend():
    release_link = '\n:ref:`Release {0} <{1}>`'.format("Test", "Test" )
    return release_link


def makeMainReleaseNoteFiles():
    templateLocation = getReleaseTemplateRoot()
    for file in os.listdir(templateLocation):
        templateFile = templateLocation + '/' + file
        newFile = HigherLevel + '/' + file
        shutil.copyfile(templateFile, newFile)
        with open(newFile, "a") as file_to_ammend:
            file_to_ammend.write(textToAppend())


def createNewRelease():
    makeReleaseNoteDirectories()
    makeMainReleaseNoteFiles()


createNewRelease()
