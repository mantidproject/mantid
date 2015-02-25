#pylint: disable=invalid-name
from mantid.simpleapi import *
from IndirectCommon import *
from IndirectImport import import_mantidplot
mp = import_mantidplot()
from mantid import config, logger
import inelastic_indirect_reducer
import sys, os.path, numpy as np

def loadData(rawfiles, outWS='RawFile', Sum=False, SpecMin=-1, SpecMax=-1,\
        Suffix=''):
    workspaces = []
    for file in rawfiles:
        ( dir, filename ) = os.path.split(file)
        ( name, ext ) = os.path.splitext(filename)
        try:
            if ( SpecMin == -1 ) and ( SpecMax == -1 ):
                Load(Filename=file, OutputWorkspace=name+Suffix, LoadLogFiles=False)
            else:
                Load(Filename=file, OutputWorkspace=name+Suffix, SpectrumMin=SpecMin,\
                    SpectrumMax=SpecMax, LoadLogFiles=False)
            workspaces.append(name+Suffix)
        except ValueError, message:
            logger.notice(message)
            sys.exit(message)
    if Sum and ( len(workspaces) > 1 ):
        MergeRuns(InputWorkspaces=','.join(workspaces), OutputWorkspace=outWS+Suffix)
        factor = 1.0 / len(workspaces)
        Scale(InputWorkspace=outWS+Suffix, OutputWorkspace=outWS+Suffix, Factor=factor)
        for ws in workspaces:
            DeleteWorkspace(ws)
        return [outWS+Suffix]
    else:
        return workspaces

def createMappingFile(groupFile, ngroup, nspec, first):
    if ngroup == 1:
        return 'All'
    if nspec == 1:
        return 'Individual'
    filename = config['defaultsave.directory']
    filename = os.path.join(filename, groupFile)
    handle = open(filename, 'w')
    handle.write(str(ngroup) +  "\n" )
    for n in range(0, ngroup):
        n1 = n * nspec + first
        handle.write(str(n+1) +  '\n' )
        handle.write(str(nspec) +  '\n')
        for i in range(1, nspec+1):
            n3 = n1 + i - 1
            handle.write(str(n3).center(4) + ' ')
        handle.write('\n')
    handle.close()
    return filename


def getInstrumentDetails(instrument):
    instr_name = '__empty_' + instrument
    if mtd.doesExist(instr_name):
        workspace = mtd[instr_name]
    else:
        idf_dir = config['instrumentDefinition.directory']
        idf = idf_dir + instrument + '_Definition.xml'
        LoadEmptyInstrument(Filename=idf, OutputWorkspace=instr_name)
        workspace = mtd[instr_name]
    instrument = workspace.getInstrument()
    ana_list_param = instrument.getStringParameter('analysers')
    if len(ana_list_param) != 1:
        return ""
    ana_list_split = ana_list_param[0].split(',')
    reflections = []
    result = ''
    for analyser in ana_list_split:
        list = []
        name = 'refl-' + analyser
        list.append( analyser )
        try:
            item = instrument.getStringParameter(name)[0]
        except IndexError:
            item = ''
        refl = item.split(',')
        list.append( refl )
        reflections.append(list)
    for i in range(0, len(reflections)):
        message = reflections[i][0] + '-'
        for j in range(0,len(reflections[i][1])):
            message += str(reflections[i][1][j])
            if j < ( len(reflections[i][1]) -1 ):
                message += ','
        result += message
        if  i < ( len(reflections) - 1) :
            result += '\n'
    return result

def getReflectionDetails(inst, analyser, refl):
    idf_dir = config['instrumentDefinition.directory']
    ws = '__empty_' + inst
    if not mtd.doesExist(ws):
        idf_file = inst + '_Definition.xml'
        idf = os.path.join(idf_dir, idf_file)
        LoadEmptyInstrument(Filename=idf, OutputWorkspace=ws)
    ipf_file = inst + '_' + analyser + '_' + refl + '_Parameters.xml'
    ipf = os.path.join(idf_dir, ipf_file)
    LoadParameterFile(Workspace=ws, Filename=ipf)
    inst = mtd[ws].getInstrument()
    result = ''
    try:
        result += str( inst.getStringParameter('analysis-type')[0] ) + '\n'
        result += str( int(inst.getNumberParameter('spectra-min')[0]) ) + '\n'
        result += str( int(inst.getNumberParameter('spectra-max')[0]) ) + '\n'
        result += str( inst.getNumberParameter('efixed-val')[0] ) + '\n'
        result += str( int(inst.getNumberParameter('peak-start')[0]) ) + '\n'
        result += str( int(inst.getNumberParameter('peak-end')[0]) ) + '\n'
        result += str( int(inst.getNumberParameter('back-start')[0]) ) + '\n'
        result += str( int(inst.getNumberParameter('back-end')[0]) ) + '\n'
        result += inst.getStringParameter('rebin-default')[0]
    except IndexError:
        pass
    return result
