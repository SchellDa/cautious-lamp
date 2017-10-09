#include "mapsadatamerger.h"
#include <iostream>
#include <fstream>
#include <memory>

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
#include <EUTelTrackerDataInterfacer.h>
// ----- include for verbosity dependend logging ---------
#include "marlin/VerbosityLevels.h"
#include "marlin/Global.h"

#include <gear/GearMgr.h>

#include <TProfile.h>
#include <TFitResult.h>

#ifdef MARLIN_USE_AIDA
#include <AIDA/ITree.h>
#include <marlin/AIDAProcessor.h>
#include <AIDA/IHistogramFactory.h>
#include <AIDA/ICloud1D.h>
#include <AIDA/IAxis.h>
//#include <AIDA/IHistogram1D.h>
#endif // MARLIN_USE_AIDA

using namespace lcio;
using namespace marlin;
using namespace eutelescope;

MapsaDataMerger aMapsaDataMerger;


MapsaDataMerger::MapsaDataMerger()
 : Processor("MapsaDataMerger"),
 _colHitData(""), _colRefClusterData(""), _colTelClusterData(""),
 _rootOutputFile("merged.root"), _mapsaFile(""),
 _includedSensorIds { 1, 2, 3, 4, 5, 6, 8 },
 _file(nullptr), _currentMpaEventNumber(0)

{
	_description = "Merge telescope data";

	// register steering parameters: name, description, class-variable, default value
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
		"MapsaDataFile",
		"Path of the MaPSA data file",
		_mapsaFile,
		_mapsaFile
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

void MapsaDataMerger::init()
{
	streamlog_out(DEBUG) << "   init called  " << std::endl;

	// usually a good idea to
	printParameters();

	bookHistos();

	streamlog_out(MESSAGE) << "Open MaPSA ROOT file " << _mapsaFile << std::endl;
	_infile = new TFile(_mapsaFile.c_str());
	if(!_infile || _infile->IsZombie()) {
		streamlog_out(ERROR) << "Cannot open MaPSA data file!" << std::endl;
		throw std::runtime_error("Cannot open MaPSA data file");
	}
	_infile->GetObject("Tree", _mpaTreeIn);
	if(!_mpaTreeIn) {
		streamlog_out(ERROR) << "Cannot find MaPSA data tree in input file!" << std::endl;
		throw std::runtime_error("Cannot find input data tree");
	}
	std::vector<int> assembly;
	std::vector<std::string> mpaNames;
	for(size_t mpaIdx = 1; mpaIdx <= 6; ++mpaIdx) {
		std::ostringstream name;
		name << "mpa_" << mpaIdx;
		// try to load data branches. If it fails, subassembly data was not acquired
		if(!_mpaTreeIn->FindBranch((std::string("counter_")+name.str()).c_str())) {
			continue;
		}
		if(!_mpaTreeIn->FindBranch((std::string("noprocessing_")+name.str()).c_str())) {
			continue;
		}
		streamlog_out(MESSAGE) << " * found " << name.str() << " data" << std::endl;
		assembly.push_back(mpaIdx);
		mpaNames.push_back(name.str());
	}
	size_t numMpa = assembly.size();
	_counterIn.resize(numMpa);
	_noProcessingIn.resize(numMpa);
	for(size_t i = 0; i < numMpa; ++i) {
		std::string counterName = std::string("counter_") + mpaNames[i];
		std::string noProcessingName = std::string("noprocessing_") + mpaNames[i];
		_mpaTreeIn->SetBranchAddress(counterName.c_str(), &_counterIn[i]);
	}
	_mpaData.resize(numMpa);

	streamlog_out(MESSAGE) << "Write merged data to file " << _rootOutputFile << std::endl;
	_file = new TFile(_rootOutputFile.c_str(), "recreate");
	_tree = new TTree("data", "");
	_tree->Branch("telescope", &_telData);
	_tree->Branch("telhits", &_telHits);
	_tree->Branch("mpaData", &_mpaData);
	for(size_t i = 0; i < numMpa; ++i) {
		_tree->Branch(mpaNames[i].c_str(), &(_mpaData[i]));
	}
}

void MapsaDataMerger::processRunHeader(LCRunHeader* run)
{
}

void MapsaDataMerger::processEvent(LCEvent* evt)
{
	// one or more input colletions not found in Event! Aborting...
	//if(!colHitData || !colTelClusterData || !colRefClusterData) {
	//	 streamlog_out(ERROR) << "One or more input collections not found!" << std::endl;
	//	return;
	//}
	for(size_t i = 0; i < 7; ++i) {
		(&_telHits.p1 + i)->x.ResizeTo(0);
		(&_telHits.p1 + i)->y.ResizeTo(0);
		(&_telHits.p1 + i)->z.ResizeTo(0);

		(&_telData.p1 + i)->x.ResizeTo(0);
		(&_telData.p1 + i)->y.ResizeTo(0);
	}
	LCCollection* colHitData = nullptr;
	LCCollection* colTelClusterData = nullptr;
	LCCollection* colRefClusterData = nullptr;
	try {
		colHitData = evt->getCollection(_colHitData);
		fillHits(colHitData);
//		streamlog_out(WARNING) << "Hits" << std::endl;
	} catch(lcio::DataNotAvailableException& e) {
//		streamlog_out(WARNING) << e.what() << std::endl;
	}
	try {
		colTelClusterData = evt->getCollection(_colTelClusterData);
		fillClusters(colTelClusterData);
//		streamlog_out(WARNING) << "Tel" << std::endl;
	} catch(lcio::DataNotAvailableException& e) {
//		streamlog_out(WARNING) << e.what() << std::endl;
	}
	try {
		colRefClusterData = evt->getCollection(_colRefClusterData);
		fillClusters(colRefClusterData);
//		streamlog_out(WARNING) << "Ref" << std::endl;
	} catch(lcio::DataNotAvailableException& e) {
//		streamlog_out(WARNING) << e.what() << std::endl;
	}
	_tree->Fill();
	streamlog_out(ERROR) << evt->getEventNumber() << " <-> " << _currentMpaEventNumber << std::endl;
	_mpaTreeIn->GetEvent(_currentMpaEventNumber++);
	for(size_t mpa = 0; mpa < _mpaData.size(); ++mpa) {
		_mpaData[mpa].counter.header = _counterIn[mpa].header;
		for(size_t i = 0; i < 48; ++i) {
			_mpaData[mpa].counter.pixels[i] = _counterIn[mpa].pixels[i];
		}
	}
}

void MapsaDataMerger::check( LCEvent * evt )
{
    // nothing to check here - could be used to fill checkplots in reconstruction processor
}

void MapsaDataMerger::end()
{
	if(_file) {
		_file->Write();
		_file->Close();
	}
}

void MapsaDataMerger::bookHistos()
{
#if defined(USE_AIDA) || defined(MARLIN_USE_AIDA)
#endif
}

void MapsaDataMerger::fillClusters(LCCollection* col)
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

void MapsaDataMerger::fillHits(LCCollection* col)
{
	assert(_includedSensorIds.size() <= 7);
	try {
		CellIDDecoder<TrackerHitImpl> cellDecoder("sensorID:7,properties:7");
		for(int i=0; i < col->getNumberOfElements(); i++) {
			auto hit = dynamic_cast<TrackerHitImpl*>(col->getElementAt(i));
			assert(hit != nullptr);
			auto currentDetectorId = static_cast<int>(cellDecoder(hit)["sensorID"]);
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
			auto td = &_telHits.p1 + sensorIndex;
			td->x.ResizeTo(td->x.GetNoElements() + 1);
			td->y.ResizeTo(td->y.GetNoElements() + 1);
			td->z.ResizeTo(td->z.GetNoElements() + 1);
			auto pos = hit->getPosition();
			td->x[td->x.GetNoElements() - 1] = pos[0];
			td->y[td->y.GetNoElements() - 1] = pos[1];
			td->z[td->z.GetNoElements() - 1] = pos[2];
		}
	} catch(const std::exception& e) {
//		std::cerr << "Warning Telescope evt " << evt->getEventNumber() <<  ": " << e.what() << std::endl;
	}
}

