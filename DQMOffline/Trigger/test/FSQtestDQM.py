import FWCore.ParameterSet.Config as cms

process = cms.Process("DQMPathChecker")

process.load("FWCore.MessageLogger.MessageLogger_cfi")
#process.load("DQMServices.Core.DQM_cfg")

#import DQMServices.Components.DQMEnvironment_cfi
#process.dqmEnvHLT= DQMServices.Components.DQMEnvironment_cfi.dqmEnv.clone()
#process.dqmEnvHLT.subSystemFolder = 'HLT'


process.load("DQMServices.Core.DQM_cfg")
process.load("DQMServices.Components.DQMEnvironment_cfi")
process.dqmEnv.subSystemFolder = 'HLT'



process.load("DQMServices.Components.DQMEnvironment_cfi")

process.load( "Configuration.StandardSequences.FrontierConditions_GlobalTag_cff" )
process.GlobalTag.globaltag = 'MCRUN2_72_V1::All'

#f='/nfs/dust/cms/user/fruboest/2014.11.HLTJec721p1/CMSSW_7_2_1_patch1/src/outputFULL.root'
f='/nfs/dust/cms/user/fruboest/2014.11.HLTJec721p1/CMSSW_7_2_1_patch1/src/outputFULL_big.root'
#f='fromMaxim/events.root'
process.source = cms.Source("PoolSource",
    fileNames = cms.untracked.vstring(
        'file:'+f
    )
)


process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(-200)
)

process.load("DQMOffline.Trigger.FSQHLTOfflineSource_cfi")
process.load("DQMOffline.Trigger.FSQHLTOfflineClient_cfi")

process.load('DQMServices.Components.DQMFileSaver_cfi')
process.dqmSaver.workflow = "/HLT/FSQ/All"


process.p = cms.Path(process.fsqHLTOfflineSource*process.fsqClient *process.dqmEnv*process.dqmSaver)
#process.MessageLogger.threshold = cms.untracked.string( "INFO" )
#process.MessageLogger.categories.append("FSQDiJetAve")

# TODO
# - apply jet callibration to offline jets
# - Fix efficiency histos - add a check, that both reference and tested path simultaneusly
#    went beyond the hlt prescale module

