#!/usr/bin/env python
from CRABClient.UserUtilities import config, getUsernameFromSiteDB
import sys
import os,time

config = config()

submitVersion = "94X_v6"

if os.environ["USER"] in ['tomc']:
  config.Data.outLFNDirBase = os.path.join('/store/user/tomc/eleId', submitVersion)
  config.Site.storageSite   = 'T2_BE_IIHE'
else:
  raise Exception('User settings not known')

config.JobType.pluginName              = 'Analysis'
config.JobType.psetName                = 'runElectrons.py'
config.JobType.sendExternalFolder      = True
config.Data.splitting                  = 'FileBased'
config.Data.unitsPerJob                = 10
config.Data.inputDBS                   = 'global'
config.Data.publication                = False
config.Data.ignoreLocality             = False


if __name__ == '__main__':

    from CRABAPI.RawCommand import crabCommand
    from CRABClient.ClientExceptions import ClientException
    from httplib import HTTPException

    # We want to put all the CRAB project directories from the tasks we submit here into one common directory.
    # That's why we need to set this parameter (here or above in the configuration file, it does not matter, we will not overwrite it).
    config.General.workArea = 'crab_projects_%s' % submitVersion

    def submit(config, requestName, inputDataset):
        config.General.requestName = requestName
        config.Data.inputDataset   = inputDataset
        try:
            crabCommand('submit', config = config)
        except HTTPException as hte:
            print "Failed submitting task: %s" % (hte.headers)
        except ClientException as cle:
            print "Failed submitting task: %s" % (cle)
        time.sleep(30)

#    submit(config, 'DY',     '/DYJetsToLL_M-50_TuneCP5_13TeV-amcatnloFXFX-pythia8/RunIIFall17MiniAOD-94X_mc2017_realistic_v10-v1/MINIAODSIM')
#    submit(config, 'DY_ext', '/DYJetsToLL_M-50_TuneCP5_13TeV-amcatnloFXFX-pythia8/RunIIFall17MiniAOD-94X_mc2017_realistic_v10_ext1-v1/MINIAODSIM')
#    submit(config, 'TT',     '/TTJets_TuneCP5_13TeV-amcatnloFXFX-pythia8/RunIIFall17MiniAOD-94X_mc2017_realistic_v10-v1/MINIAODSIM')
#    submit(config, 'GJ',     '/GJet_Pt-20toInf_DoubleEMEnriched_MGG-40to80_TuneCP5_13TeV_Pythia8/RunIIFall17MiniAOD-94X_mc2017_realistic_v10-v1/MINIAODSIM')
    config.JobType.psetName = 'runElectrons_AOD.py'
    submit(config, 'DoubleEle1to300', '/DoubleElectron_FlatPt-1To300/RunIIFall17DRStdmix-FlatPU0to60_94X_mc2017_realistic_v10-v1/AODSIM')
    submit(config, 'DoubleEle300to6500', '/DoubleElectron_FlatPt-300To6500/RunIIFall17DRStdmix-FlatPU0to60_94X_mc2017_realistic_v10-v1/AODSIM')
