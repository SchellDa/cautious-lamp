#include "alibavadatamerger.h"
#include <iostream>
#include <sstream>

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
//#include "AsciiRoot.h"

using namespace lcio;
using namespace marlin;
using namespace eutelescope;

AlibavaDataMerger aALibavaDataMerger;

AlibavaDataMerger::AlibavaDataMerger() 
    : Processor("AlibavaDataMerger"), _colHitData(""), _colTelClusterData(""), 
      _colRefClusterData(""), _alibavaFile(""), _alibavaPedFile(""), _alibavaCalibFile(""),
	  _rootOutputFile("merged.root"), _includedSensorIds{ 1, 2, 3, 4, 5, 6 }, _seedCut(5)
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
	"AlibavaPedestalFile",
	"Path of the Alibava pedestal file",
	_alibavaPedFile,
	_alibavaPedFile
	);
    registerProcessorParameter(
	"AlibavaCalibFile",
	"Path of the Alibava calibration file",
	_alibavaCalibFile,
	_alibavaCalibFile
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
    registerProcessorParameter(
	"SeedCut",
	"Seed cut for clustering algorithm",
	_seedCut,
	_seedCut
	);
    registerProcessorParameter(
	"NeighborCut",
	"Neighbor cut for clustering algorithm",
	_neighborCut,
	_neighborCut
	);
    registerProcessorParameter(
	"MinNoise",
	"Minimal noise cut",
	_minNoise,
	_minNoise
	);
    registerProcessorParameter(
	"MaxNoise",
	"Maximum noise cut",
	_maxNoise,
	_maxNoise
	);
	registerProcessorParameter(
	"NPedestalEvents",
	"Number of pedestal events",
	_nPedEvents,
	_nPedEvents
	);
    registerProcessorParameter(
	"PedestalStartEvent",
	"First pedestal event",
	_pedStartEvent,
	_pedStartEvent
	);
	registerProcessorParameter(
	"Polarity",
	"Signal polarity",
	_sigPol,
	_sigPol
	);

}

void AlibavaDataMerger::init()
{
    streamlog_out(DEBUG) << " init called " << std::endl;

    printParameters();
    //bookHistos();

	std::string pedFile = "/tmp/alibava.ped";
	streamlog_out(MESSAGE) << "Open ALiBaVa pedestal file " << _alibavaPedFile 
						   << std::endl;

	std::ostringstream pedRootFile;
	pedRootFile << _alibavaFile.substr(0, _alibavaFile.find_last_of(".")) << "_ped.root";
	
	_alibavaPed.open(_alibavaPedFile.c_str());
	_alibavaPed.set_noise_cuts(_maxNoise, _minNoise);
	_alibavaPed.compute_pedestals(_pedStartEvent, _nPedEvents, true, 
								  const_cast<char*>(pedRootFile.str().c_str()), _sigPol);
	_alibavaPed.save_pedestals(pedFile.c_str());
	_noise = _alibavaPed.noisevalue;

	streamlog_out(MESSAGE) << "Noise: " << _noise << std::endl;
	/*
	streamlog_out(MESSAGE) << "Open ALiBaVa calibration file: " << std::endl;	
	
	_alibavaCalib.open(_alibavaCalibFile.c_str());
	_alibavaCalib.set_noise_cuts(_maxNoise, _minNoise);
	_alibavaCalib.load_pedestals(pedFile.c_str());
	*/
    streamlog_out(MESSAGE) << "Open ALiBaVa File " << _alibavaFile 
			<< std::endl;
  
    _alibavaIn.open(_alibavaFile.c_str());
    
    if(!_alibavaIn.valid())
    {
		streamlog_out(ERROR) << "Cannot open ALiBaVa file!" << std::endl;
		throw std::runtime_error("Cannot open ALiBaVa file!");
    }
    
    _alibavaIn.set_noise_cuts(_maxNoise, _minNoise);
	_alibavaIn.load_pedestals(pedFile.c_str());
    _alibavaIn.set_cuts(_seedCut, _neighborCut);
    
    // Output file

    _file = new TFile(_rootOutputFile.c_str(), "RECREATE");
    _tree = new TTree("data", "");
    _tree->Branch("telescope", &_telData);
    _tree->Branch("telHits", &_telHits);
    _tree->Branch("alibava", &_aliData);

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
    
    // Not necessary
    //&_telHits.ref->x.ResizeTo(0);
    //&_telHits.ref->y.ResizeTo(0);

    // Optimization needed here
    _aliData.event.ResizeTo(0);
    _aliData.center.ResizeTo(0);
    _aliData.clock.ResizeTo(0);
    _aliData.time.ResizeTo(0);
    _aliData.temp.ResizeTo(0);
    _aliData.clusterSignal.ResizeTo(0);    

    try {
	colHitData = evt->getCollection(_colHitData);
	fillHits(colHitData);
    } catch(lcio::DataNotAvailableException& e) {
	streamlog_out(DEBUG) << e.what() << std::endl;
    } 
    
    try {
	colTelClusterData = evt->getCollection(_colTelClusterData);
	fillClusters(colTelClusterData);
    } catch(lcio::DataNotAvailableException& e) {
	streamlog_out(DEBUG) << e.what() << std::endl;
    } 

    try {
	colRefClusterData = evt->getCollection(_colRefClusterData);
	fillClusters(colRefClusterData);
    } catch(lcio::DataNotAvailableException& e) {
	streamlog_out(DEBUG) << e.what() << std::endl;
    } 

    _alibavaIn.read_event();

    _alibavaIn.process_event();
    _alibavaIn.find_clusters(-1); // only electron mode

    if( !_alibavaIn.empty() ) {

	    streamlog_out(DEBUG) << "Found ALiBaVa data!" << std::endl;
	    // Optimization needed here
	    _aliData.event.ResizeTo(_alibavaIn.nhits());
	    _aliData.center.ResizeTo(_alibavaIn.nhits());
	    _aliData.clock.ResizeTo(_alibavaIn.nhits());
	    _aliData.time.ResizeTo(_alibavaIn.nhits());
	    _aliData.temp.ResizeTo(_alibavaIn.nhits());
	    _aliData.clusterSignal.ResizeTo(_alibavaIn.nhits());

	    int icluster = 0;
	    AsciiRoot::HitList::iterator ip;
	    for(ip = _alibavaIn.begin(); ip != _alibavaIn.end(); ++ip)
	    {
		    _aliData.event[icluster] = evt->getEventNumber();
		    _aliData.clock[icluster] = _alibavaIn.clock();
		    _aliData.time[icluster] = _alibavaIn.time();
		    _aliData.temp[icluster] = _alibavaIn.temp();

		    Hit &h = *ip;
		    _aliData.clusterSignal[icluster] = h.signal();
		    _aliData.center = h.center();
		    ++icluster;
	    }
    } else {	
	    
	    _aliData.event.ResizeTo(1);	    
	    _aliData.clock.ResizeTo(1);
	    _aliData.time.ResizeTo(1);
	    //_aliData.temp.ResizeTo(1);
	    //_aliData.clusterSignal.ResizeTo(1);
            //_aliData.center.ResizeTo(1);
	    
	    _aliData.event[0] = evt->getEventNumber();
	    _aliData.clock[0] = _alibavaIn.clock();
	    _aliData.time[0] = _alibavaIn.time();
	    //_aliData.temp[0] = _alibavaIn.temp();
	    //_aliData.clusterSignal[0] = 0;
	    //_aliData.center = -1;
	    
    }	    
    _tree->Fill();

}

void AlibavaDataMerger::end()
{
    if(_file) {
	_file->Write();
	_file->Close();
    }
}

void AlibavaDataMerger::fillClusters(LCCollection* col)
{
	assert(_includedSensorIds.size() <= 7);
	try {
		CellIDDecoder<TrackerPulseImpl> cellDecoder(col);
		for(int i=0; i < col->getNumberOfElements(); i++) {
			auto pulse = dynamic_cast<lcio::TrackerPulseImpl*>(col->getElementAt(i));
			assert(pulse != nullptr);
			ClusterType type = static_cast<ClusterType>(static_cast<int>(cellDecoder(pulse)["type"]));
			auto currentDetectorId = static_cast<int>(cellDecoder(pulse)["sensorID"]);
			int sensorIndex = -1;
			for(size_t idx = 0; idx < _includedSensorIds.size(); ++idx) {
				if(currentDetectorId == _includedSensorIds[idx]) {
					sensorIndex = idx;
					break;
				}
			}
			if(sensorIndex < 0) {
				continue;
			}
			assert(type == kEUTelSparseClusterImpl);
			auto cluster = new EUTelSparseClusterImpl<EUTelGenericSparsePixel>(static_cast<TrackerDataImpl*>(pulse->getTrackerData()));
			auto td = &_telData.p1 + sensorIndex;
			td->x.ResizeTo(td->x.GetNoElements() + 1);
			td->y.ResizeTo(td->y.GetNoElements() + 1);
			cluster->getCenterOfGravity(td->x[td->x.GetNoElements() - 1], td->y[td->x.GetNoElements() - 1]);
		}
	} catch(const std::exception& e) {
//		std::cerr << "Warning Telescope evt " << evt->getEventNumber() <<  ": " << e.what() << std::endl;
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

