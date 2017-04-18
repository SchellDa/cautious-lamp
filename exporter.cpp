
#include "exporter.h"
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

Exporter::Exporter(std::string mpaFile, std::string outputFile, int mpaShift, int numEvents) :
 _infile(new TFile(mpaFile.c_str(), "readonly")), _outfile(new TFile(outputFile.c_str(), "recreate")), _shift(mpaShift),
 _eventsRead(0), _eventsMax(numEvents), _conditionalData(new Conditionals), _telescopeData(new TelescopeData),
 _conditionalTreeIn(nullptr), _conditionalTreeOut(nullptr)
{
	if(!_infile || _infile->IsZombie()) {
		throw std::runtime_error("Cannot open MaPSA data file");
	}
	if(!_outfile || _outfile->IsZombie()) {
		throw std::runtime_error("Cannot open ROOT output file");
	}
	_outfile->cd();
	_conditionalTreeOut = new TTree("conditionals", "Conditional data when recording");
	_infile->GetObject("conditionals", _conditionalTreeIn); 
	if(!_conditionalTreeIn) {
		throw std::runtime_error("MaPSA file does not contain conditionals tree");
	}
	//_conditionalTreeIn->SetBranchAddress("conditionals", &_conditionalData);
	//_conditionalTreeOut->Branch("conditionals", &_conditionalData, "threshold/F:voltage/F:current/F:temperature/F:z_pos/F:angle/F");
	//_conditionalTreeIn->GetEntry(0);
	//_conditionalTreeOut->Fill();
	for(size_t i = 1; i <= 6; ++i) {
		std::string name = "mpa_";
		name += std::to_string(i);
		TTree* mpaTree = nullptr;
		_infile->GetObject(name.c_str(), mpaTree);
		if(mpaTree) {
			std::cout << " * found tree " << name << std::endl;
			_mpaTreeIn.push_back(mpaTree);
			_mpaTreeOut.push_back(new TTree(name.c_str(), name.c_str()));
		}
	}
	size_t numMPA = _mpaTreeIn.size();
	std::cout << "Found " << numMPA << " MPA data streams" << std::endl;
	if(numMPA == 0) {
		throw std::runtime_error("No input data tree found");
	}
	_mpaData.resize(numMPA);
	for(size_t i = 0; i < numMPA; ++i) {
		_mpaData[i].counter = new RippleCounter();
		_mpaData[i].noProcessing = new MemoryNoProcessing();
		_mpaTreeIn[i]->SetBranchAddress("counter", &_mpaData[i].counter);
//		_mpaTreeIn[i]->SetBranchAddress("noProcessing", &_mpaData[i].noProcessing);
		_mpaTreeOut[i]->Branch("counter", &_mpaData[i].counter);
		_mpaTreeOut[i]->Branch("noProcessing", &_mpaData[i].noProcessing, "pixels[96]/l:bunchCrossingId[96]/s:header[96]/b:numEvents/b:corrupt/b");
	}
	_telescopeTreeOut = new TTree("telescope", "telescope data");
	_telescopeData = new TelescopeData;
	_telescopeTreeOut->Branch("telescope", _telescopeData);
}

Exporter::~Exporter()
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
	delete _telescopeData;
}

void Exporter::processEvent(lcio::LCEvent* event)
{
	if(event->getEventNumber() % 10 == 0) {
		std::cout << "\r\x1b[K Processing event "
		          << event->getEventNumber()
			  << std::flush;
	}
	for(auto& tree: _mpaTreeIn) {
		tree->GetEntry(event->getEventNumber());
		for(size_t i = 0; i < _mpaData.size(); ++i) {
			_mpaTreeOut[i]->SetBranchAddress("counter", &(_mpaData[i].counter));
			//_mpaTreeOut[i]->SetBranchAddress("noProcessing", &(_mpaData[i].noProcessing));
		}
	}
	static const std::vector<int> planeIds = {1, 2, 3, 4, 5, 6};
	assert(planeIds.size() <= 6);
	for(size_t i = 0; i < planeIds.size(); ++i) {
		std::vector<float> x;
		std::vector<float> y;
		getTelescopeClusters(event, x, y, planeIds[i]);
		assert(x.size() == y.size());
		TelescopePlaneClusters* plane = (&_telescopeData->p1) + i;
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
	_telescopeData->ref.x.Clear();
	_telescopeData->ref.y.Clear();
	_telescopeData->ref.x.ResizeTo(x.size());
	_telescopeData->ref.y.ResizeTo(y.size());
	for(size_t j = 0; j < x.size(); ++j) {
		_telescopeData->ref.x[j] = x[j];
		_telescopeData->ref.y[j] = y[j];
	}
	for(auto& tree: _mpaTreeOut) {
		tree->Fill();
	}
	_telescopeTreeOut->Fill();
}

void Exporter::getTelescopeClusters(lcio::LCEvent* evt, std::vector<float>& xcord, std::vector<float>& ycord, int detectorID)
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

void Exporter::getRefHits(lcio::LCEvent* evt, std::vector<float>& xcord, std::vector<float>& ycord, int detectorID)
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

