#ifndef JOHANNES_EXPORTER_H
#define JOHANNES_EXPORTER_H

#include <lcio.h>
#include <IO/LCEventListener.h>
#include <EVENT/LCEvent.h>
#include <TFile.h>
#include <TTree.h>
#include <vector>
#include <map>
#include "datastructures.h"

class JohannesExporter : public lcio::LCEventListener
{
public:
	JohannesExporter(std::string mpaFile, std::string outputFile, int mpaShift, int numEvents);
	virtual ~JohannesExporter();
	void processEvent(lcio::LCEvent* event);
	void modifyEvent(lcio::LCEvent* event) {}

private:
	void getTelescopeClusters(lcio::LCEvent* evt, std::vector<float>& xcord, std::vector<float>& ycord, int detectorID);
	void getRefHits(lcio::LCEvent* evt, std::vector<float>& xcord, std::vector<float>& ycord, int detectorID);

	TFile* _infile;
	TFile* _outfile;
	int _shift;
	size_t _eventsRead;
	size_t _eventsMax;
	Conditionals* _conditionalData;
	std::vector<MpaData> _mpaData;
	TelescopeData _telescopeData;
	TTree* _conditionalTree;
	TTree* _mpaTreeIn;
	//std::vector<TTree*> _counterTreeIn;
	TTree* _mpaTreeOut;
	std::vector<PlainRippleCounter> _counterIn;
	std::vector<PlainMemoryNoProcessing> _noProcessingIn;
};

#endif//EXPORTER_H
