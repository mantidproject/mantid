# Automatic creation of installer source file (.wxs)
import os
import xml
import xml.dom.minidom
import msilib
import md5
import uuid
import string
import platform

QTDIR = 'c:/qt' #hardcoded to c:/qt location - this is true for build servers and most developers
QTLIBDIR = QTDIR + '/lib'
QTPLUGINDIR = QTDIR + '/plugins'
SIPDIR = 'C:/Python25/Lib/site-packages'
PYQTDIR = SIPDIR + '/PyQt4'
USERALGORITHMSDIR = '../Mantid/UserAlgorithms'
MANTIDRELEASE = '../Mantid/release/'

vfile = open('build_number.txt','r')
vstr = vfile.read()
vlen = len(vstr)
vfile.close()

def getFileVersion():
  try:
    VERSIONFILE = '../MantidVersion.txt'
    f = open(VERSIONFILE,'r')
    line = f.readline()
    f.close()
    return line.rstrip()
  except:
    return "0.0"
#end def

MantidVersion = getFileVersion() + '.' + vstr[12:vlen-2]
print('Mantid version '+MantidVersion)

# Architecture
if platform.architecture()[0] == '64bit':
    ARCH = '64'
    toget = 'toget64'
    upgrade_uuid = '{ae4bb5c4-6b5f-4703-be9a-b5ce662b81ce}'
else:
    ARCH = '32'
    toget = 'toget'
    upgrade_uuid = '{E9B6F1A9-8CB7-4441-B783-4E7A921B37F0}'
    

# To perform a major upgrade, i.e. uninstall the old version if an old one exists, 
# the product and package GUIDs need to change everytime
product_uuid = '{' + str(uuid.uuid1()) + '}'
package_uuid = '{' + str(uuid.uuid1()) + '}'

# Setup a GUID lookup table for each of the components
# These are different for each architecture to ensure removal of the correct one when uninstalling
comp_guid = {}
if ARCH == '32':
    comp_guid['MantidDLLs'] = '{FABC0481-C18D-415e-A0B1-CCB76C35FBE8}'
    comp_guid['QTIPlot'] = '{03ABDE5C-9084-4ebd-9CF8-31648BEFDEB7}'
    comp_guid['Plugins'] = '{EEF0B4C9-DE52-4f99-A8D0-9D3C3941FA73}'
    comp_guid['Documents'] = '{C16B2B59-17C8-4cc9-8A7F-16254EB8B2F4}'
    comp_guid['Logs'] = '{0918C9A4-3481-4f21-B941-983BE21F9674}'
    comp_guid['IncludeMantidAlgorithms'] = '{EDB85D81-1CED-459a-BF87-E148CEE6F9F6}'
    comp_guid['IncludeMantidAPI'] = '{4761DDF6-813C-4470-8852-98CB9A69EBC9}'    
    comp_guid['IncludeMantidCurveFitting'] = '{44d0bdf5-e13a-4a27-8609-e273965ee860}'
    comp_guid['IncludeMantidDataHandling'] = '{DDD2DD4A-9A6A-4181-AF66-891B99DF8FFE}'
    comp_guid['IncludeMantidDataObjects'] = '{06445843-7E74-4457-B02E-4850B4911438}'
    comp_guid['IncludeMantidKernel'] = '{AF40472B-5822-4ff6-8E05-B4DA5224AA87}'
    comp_guid['IncludeMantidNexus'] = '{BAC18721-6DF1-4870-82FD-2FB37260AE35}'
    comp_guid['IncludeMantidPythonAPI'] = '{052A15D4-97A0-4ce5-A872-E6871485E734}'
    comp_guid['Temp'] = '{02D25B60-A114-4f2a-A211-DE88CF648C61}'
    comp_guid['Data'] = '{6D9A0A53-42D5-46a5-8E88-6BB4FB7A5FE1}'
    comp_guid['UserAlgorithms'] = '{A82B4540-3CDB-45fa-A7B3-42F392378D3F}'
    comp_guid['Sip'] = '{A051F48C-CA96-4cd5-B936-D446CBF67588}'
    comp_guid['Colormaps'] = '{902DBDE3-42AE-49d3-819D-1C83C18D280A}'
    comp_guid['QtImagePlugins'] = '{6e3c6f03-5933-40b1-9733-1bd71132404c}'
    comp_guid['MantidQtPlugins'] = '{d035e5aa-2815-4869-836d-8fc4b8e7a418}'
else:
    comp_guid['MantidDLLs'] = '{c9748bae-5934-44ab-b144-420589db1623}'
    comp_guid['QTIPlot'] = '{bfe90c00-9f39-4fde-8dbc-17f419210e12}'
    comp_guid['Plugins'] = '{8ef1c4db-c54d-4bb1-8b66-a9421db24faf}'
    comp_guid['Documents'] = '{bb774537-d0c6-4541-93f2-7aa5f5132d21}'
    comp_guid['Logs'] = '{0cdce87e-976a-40a5-a3d5-73dd8bce9e2e}'
    comp_guid['IncludeMantidAlgorithms'] = '{e21ee699-be01-419c-8e9a-2678c4da1e6a}'
    comp_guid['IncludeMantidAPI'] = '{878ff1f2-7d09-4817-972b-3590c45ea0c9}'    
    comp_guid['IncludeMantidCurveFitting'] = '{a766e379-deb0-4ca3-8578-0036f4722b0a}'
    comp_guid['IncludeMantidDataHandling'] = '{c937a21a-8fb9-4111-9886-d59d705e2fd8}'
    comp_guid['IncludeMantidDataObjects'] = '{955f31fd-ac47-475c-911b-55d0cd006fa0}'
    comp_guid['IncludeMantidKernel'] = '{187317c0-cc23-4a21-bf19-0e347866620c}'
    comp_guid['IncludeMantidNexus'] = '{7c60491a-36b2-402e-b989-5b8f13667cee}'
    comp_guid['IncludeMantidPythonAPI'] = '{71c4df47-5564-49ca-8c7c-5ed4d8ceb1e1}'
    comp_guid['Temp'] = '{212cc3fe-95fb-40d9-a3a7-8421791ac19f}'
    comp_guid['Data'] = '{c9577b5b-75e5-4a4a-b2d5-f4905174627c}'
    comp_guid['UserAlgorithms'] = '{496555f0-f719-4db7-bd8e-5bbcd9fe837d}'
    comp_guid['Sip'] = '{e057fcf0-47ba-4a32-a1af-d6c70e1ff8e4}'
    comp_guid['Colormaps'] = '{9e4a6fc4-39ea-4b8f-ba49-265d6dcfbb4c}'
    comp_guid['QtImagePlugins'] = '{7c1ec169-d331-4b9c-b0e4-3214bcf2cbf4}'
    comp_guid['MantidQtPlugins'] = '{22fa661e-17d5-4e33-8f2c-654c473268c3}'
    
MantidInstallDir = 'MantidInstall'

pfile = open('mantid_version.txt','w')
pfile.write(MantidVersion+'\n')
pfile.write(product_uuid)
pfile.close()

globalFileCount = 0

# Adds directory longName to parent.
# parent is a python variable (not string) representing an xml element
# Id, name, and longName are strings
def addDirectory(Id,name,longName,parent):
    e = doc.createElement('Directory')
    e.setAttribute('Id',Id)
    e.setAttribute('Name',name)
    if name != longName:
        e.setAttribute('LongName',longName)
    parent.appendChild(e)
    return e

def addFile(Id,name,longName,source,vital,parent):
    e = doc.createElement('File')
    e.setAttribute('Id',Id)
    e.setAttribute('Name',name)
    e.setAttribute('LongName',longName)
    e.setAttribute('DiskId','1')
    e.setAttribute('Source',source)
    e.setAttribute('Vital',vital)
    parent.appendChild(e)
    return e

def addFileV(Id,name,longName,source,parent):
    return addFile(Id,name,longName,source,'yes',parent)

def addFileN(Id,name,longName,source,parent):
    return addFile(Id,name,longName,source,'no',parent)

def addComponent(Id,guid,parent):
    e = doc.createElement('Component')
    e.setAttribute('Id',Id)
    e.setAttribute('Guid',guid)
    if ARCH == '64':
        e.setAttribute('Win64','yes')
    parent.appendChild(e)
    return e

# adds all dlls from location to parent.
# rules are applied to exclude debug libraries
# name is a short name to which a number will be added
def addDlls(location,name,parent, exclude_files = []):
    #print 'Include dlls from',os.path.abspath(location);
    sdlls = os.listdir(location);
    i = 0
    for fil in sdlls:
        if fil in exclude_files:
            continue
        lst = fil.split('.')
        l = len(lst)
        if l < 2 or lst[l-1] != 'dll':
            continue
        del lst[l-1]
        fn0 = string.join(lst,'_')
        fn = fn0.replace('-','_')
        if not ((fil.find('-gd-') >= 0) or (fil.find('-gyd-') >= 0) or
                (fil.find('d.dll')>=0 and fil.replace('d.dll','.dll') in sdlls) or
                (fil.find('d4.dll')>=0 and fil.replace('d4.dll','4.dll') in sdlls) or
                (fil.find('_d.dll')>=0 and fil.replace('_d.dll','.dll') in sdlls)):
            #print fil
            addFileV(fn+'DLL',name+str(i),fil,location+'/'+fil,parent)
        i += 1

def addAllFiles(location,name,parent):
    #print 'Include files from',os.path.abspath(location);
    sfiles = os.listdir(location);
    i = 0
    for fil in sfiles:
        #print fil
        fn = fil.replace('-','_')
        fn = fn.replace('+','_')
        fn = fn.replace(' ','_')
        if (fil.find('.svn') < 0 and os.path.isfile(location+'/'+fil)):
            addFileV(name+'_'+fn+'_file',name+str(i),fil,location+'/'+fil,parent)
            i += 1

def addAllFilesExt(location,name,ext,parent):
    #print 'Include files from',os.path.abspath(location);
    sfiles = os.listdir(location);
    i = 0
    for fil in sfiles:
        fn = fil.replace('-','_')
        fn = fn.replace('+','_')
        if (fil.find('.svn') < 0 and fil.endswith('.'+ext) > 0):
            #print fil
            addFileV(name+'_'+fn+'_file',name+str(i),fil,location+'/'+fil,parent)
            i += 1

def addSingleFile(location,fil,name,parent):
    #print 'Include single file'
    location = os.path.abspath(location);
    fn = name.replace('-','_')
    fn = fn.replace('+','_')
    addFileV(fn+'_file',name,fil,location+'/'+fil,parent)

def addFeature(Id,title,description,level,parent,absent='allow',allowAdvertise='yes'):
    e = doc.createElement('Feature')
    e.setAttribute('Id',Id)
    e.setAttribute('Title',title)
    e.setAttribute('Description',description)
    e.setAttribute('Level',level)
    e.setAttribute('Absent',absent)
    e.setAttribute('AllowAdvertise',allowAdvertise)
    parent.appendChild(e)
    return e

def addHiddenFeature(Id,parent):
    e = doc.createElement('Feature')
    e.setAttribute('Id',Id)
    e.setAttribute('Level','1')
    e.setAttribute('Display','hidden')
    e.setAttribute('Absent','allow')
    e.setAttribute('AllowAdvertise','no')
    parent.appendChild(e)
    return e

def addRootFeature(Id,title,description,level,parent):
    e = doc.createElement('Feature')
    e.setAttribute('Id',Id)
    e.setAttribute('Title',title)
    e.setAttribute('Description',description)
    e.setAttribute('Level',level)
    e.setAttribute('Display','expand')
    e.setAttribute('ConfigurableDirectory','INSTALLDIR')
    parent.appendChild(e)
    return e

def addCRef(Id,parent):
    e = doc.createElement('ComponentRef')
    e.setAttribute('Id',Id)
    parent.appendChild(e)

# adds to parent an element tag with dictionary of attributes attr
def addTo(parent,tag,attr):
    e = doc.createElement(tag)
    for name,value in attr.iteritems():
        e.setAttribute(name,value)
    parent.appendChild(e)
    return e

def fileSearch(Id,name,parent):
    p = addTo(parent,'Property',{'Id':Id})
    e = addTo(p,'FileSearch',{'Id':Id+'_search','LongName':name})
    return e
    
def addText(text,parent):
    e = doc.createTextNode(text)
    parent.appendChild(e)
    return e

# Copies files in nested folders from location to parent directory
# Returns a list of component names to be used in addCRefs
def addCompList(Id,location,name,parent, include_suffix=[],exclude_suffix=[]):
    global globalFileCount
    directory = addDirectory(Id+'_dir','dir',name,parent)
    lst = []
    idir = 0
#    ifil = 0
    if ARCH == '32':
        m = md5.new(location)
    else:
        m = md5.new(location + ARCH)
    u = m.hexdigest()
    uuid = '{'+u[0:8]+'-'+u[8:12]+'-'+u[12:16]+'-'+u[16:20]+'-'+u[20:]+'}'
    comp = addComponent(Id,uuid,directory)
    lst.append(Id)
    files = os.listdir(location)
    for fil in files:
        if (fil.find('.svn') < 0 and fil.find('UNIT_TESTING') < 0):
            if ( os.path.isdir(location+'/'+fil) ):
                idir += 1
                lst = lst + addCompList(Id+'_'+str(idir), location+'/'+fil, fil, directory)[0]
            else:
                keep = False
                if len(include_suffix) > 0: 
                    for sfx in include_suffix:
                        if fil.endswith(sfx):
                            keep = True
                            break
                else:
                    keep = True
                if len(exclude_suffix) > 0: 
                    for sfx in exclude_suffix:
                        if fil.endswith(sfx):
                            keep = False
                            break
                if keep == False:
                    continue
                globalFileCount += 1
                ifil = globalFileCount
                fn = fil.replace(' ','_')
                fn = fil.replace('-','_')
                fn = fn.replace('+','_')
                fileId = 'd'+fn+'_file'+str(ifil)
                fileName = 'file'+str(ifil)
                fileLongName = fil
                addFileV(fileId,fileName,fileLongName,location+'/'+fil,comp)
    return lst,comp
		
def addCRefs(lstId,parent):
    for Id in lstId:
        e = doc.createElement('ComponentRef')
        e.setAttribute('Id',Id)
        parent.appendChild(e)
 
def createPropertiesFile(filename):
    # Field replacements
    replacements = {
    "plugins.directory":"plugins.directory = ../plugins",
    "mantidqt.plugins.directory" : "mantidqt.plugins.directory = ../plugins/qtplugins/mantid",
    "instrumentDefinition.directory":"instrumentDefinition.directory = ../instrument",
    "parameterDefinition.directory":"parameterDefinition.directory = ../instrument",    
    "requiredpythonscript.directories":"""requiredpythonscript.directories = ../scripts/Crystallography;../scripts/Disordered Materials;../scripts/Engineering;\\
../scripts/Excitations;../scripts/Large Scale Structures;../scripts/Molecular Spectroscopy;\\
../scripts/Muons;../scripts/Neutrinos;../scripts/SANS""",
    "pythonscripts.directory":"pythonscripts.directory = ../scripts",
    "pythonscripts.directories":"pythonscripts.directories = ../scripts",
    "pythonalgorithms.directories":"pythonalgorithms.directories=../plugins/PythonAlgs",
    "icatDownload.directory":"icatDownload.directory = ../data"
    }

    template = open(filename,'r')
    original = template.readlines()
    prop_file = open('Mantid.properties','w')
    continuation = False
    nlines = len(original)
    index = 0
    while( index < nlines ):
        line = original[index]
        key = ""
        for rep in replacements.iterkeys():
            if line.startswith(rep):
                key = rep
                break
        if key != "":
            prop_file.write(replacements[key] + "\n")
            # Skip any backslashed lines
            while line.rstrip().endswith("\\") and index < nlines:
                index += 1
                line = original[index]
        else:
            prop_file.write(line)
        index += 1
    
    template.close()
    prop_file.close()

    
doc = xml.dom.minidom.Document()
#doc.encoding('Windows-1252')
wix = doc.createElement('Wix')
wix.setAttribute('xmlns','http://schemas.microsoft.com/wix/2003/01/wi')
doc.appendChild(wix)

Product = doc.createElement('Product')
Product.setAttribute('Id',product_uuid)
Product.setAttribute('Codepage','1252')
Product.setAttribute('UpgradeCode',upgrade_uuid)
Product.setAttribute('Version',MantidVersion)
Product.setAttribute('Manufacturer','STFC Rutherford Appleton Laboratories')
Product.setAttribute('Language','1033')
wix.appendChild(Product)

Package = doc.createElement('Package')
Package.setAttribute('Id',package_uuid)
Package.setAttribute('Keywords','Installer')
Package.setAttribute('Description','Mantid Installer')
#Package.setAttribute('Comments','')
Package.setAttribute('Manufacturer','STFC Rutherford Appleton Laboratories')
Package.setAttribute('Languages','1033')
Package.setAttribute('Compressed','yes')
Package.setAttribute('SummaryCodepage','1252')
Product.appendChild(Package)

# Architecture specific stuff
if ARCH == '64':
    Product.setAttribute('Name','Mantid ' + MantidVersion + ' (64-bit)')
    Package.setAttribute('InstallerVersion','200')
    Package.setAttribute('Platforms','x64')
else:
    Product.setAttribute('Name','Mantid ' + MantidVersion)
    Package.setAttribute('InstallerVersion','100')
    Package.setAttribute('Platforms','Intel')

Upgrade = addTo(Product,'Upgrade',{'Id':upgrade_uuid})
addTo(Upgrade,'UpgradeVersion',{'OnlyDetect':'no','Property':'PREVIOUSFOUND','Minimum': '1.0.0','IncludeMinimum':'yes','Maximum':MantidVersion,'IncludeMaximum':'no'})
addTo(Upgrade,'UpgradeVersion',{'OnlyDetect':'yes','Property':'NEWERFOUND','Minimum':MantidVersion,'IncludeMinimum':'no'})

addTo(Product,'CustomAction',{'Id':'NoDowngrade','Error':'A later version of [ProductName] is already installed.'})

exeSec = addTo(Product,'InstallExecuteSequence',{})
NoDowngrade = addTo(exeSec,'Custom',{'Action':'NoDowngrade','After':'FindRelatedProducts'})
addText('NEWERFOUND',NoDowngrade)
addTo(exeSec,'RemoveExistingProducts',{'After':'InstallInitialize'})

Media = doc.createElement('Media')
Media.setAttribute('Id','1')
Media.setAttribute('Cabinet','Mantid.cab')
Media.setAttribute('EmbedCab','yes')
Media.setAttribute('DiskPrompt','CD-ROM #1')
Product.appendChild(Media)

Prop = doc.createElement('Property')
Prop.setAttribute('Id','DiskPrompt')
Prop.setAttribute('Value','Mantid Installation')
Product.appendChild(Prop)

# PYTHON25DIR is the path to Python 2.5 
PyProp = addTo(Product,'Property',{'Id':'PYTHON25DIR'})
py_lookup = {'Id':'Python25Registry1','Type':'raw','Root':'HKLM','Key':'Software\\Python\\PythonCore\\2.5\\InstallPath'}
if ARCH == '64':
    py_lookup['Win64'] = 'yes'
# All user search
addTo(PyProp,'RegistrySearch',py_lookup)
# Current user search
py_lookup['Id'] = 'Python25Registry2'
py_lookup['Root'] = 'HKCU'
addTo(PyProp,'RegistrySearch',py_lookup)

# Below lines removed due to false positives it creates
# Just in case these completely fail but Python.exe exists on the standard location check for that
#py25dir_search = addTo(PyProp, 'DirectorySearch', {'Id':'CheckPy25Dir', 'Path':'C:\\Python25', 'Depth':'0'})
#addTo(py25dir_search, 'FileSearch', {'Id':'CheckPy25Exe', 'Name' : 'python.exe'})

Cond = doc.createElement('Condition')
if ARCH == '64':
    Cond.setAttribute('Message','Mantid requires Python 2.5 64bit to be installed on your machine. It can be downloaded and installed from http://www.python.org/download/. Please ensure you download the 64bit version.')
else:
    Cond.setAttribute('Message','Mantid requires Python 2.5 to be installed on your machine. It can be downloaded and installed from http://www.python.org/download/')
Cond.appendChild(doc.createTextNode('PYTHON25DIR'))
Product.appendChild(Cond)

TargetDir = addDirectory('TARGETDIR','SourceDir','SourceDir',Product)
InstallDir = addDirectory('INSTALLDIR','MInstall',MantidInstallDir,TargetDir)
binDir = addDirectory('MantidBin','bin','bin',InstallDir)

MantidDlls = addComponent('MantidDLLs',comp_guid['MantidDLLs'],binDir)
addTo(MantidDlls,'Registry',{'Id':'RegInstallDir','Root':'HKLM','Key':'Software\Mantid','Name':'InstallDir','Action':'write','Type':'string','Value':'[INSTALLDIR]'})
addTo(MantidDlls,'Registry',{'Id':'RegMantidVersion','Root':'HKLM','Key':'Software\Mantid','Name':'Version','Action':'write','Type':'string','Value':MantidVersion})
addTo(MantidDlls,'Registry',{'Id':'RegMantidGUID','Root':'HKLM','Key':'Software\Mantid','Name':'GUID','Action':'write','Type':'string','Value':product_uuid})

# Need to create Mantid.properties file. A template exists but some entries point to the incorrect locations so those need modifying
createPropertiesFile('../Mantid/Properties/Mantid.properties')
addFileV('MantidProperties','Mantid.pro','Mantid.properties','Mantid.properties',MantidDlls)

MantidScript = addFileV('MantidScript','MScr.bat','MantidScript.bat','../Mantid/PythonAPI/MantidScript.bat',MantidDlls)
addTo(MantidScript,'Shortcut',{'Id':'startmenuMantidScript','Directory':'ProgramMenuDir','Name':'Script','LongName':'Mantid Script','WorkingDirectory':'MantidBin'})
addFileV('MantidStartup','MStart.py','MantidStartup.py','../Mantid/PythonAPI/MantidStartup.py',MantidDlls)
addFileV('MantidPythonAPI_pyd','MPAPI.pyd','MantidPythonAPI.pyd',MANTIDRELEASE + 'MantidPythonAPI.pyd',MantidDlls)
addFileV('MantidAPI','MAPI.dll','MantidAPI.dll',MANTIDRELEASE + 'MantidAPI.dll',MantidDlls)
addFileV('MantidGeometry','MGeo.dll','MantidGeometry.dll',MANTIDRELEASE + 'MantidGeometry.dll',MantidDlls)
addFileV('MantidKernel','MKern.dll','MantidKernel.dll',MANTIDRELEASE + 'MantidKernel.dll',MantidDlls)

# Add qt API  library
addFileV('MantidQtAPI','MQTAPI.dll','MantidQtAPI.dll',MANTIDRELEASE + 'MantidQtAPI.dll',MantidDlls)
addFileV('MantidWidgets','MWid.dll','MantidWidgets.dll',MANTIDRELEASE + 'MantidWidgets.dll',MantidDlls)

# Add Qt Property Browser
addFileV('QtPropertyBrowser','QTPB.dll','QtPropertyBrowser.dll',MANTIDRELEASE + 'QtPropertyBrowser.dll',MantidDlls)

addDlls('../Mantid/Build/Plugins','PnDll',MantidDlls)
addDlls('../Third_Party/lib/win' + ARCH,'3dDll',MantidDlls,['hd421m.dll','hdf5dll.dll','hm421m.dll','libNeXus-0.dll'])

#------------- Environment settings ---------------------- 
addTo(MantidDlls,'Environment',{'Id':'UpdatePath','Name':'PATH','Action':'set','Part':'last','Value':'[PYTHON25DIR]'})
# MantidPATH to point to the bin directory
mantidbin = '[INSTALLDIR]\\bin'
addTo(MantidDlls,'Environment',{'Id':'SetMtdPath','Name':'MANTIDPATH','Action':'set','Part':'all','Value':mantidbin})
# Also add binary directory to the path
addTo(MantidDlls,'Environment',{'Id':'AddMtdPath','Name':'PATH','Action':'set','Part':'last','Value':'%MANTIDPATH%'})

# ---------------------- Matlab bindings -------------------------
# Only on 32bit windows for the moment
if ARCH == '32':
    addFileV('MantidMatlabAPI','MMAPI.dll','MantidMatlabAPI.dll',MANTIDRELEASE + 'MantidMatlabAPI.dll',MantidDlls)
    Matlab=addCompList('MatlabMFiles','../Mantid/MatlabAPI/mfiles','Matlab',binDir)[0]

    #Add mantid_setup file
    setupfile = open('mantid_setup.m','w')
    setupfile.write('mantid=\'./\';\n')
    setupfile.write('addpath(strcat(mantid,\'Matlab\'),strcat(mantid,\'Matlab/MantidGlobal\'));\n')
    setupfile.write('MantidMatlabAPI(\'SimpleAPI\',\'Create\',\'Matlab\');\n')
    setupfile.write('addpath(strcat(mantid,\'Matlab/MantidSimpleAPI\'));\n')
    setupfile.close()
    addFileV('Matlabsetup','mtd_set','mantid_setup.m','mantid_setup.m',MantidDlls)
else:
    Matlab = []
#---------------------------------------------------------------

QTIPlot = addComponent('QTIPlot',comp_guid['QTIPlot'],binDir)
addDlls(QTLIBDIR,'qt',QTIPlot)
QTIPlotEXE = addFileV('QTIPlotEXE','MPlot.exe','MantidPlot.exe',MANTIDRELEASE + 'MantidPlot.exe',QTIPlot)
# TODO: Currently the MantidLauncher only works for the 32-bit system since the registry access seems more of a pain on a 64 bit system
if ARCH== '32':
    MantidLauncher = addFileV('MantidLauncher','SMPlot.exe','StartMantidPlot.exe','MantidLauncher/Release/MantidLauncher.exe',QTIPlot)
    startmenuQTIPlot = addTo(MantidLauncher,'Shortcut',{'Id':'startmenuQTIPlot','Directory':'ProgramMenuDir','Name':'MPlot','LongName':'MantidPlot','WorkingDirectory':'MantidBin','Icon':'MantidPlot.exe'})
    desktopQTIPlot = addTo(MantidLauncher,'Shortcut',{'Id':'desktopQTIPlot','Directory':'DesktopFolder','Name':'MPlot','LongName':'MantidPlot','WorkingDirectory':'MantidBin','Icon':'MantidPlot.exe','IconIndex':'0'})
else:
    startmenuQTIPlot = addTo(QTIPlotEXE,'Shortcut',{'Id':'startmenuQTIPlot','Directory':'ProgramMenuDir','Name':'MPlot','LongName':'MantidPlot','WorkingDirectory':'MantidBin','Icon':'MantidPlot.exe'})
    desktopQTIPlot = addTo(QTIPlotEXE,'Shortcut',{'Id':'desktopQTIPlot','Directory':'DesktopFolder','Name':'MPlot','LongName':'MantidPlot','WorkingDirectory':'MantidBin','Icon':'MantidPlot.exe','IconIndex':'0'})
    
addFileV('qtiplotrc', 'qtirc.py', 'qtiplotrc.py', '../qtiplot/qtiplot/qtiplotrc.py', MantidDlls)
addFileV('qtiplotutil', 'qtiUtil.py', 'qtiUtil.py', '../qtiplot/qtiplot/qtiUtil.py', MantidDlls)
addFileV('mantidplotrc', 'mtdrc.py', 'mantidplotrc.py', '../qtiplot/qtiplot/mantidplotrc.py', MantidDlls)
addFileV('mantidplot', 'mtdplot.py', 'mantidplot.py', '../qtiplot/qtiplot/mantidplot.py', MantidDlls)

# Remove files that may have been created
files_to_remove = ['qtiplotrc.pyc','qtiUtil.pyc','mantidplotrc.pyc','mantidplot.pyc','MantidFramework.pyc','MantidHeader.pyc',\
                   'mantidsimple.py', 'mantidsimple.pyc','mtdpyalgorithm_keywords.txt']
for index, name in enumerate(files_to_remove):
    addTo(MantidDlls,'RemoveFile',{'Id':'RemFile_' + str(index),'On':'uninstall','LongName': name, 'Name':name[:8]})

if (QTLIBDIR == 'C:/Qt/4_4_0/bin'): 	 
	     manifestFile = addFileV('qtiplot_manifest','qtiexe.man','MantidPlot.exe.manifest',MANTIDRELEASE + 'MantidPlot.exe.manifest',QTIPlot)

addTo(MantidDlls,'RemoveFile',{'Id':'LogFile','On':'uninstall','Name':'mantid.log'})
addTo(Product,'Icon',{'Id':'MantidPlot.exe','SourceFile':MANTIDRELEASE + 'MantidPlot.exe'})

#plugins
pluginsDir = addDirectory('PluginsDir','plugins','plugins',InstallDir)
Plugins = addComponent('Plugins',comp_guid['Plugins'],pluginsDir)
addFileV('MantidAlgorithms','MAlg.dll','MantidAlgorithms.dll',MANTIDRELEASE + 'MantidAlgorithms.dll',Plugins)
addFileV('MantidDataHandling','MDH.dll','MantidDataHandling.dll',MANTIDRELEASE + 'MantidDataHandling.dll',Plugins)
addFileV('MantidDataObjects','MDO.dll','MantidDataObjects.dll',MANTIDRELEASE + 'MantidDataObjects.dll',Plugins)
addFileV('MantidCurveFitting','MCF.dll','MantidCurveFitting.dll',MANTIDRELEASE + 'MantidCurveFitting.dll',Plugins)
addFileV('MantidICat','MIC.dll','MantidICat.dll',MANTIDRELEASE + 'MantidICat.dll',Plugins)
addFileV('MantidNexus','MNex.dll','MantidNexus.dll',MANTIDRELEASE + 'MantidNexus.dll',Plugins)
addFileV('hdf5dlldll','hdf5dll.dll','hdf5dll.dll','../Third_Party/lib/win' + ARCH + '/hdf5dll.dll',Plugins)

#Add hdf4 for 32bit systems
if ARCH == '32':
    addFileV('hd421mdll','hd421m.dll','hd421m.dll','../Third_Party/lib/win32/hd421m.dll',Plugins)
    addFileV('hm421mdll','hm421m.dll','hm421m.dll','../Third_Party/lib/win32/hm421m.dll',Plugins)
    
addFileV('libNeXus0dll','lNeXus-0.dll','libNeXus-0.dll','../Third_Party/lib/win' + ARCH + '/libNeXus-0.dll',Plugins)

# Python algorithms
pyalgsList = addCompList("PyAlgsDir","../Mantid/PythonAPI/PythonAlgorithms","PythonAlgs",pluginsDir,exclude_suffix=['.pyc'])[0]


##
# Qt plugins
#
# Qt requires several image plugins to be able to load icons such as gif, jpeg and these need to live in
# a directory QTPLUGINSDIR/imageformats
#
# We will have the structure
# --[MantidInstall]/plugins/
#         --qtplugins
#               --imageformats/
#               --MantidQtCustom*.dll
##
qtpluginsDir = addDirectory('QtPlugInsDir','qplugdir','qtplugins',pluginsDir)
qtimageformatsDir = addDirectory('QtImageDllsDir','imgdldir','imageformats',qtpluginsDir)
qtimagedlls = addComponent('QtImagePlugins',comp_guid['QtImagePlugins'],qtimageformatsDir)
addDlls(QTPLUGINDIR + '/imageformats', 'imgdll',qtimagedlls)

# Now we need a file in the main Qt library to tell Qt where the plugins are using the qt.conf file
addSingleFile('./','qt.conf','qtcfile', MantidDlls)

# Qt plugins
mtdqtdllDir = addDirectory('MantidQtPluginsDir','mqtdir','mantid',qtpluginsDir)
mtdqtdlls = addComponent('MantidQtPlugins', comp_guid['MantidQtPlugins'], mtdqtdllDir)
addFileV('MantidQtCustomDialogs','MQTCD.dll','MantidQtCustomDialogs.dll',MANTIDRELEASE + 'MantidQtCustomDialogs.dll',mtdqtdlls)
addFileV('MantidQtCustomInterfaces','MQTCInt.dll','MantidQtCustomInterfaces.dll',MANTIDRELEASE + 'MantidQtCustomInterfaces.dll',mtdqtdlls)

documentsDir = addDirectory('DocumentsDir','docs','docs',InstallDir)
Documents = addComponent('Documents',comp_guid['Documents'],documentsDir)
addTo(Documents,'CreateFolder',{})

logsDir = addDirectory('LogsDir','logs','logs',InstallDir)
Logs = addComponent('Logs',comp_guid['Logs'],logsDir)
addTo(Logs,'CreateFolder',{})

#-------------------  Includes  -------------------------------------
includeDir = addDirectory('IncludeDir','include','include',InstallDir)
includeMantidAlgorithmsDir = addDirectory('IncludeMantidAlgorithmsDir','MAlgs','MantidAlgorithms',includeDir)
IncludeMantidAlgorithms = addComponent('IncludeMantidAlgorithms',comp_guid['IncludeMantidAlgorithms'],includeMantidAlgorithmsDir)
addAllFiles('../Mantid/includes/MantidAlgorithms','alg',IncludeMantidAlgorithms)

includeMantidAPIDir = addDirectory('IncludeMantidAPIDir','MAPI','MantidAPI',includeDir)
IncludeMantidAPI = addComponent('IncludeMantidAPI',comp_guid['IncludeMantidAPI'],includeMantidAPIDir)
addAllFiles('../Mantid/includes/MantidAPI','api',IncludeMantidAPI)

includeMantidCurveFittingDir = addDirectory('IncludeMantidCurveFittingDir','MAlgs','MantidCurveFitting',includeDir)
IncludeMantidCurveFitting = addComponent('IncludeMantidCurveFitting',comp_guid['IncludeMantidCurveFitting'],includeMantidCurveFittingDir)
addAllFiles('../Mantid/includes/MantidCurveFitting','alg',IncludeMantidCurveFitting)

includeMantidDataHandlingDir = addDirectory('IncludeMantidDataHandlingDir','MDH','MantidDataHandling',includeDir)
IncludeMantidDataHandling = addComponent('IncludeMantidDataHandling',comp_guid['IncludeMantidDataHandling'],includeMantidDataHandlingDir)
addAllFiles('../Mantid/includes/MantidDataHandling','dh',IncludeMantidDataHandling)

includeMantidDataObjectsDir = addDirectory('IncludeMantidDataObjectsDir','MDO','MantidDataObjects',includeDir)
IncludeMantidDataObjects = addComponent('IncludeMantidDataObjects',comp_guid['IncludeMantidDataObjects'],includeMantidDataObjectsDir)
addAllFiles('../Mantid/includes/MantidDataObjects','do',IncludeMantidDataObjects)

includeMantidGeometryDirList = addCompList('IncludeMantidGeometryDirList','../Mantid/includes/MantidGeometry','MantidGeometry',includeDir)[0]

includeMantidKernelDir = addDirectory('IncludeMantidKernelDir','KER','MantidKernel',includeDir)
IncludeMantidKernel = addComponent('IncludeMantidKernel',comp_guid['IncludeMantidKernel'],includeMantidKernelDir)
addAllFiles('../Mantid/includes/MantidKernel','ker',IncludeMantidKernel)

includeMantidNexusDir = addDirectory('IncludeMantidNexusDir','NEX','MantidNexus',includeDir)
IncludeMantidNexus = addComponent('IncludeMantidNexus',comp_guid['IncludeMantidNexus'],includeMantidNexusDir)
addAllFiles('../Mantid/includes/MantidNexus','nex',IncludeMantidNexus)

includeMantidPythonAPIDir = addDirectory('IncludeMantidPythonAPIDir','PAPI','MantidPythonAPI',includeDir)
IncludeMantidPythonAPI = addComponent('IncludeMantidPythonAPI',comp_guid['IncludeMantidPythonAPI'],includeMantidPythonAPIDir)
addAllFiles('../Mantid/includes/MantidPythonAPI','papi',IncludeMantidPythonAPI)

boostList = addCompList('boost','../Third_Party/include/boost','boost',includeDir)[0]
pocoList = addCompList('poco','../Third_Party/include/Poco','Poco',includeDir)[0]
#-------------------  end of Includes ---------------------------------------

sconsList = addCompList('scons','../Third_Party/src/scons-local','scons-local',InstallDir)[0]

ins_def_dir = '../../Test/Instrument'
ins_suffix = '.xml'
instrument_ids, instr_comp = addCompList('instrument',ins_def_dir,'instrument',InstallDir, include_suffix=[ins_suffix])
# At r4214 instrument cache files were moved to be written to managed workspace temp directory
# so here we'll check if old files exist next to the instrument definitions and remove them
idf_files = os.listdir(ins_def_dir)
for index, file in enumerate(idf_files):
    if not file.endswith(ins_suffix): 
        continue
    file = file.rstrip(ins_suffix)
    file += ".vtp"
    addTo(instr_comp,'RemoveFile',{'Id':'RmVTP_' + str(index),'On':'both','LongName': file, 'Name':file[:8]})

tempDir = addDirectory('TempDir','temp','temp',InstallDir)
Temp = addComponent('Temp',comp_guid['Temp'],tempDir)
addTo(Temp,'CreateFolder',{})

dataDir = addDirectory('DataDir','data','data',InstallDir)
Data = addComponent('Data',comp_guid['Data'],dataDir)
addTo(Data,'CreateFolder',{})

#-------------------  Source  ------------------------------------------
#sourceDir = addDirectory('SourceDir','source','source',InstallDir)

#sourceMantidAlgorithmsDir = addDirectory('SourceMantidAlgorithmsDir','MAlgs','MantidAlgorithms',sourceDir)
#SourceMantidAlgorithms = addComponent('SourceMantidAlgorithms','{C96FA514-351A-4e60-AC4F-EF07216BBDC3}',sourceMantidAlgorithmsDir)
#addAllFilesExt('../Mantid/Algorithms/src','alg','cpp',SourceMantidAlgorithms)

#sourceMantidAPIDir = addDirectory('SourceMantidAPIDir','MAPI','MantidAPI',sourceDir)
#SourceMantidAPI = addComponent('SourceMantidAPI','{3186462A-E033-4682-B992-DA80BAF457F2}',sourceMantidAPIDir)
#addAllFilesExt('../Mantid/API/src','api','cpp',SourceMantidAPI)

# sourceMantidDataHandlingDir = addDirectory('SourceMantidDataHandlingDir','Mdh','MantidDataHandling',sourceDir)
# SourceMantidDataHandling = addComponent('SourceMantidDataHandling','{3DE8C8E7-86F1-457f-8933-149AD79EA9D7}',sourceMantidDataHandlingDir)
# addAllFilesExt('../Mantid/DataHandling/src','dh','cpp',SourceMantidDataHandling)

# sourceMantidDataObjectsDir = addDirectory('SourceMantidDataObjectsDir','Mdo','MantidDataObjects',sourceDir)
# SourceMantidDataObjects = addComponent('SourceMantidDataObjects','{0C071065-8E0C-4e9c-996E-454692803E7F}',sourceMantidDataObjectsDir)
# addAllFilesExt('../Mantid/DataObjects/src','dh','cpp',SourceMantidDataObjects)

# sourceMantidGeometryDir = addDirectory('SourceMantidGeometryDir','MGeo','MantidGeometry',sourceDir)
# SourceMantidGeometry = addComponent('SourceMantidGeometry','{949C5B12-7D4B-4a8a-B132-718F6AEA9E69}',sourceMantidGeometryDir)
# addAllFilesExt('../Mantid/Geometry/src','geo','cpp',SourceMantidGeometry)

# sourceMantidKernelDir = addDirectory('SourceMantidKernelDir','MKer','MantidKernel',sourceDir)
# SourceMantidKernel = addComponent('SourceMantidKernel','{B7126F68-544C-4e50-9438-E0D6F6155D82}',sourceMantidKernelDir)
# addAllFilesExt('../Mantid/Kernel/src','ker','cpp',SourceMantidKernel)

# sourceMantidNexusDir = addDirectory('SourceMantidNexusDir','MNex','MantidNexus',sourceDir)
# SourceMantidNexus = addComponent('SourceMantidNexus','{35AABB59-CDE3-49bf-9F96-7A1AFB72FD2F}',sourceMantidNexusDir)
# addAllFilesExt('../Mantid/Nexus/src','nex','cpp',SourceMantidNexus)

# sourceMantidPythonAPIDir = addDirectory('SourceMantidPythonAPIDir','MPAPI','MantidPythonAPI',sourceDir)
# SourceMantidPythonAPI = addComponent('SourceMantidPythonAPI','{CACED707-92D7-47b9-8ABC-378275D99082}',sourceMantidPythonAPIDir)
# addAllFilesExt('../Mantid/PythonAPI/src','papi','cpp',SourceMantidPythonAPI)

#----------------- end of Source ---------------------------------------

#----------------- User Algorithms -------------------------------------
UserAlgorithmsDir = addDirectory('UserAlgorithmsDir','UAlgs','UserAlgorithms',InstallDir)
UserAlgorithms = addComponent('UserAlgorithms',comp_guid['UserAlgorithms'],UserAlgorithmsDir)
#all cpp, h and three specific files
addAllFilesExt(USERALGORITHMSDIR,'ualg','cpp',UserAlgorithms)
addAllFilesExt(USERALGORITHMSDIR,'ualg','h',UserAlgorithms)
addSingleFile(USERALGORITHMSDIR,'build.bat','UA_build.bat',UserAlgorithms)
addSingleFile(USERALGORITHMSDIR,'createAlg.py','UA_ca.py',UserAlgorithms)
addSingleFile(USERALGORITHMSDIR,'SConstruct','UA_Scon',UserAlgorithms)
addFileV('MantidKernel_lib','MKernel.lib','MantidKernel.lib','../Mantid/Kernel/lib/MantidKernel.lib',UserAlgorithms)
addFileV('MantidAPI_lib','MAPI.lib','MantidAPI.lib','../Mantid/API/lib/MantidAPI.lib',UserAlgorithms)
addFileV('MantidDataObjects_lib','MDObject.lib','MantidDataObjects.lib','../Mantid/DataObjects/lib/MantidDataObjects.lib',UserAlgorithms)
addFileV('MantidGeometry_lib','MGeo.lib','MantidGeometry.lib','../Mantid/Geometry/lib/MantidGeometry.lib',UserAlgorithms)
addFileV('MantidCurveFitting_lib','MFit.lib','MantidCurveFitting.lib','../Mantid/CurveFitting/lib/MantidCurveFitting.lib',UserAlgorithms)
addFileV('poco_foundation_lib','poco_f.lib','PocoFoundation.lib','../Third_Party/lib/win' + ARCH + '/PocoFoundation.lib',UserAlgorithms)
addFileV('boost_date_time_lib','boost_dt.lib','boost_date_time-vc100-mt-1_43.lib','../Third_Party/lib/win' + ARCH + '/boost_date_time-vc100-mt-1_43.lib',UserAlgorithms)

#--------------- Python ---------------------------------------------------------------------------------
Sip = addComponent('Sip',comp_guid['Sip'],binDir)
addSingleFile(SIPDIR,'sip.pyd','sip',Sip)
PyQtList = addCompList('PyQtDir', PYQTDIR,'PyQt4',binDir, exclude_suffix=['_d.pyd','.pyc'])[0]
addFileV('MtdFramework_py', 'MFWork.py', 'MantidFramework.py', '../Mantid/PythonAPI/MantidFramework.py', MantidDlls)

#-------------------------- Scripts directory and all sub-directories ------------------------------------
scriptsList = addCompList("ScriptsDir","../Mantid/PythonAPI/scripts","scripts",InstallDir)[0]
#-----------------------------------------------------------------------

#-------------------------- Colormaps ------------------------------------
ColormapsDir = addDirectory('ColormapsDir','colors','colormaps',InstallDir)
Colormaps = addComponent('Colormaps',comp_guid['Colormaps'],ColormapsDir)
addAllFiles('../qtiplot/colormaps','col',Colormaps)
#-----------------------------------------------------------------------

ProgramMenuFolder = addDirectory('ProgramMenuFolder','PMenu','Programs',TargetDir)
ProgramMenuDir = addDirectory('ProgramMenuDir','Mantid','Mantid',ProgramMenuFolder)

DesktopFolder = addDirectory('DesktopFolder','Desktop','Desktop',TargetDir)

#-----------------------------------------------------------------------

Complete = addRootFeature('Complete','Mantid','The complete package','1',Product)
MantidExec = addFeature('MantidExecAndDlls','Mantid binaries','The main executable.','1',Complete)
addCRef('MantidDLLs',MantidExec)
addCRef('Plugins',MantidExec)
addCRef('UserAlgorithms',MantidExec)
addCRef('Documents',MantidExec)
addCRef('Logs',MantidExec)
addCRefs(scriptsList,MantidExec)
addCRef('Colormaps',MantidExec)
addCRef('Temp',MantidExec)
addCRef('Data',MantidExec)
addCRefs(Matlab,MantidExec)
addCRefs(instrument_ids,MantidExec)
addCRefs(sconsList,MantidExec)
addCRef('Sip',MantidExec)
addCRefs(PyQtList,MantidExec)
addCRefs(pyalgsList,MantidExec)
addCRef('QtImagePlugins', MantidExec)
addCRef('MantidQtPlugins', MantidExec)

# C/C++ runtime. The msm files are essentially themseleves installers  and merging them in this manner causes their contents to be installed during the Mantid install
# procedure. Some dependencies still require the VC80 runtime so include these as well
Redist = addHiddenFeature('Redist',Complete)
if ARCH == '32':
    msm_files = ['Microsoft_VC80_CRT_x86.msm', 'Microsoft_VC80_OpenMP_x86.msm',\
                 'policy_8_0_Microsoft_VC80_CRT_x86.msm', 'policy_8_0_Microsoft_VC80_OpenMP_x86.msm',\
                 'Microsoft_VC100_CRT_x86.msm','Microsoft_VC100_OpenMP_x86.msm'\
                ]
    msm_dir = r'C:\Program Files\Common Files\Merge Modules'
else:
    msm_files = ['Microsoft_VC80_CRT_x86_x64.msm', 'Microsoft_VC80_OpenMP_x86_x64.msm',\
                 'policy_8_0_Microsoft_VC80_CRT_x86_x64.msm', 'policy_8_0_Microsoft_VC80_OpenMP_x86_x64.msm',\
                 'Microsoft_VC100_CRT_x64.msm','Microsoft_VC100_OpenMP_x64.msm'\
                ]
    msm_dir = r'C:\Program Files (x86)\Common Files\Merge Modules'
for index, mod in enumerate(msm_files):
    id = 'VCRed_' + str(index)
    addTo(TargetDir,'Merge',{'Id':id, 'SourceFile': os.path.join(msm_dir,mod), 'DiskId':'1','Language':'1033'})
    addTo(Redist,'MergeRef',{'Id':id})

# Header files
Includes = addFeature('Includes','Includes','Mantid and third party header files.','2',Complete)
addCRef('IncludeMantidAlgorithms',Includes)
addCRef('IncludeMantidAPI',Includes)
addCRef('IncludeMantidCurveFitting',Includes)
addCRef('IncludeMantidDataHandling',Includes)
addCRef('IncludeMantidDataObjects',Includes)
addCRefs(includeMantidGeometryDirList,Includes)
addCRef('IncludeMantidKernel',Includes)
addCRef('IncludeMantidNexus',Includes)
addCRef('IncludeMantidPythonAPI',Includes)
addCRefs(boostList,Includes)
addCRefs(pocoList,Includes)

QTIPlotExec = addFeature('QTIPlotExec','MantidPlot','MantidPlot','1',MantidExec)
addCRef('QTIPlot',QTIPlotExec)
addTo(Product,'UIRef',{'Id':'WixUI_FeatureTree'})
addTo(Product,'UIRef',{'Id':'WixUI_ErrorProgressText'})

# Output the file so that the next step in the chain can process it
f = open('tmp.wxs','w')
doc.writexml(f,newl="\r\n")
f.close()
