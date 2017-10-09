#include "alibavadatamerger.h"
#include <iostream>

#include <EVENT/LCCollection.h>
#include <EVENT/MCParticle.h>
#include <EVENT/TrackerPulse.h>
#include <EVENT/TrackerData.h>
#include <EVENT/Track.h>
#include <UTIL/LCTime.h>
#include <UTIL/CellIDDecoder.h>
#include <UTIL/CellIDEncoder.h>
#include <IMPL/LCCollectionVec.h>
#include <IMPL/LCRunHeaderImpl.h>
#include <IMPL/LCEventImpl.h>
#include <IMPL/TrackerDataImpl.h>
#include <IMPL/TrackerHitImpl.h>
#include <IMPL/TrackerPulseImpl.h>
#include <EUTelAlignmentConstant.h>
#include <EUTelSparseClusterImpl.h>

#include "marlin/VerbosityLevels.h"
#include "marlin/Global.h"

#include <gear/GearMgr.h>

#ifdef MARLIN_USE_AIDA
#include <AIDA/ITree.h>
#include <marlin/AIDAProcessor.h>
#include <AIDA/IHistogramFactory.h>
#include <AIDA/ICloud1D.h>
#include <AIDA/IAxis.h>
#endif // MARLIN_USE_AIDA

#include "datastructures.h"


using namespace lcio;
using namespace marlin;
using namespace eutelescope;

AlibavaDataMerger aALibavaDataMerger;

AlibavaDataMerger::AlibavaDataMerger() 
    : Processor("AlibavaDataMerger"), _colHitData(""), _colTelClusterData(""), 
      _colRefClusterData(""), _alibavaFile(""), _rootOutputFile("merged.root"),
      _includedSensorIds{ 1, 2, 3, 4, 5, 6 }, _file(nullptr) 
{

    _description = "Merge telescope and ALiBaVa data";
    
    /* register steering parameters: name, description, class-variable, 
     * default value
     */
    registerInputCollection(
	LCIO::TRACKERHIT,
	"HitDataCollectionName",
	"Name of the collection containing telescope and REF hits",
	_colHitData,
	_colHitData
	);
    registerInputCollection(
	LCIO::TRACKERDATA,
	"TelClusterCollectionName",
	"Name of the collection containing telescope and REF clusters",
	_colTelClusterData,
	_colTelClusterData
	);
    registerInputCollection(
	LCIO::TRACKERDATA,
	"RefClusterCollectionName",
	"Name of the collection containing telescope and REF clusters",
	_colRefClusterData,
	_colRefClusterData
	);
    registerProcessorParameter(
	"AlibavaDataFile",
	"Path of the Alibava data file",
	_alibavaFile,
	_alibavaFile
	);
    registerProcessorParameter(
	"RootOutputFilename",
	"Path of the ROOT output file",
	_rootOutputFile,
	_rootOutputFile
	);
    registerProcessorParameter(
	"includedSensorIds",
	"List of included sensor Ids",
	_includedSensorIds,
	_includedSensorIds
	);
}

void AlibavaDataMerger::init()
{
    streamlog_out(DEBUG) << " init called " << std::endl;

    printParameters();
    //bookHistos();

    /*
     * ALIBAVA STUFF and root output file structure
     */
    
    // Output file

    _file = new TFile(_rootOutputFile.c_str(), "RECREATE");
    _tree = new TTree("data", "");
    _tree->Branch("telescope", &_telData);
    _tree->Branch("telHits", &_telHits);

}

void AlibavaDataMerger::processRunHeader( LCRunHeader* run)
{
}

void AlibavaDataMerger::processEvent(LCEvent* evt)
{
    LCCollection* colHitData = nullptr;
    LCCollection* colTelClusterData = nullptr;
    LCCollection* colRefClusterData = nullptr;


    // clear TVector
    for(size_t i = 0; i < 7; ++i) {
	(&_telHits.p1 + i)->x.ResizeTo(0);
	(&_telHits.p1 + i)->y.ResizeTo(0);
	(&_telHits.p1 + i)->z.ResizeTo(0);
	
	// cluster doesn't contain any z information
	(&_telData.p1 + i)->x.ResizeTo(0);
	(&_telData.p1 + i)->y.ResizeTo(0);
    }


    try {
	colHitData = evt->getCollection(_colHitData);
	fillHits(colHitData);
    } catch(lcio::DataNotAvailableException& e) {
	streamlog_out(DEBUG) << e.what() << std::endl;
    } 
    
/*
    try {
	colTelClusterData = evt->getCollection(_colTelClusterData);
    } catch(lcio::DataNotAvailableException& e) {
	streamlog_out(DEBUG) << e.what() << std::endl;
    } 
*/
    /*
    try {
	colRefClusterData = evt->getCollection(_colRefClusterData);
    } catch(lcio::DataNotAvailableException& e) {
	streamlog_out(DEBUG) << e.what() << std::endl;
    } 
    */

    _tree->Fill();
}

void AlibavaDataMerger::end()
{
    if(_file) {
	_file->Write();
	_file->Close();
    }
}

void AlibavaDataMerger::fillHits(LCCollection* col)
{
    assert(_includedSensorIds.size() <= 7); // 6 telescope + 1 REF
    try {
	// Encoding
	CellIDDecoder<TrackerHitImpl> cellDecoder("sensorID:7,properties:7");
	for(int i=0; i < col->getNumberOfElements(); i++) {
	    auto hit = dynamic_cast<TrackerHitImpl*> (col->getElementAt(i));
	    assert(hit != nullptr);
	    auto currentDetectorId = static_cast<int> (cellDecoder(hit)
						       ["sensorID"]);
	    int sensorIndex = -1; //reset 
	    for(size_t idx=0; idx < _includedSensorIds.size(); ++idx){
		if(currentDetectorId == _includedSensorIds[idx]) {
		    sensorIndex = idx;
		    break;
		}
	    }

	    if(sensorIndex < 0) {
		continue;
	    }
	    
	    // Resize TVectors
	    auto td = &_telHits.p1 + sensorIndex;
	    td->x.ResizeTo(td->x.GetNoElements() + 1);
	    td->y.ResizeTo(td->y.GetNoElements() + 1);
	    td->z.ResizeTo(td->z.GetNoElements() + 1);
	    
	    // Fill TVector
	    auto pos = hit->getPosition();
	    td->x[td->x.GetNoElements() - 1] = pos[0];
	    td->y[td->y.GetNoElements() - 1] = pos[1];
	    td->z[td->z.GetNoElements() - 1] = pos[2];
	    
	}
    } catch (const std::exception& e) {
    }
}					       

