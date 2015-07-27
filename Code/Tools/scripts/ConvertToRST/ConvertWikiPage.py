###########################################################################################
# Converts mediawiki pages to rst pages for sphinx.
# run with -h for command line arguments
#
# sample command C:\MantidInstall\bin\python.exe C:/Mantid/Code/Tools/scripts/ConvertToRST/ConvertWikiPage.py
#                       -o C:/Mantid/Code/Mantid/docs/source/training/MBC_Displaying_data_Formatting.rst
#
# pandoc must be installed an in the path
#
# extends pandoc by downloading and correcting image links, and adding links for algorithms,
# fit functions and common concepts (workspace types)
#
# Limitations:
# 1. in adding text to links or images in tables the table formatting will be disrupted
# 2. pandoc creates some images in an inline format, these cannot have the align tags added
#    back on, this is marked with a comment, the solution is probably to move the image from
#    the inline to normal format.
# 3. Image links cannot have spaces in the filename in rst.
#    The spaces are removed in the downloaded file names, but not the links in the rst files.
#
##########################################################################################
import os
import re
import sys
import urllib2
import urlparse
import argparse
import subprocess
import mantid

def readWebPage(url):
    # set your environment HTTP_PROXY to be your proxy
    # for ral HTTP_PROXY=http://wwwcache.rl.ac.uk:8080
    aResp = urllib2.urlopen(url)
    web_pg = aResp.read()
    return web_pg

def downloadImage(imgUrl, filePath):
    downloadedImage = file(filePath, "wb")

    imageOnWeb = urllib2.urlopen(imgUrl)
    while True:
        buf = imageOnWeb.read(65536)
        if len(buf) == 0:
            break
        downloadedImage.write(buf)
    downloadedImage.close()
    imageOnWeb.close()

    return filePath

def convertURLToRaw(url):
    return url + "?action=raw"

def covertMediaWikiToRST(url):
    cmd = 'pandoc -f mediawiki -t rst "' + url + '"'
    print cmd

    return runProcess(cmd)

def runProcess(cmd):
    # instantiate a startupinfo obj:
    startupinfo = subprocess.STARTUPINFO()
    # set the use show window flag, might make conditional on being in Windows:
    startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW

    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                     startupinfo=startupinfo)
    output, error = p.communicate()
    output = output.replace("\r\n","\n")
    print error
    return output

def processImageLinks(mediawikiText,rstText, imagePath, relImagePath):
    retRstText = rstText
    mwImagePattern = re.compile(r'\[\[(Image|File):(.+?)(\|.*?)?(\|.*?)?(\|.*?)?(\|.*?)?(\|.*?)?(\|.*?)?\]\]',
                                re.IGNORECASE)
    rstImagePattern = re.compile(r'figure:: (([\w\-\_\\\/]+)\.(\w{2,5}))(.{0,50}\3){2}',
                            re.DOTALL)
    rstSubstitutionImagePattern = re.compile(r'(figure|image):: (([\w\-\_\\\/]+)\.(\w{2,5}))')


    imgLinkDict ={}
    #for all of the mediawiki links
    for m in re.finditer(mwImagePattern, mediawikiText):
        print ("processing image link",m.group(0))
        rstLink = generateRstImageLink(m,relImagePath)
        imgLinkDict[m.group(2)] = rstLink
        print (rstLink)

    #for all of the rst figure links
    replacements = []
    for m in re.finditer(rstImagePattern, retRstText):
        rstLink = imgLinkDict[m.group(1)]
        replacements.append((m.start(), m.end(), m.group(1), rstLink))

    #perform replacements in reverse order
    for (start,end,imagefile,rstLink) in reversed(replacements):
        retRstText = retRstText[0:start] + rstLink + retRstText[end:]

    replacements = []
    for m in re.finditer(rstSubstitutionImagePattern, retRstText):
        rstLink = imgLinkDict[m.group(2)]
        rstLink = cleanInlineImagesDefinition(rstLink)
        replacements.append((m.start(), m.end(), m.group(2), rstLink))

    #perform replacements in reverse order
    for (start,end,imagefile,rstLink) in reversed(replacements):
        retRstText = retRstText[0:start] + rstLink + retRstText[end:]

    #get all of the image files
    for imageName in imgLinkDict.keys():
        saveImageFromMW(imageName,imagePath)


    return retRstText

def saveImageFromMW(imageName,path):
    url =  "http://www.mantidproject.org/File:"+imageName.replace(" ","_")
    print "Downloading image: ", url
    imagePage = readWebPage(url)
    mwImagePattern = re.compile(r'<div class="fullImageLink" id="file"><a href="([\/\w\.\-]+)">')

    imagePath = path + "/" + imageName.replace(" ","")

    match = re.search(mwImagePattern,imagePage)
    if match is not None:
        imageURL = match.group(1)
        imageURL = "http://www.mantidproject.org" + imageURL
        if not os.path.exists(imagePath):
            print "saving ", imageName, "to", imagePath
            downloadImage(imageURL,imagePath)

def generateRstImageLink(match,relImagePath):
    link = "image:: " + relImagePath+ "/" + match.group(2) + "\n"
    for i in range(3,len(match.groups())):
        if match.group(i) is None:
            break
        #strip off the first character as it is the | pipe
        imageOption = addImageOption(match.group(i)[1:])
        if imageOption is not None:
            link += "\t\t\t" + imageOption + "\n"
    return link

def addImageOption(mwImageOption):
    imageOption = mwImageOption.strip()
    if len(imageOption)>0:
        if imageOption.endswith("px"):
            return ":width: " + imageOption
        elif imageOption in ["right","left","middle","centre"]:
            return ":align: " + imageOption
        else:
            return ":alt: " + imageOption

def cleanInlineImagesDefinition(rstLink):
    match = re.search(r'^\s+:align:\s+\w+\s*$',rstLink,re.MULTILINE)
    if match is not None:
        #take the align out
        rstLink = rstLink[0:match.start()] + rstLink[match.end()+1:]
        #then add it at the end as a comment
        rstLink += ".. FIXME (inline definition does not allow align)" + match.group(0)
    return rstLink

def ensureDirectoriesExist(path):
    try:
        os.makedirs(path)
    except OSError:
        pass

def addLocalLinks(rstText,list,prefix):
    retRstText = rstText
    #build regex string for simple words
    regex = r"[^`<]((\*{0,2})(\b" + r"\b|\b".join(list) + r"\b)\2)[^`_]"
    pattern = re.compile(regex)

    replacements = []
    for m in re.finditer(pattern, retRstText):
        rstLink = ":ref:`" + m.group(3) + " <" + prefix + m.group(3) + ">`"
        print ("processing new link",m.group(1), rstLink)
        replacements.append((m.start(1), m.end(1), m.group(1), rstLink))

    #perform replacements in reverse order
    for (start,end,item,rstLink) in reversed(replacements):
        retRstText = retRstText[0:start] + rstLink + retRstText[end:]

    #build regex string for links
    regexLink = r"`(.+?)<(\b" + r"\b|\b".join(list) + r"\b)>`__"
    patternLink = re.compile(regexLink)
    replacements = []
    for m in re.finditer(patternLink, retRstText):
        rstLink = ":ref:`" + m.group(1) + " <" + prefix + m.group(2) + ">`"
        print ("processing existing link",m.group(0), rstLink)
        replacements.append((m.start(), m.end(), m.group(0), rstLink))

    #perform replacements in reverse order
    for (start,end,item,rstLink) in reversed(replacements):
        retRstText = retRstText[0:start] + rstLink + retRstText[end:]

    return retRstText

def fixUnderscoresInRefLinks(rstText):
    retRstText = rstText
    retRstText = re.sub(r"\b\\\_","_",retRstText)

    return retRstText

################################################################################################################

parser = argparse.ArgumentParser(description='Converts mediawiki pages to rst pages for sphinx.')
parser.add_argument('inputURL', help='the url of a mediawiki page')
parser.add_argument('-o', '--o',
                    help='Provide a path to output to a file')
parser.add_argument('-i', '--i',
                    help='Provide a relative path from the file to the images directory')
parser.add_argument('-rp', '--rp',
                    help='Provide a reference link prefix')
parser.add_argument('-r', '--r',
                    help='Provide a reference link')

args = parser.parse_args()
print args.inputURL
print args.o

urlSegments = urlparse.urlparse(args.inputURL)
pageName = urlSegments.path[1:]
print "Parsing ", pageName

#run pandoc and get the output in rst
mediawikiText = readWebPage(convertURLToRaw(args.inputURL))

#run pandoc and get the output in rst
rstText = covertMediaWikiToRST(convertURLToRaw(args.inputURL))

#print "*****************"
#print rstText
#print "*****************"

relPath = "../images"
if args.i is not None:
    relPath = os.path.dirname(args.i)
outDir = os.curdir
if args.o is not None:
    outDir = os.path.dirname(args.o)

imagePath = os.path.join(outDir,relPath)
imagePath = os.path.abspath(imagePath)
print imagePath, outDir

ensureDirectoriesExist(imagePath)
ensureDirectoriesExist(outDir)

#perform any processing beyond pandoc
rstText = fixUnderscoresInRefLinks(rstText)

rstText = processImageLinks(mediawikiText,rstText, imagePath, relPath)

wsList = ['EventWorkspace','MatrixWorkspace','PeaksWorkspace','MDWorkspace','Table Workspaces','WorkspaceGroup',
          'RectangularDetector','RAW File','Facilities File','FitConstraint','Framework Manager',
          'Instrument Data Service','InstrumentDefinitionFile','InstrumentParameterFile','Properties File',
          'MDHistoWorkspace','MDNorm','Nexus file','PeaksWorkspace','Point and space groups','RAW File'
          'Shared Pointer','Symmetry groups','Unit Factory','UserAlgorithms','Workflow Algorithm',
          'Workspace','Workspace2D','WorkspaceGroup']
rstText = addLocalLinks(rstText,wsList,"")

algList = mantid.AlgorithmFactory.getRegisteredAlgorithms(False).keys()
rstText = addLocalLinks(rstText,algList,"algm-")

funcList = mantid.FunctionFactory.getFunctionNames()
rstText = addLocalLinks(rstText,funcList,"func-")

#add reference name
refPrefix = "train-"
if args.rp is not None:
    refPrefix = args.rp
refName = pageName
if args.r is not None:
    refName = args.r
rstText = ".. _" + refPrefix + refName + ":\n\n" + rstText

#save output
if args.o is not None:
    sys.stdout = open(args.o, 'w')
print rstText

