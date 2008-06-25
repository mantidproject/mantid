# Automatic creation of installer source file (.wxs)
import os
import xml
import xml.dom.minidom

QTDIR = 'C:/qt/bin'
#QTDIR = 'C:/Qt/4_4_0'

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
def addDlls(location,name,parent):
    print 'Include dlls from',os.path.abspath(location);
    sdlls = os.listdir(location);
    i = 0
    for fil in sdlls:
        fn = fil.split('.')
        if len(fn) == 2 and fn[1] == 'dll':
            fn0 = fn[0].replace('-','_')
            if not ((fil.find('-gd-') >= 0) or
                    (fil.find('d.dll')>=0 and fil.replace('d.dll','.dll') in sdlls) or
                    (fil.find('d4.dll')>=0 and fil.replace('d4.dll','4.dll') in sdlls)):
                print fn[0]+'.'+fn[1]
                addFileV(fn0+fn[1],name+str(i),fil,location+'/'+fil,parent)
            i += 1

def addAllFiles(location,name,parent):
    print 'Include files from',os.path.abspath(location);
    sfiles = os.listdir(location);
    i = 0
    for fil in sfiles:
        print fil
        fn = fil.replace('-','_')
        if (fil.find('.svn') < 0):
            addFileV(fn+'_file',name+str(i),fil,location+'/'+fil,parent)
            i += 1

def addFeature(Id,title,description,level,parent):
    e = doc.createElement('Feature')
    e.setAttribute('Id',Id)
    e.setAttribute('Title',title)
    e.setAttribute('Description',description)
    e.setAttribute('Level',level)
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

doc = xml.dom.minidom.Document()
#doc.encoding('Windows-1252')
wix = doc.createElement('Wix')
wix.setAttribute('xmlns','http://schemas.microsoft.com/wix/2003/01/wi')
doc.appendChild(wix)

Product = doc.createElement('Product')
Product.setAttribute('Name','Mantid')
Product.setAttribute('Id','{CA88C1C7-AEB8-4bd6-A62C-9C436FA31211}')
Product.setAttribute('Language','1033')
Product.setAttribute('Codepage','1252')
Product.setAttribute('Version','1.0.0')
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

Prop = doc.createElement('Property')
Prop.setAttribute('Id','PYTHON_25_DIR_EXISTS')
Product.appendChild(Prop)
DS = doc.createElement('DirectorySearch')
DS.setAttribute('Id','CheckPyDir')
DS.setAttribute('Path','C:\\Python25')
DS.setAttribute('Depth','0')
Prop.appendChild(DS)

Cond = doc.createElement('Condition')
Cond.setAttribute('Message','Mantid requires Python 2.5 to be installed on your machine. It can be downloaded and installed from http://www.python.org/download/')
Cond.appendChild(doc.createTextNode('PYTHON_25_DIR_EXISTS'))
Product.appendChild(Cond)

TargetDir = addDirectory('TARGETDIR','SourceDir','SourceDir',Product)
InstallDir = addDirectory('INSTALLDIR','MInstall','MantidInstall',TargetDir)
binDir = addDirectory('MantidBin','bin','bin',InstallDir)

MantidDlls = addComponent('MantidDLLs','{FABC0481-C18D-415e-A0B1-CCB76C35FBE8}',binDir)
addFileV('MantidProperties','Mantid.pro','Mantid.properties','../Mantid/release/Mantid.properties',MantidDlls)
addDlls('../Mantid/Bin/Shared','SDll',MantidDlls)
addDlls('../Mantid/Bin/Plugins','PnDll',MantidDlls)
addDlls('../Third_Party/lib/win32','3dDll',MantidDlls)
addAllFiles('toget/MSVCruntime','ms',MantidDlls)

QTIPlot = addComponent('QTIPlot','{03ABDE5C-9084-4ebd-9CF8-31648BEFDEB7}',binDir)
addDlls(QTDIR,'qt',QTIPlot)
QTIPlotEXE = addFileV('QTIPlotEXE','qtiplot.exe','qtiplot.exe','../qtiplot/qtiplot/qtiplot.exe',QTIPlot)
startmenuQTIPlot = addTo(QTIPlotEXE,'Shortcut',{'Id':'startmenuQTIPlot','Directory':'ProgramMenuDir','Name':'QTIPlot','WorkingDirectory':'binDir'})
desktopQTIPlot = addTo(QTIPlotEXE,'Shortcut',{'Id':'desktopQTIPlot','Directory':'DesktopFolder','Name':'QTIPlot','WorkingDirectory':'binir'})
addAllFiles('toget/pyc','pyc',QTIPlot)
manifestFile = addFileV('qtiplot_manifest','qtiexe.man','qtiplot.exe.manifest','../qtiplot/qtiplot/qtiplot.exe.manifest',QTIPlot)

addTo(MantidDlls,'RemoveFile',{'Id':'LogFile','On':'uninstall','Name':'mantid.log'})

pluginsDir = addDirectory('PluginsDir','plugins','plugins',InstallDir)
Plugins = addComponent('Plugins','{EEF0B4C9-DE52-4f99-A8D0-9D3C3941FA73}',pluginsDir)
addTo(Plugins,'CreateFolder',{})

documentsDir = addDirectory('DocumentsDir','docs','docs',InstallDir)
Documents = addComponent('Documents','{C16B2B59-17C8-4cc9-8A7F-16254EB8B2F4}',documentsDir)
addTo(Documents,'CreateFolder',{})

logsDir = addDirectory('LogsDir','logs','logs',InstallDir)
Logs = addComponent('Logs','{0918C9A4-3481-4f21-B941-983BE21F9674}',logsDir)
addTo(Logs,'CreateFolder',{})

includeDir = addDirectory('IncludeDir','include','include',InstallDir)
Include = addComponent('Include','{EDB85D81-1CED-459a-BF87-E148CEE6F9F6}',includeDir)
addTo(Include,'CreateFolder',{})

tempDir = addDirectory('TempDir','temp','temp',InstallDir)
Temp = addComponent('Temp','{02D25B60-A114-4f2a-A211-DE88CF648C61}',tempDir)
addTo(Temp,'CreateFolder',{})

dataDir = addDirectory('DataDir','data','data',InstallDir)
Data = addComponent('Data','{6D9A0A53-42D5-46a5-8E88-6BB4FB7A5FE1}',dataDir)
addTo(Data,'CreateFolder',{})

sourceDir = addDirectory('SourceDir','source','source',InstallDir)
Source = addComponent('Source','{9A6BF2E5-2606-441c-B250-2EAE81C64B01}',sourceDir)
addTo(Source,'CreateFolder',{})

Python25Dir = addDirectory('Python25Dir','Python25','Python25',TargetDir)
LibDir = addDirectory('LibDir','Lib','Lib',Python25Dir)
SitePackagesDir = addDirectory('SitePackagesDir','sitepack','site-packages',LibDir)
PyQtDir = addDirectory('PyQtDir','PyQt4','PyQt4',SitePackagesDir)
PyQt = addComponent('PyQt','{18028C0B-9DF4-48f6-B8FC-DE195FE994A0}',PyQtDir)
addAllFiles('toget/PyQt4','PyQt',PyQt)

ProgramMenuFolder = addDirectory('ProgramMenuFolder','PMenu','Programs',TargetDir)
ProgramMenuDir = addDirectory('ProgramMenuDir','Mantid','Mantid',ProgramMenuFolder)

DesktopFolder = addDirectory('DesktopFolder','Desktop','Desktop',TargetDir)

#-------------------------------------------------------
Py25Exists = addTo(Product,'Property',{'Id':'DIREXISTS'})
addTo(Py25Exists,'DirectorySearch',{'Id':'CheckDir','Path':'C:\Python25\Lib\site-packages\PyQt4','Depth':'0'})

Complete = addRootFeature('Complete','Mantid','The complete package','1',Product)
MantidExec = addFeature('MantidExecAndDlls','Mantid binaries','The main executable.','1',Complete)
addCRef('MantidDLLs',MantidExec)
addCRef('Plugins',MantidExec)
addCRef('Documents',MantidExec)
addCRef('Logs',MantidExec)
addCRef('Include',MantidExec)
addCRef('Temp',MantidExec)
addCRef('Data',MantidExec)

QTIPlotExec = addFeature('QTIPlotExec','QtiPlot','QtiPlot','1',MantidExec)
addCRef('QTIPlot',QTIPlotExec)

PyQtF = addFeature('PyQtF','PyQt4','PyQt4','1',MantidExec)
addCRef('PyQt',PyQtF)

SourceFiles = addFeature('SourceFiles','SourceFiles','SourceFiles','1',Complete)
addCRef('Source',SourceFiles)

addTo(Product,'UIRef',{'Id':'WixUI_Mondo'})
addTo(Product,'UIRef',{'Id':'WixUI_ErrorProgressText'})

f = open('tmp.wxs','w')
doc.writexml(f)
f.close()
