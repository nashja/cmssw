#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Common/interface/TriggerNames.h"
#include "DataFormats/HLTReco/interface/TriggerObject.h"
#include "HLTriggerOffline/SUSYBSM/interface/SUSY_HLT_Razor.h"


SUSY_HLT_Razor::SUSY_HLT_Razor(const edm::ParameterSet& ps)
{
  edm::LogInfo("SUSY_HLT_Razor") << "Constructor SUSY_HLT_Razor::SUSY_HLT_Razor " << std::endl;
  // Get parameters from configuration file
  theTrigSummary_ = consumes<trigger::TriggerEvent>(ps.getParameter<edm::InputTag>("trigSummary"));
  thePfMETCollection_ = consumes<edm::View<reco::MET> >(ps.getParameter<edm::InputTag>("pfMETCollection"));
  theHemispheres_ = consumes<std::vector<math::XYZTLorentzVector> >(ps.getParameter<edm::InputTag>("hemispheres"));
  triggerResults_ = consumes<edm::TriggerResults>(ps.getParameter<edm::InputTag>("TriggerResults"));
  triggerPath_ = ps.getParameter<std::string>("TriggerPath");
  triggerFilter_ = ps.getParameter<edm::InputTag>("TriggerFilter");
  caloFilter_ = ps.getParameter<edm::InputTag>("CaloFilter");
  thePfJetCollection_ = consumes<reco::PFJetCollection>(ps.getParameter<edm::InputTag>("pfJetCollection"));
}

SUSY_HLT_Razor::~SUSY_HLT_Razor()
{
   edm::LogInfo("SUSY_HLT_Razor") << "Destructor SUSY_HLT_Razor::~SUSY_HLT_Razor " << std::endl;
}

void SUSY_HLT_Razor::dqmBeginRun(edm::Run const &, edm::EventSetup const &)
{
  edm::LogInfo("SUSY_HLT_Razor") << "SUSY_HLT_Razor::beginRun" << std::endl;
}

 void SUSY_HLT_Razor::bookHistograms(DQMStore::IBooker & ibooker_, edm::Run const &, edm::EventSetup const &)
{
  edm::LogInfo("SUSY_HLT_Razor") << "SUSY_HLT_Razor::bookHistograms" << std::endl;
  //book at beginRun
  bookHistos(ibooker_);
}

void SUSY_HLT_Razor::beginLuminosityBlock(edm::LuminosityBlock const& lumiSeg,
  edm::EventSetup const& context)
{
   edm::LogInfo("SUSY_HLT_Razor") << "SUSY_HLT_Razor::beginLuminosityBlock" << std::endl;
}

void SUSY_HLT_Razor::analyze(edm::Event const& e, edm::EventSetup const& eSetup){
    
  edm::LogInfo("SUSY_HLT_Razor") << "SUSY_HLT_Razor::analyze" << std::endl;

  using namespace std;
  using namespace edm;
  using namespace reco;

  // get hold of collection of objects
  Handle< vector<math::XYZTLorentzVector> > hemispheres;
  e.getByToken (theHemispheres_,hemispheres);
  // get hold of the MET Collection
  edm::Handle<edm::View<reco::MET> > inputMet;
  e.getByToken(thePfMETCollection_,inputMet);
  if ( !inputMet.isValid() ){
    edm::LogError ("SUSY_HLT_Razor") << "invalid collection: PFMET" << "\n";
   return;
  }
  edm::Handle<reco::PFJetCollection> pfJetCollection;
  e.getByToken (thePfJetCollection_,pfJetCollection);
  if ( !pfJetCollection.isValid() ){
    edm::LogError ("SUSY_HLT_Razor") << "invalid collection: PFJets" << "\n";
    return;
  }

  //check what is in the menu
  edm::Handle<edm::TriggerResults> hltresults;
  e.getByToken(triggerResults_,hltresults);
  if(!hltresults.isValid()){
    edm::LogError ("SUSY_HLT_Razor") << "invalid collection: TriggerResults" << "\n";
    return;
  }
  
  //-------------------------------
  //--- Trigger
  //-------------------------------
  edm::Handle<trigger::TriggerEvent> triggerSummary;
  e.getByToken(theTrigSummary_, triggerSummary);
  if(!triggerSummary.isValid()) {
    edm::LogError ("SUSY_HLT_Razor") << "invalid collection: TriggerSummary" << "\n";
    return;
  }
  //get online objects
  //HLTriggerOffline/Egamma/python/TriggerTypeDefs.py contains the trigger object IDs
  double onlineMR = 0, onlineRsq = 0;
  double caloMR = 0, caloRsq = 0; //online razor variables computed using calo quantities
  size_t filterIndex = triggerSummary->filterIndex( triggerFilter_ );
  size_t caloFilterIndex = triggerSummary->filterIndex( caloFilter_ );
  //search for online MR and Rsq objects
  trigger::TriggerObjectCollection triggerObjects = triggerSummary->getObjects();
  if( !(filterIndex >= triggerSummary->sizeFilters()) ){
      const trigger::Keys& keys = triggerSummary->filterKeys( filterIndex );
      for( size_t j = 0; j < keys.size(); ++j ){
          trigger::TriggerObject foundObject = triggerObjects[keys[j]];
          if(foundObject.id() == 0){ //the MET object containing MR and Rsq will show up with ID = 0
              onlineMR = foundObject.px(); //razor variables stored in dummy reco::MET objects
              onlineRsq = foundObject.py();
          }
      }
  }

  //search for calo MR and Rsq objects
  if( !(caloFilterIndex >= triggerSummary->sizeFilters()) ){
      const trigger::Keys& keys = triggerSummary->filterKeys( caloFilterIndex );
      for( size_t j = 0; j < keys.size(); ++j ){
          trigger::TriggerObject foundObject = triggerObjects[keys[j]];
          if(foundObject.id() == 0){ 
              caloMR = foundObject.px(); //razor variables stored in dummy reco::MET objects
              caloRsq = foundObject.py();
          }
      }
  }

  bool hasFired = false;
  std::string denomPath = "HLT_Ele27_eta2p1_WP85_Gsf_v1"; //trigger path used as a reference for computing turn-ons and efficiencies
  bool denomFired = false;
  const edm::TriggerNames& trigNames = e.triggerNames(*hltresults);
  unsigned int numTriggers = trigNames.size();
  for( unsigned int hltIndex=0; hltIndex<numTriggers; ++hltIndex ){
      if (trigNames.triggerName(hltIndex).find(triggerPath_) != std::string::npos && hltresults->wasrun(hltIndex) && hltresults->accept(hltIndex)) hasFired = true;
      if (trigNames.triggerName(hltIndex).find(denomPath) != std::string::npos && hltresults->wasrun(hltIndex) && hltresults->accept(hltIndex)) denomFired = true;
  }

  float pfHT = 0.0;
  for (reco::PFJetCollection::const_iterator i_pfjet = pfJetCollection->begin(); i_pfjet != pfJetCollection->end(); ++i_pfjet){
      if (i_pfjet->pt() < 40) continue;
      if (fabs(i_pfjet->eta()) > 3.0) continue;
      pfHT += i_pfjet->pt();
  }

  float pfMET = (inputMet->front()).pt();

  //this part is adapted from HLTRFilter.cc 

  // check that the input collections are available
  if (not hemispheres.isValid()){
      //This is happening many times (which is normal) and it's noisy for the output, we removed the error message
      //edm::LogError("SUSY_HLT_Razor") << "Hemisphere object is invalid!" << "\n";
      return;
  }

  if(hasFired && denomFired){

      if(hemispheres->size() ==0){  // the Hemisphere Maker will produce an empty collection of hemispheres if the number of jets in the
          edm::LogError("SUSY_HLT_Razor") << "Cannot calculate M_R and R^2 because there are too many jets! (trigger passed automatically without forming the hemispheres)" << endl;
          return;
      }

      //***********************************
      //Calculate R 

      if(hemispheres->size() != 0 && hemispheres->size() != 2 && hemispheres->size() != 5 && hemispheres->size() != 10){
          edm::LogError("SUSY_HLT_Razor") << "Invalid hemisphere collection!  hemispheres->size() = " << hemispheres->size() << endl;
          return;
      }

      TLorentzVector ja(hemispheres->at(0).x(),hemispheres->at(0).y(),hemispheres->at(0).z(),hemispheres->at(0).t());
      TLorentzVector jb(hemispheres->at(1).x(),hemispheres->at(1).y(),hemispheres->at(1).z(),hemispheres->at(1).t());

      //dummy vector (this trigger does not care about muons)
      std::vector<math::XYZTLorentzVector> muonVec;

      double MR = CalcMR(ja,jb);
      double R  = CalcR(MR,ja,jb,inputMet,muonVec);
      double Rsq = R*R;    

      if(Rsq > 0.15) h_mr->Fill(MR);
      if(MR > 300) h_rsq->Fill(Rsq);
      h_mrRsq->Fill(MR, Rsq);
      if(Rsq > 0.15) h_mr_denom->Fill(MR);
      if(MR > 300) h_rsq_denom->Fill(Rsq);
      h_mrRsq_denom->Fill(MR, Rsq);

      h_rsq_loose->Fill(Rsq);
      h_rsq_loose_denom->Fill(Rsq);

      if(Rsq > 0.25){
          h_mr_tight->Fill(MR);
          h_mr_tight_denom->Fill(MR);
      }
      if(MR > 400){
          h_rsq_tight->Fill(Rsq);
          h_rsq_tight_denom->Fill(Rsq);
      }

      h_ht->Fill(pfHT);
      h_met->Fill(pfMET);
      h_htMet->Fill(pfHT, pfMET);
      h_ht_denom->Fill(pfHT);
      h_met_denom->Fill(pfMET);
      h_htMet_denom->Fill(pfHT, pfMET);

      h_online_mr_vs_mr->Fill(MR, onlineMR);
      h_online_rsq_vs_rsq->Fill(Rsq, onlineRsq);
      h_online_mr_vs_mr_all->Fill(MR, onlineMR);
      h_online_rsq_vs_rsq_all->Fill(Rsq, onlineRsq);

      h_calo_mr_vs_mr->Fill(MR, caloMR);
      h_calo_rsq_vs_rsq->Fill(Rsq, caloRsq);
      h_calo_mr_vs_mr_all->Fill(MR, caloMR);
      h_calo_rsq_vs_rsq_all->Fill(Rsq, caloRsq);

  } 
  else if(denomFired){ //calculate M_R and R^2 for the denominator histograms

      if(hemispheres->size() ==0){  // the Hemisphere Maker will produce an empty collection of hemispheres if the number of jets in the event is larger than the threshold.  In this case we cannot compute razor variables
          return;
      }


      if(hemispheres->size() != 0 && hemispheres->size() != 2 && hemispheres->size() != 5 && hemispheres->size() != 10){
          return;
      }

      TLorentzVector ja(hemispheres->at(0).x(),hemispheres->at(0).y(),hemispheres->at(0).z(),hemispheres->at(0).t());
      TLorentzVector jb(hemispheres->at(1).x(),hemispheres->at(1).y(),hemispheres->at(1).z(),hemispheres->at(1).t());
      //dummy vector (this trigger does not care about muons)
      std::vector<math::XYZTLorentzVector> muonVec;

      double MR = CalcMR(ja,jb);
      double R  = CalcR(MR,ja,jb,inputMet,muonVec);
      double Rsq = R*R;    

      if(Rsq > 0.15) h_mr_denom->Fill(MR);
      if(MR > 300) h_rsq_denom->Fill(Rsq);
      h_mrRsq_denom->Fill(MR, Rsq);

      h_rsq_loose_denom->Fill(Rsq);

      if(Rsq > 0.25) h_mr_tight_denom->Fill(MR);
      if(MR > 400) h_rsq_tight_denom->Fill(Rsq);

      h_ht_denom->Fill(pfHT);
      h_met_denom->Fill(pfMET);
      h_htMet_denom->Fill(pfHT, pfMET);

      h_online_mr_vs_mr_all->Fill(MR, onlineMR);
      h_online_rsq_vs_rsq_all->Fill(Rsq, onlineRsq);

      h_calo_mr_vs_mr_all->Fill(MR, caloMR);
      h_calo_rsq_vs_rsq_all->Fill(Rsq, caloRsq);
  }
}

void SUSY_HLT_Razor::endLuminosityBlock(edm::LuminosityBlock const& lumiSeg, edm::EventSetup const& eSetup)
{
  edm::LogInfo("SUSY_HLT_Razor") << "SUSY_HLT_Razor::endLuminosityBlock" << std::endl;
}


void SUSY_HLT_Razor::endRun(edm::Run const& run, edm::EventSetup const& eSetup)
{
  edm::LogInfo("SUSY_HLT_Razor") << "SUSY_HLT_Razor::endRun" << std::endl;
}

void SUSY_HLT_Razor::bookHistos(DQMStore::IBooker & ibooker_)
{
  ibooker_.cd();
  ibooker_.setCurrentFolder("HLT/SUSYBSM/" + triggerPath_);

  h_mr = ibooker_.book1D("mr", "M_{R} (R^{2} > 0.15) ; GeV", 100, 0.0, 4000);
  h_rsq = ibooker_.book1D("rsq", "R^{2} (M_{R} > 300)", 100, 0.0, 1.5);
  h_mrRsq = ibooker_.book2D("mrRsq", "R^{2} vs M_{R}; GeV; ", 100, 0.0, 4000.0, 100, 0.0, 1.5);
  h_mr_denom = ibooker_.book1D("mr_denom", "M_{R} (R^{2} > 0.15); GeV", 100, 0.0, 4000);
  h_rsq_denom = ibooker_.book1D("rsq_denom", "R^{2} (MR > 300)", 100, 0.0, 1.5);
  h_mrRsq_denom = ibooker_.book2D("mrRsq_denom", "R^{2} vs M_{R}; GeV; ", 100, 0.0, 4000.0, 100, 0.0, 1.5);

  h_mr_tight = ibooker_.book1D("mr_tight", "M_{R} (R^{2} > 0.25) ; GeV", 100, 0.0, 4000);
  h_rsq_tight = ibooker_.book1D("rsq_tight", "R^{2} (M_{R} > 400) ; ", 100, 0.0, 1.5);
  h_mr_tight_denom = ibooker_.book1D("mr_tight_denom", "M_{R} (R^{2} > 0.25) ; GeV", 100, 0.0, 4000);
  h_rsq_tight_denom = ibooker_.book1D("rsq_tight_denom", "R^{2} (M_{R} > 400) ; ", 100, 0.0, 1.5);

  h_rsq_loose = ibooker_.book1D("rsq_loose", "R^{2} (M_{R} > 0) ; ", 100, 0.0, 1.5);
  h_rsq_loose_denom = ibooker_.book1D("rsq_loose_denom", "R^{2} (M_{R} > 0) ; ", 100, 0.0, 1.5);

  h_ht = ibooker_.book1D("ht", "HT; GeV; ", 100, 0.0, 4000.0);
  h_met = ibooker_.book1D("met", "MET; GeV; ", 100, 0.0, 1000);
  h_htMet = ibooker_.book2D("htMet", "MET vs HT; GeV; ", 100, 0.0, 4000.0, 100, 0.0, 1000);
  h_ht_denom = ibooker_.book1D("ht_denom", "HT; GeV; ", 100, 0.0, 4000.0);
  h_met_denom = ibooker_.book1D("met_denom", "MET; GeV; ", 100, 0.0, 1000);
  h_htMet_denom = ibooker_.book2D("htMet_denom", "MET vs HT; GeV; ", 100, 0.0, 4000.0, 100, 0.0, 1000);

  h_online_mr_vs_mr = ibooker_.book2D("online_mr_vs_mr", "Online M_{R} vs  Offline M_{R} (events passing trigger); Offline M_{R} (GeV); Online M_{R} (GeV); ", 100, 0.0, 4000.0, 100, 0.0, 4000.0);
  h_calo_mr_vs_mr = ibooker_.book2D("calo_mr_vs_mr", "Calo M_{R} vs  Offline M_{R} (events passing trigger); Offline M_{R} (GeV); Calo M_{R} (GeV); ", 100, 0.0, 4000.0, 100, 0.0, 4000.0);
  h_online_rsq_vs_rsq = ibooker_.book2D("online_rsq_vs_rsq", "Online R^{2} vs Offline R^{2} (events passing trigger); Offline R^{2}; Online R^{2}; ", 100, 0.0, 1.5, 100, 0.0, 1.5);
  h_calo_rsq_vs_rsq = ibooker_.book2D("calo_rsq_vs_rsq", "Calo R^{2} vs Offline R^{2} (events passing trigger); Offline R^{2}; Calo R^{2}; ", 100, 0.0, 1.5, 100, 0.0, 1.5);

  h_online_mr_vs_mr_all = ibooker_.book2D("online_mr_vs_mr_all", "Online M_{R} vs  Offline M_{R} (events passing reference trigger); Offline M_{R} (GeV); Online M_{R} (GeV); ", 100, 0.0, 4000.0, 100, 0.0, 4000.0);
  h_calo_mr_vs_mr_all = ibooker_.book2D("calo_mr_vs_mr_all", "Calo M_{R} vs  Offline M_{R} (events passing reference trigger); Offline M_{R} (GeV); Calo M_{R} (GeV); ", 100, 0.0, 4000.0, 100, 0.0, 4000.0);
  h_online_rsq_vs_rsq_all = ibooker_.book2D("online_rsq_vs_rsq_all", "Online R^{2} vs Offline R^{2} (events passing reference trigger); Offline R^{2}; Online R^{2}; ", 100, 0.0, 1.5, 100, 0.0, 1.5);
  h_calo_rsq_vs_rsq_all = ibooker_.book2D("calo_rsq_vs_rsq_all", "Calo R^{2} vs Offline R^{2} (events passing reference trigger); Offline R^{2}; Calo R^{2}; ", 100, 0.0, 1.5, 100, 0.0, 1.5);

  ibooker_.cd();
}

//CalcMR and CalcR borrowed from HLTRFilter.cc
double 
SUSY_HLT_Razor::CalcMR(TLorentzVector ja, TLorentzVector jb){
  if(ja.Pt()<=0.1) return -1;

  ja.SetPtEtaPhiM(ja.Pt(),ja.Eta(),ja.Phi(),0.0);
  jb.SetPtEtaPhiM(jb.Pt(),jb.Eta(),jb.Phi(),0.0);
  
  if(ja.Pt() > jb.Pt()){
    TLorentzVector temp = ja;
    ja = jb;
    jb = temp;
  }
  
  double A = ja.P();
  double B = jb.P();
  double az = ja.Pz();
  double bz = jb.Pz();
  TVector3 jaT, jbT;
  jaT.SetXYZ(ja.Px(),ja.Py(),0.0);
  jbT.SetXYZ(jb.Px(),jb.Py(),0.0);
  double ATBT = (jaT+jbT).Mag2();
  
  double MR = sqrt((A+B)*(A+B)-(az+bz)*(az+bz)-
		   (jbT.Dot(jbT)-jaT.Dot(jaT))*(jbT.Dot(jbT)-jaT.Dot(jaT))/(jaT+jbT).Mag2());
  
  double mybeta = (jbT.Dot(jbT)-jaT.Dot(jaT))/
    sqrt(ATBT*((A+B)*(A+B)-(az+bz)*(az+bz)));
  
  double mygamma = 1./sqrt(1.-mybeta*mybeta);
  
  //use gamma times MRstar
  return MR*mygamma;  
}

double 
  SUSY_HLT_Razor::CalcR(double MR, TLorentzVector ja, TLorentzVector jb, edm::Handle<edm::View<reco::MET> > inputMet, const std::vector<math::XYZTLorentzVector>& muons){
  //now we can calculate MTR
  TVector3 met;
  met.SetPtEtaPhi((inputMet->front()).pt(),0.0,(inputMet->front()).phi());
  
  std::vector<math::XYZTLorentzVector>::const_iterator muonIt;
  for(muonIt = muons.begin(); muonIt!=muons.end(); muonIt++){
    TVector3 tmp;
    tmp.SetPtEtaPhi(muonIt->pt(),0,muonIt->phi());
    met-=tmp;
  }

  double MTR = sqrt(0.5*(met.Mag()*(ja.Pt()+jb.Pt()) - met.Dot(ja.Vect()+jb.Vect())));
  
  //filter events
  return float(MTR)/float(MR); //R
  
}

 //define this as a plug-in
DEFINE_FWK_MODULE(SUSY_HLT_Razor);
