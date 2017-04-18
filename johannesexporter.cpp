
#include "johannesexporter.h"
#include <stdexcept>
#include <iostream>
#include <EVENT/LCCollection.h>
#include <EVENT/TrackerPulse.h>
#include <EVENT/TrackerData.h>
#include <UTIL/CellIDDecoder.h>
#include <IMPL/TrackerPulseImpl.h>
#include <EUTelSparseClusterImpl.h>
#include <EUTelGenericSparsePixel.h>
#include <EUTelSparseClusterImpl.h>
#include <cassert>
#include <memory>

using namespace eutelescope;
using namespace lcio;

JohannesExporter::JohannesExporter(std::string mpaFile, std::string outputFile, int mpaShift, int numEvents, bool exportHits) :
 _infile(new TFile(mpaFile.c_str())), _outfile(new TFile(outputFile.c_str(), "recreate")), _shift(mpaShift),
 _eventsRead(0), _eventsMax(numEvents), _exportHits(exportHits), _conditionalData(new Conditionals),
 _conditionalTree(nullptr)
{
	if(!_infile || _infile->IsZombie()) {
		throw std::runtime_error("Cannot open MaPSA data file");
	}
	if(!_outfile || _outfile->IsZombie()) {
		throw std::runtime_error("Cannot open ROOT output file");
	}
	_outfile->cd();
	_conditionalTree = new TTree("conditionals", "Conditional data when recording");
	//_conditionalTreeIn->SetBranchAddress("conditionals", &_conditionalData);
	//_conditionalTreeOut->Branch("conditionals", &_conditionalData, "threshold/F:voltage/F:current/F:temperature/F:z_pos/F:angle/F");
	//_conditionalTreeIn->GetEntry(0);
	//_conditionalTreeOut->Fill();
	_infile->GetObject("Tree", _mpaTreeIn);
	if(!_mpaTreeIn) {
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
		std::cout << " * found " << name.str() << " data" << std::endl;
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
	_mpaTreeOut = new TTree("data", "");
	_mpaData.resize(numMpa);
	for(size_t i = 0; i < numMpa; ++i) {
		_mpaTreeOut->Branch(mpaNames[i].c_str(), &(_mpaData[i]));
	}
	_mpaTreeOut->Branch("telescope", &_telescopeData);
}

JohannesExporter::~JohannesExporter()
{
	if(_infile) {
		if(!_infile->IsZombie()) {
			_infile->Close();
		}
		delete _infile;
	}
	if(_outfile) {
		if(!_outfile->IsZombie()) {
			_outfile->Write();
			_outfile->Close();
		}
		delete _outfile;
	}
	delete _conditionalData;
}

void JohannesExporter::processEvent(lcio::LCEvent* event)
{
	if(event->getEventNumber() % 10 == 0) {
		std::cout << "\r\x1b[K Processing event "
		          << event->getEventNumber()
			  << std::flush;
	}
	auto bytesRead = _mpaTreeIn->GetEntry(_eventsRead);
	for(size_t mpa = 0; mpa < _mpaData.size(); ++mpa) {
		_mpaData[mpa].counter.header = _counterIn[mpa].header;
		for(size_t i = 0; i < 48; ++i) {
			_mpaData[mpa].counter.pixels[i] = _counterIn[mpa].pixels[i];
		}
	}
	static const std::vector<int> planeIds = {1, 2, 3, 4, 5, 6};
	assert(planeIds.size() <= 6);
	for(size_t i = 0; i < planeIds.size(); ++i) {
		std::vector<float> x;
		std::vector<float> y;
		if(_exportHits) {
			getTelescopeHits(event, x, y, planeIds[i]);
		} else {
			getTelescopeClusters(event, x, y, planeIds[i]);
		}
		assert(x.size() == y.size());
		TelescopePlaneClusters* plane = (&_telescopeData.p1) + i;
		plane->x.Clear();
		plane->y.Clear();
		plane->x.ResizeTo(x.size());
		plane->y.ResizeTo(y.size());
		for(size_t j = 0; j < x.size(); ++j) {
			plane->x[j] = x[j];
			plane->y[j] = y[j];
		}
	}
	std::vector<float> x;
	std::vector<float> y;
	getRefHits(event, x, y, 8);
	_telescopeData.ref.x.Clear();
	_telescopeData.ref.y.Clear();
	_telescopeData.ref.x.ResizeTo(x.size());
	_telescopeData.ref.y.ResizeTo(y.size());
	for(size_t j = 0; j < x.size(); ++j) {
		_telescopeData.ref.x[j] = x[j];
		_telescopeData.ref.y[j] = y[j];
	}
	_mpaTreeOut->Fill();
	++_eventsRead;
}

void JohannesExporter::getTelescopeClusters(lcio::LCEvent* evt, std::vector<float>& xcord, std::vector<float>& ycord, int detectorID)
{
	try {
		auto collection = evt->getCollection("cluster_m26_free");
		CellIDDecoder<TrackerPulseImpl> cellDecoder(collection);
		xcord.clear();
		ycord.clear();
		for(int i=0; i < collection->getNumberOfElements(); i++) {
			auto pulse = dynamic_cast<lcio::TrackerPulseImpl*>(collection->getElementAt(i));
			assert(pulse != nullptr);
			ClusterType type = static_cast<ClusterType>(static_cast<int>(cellDecoder(pulse)["type"]));
			auto currentDetectorID = static_cast<int>(cellDecoder(pulse)["sensorID"]);
			if(currentDetectorID != detectorID) {
				continue;
			}
			assert(type == kEUTelSparseClusterImpl);
			auto cluster = new EUTelSparseClusterImpl<EUTelGenericSparsePixel>(static_cast<TrackerDataImpl*>(pulse->getTrackerData()));
			float xPos, yPos;
			cluster->getCenterOfGravity(xPos, yPos);
			xcord.push_back(xPos);
			ycord.push_back(yPos);
		}
	} catch(const std::exception& e) {
		std::cerr << "Warning Telescope evt " << evt->getEventNumber() <<  ": " << e.what() << std::endl;
	}
}

void JohannesExporter::getTelescopeHits(lcio::LCEvent* evt, std::vector<float>& xcord, std::vector<float>& ycord, int detectorID)
{
	try {
		auto collection = evt->getCollection("zsdata_m26");
		CellIDDecoder<TrackerDataImpl> cellDecoder(collection);
		xcord.clear();
		ycord.clear();
		for(int i=0; i < collection->getNumberOfElements(); i++) {
			auto data = dynamic_cast<TrackerDataImpl*>(collection->getElementAt(i));
			assert(data != nullptr);
			auto type = static_cast<SparsePixelType>(static_cast<int>(cellDecoder(data)["sparsePixelType"]));
			auto currentDetectorID = static_cast<int>(cellDecoder(data)["sensorID"]);
			if(currentDetectorID != detectorID) {
				continue;
			}
			assert(type == kEUTelGenericSparsePixel);
			auto sparseData(new EUTelTrackerDataInterfacerImpl<EUTelGenericSparsePixel> (data));
			auto sparsePixel(new EUTelGenericSparsePixel);
			for(size_t i=0; i < sparseData->size(); i++) {
				sparseData->getSparsePixelAt(i, sparsePixel);
				xcord.push_back(sparsePixel->getXCoord());
				ycord.push_back(sparsePixel->getYCoord());
			}
		}
	} catch(const std::exception& e) {
		std::cerr << "Warning Telescope evt " << evt->getEventNumber() <<  ": " << e.what() << std::endl;
	}
}

void JohannesExporter::getRefHits(lcio::LCEvent* evt, std::vector<float>& xcord, std::vector<float>& ycord, int detectorID)
{
	try {
		xcord.clear();
		ycord.clear();
		// auto collection = evt->getCollection("CMSPixelDUT");
		LCCollection* collection = nullptr;
		try {
			collection = evt->getCollection("zsdata_apix");
		} catch(const lcio::DataNotAvailableException& e) {
		}
		try {
			if(!collection)
				collection = evt->getCollection("CMSPixelREF");
		} catch(const lcio::DataNotAvailableException& e) {
			throw;
		}
		CellIDDecoder<TrackerDataImpl> cellDecoder(collection);
		assert(collection->getNumberOfElements() == 1);
		auto data = dynamic_cast<TrackerDataImpl*>(collection->getElementAt(000));
		auto type = static_cast<SparsePixelType>(static_cast<int>(cellDecoder(data)["sparsePixelType"]));
		assert(type == kEUTelGenericSparsePixel);
		auto sparseData(new EUTelTrackerDataInterfacerImpl<EUTelGenericSparsePixel> (data));
		auto sparsePixel(new EUTelGenericSparsePixel);
		for(size_t i=0; i < sparseData->size(); i++) {
			sparseData->getSparsePixelAt(i, sparsePixel);
			xcord.push_back(sparsePixel->getXCoord());
			ycord.push_back(sparsePixel->getYCoord());
		}
	} catch(const std::exception& e) {
		std::cerr << "Warning DUT evt " << evt->getEventNumber() <<  ": " << e.what() << std::endl;
	}
}

