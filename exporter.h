#ifndef EXPORTER_H
#define EXPORTER_H

#include <lcio.h>
#include <IO/LCEventListener.h>
#include <EVENT/LCEvent.h>
#include <TFile.h>
#include <TTree.h>
#include <vector>
#include <map>
#include "datastructures.h"

struct MpaData {
	RippleCounter* counter;
	MemoryNoProcessing* noProcessing;
};

class Exporter : public lcio::LCEventListener
{
public:
	Exporter(std::string mpaFile, std::string outputFile, int mpaShift, int numEvents, bool telClusters);
	virtual ~Exporter();
	void processEvent(lcio::LCEvent* event);
	void modifyEvent(lcio::LCEvent* event) {}

private:
	void getTelescopeClusters(lcio::LCEvent* evt, std::vector<float>& xcord, std::vector<float>& ycord, int detectorID);
	void getTelescopeHits(lcio::LCEvent* evt, std::vector<float>& xcord, std::vector<float>& ycord, int detectorID);
	void getRefHits(lcio::LCEvent* evt, std::vector<float>& xcord, std::vector<float>& ycord, int detectorID);

	TFile* _infile;
	TFile* _outfile;
	int _shift;
	size_t _eventsRead;
	size_t _eventsMax;
	bool _exportClusters;
	Conditionals* _conditionalData;
	std::vector<MpaData> _mpaData;
	TelescopeData* _telescopeData;
	TTree* _conditionalTreeIn;
	TTree* _conditionalTreeOut;
	TTree* _telescopeTreeOut;
	std::vector<TTree*> _mpaTreeIn;
	std::vector<TTree*> _mpaTreeOut;
};

#endif//EXPORTER_H
