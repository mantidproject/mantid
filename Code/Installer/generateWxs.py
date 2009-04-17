# Automatic creation of installer source file (.wxs)
import os
import os.path
import xml
import xml.dom.minidom
import msilib
import md5

import string

QTDIR = 'toget/qt'
#QTDIR = 'c:/qt/bin'

vfile = open('build_number.txt','r')
vstr = vfile.read()
vlen = len(vstr)
vfile.close()

MantidVersion = '1.0.' + vstr[12:vlen-1]
print('Mantid version '+MantidVersion)

#product_uuid = '{5EE8BEAB-286E-4968-9D80-6018DE38E9A4}'
#product_uuid = '{83F7E727-C069-4d45-A3F9-F47797F7970F}'
#product_uuid='{EC1C2D8E-4214-4776-B07F-FB11F98845D1}'
product_uuid='{1ba91460-2b2a-11de-8c30-0800200c9a66}'

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
    parent.appendChild(e)
    return e

# adds all dlls from location to parent.
# rules are applied to exclude debug libraries
# name is a short name to which a number will be added
def addDlls(location,name,parent, exclud = []):
    #print 'Include dlls from',os.path.abspath(location);
    sdlls = os.listdir(location);
    i = 0
    for fil in sdlls:
        if fil in exclud:
            continue
        lst = fil.split('.')
        l = len(lst)
        if l < 2 or lst[l-1] != 'dll':
            continue
        del lst[l-1]
        fn0 = string.join(lst,'_')
        fn = fn0.replace('-','_')
        if not ((fil.find('-gd-') >= 0) or
                (fil.find('d.dll')>=0 and fil.replace('d.dll','.dll') in sdlls) or
                (fil.find('d4.dll')>=0 and fil.replace('d4.dll','4.dll') in sdlls)):
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
def addCompList(Id,location,name,parent):
    global globalFileCount
    directory = addDirectory(Id+'_dir','dir',name,parent)
    lst = []
    idir = 0
#    ifil = 0
    m = md5.new(location)
    u = m.hexdigest()
    uuid = '{'+u[0:8]+'-'+u[8:12]+'-'+u[12:16]+'-'+u[16:20]+'-'+u[20:]+'}'
    comp = addComponent(Id,uuid,directory)
    lst.append(Id)
    files = os.listdir(location)
    for fil in files:
        if ( fil.find('.svn') < 0 and os.path.isdir(location+'/'+fil) ):
            idir += 1
            lst = lst + addCompList(Id+'_'+str(idir), location+'/'+fil, fil, directory)
        elif fil.find('.svn') < 0:
            globalFileCount += 1
            ifil = globalFileCount
            fn = fil.replace('-','_')
            fn = fn.replace('+','_')
            fileId = 'd'+fn+'_file'+str(ifil)
            fileName = 'file'+str(ifil)
            fileLongName = fil
            addFileV(fileId,fileName,fileLongName,location+'/'+fil,comp)
    return lst
		
def addCRefs(lstId,parent):
    for Id in lstId:
        e = doc.createElement('ComponentRef')
        e.setAttribute('Id',Id)
        parent.appendChild(e)
 
doc = xml.dom.minidom.Document()
#doc.encoding('Windows-1252')
wix = doc.createElement('Wix')
wix.setAttribute('xmlns','http://schemas.microsoft.com/wix/2003/01/wi')
doc.appendChild(wix)

Product = doc.createElement('Product')
Product.setAttribute('Name','Mantid '+ MantidVersion)
#Product.setAttribute('Id','{5EE8BEAB-286E-4968-9D80-6018DE38E9A4}')
Product.setAttribute('Id',product_uuid)
Product.setAttribute('Language','1033')
Product.setAttribute('Codepage','1252')
Product.setAttribute('UpgradeCode','{E9B6F1A9-8CB7-4441-B783-4E7A921B37F0}')
Product.setAttribute('Version',MantidVersion)
Product.setAttribute('Manufacturer','STFC Rutherford Appleton Laboratories')
wix.appendChild(Product)

Package = doc.createElement('Package')
Package.setAttribute('Id','????????-????-????-????-????????????')
Package.setAttribute('Keywords','Installer')
Package.setAttribute('Description','Mantid Installer')
#Package.setAttribute('Comments','')
Package.setAttribute('Manufacturer','STFC Rutherford Appleton Laboratories')
Package.setAttribute('InstallerVersion','100')
Package.setAttribute('Languages','1033')
Package.setAttribute('Compressed','yes')
Package.setAttribute('SummaryCodepage','1252')
Product.appendChild(Package)

Upgrade = addTo(Product,'Upgrade',{'Id':'{E9B6F1A9-8CB7-4441-B783-4E7A921B37F0}'})
addTo(Upgrade,'UpgradeVersion',{'OnlyDetect':'yes','Property':'PATCHFOUND','Minimum':MantidVersion,'IncludeMinimum':'yes','Maximum':MantidVersion,'IncludeMaximum':'yes'})
addTo(Upgrade,'UpgradeVersion',{'OnlyDetect':'yes','Property':'NEWERFOUND','Minimum':MantidVersion,'IncludeMinimum':'no'})

Media = doc.createElement('Media')
Media.setAttribute('Id','1')
Media.setAttribute('Cabinet','Mantid.cab')
Media.setAttribute('EmbedCab','yes')
Media.setAttribute('DiskPrompt','CD-ROM #1')
Product.appendChild(Media)

addTo(Product,'CustomAction',{'Id':'AlreadyUpdated','Error':'[ProductName] is already installed.'})
addTo(Product,'CustomAction',{'Id':'NoDowngrade','Error':'A later version of [ProductName] is already installed.'})

Prop = doc.createElement('Property')
Prop.setAttribute('Id','DiskPrompt')
Prop.setAttribute('Value','Mantid Installation')
Product.appendChild(Prop)

# PYTHON25DIR is the path to Python 2.5 
PyProp = addTo(Product,'Property',{'Id':'PYTHON25DIR'})
addTo(PyProp,'RegistrySearch',{'Id':'Python25Registry1','Type':'raw','Root':'HKLM','Key':'Software\\Python\\PythonCore\\2.5\\InstallPath'})
addTo(PyProp,'RegistrySearch',{'Id':'Python25Registry2','Type':'raw','Root':'HKCU','Key':'Software\\Python\\PythonCore\\2.5\\InstallPath'})

Cond = doc.createElement('Condition')
Cond.setAttribute('Message','Mantid requires Python 2.5 to be installed on your machine. It can be downloaded and installed from http://www.python.org/download/')
Cond.appendChild(doc.createTextNode('PYTHON25DIR'))
Product.appendChild(Cond)

#TargetDir = addDirectory('TARGETDIR','SourceDir','SourceDir',Product)
TargetDir = addDirectory('TARGETDIR','WVolume','WindowsVolume',Product)
InstallDir = addDirectory('INSTALLDIR','MInstall',MantidInstallDir,TargetDir)
binDir = addDirectory('MantidBin','bin','bin',InstallDir)

MantidDlls = addComponent('MantidDLLs','{FABC0481-C18D-415e-A0B1-CCB76C35FBE8}',binDir)
addTo(MantidDlls,'Registry',{'Id':'RegInstallDir','Root':'HKLM','Key':'Software\Mantid','Name':'InstallDir','Action':'write','Type':'string','Value':'[INSTALLDIR]'})
addTo(MantidDlls,'Registry',{'Id':'RegMantidVersion','Root':'HKLM','Key':'Software\Mantid','Name':'Version','Action':'write','Type':'string','Value':MantidVersion})
addTo(MantidDlls,'Registry',{'Id':'RegMantidGUID','Root':'HKLM','Key':'Software\Mantid','Name':'GUID','Action':'write','Type':'string','Value':product_uuid})
# Modify Mantid.properties to set directories right
prop_file = open('../Mantid/Properties/Mantid.properties','r')
prop_file_ins = open('Mantid.properties','w')
for line in prop_file:
    if line.find('ManagedWorkspace.LowerMemoryLimit') >= 0:
        prop_file_ins.write('ManagedWorkspace.LowerMemoryLimit = 40\n')
    elif line.find('plugins.directory') >= 0:
        prop_file_ins.write('plugins.directory = ../plugins\n')
    elif line.find('pythonscripts.directory') >= 0:
        prop_file_ins.write('pythonscripts.directory = ../scripts\n')
    elif line.find('instrumentDefinition.directory') >= 0:
        prop_file_ins.write('instrumentDefinition.directory = ../instrument\n')
    else:
        prop_file_ins.write(line)
prop_file_ins.close()
prop_file.close()
addFileV('MantidProperties','Mantid.pro','Mantid.properties','Mantid.properties',MantidDlls)
MantidScript = addFileV('MantidScript','MScr.bat','MantidScript.bat','../Mantid/PythonAPI/MantidScript.bat',MantidDlls)
addTo(MantidScript,'Shortcut',{'Id':'startmenuMantidScript','Directory':'ProgramMenuDir','Name':'Script','LongName':'Mantid Script','WorkingDirectory':'MantidBin'})
addFileV('MantidStartup','MStart.py','MantidStartup.py','../Mantid/PythonAPI/MantidStartup.py',MantidDlls)
addFileV('MantidHeader','MHeader.py','MantidHeader.py','../Mantid/PythonAPI/MantidHeader.py',MantidDlls)
addFileV('MantidPythonAPI_pyd','MPAPI.pyd','MantidPythonAPI.pyd','../Mantid/Bin/Shared/MantidPythonAPI.dll',MantidDlls)
addFileV('MantidAPI','MAPI.dll','MantidAPI.dll','../Mantid/Bin/Shared/MantidAPI.dll',MantidDlls)
addFileV('MantidGeometry','MGeo.dll','MantidGeometry.dll','../Mantid/Bin/Shared/MantidGeometry.dll',MantidDlls)
addFileV('MantidKernel','MKern.dll','MantidKernel.dll','../Mantid/Bin/Shared/MantidKernel.dll',MantidDlls)
addFileV('MantidPythonAPI','MPAPI.dll','MantidPythonAPI.dll','../Mantid/Bin/Shared/MantidPythonAPI.dll',MantidDlls)

# Add qt API  library
addFileV('MantidQtAPI','MQTAPI.dll','MantidQtAPI.dll','../qtiplot/MantidQt/lib/MantidQtAPI.dll',MantidDlls)


addAllFiles('toget/MSVCruntime','ms',MantidDlls)

#  these two should go to plugins 
#addFileV('MantidDataHandling_tmp','MDH.dll','MantidDataHandling.dll','../Mantid/Bin/Shared/MantidDataHandling.dll',MantidDlls)
#addFileV('MantidDataObjects_tmp','MDO.dll','MantidDataObjects.dll','../Mantid/Bin/Shared/MantidDataObjects.dll',MantidDlls)

addDlls('../Mantid/Bin/Plugins','PnDll',MantidDlls)
addDlls('../Third_Party/lib/win32','3dDll',MantidDlls,['hd421m.dll','hdf5dll.dll','hm421m.dll','libNeXus-0.dll'])


# ---------------------- Matlab bindings -------------------------
addFileV('MantidMatlabAPI','MMAPI.dll','MantidMatlabAPI.dll','../Mantid/Bin/Shared/MantidMatlabAPI.dll',MantidDlls)
Matlab=addCompList('MatlabMFiles','toget/Matlab','Matlab',binDir)

#Add mantid_setup file
setupfile = open('mantid_setup.m','w')
setupfile.write('mantid=\'./\';\n')
setupfile.write('addpath(strcat(mantid,\'Matlab\'),strcat(mantid,\'Matlab/MantidGlobal\'));\n')
setupfile.write('MantidMatlabAPI(\'SimpleAPI\',\'Create\',\'Matlab\');\n')
setupfile.write('addpath(strcat(mantid,\'Matlab/MantidSimpleAPI\'));\n')
setupfile.close()

addFileV('Matlabsetup','mtd_set','mantid_setup.m','mantid_setup.m',MantidDlls)
#---------------------------------------------------------------

QTIPlot = addComponent('QTIPlot','{03ABDE5C-9084-4ebd-9CF8-31648BEFDEB7}',binDir)
addDlls(QTDIR,'qt',QTIPlot)
QTIPlotEXE = addFileV('QTIPlotEXE','MPlot.exe','MantidPlot.exe','../qtiplot/qtiplot/qtiplot.exe',QTIPlot)
MantidLauncher = addFileV('MantidLauncher','SMPlot.exe','StartMantidPlot.exe','MantidLauncher/Release/MantidLauncher.exe',QTIPlot)
startmenuQTIPlot = addTo(MantidLauncher,'Shortcut',{'Id':'startmenuQTIPlot','Directory':'ProgramMenuDir','Name':'MPlot','LongName':'MantidPlot','WorkingDirectory':'MantidBin','Icon':'MantidPlot.exe'})
desktopQTIPlot = addTo(MantidLauncher,'Shortcut',{'Id':'desktopQTIPlot','Directory':'DesktopFolder','Name':'MPlot','LongName':'MantidPlot','WorkingDirectory':'MantidBin','Icon':'MantidPlot.exe','IconIndex':'0'})
addAllFiles('toget/pyc','pyc',QTIPlot)
if (QTDIR == 'C:/Qt/4_4_0/bin'): 	 
	     manifestFile = addFileV('qtiplot_manifest','qtiexe.man','MantidPlot.exe.manifest','../qtiplot/qtiplot/qtiplot.exe.manifest',QTIPlot)

addTo(MantidDlls,'RemoveFile',{'Id':'LogFile','On':'uninstall','Name':'mantid.log'})
addTo(Product,'Icon',{'Id':'MantidPlot.exe','SourceFile':'../qtiplot/qtiplot/qtiplot.exe'})

#plugins
pluginsDir = addDirectory('PluginsDir','plugins','plugins',InstallDir)
Plugins = addComponent('Plugins','{EEF0B4C9-DE52-4f99-A8D0-9D3C3941FA73}',pluginsDir)
addFileV('MantidAlgorithms','MAlg.dll','MantidAlgorithms.dll','../Mantid/Bin/Shared/MantidAlgorithms.dll',Plugins)
addFileV('MantidDataHandling','MDH.dll','MantidDataHandling.dll','../Mantid/Bin/Shared/MantidDataHandling.dll',Plugins)
addFileV('MantidDataObjects','MDO.dll','MantidDataObjects.dll','../Mantid/Bin/Shared/MantidDataObjects.dll',Plugins)
addFileV('MantidCurveFitting','MCF.dll','MantidCurveFitting.dll','../Mantid/Bin/Shared/MantidCurveFitting.dll',Plugins)
#nexusDir = addDirectory('NexusDir','Nexus','Nexus',pluginsDir)
#Nexus = addComponent('Nexus','{A67F6FC8-7BBC-4aa5-A38F-1A522287D236}',nexusDir)
addFileV('MantidNexus','MNex.dll','MantidNexus.dll','../Mantid/Bin/Shared/MantidNexus.dll',Plugins)
addFileV('hd421mdll','hd421m.dll','hd421m.dll','../Third_Party/lib/win32/hd421m.dll',Plugins)
addFileV('hdf5dlldll','hdf5dll.dll','hdf5dll.dll','../Third_Party/lib/win32/hdf5dll.dll',Plugins)
addFileV('hm421mdll','hm421m.dll','hm421m.dll','../Third_Party/lib/win32/hm421m.dll',Plugins)
addFileV('libNeXus0dll','lNeXus-0.dll','libNeXus-0.dll','../Third_Party/lib/win32/libNeXus-0.dll',Plugins)

# Add qt custom dialogs library
addFileV('MantidQtCustomDialogs','MQTCD.dll','MantidQtCustomDialogs.dll','../qtiplot/MantidQt/lib/MantidQtCustomDialogs.dll',Plugins)
# Add qt custom interfaces library
addFileV('MantidQtCustomInterfaces','MQTCInt.dll','MantidQtCustomInterfaces.dll','../qtiplot/MantidQt/lib/MantidQtCustomInterfaces.dll',Plugins)

documentsDir = addDirectory('DocumentsDir','docs','docs',InstallDir)
Documents = addComponent('Documents','{C16B2B59-17C8-4cc9-8A7F-16254EB8B2F4}',documentsDir)
addTo(Documents,'CreateFolder',{})

logsDir = addDirectory('LogsDir','logs','logs',InstallDir)
Logs = addComponent('Logs','{0918C9A4-3481-4f21-B941-983BE21F9674}',logsDir)
addTo(Logs,'CreateFolder',{})

#-------------------  Includes  -------------------------------------
includeDir = addDirectory('IncludeDir','include','include',InstallDir)
includeMantidAlgorithmsDir = addDirectory('IncludeMantidAlgorithmsDir','MAlgs','MantidAlgorithms',includeDir)
IncludeMantidAlgorithms = addComponent('IncludeMantidAlgorithms','{EDB85D81-1CED-459a-BF87-E148CEE6F9F6}',includeMantidAlgorithmsDir)
addAllFiles('../Mantid/includes/MantidAlgorithms','alg',IncludeMantidAlgorithms)

includeMantidAPIDir = addDirectory('IncludeMantidAPIDir','MAPI','MantidAPI',includeDir)
IncludeMantidAPI = addComponent('IncludeMantidAPI','{4761DDF6-813C-4470-8852-98CB9A69EBC9}',includeMantidAPIDir)
addAllFiles('../Mantid/includes/MantidAPI','api',IncludeMantidAPI)

includeMantidDataHandlingDir = addDirectory('IncludeMantidDataHandlingDir','MDH','MantidDataHandling',includeDir)
IncludeMantidDataHandling = addComponent('IncludeMantidDataHandling','{DDD2DD4A-9A6A-4181-AF66-891B99DF8FFE}',includeMantidDataHandlingDir)
addAllFiles('../Mantid/includes/MantidDataHandling','dh',IncludeMantidDataHandling)

includeMantidDataObjectsDir = addDirectory('IncludeMantidDataObjectsDir','MDO','MantidDataObjects',includeDir)
IncludeMantidDataObjects = addComponent('IncludeMantidDataObjects','{06445843-7E74-4457-B02E-4850B4911438}',includeMantidDataObjectsDir)
addAllFiles('../Mantid/includes/MantidDataObjects','do',IncludeMantidDataObjects)

includeMantidGeometryDir = addDirectory('IncludeMantidGeometryDir','GEO','MantidGeometry',includeDir)
IncludeMantidGeometry = addComponent('IncludeMantidGeometry','{AF39B1A0-5068-4f2d-B9B9-D69926404686}',includeMantidGeometryDir)
addAllFiles('../Mantid/includes/MantidGeometry','geo',IncludeMantidGeometry)

includeMantidKernelDir = addDirectory('IncludeMantidKernelDir','KER','MantidKernel',includeDir)
IncludeMantidKernel = addComponent('IncludeMantidKernel','{AF40472B-5822-4ff6-8E05-B4DA5224AA87}',includeMantidKernelDir)
addAllFiles('../Mantid/includes/MantidKernel','ker',IncludeMantidKernel)

includeMantidNexusDir = addDirectory('IncludeMantidNexusDir','NEX','MantidNexus',includeDir)
IncludeMantidNexus = addComponent('IncludeMantidNexus','{BAC18721-6DF1-4870-82FD-2FB37260AE35}',includeMantidNexusDir)
addAllFiles('../Mantid/includes/MantidNexus','nex',IncludeMantidNexus)

includeMantidPythonAPIDir = addDirectory('IncludeMantidPythonAPIDir','PAPI','MantidPythonAPI',includeDir)
IncludeMantidPythonAPI = addComponent('IncludeMantidPythonAPI','{052A15D4-97A0-4ce5-A872-E6871485E734}',includeMantidPythonAPIDir)
addAllFiles('../Mantid/includes/MantidPythonAPI','papi',IncludeMantidPythonAPI)

boostList = addCompList('boost','../Third_Party/include/boost','boost',includeDir)
pocoList = addCompList('poco','../Third_Party/include/Poco','Poco',includeDir)
#-------------------  end of Includes ---------------------------------------

sconsList = addCompList('scons','../Third_Party/src/scons-local','scons-local',InstallDir)

instrument = addCompList('instrument','../../Test/Instrument','instrument',InstallDir)

tempDir = addDirectory('TempDir','temp','temp',InstallDir)
Temp = addComponent('Temp','{02D25B60-A114-4f2a-A211-DE88CF648C61}',tempDir)
addTo(Temp,'CreateFolder',{})

dataDir = addDirectory('DataDir','data','data',InstallDir)
Data = addComponent('Data','{6D9A0A53-42D5-46a5-8E88-6BB4FB7A5FE1}',dataDir)
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
UserAlgorithms = addComponent('UserAlgorithms','{A82B4540-3CDB-45fa-A7B3-42F392378D3F}',UserAlgorithmsDir)
addAllFilesExt('../Mantid/UserAlgorithms','ualg','cpp',UserAlgorithms)
addAllFilesExt('../Mantid/UserAlgorithms','ualg','h',UserAlgorithms)
#addFileV('Sconstruct','Sconstr','Sconstruct','toget/UserAlgorithms/Sconstruct',UserAlgorithms)
#addFileV('build_bat','build.bat','build.bat','toget/UserAlgorithms/build.bat',UserAlgorithms)
addAllFiles('toget/UserAlgorithms','UA',UserAlgorithms)
addFileV('MantidKernel_lib','MKernel.lib','MantidKernel.lib','../Mantid/Kernel/lib/MantidKernel.lib',UserAlgorithms)
addFileV('MantidAPI_lib','MAPI.lib','MantidAPI.lib','../Mantid/API/lib/MantidAPI.lib',UserAlgorithms)
addFileV('MantidDataObjects_lib','MDObject.lib','MantidDataObjects.lib','../Mantid/DataObjects/lib/MantidDataObjects.lib',UserAlgorithms)
addFileV('MantidGeometry_lib','MGeo.lib','MantidGeometry.lib','../Mantid/Geometry/lib/MantidGeometry.lib',UserAlgorithms)
addFileV('poco_foundation_lib','poco_f.lib','PocoFoundation.lib','../Third_Party/lib/win32/PocoFoundation.lib',UserAlgorithms)

#--------------- Python ------------------------------------------------

Python25Dir = addTo(TargetDir,'Directory',{'Id':'PYTHON25DIR'})
LibDir = addDirectory('LibDir','Lib','Lib',Python25Dir)
SitePackagesDir = addDirectory('SitePackagesDir','sitepack','site-packages',LibDir)
PyQtDir = addDirectory('PyQtDir','PyQt4','PyQt4',SitePackagesDir)
Sip = addComponent('Sip','{A051F48C-CA96-4cd5-B936-D446CBF67588}',SitePackagesDir)
addAllFiles('toget/sip','sip',Sip)
PyQt = addComponent('PyQt','{18028C0B-9DF4-48f6-B8FC-DE195FE994A0}',PyQtDir)
addAllFiles('toget/PyQt4','PyQt',PyQt)

#-------------------------- Scripts directory and all sub-directories ------------------------------------
scriptsList = addCompList("ScriptsDir","../Mantid/PythonAPI/scripts","scripts",InstallDir)
#-----------------------------------------------------------------------

#-------------------------- Colormaps ------------------------------------
ColormapsDir = addDirectory('ColormapsDir','colors','colormaps',InstallDir)
Colormaps = addComponent('Colormaps','{902DBDE3-42AE-49d3-819D-1C83C18D280A}',ColormapsDir)
addAllFiles('../qtiplot/colormaps','col',Colormaps)
#-----------------------------------------------------------------------

ProgramMenuFolder = addDirectory('ProgramMenuFolder','PMenu','Programs',TargetDir)
ProgramMenuDir = addDirectory('ProgramMenuDir','Mantid','Mantid',ProgramMenuFolder)

DesktopFolder = addDirectory('DesktopFolder','Desktop','Desktop',TargetDir)

#-----------------------------------------------------------------------
sfiles = os.listdir('toget/PyQt4');
i=1;
for file in sfiles:
  if( file == '.svn' ):
    continue;
  decomp=file.partition('.')
  id=decomp[0].upper()
  id+='EXISTS'
  PyQtFileExists = addTo(Product,'Property',{'Id':id})
  DirSearch = addTo(PyQtFileExists, 'DirectorySearch',{'Id':'PyQtDir_' + str(i),'Path':'[PYTHON25DIR]\Lib\site-packages\PyQt4','Depth':'0'})
  # pyc files treated differenty as they could just have the .py file that has not been compiled yet
  if( decomp[2] == 'pyc' ):
    file = decomp[0] + decomp[1] + 'py'
  addTo(DirSearch,'FileSearch',{'Id':'PyQt_' + str(i),'LongName':file})
  i += 1;
  
sfiles = os.listdir('toget/sip');
i=1
for file in sfiles:
  if( file == '.svn' ):
    continue;
  decomp=file.partition('.')
  id=decomp[0].upper()
  id+='EXISTS'
  SipFileExists = addTo(Product,'Property',{'Id':id})
  DirSearch = addTo(SipFileExists, 'DirectorySearch',{'Id':'SipDirSearch' + str(i),'Path':'[PYTHON25DIR]\Lib\site-packages','Depth':'0'})
  # pyc files treated differenty as they could just have the .py file that has not been compiled yet
  if( decomp[2] == 'pyc' ):
    file = decomp[0] + decomp[1] + 'py'
  addTo(DirSearch,'FileSearch',{'Id':'SipFileSearch' + str(i),'LongName':file})
  i += 1;
 
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
addCRefs(instrument,MantidExec)
addCRefs(sconsList,MantidExec)

Includes = addFeature('Includes','Includes','Mantid and third party header files.','1',Complete)
addCRef('IncludeMantidAlgorithms',Includes)
addCRef('IncludeMantidAPI',Includes)
addCRef('IncludeMantidDataHandling',Includes)
addCRef('IncludeMantidDataObjects',Includes)
addCRef('IncludeMantidGeometry',Includes)
addCRef('IncludeMantidKernel',Includes)
addCRef('IncludeMantidNexus',Includes)
addCRef('IncludeMantidPythonAPI',Includes)
addCRefs(boostList,Includes)
addCRefs(pocoList,Includes)

QTIPlotExec = addFeature('QTIPlotExec','MantidPlot','MantidPlot','1',MantidExec)
addCRef('QTIPlot',QTIPlotExec)

#Prevent overwriting existing PyQt installation.
PyQtF = addFeature('PyQtF','PyQt','PyQt4 v4.4.3','1',QTIPlotExec,'disallow','no')
addCRef('PyQt',PyQtF)
sfiles = os.listdir('toget/PyQt4');
PyQtTest='(UILevel <> 3)'
for file in sfiles:
  if( file == '.svn' ):
    continue;
  id=file.partition('.')[0].upper()
  id+='EXISTS'
  PyQtTest += ' AND ' + id

addText(PyQtTest, addTo(PyQtF,'Condition',{'Level':'0'}))

# Prevent overwriting exising sip files
SipPyd = addFeature('SipPyd','Sip','Sip v4.7.7','1',QTIPlotExec, 'disallow','no')
addCRef('Sip',SipPyd)
sfiles = os.listdir('toget/sip');
SipTest='(UILevel <> 3)'
for file in sfiles:
  if( file == '.svn' ):
    continue;
  id=file.partition('.')[0].upper()
  id+='EXISTS'
  SipTest += ' AND ' + id
  
addText(SipTest, addTo(SipPyd,'Condition',{'Level':'0'}))

#------------- Source files ------------------------
#SourceFiles = addFeature('SourceFiles','SourceFiles','SourceFiles','1',Complete)
#addCRef('SourceMantidAlgorithms',SourceFiles)
#addCRef('SourceMantidAPI',SourceFiles)
# addCRef('SourceMantidDataHandling',SourceFiles)
# addCRef('SourceMantidDataObjects',SourceFiles)
# addCRef('SourceMantidGeometry',SourceFiles)
# addCRef('SourceMantidKernel',SourceFiles)
# addCRef('SourceMantidNexus',SourceFiles)
# addCRef('SourceMantidPythonAPI',SourceFiles)



#tstDir = addDirectory('TstDir','tst','tst',InstallDir)
#TstTst = addComponent('TSTST','{42AD79C6-8A51-4dcd-8E03-289077D880AA}',tstDir)
#addAllFilesExt('../Mantid/Algorithms/src','alg','cpp',TstTst)
#addAllFilesExt('../Mantid/API/src','alg','cpp',TstTst)
#TstFiles = addFeature('TstFiles1','TstFiles1','TstFiles1','1',Complete)
#addCRef('TSTST',TstFiles)


#addTo(Product,'UIRef',{'Id':'WixUI_Mondo'})
addTo(Product,'UIRef',{'Id':'WixUI_FeatureTree'})
addTo(Product,'UIRef',{'Id':'WixUI_ErrorProgressText'})

exeSec = addTo(Product,'InstallExecuteSequence',{})
AlreadyUpdated = addTo(exeSec,'Custom',{'Action':'AlreadyUpdated','After':'FindRelatedProducts'})
addText('PATCHFOUND',AlreadyUpdated)
NoDowngrade = addTo(exeSec,'Custom',{'Action':'NoDowngrade','After':'FindRelatedProducts'})
addText('NEWERFOUND',NoDowngrade)
addTo(exeSec,'RemoveExistingProducts',{'After':'InstallFinalize'})

f = open('tmp.wxs','w')
doc.writexml(f,newl="\r\n")
f.close()
