#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>

#include <TH1F.h>
#include <TROOT.h>
#include <TFile.h>
#include <TSystem.h>

#include "DataFormats/FWLite/interface/Event.h"
#include "DataFormats/Common/interface/Handle.h"

#include "FWCore/FWLite/interface/AutoLibraryLoader.h"
// In the near future the above should be replaced with:
// #include "FWCore/FWLite/interface/FWLiteEnabler.h"

#include "DataFormats/FWLite/interface/InputSource.h"
#include "DataFormats/FWLite/interface/OutputFiles.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSetReader/interface/ParameterSetReader.h"

#include "PhysicsTools/FWLite/interface/TFileService.h"

#include "DataFormats/EgammaCandidates/interface/GsfElectron.h"
#include "DataFormats/EgammaCandidates/interface/GsfElectronFwd.h"
#include "DataFormats/PatCandidates/interface/Electron.h"

#include "RecoEgamma/ElectronIdentification/interface/VersionedGsfElectronSelector.h"

int main(int argc, char* argv[]) 
{
  // ----------------------------------------------------------------------
  // First Part: 
  //
  //  * enable FWLite 
  //  * enable the IDs you want to use
  //  * open the input file
  // ----------------------------------------------------------------------

  // load framework libraries
  gSystem->Load( "libFWCoreFWLite" );
  AutoLibraryLoader::enable();
// In the near future the above should be replaced with:
  //  FWLiteEnabler::enable();

  // only allow one argument for this simple example which should be the
  // the python cfg file
  if ( argc < 2 ) {
    std::cout << "Usage : " << argv[0] << " [parameters.py]" << std::endl;
    return 0;
  }
  if( !edm::readPSetsFrom(argv[1])->existsAs<edm::ParameterSet>("process") ){
    std::cout << " ERROR: ParametersSet 'process' is missing in your configuration file" << std::endl; exit(0);
  }
  // get the python configuration
  const edm::ParameterSet& process = edm::readPSetsFrom(argv[1])->getParameter<edm::ParameterSet>("process");
  fwlite::InputSource inputHandler_(process); fwlite::OutputFiles outputHandler_(process);
  
  // now get each parameter
  const edm::ParameterSet& ana = process.getParameter<edm::ParameterSet>("electronAnalyzer");
  edm::InputTag electronsMiniAOD_( ana.getParameter<edm::InputTag>("electronsMiniAOD") );
  edm::InputTag electronsAOD_    ( ana.getParameter<edm::InputTag>("electronsAOD") );
  
  // setup an ID for use
  // the parameter values are set in the python configuration script
  const edm::ParameterSet& my_ids = process.getParameterSet("my_vid_configuration");
  const edm::ParameterSet& loose_id_conf = my_ids.getParameterSet("loose");
  const edm::ParameterSet& medium_id_conf = my_ids.getParameterSet("medium");
  const edm::ParameterSet& tight_id_conf = my_ids.getParameterSet("tight");
    
  VersionedGsfElectronSelector loose_id(loose_id_conf);
  VersionedGsfElectronSelector medium_id(medium_id_conf);
  VersionedGsfElectronSelector tight_id(tight_id_conf);  

  // loop the events
  int ievt=0;  
  int maxEvents_( inputHandler_.maxEvents() );
  for(unsigned int iFile=0; iFile<inputHandler_.files().size(); ++iFile){
    // open input file (can be located on castor)
    TFile* inFile = TFile::Open(inputHandler_.files()[iFile].c_str());
    if( inFile ){
      // ----------------------------------------------------------------------
      // Second Part: 
      //
      //  * loop the events in the input file 
      //  * receive the collections of interest via fwlite::Handle
      //  * fill the histograms
      //  * after the loop close the input file
      // ----------------------------------------------------------------------      
      fwlite::Event ev(inFile);
      for(ev.toBegin(); !ev.atEnd(); ++ev, ++ievt){
        edm::EventBase const & event = ev;
        // break loop if maximal number of events is reached 
        if(maxEvents_>0 ? ievt+1>maxEvents_ : false) break;
        // simple event counter
        if(inputHandler_.reportAfter()!=0 ? (ievt>0 && ievt%inputHandler_.reportAfter()==0) : false) 
          std::cout << "  processing event: " << ievt << std::endl;

        // Handle to the electron collection
	// 
	// There does not appear to be a way to define a single
	// handle variable that would be filled with either the 
	// AOD or miniAOD (reco or pat) collection, so we try one
	// first, and if that fails, we try the other.
	//   With the full CMSSW this can be done more gracefully,
	// but FWLite has some limitations.
        edm::Handle<std::vector<pat::Electron> > electronsPat;
        edm::Handle<std::vector<reco::GsfElectron> > electronsGsf;
	unsigned int nElectrons = 0;
	try{
	  event.getByLabel(electronsMiniAOD_, electronsPat);
	  nElectrons = electronsPat->size();
	}catch(cms::Exception const& e){
	  event.getByLabel(electronsAOD_, electronsGsf);
	  nElectrons = electronsGsf->size();
	}
        
        // loop over the physics object collection and do what needs to be done
        for(unsigned i=0; i<nElectrons; ++i){
	  
	  bool passedLoose  = false;
	  bool passedMedium = false;
	  bool passedTight  = false;
	  // Now we look up the ID decision from the valid collection (AOD or miniAOD)
	  if( electronsPat.isValid() ){
	    passedLoose  = loose_id (electronsPat->at(i),event);
	    passedMedium = medium_id(electronsPat->at(i),event);
	    passedTight  = tight_id (electronsPat->at(i),event);
	  }else{
	    passedLoose  = loose_id (electronsGsf->at(i),event);
	    passedMedium = medium_id(electronsGsf->at(i),event);
	    passedTight  = tight_id (electronsGsf->at(i),event);
	  }
	  
          if( passedLoose ) {
            std::cout << "Electron at " << i << " passed loose ID!" << std::endl;
          }
          if( passedMedium ) {
            std::cout << "Electron at " << i << " passed medium ID!" << std::endl;
          }
          if( passedTight ) {
            std::cout << "Electron at " << i << " passed tight ID!" << std::endl;
          }
        }
      }  
      // close input file
      inFile->Close();
    }
    // break loop if maximal number of events is reached:
    // this has to be done twice to stop the file loop as well
    if(maxEvents_>0 ? ievt+1>maxEvents_ : false) break;
  }
  return 0;
}
