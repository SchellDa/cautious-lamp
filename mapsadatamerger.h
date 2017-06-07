#ifndef DATA_MERGER_H
#define DATA_MERGER_H

#include "marlin/Processor.h"
#include "lcio.h"
#include <string>
#include <Eigen/Dense>
#include <memory>
#include <gear/SiPlanesParameters.h>
#include <gear/SiPlanesLayerLayout.h>
#include <TH2D.h>
#include <TH1D.h>
#if defined(USE_AIDA) || defined(MARLIN_USE_AIDA)
#include <AIDA/IHistogram2D.h>
#include <AIDA/IProfile1D.h>
#endif
#include <TFile.h>
#include <TTree.h>
#include "datastructures.h"

using namespace lcio;
using namespace marlin;

/**
 * @author Gregor Vollmer
 * @version 1.42
 */

class MapsaDataMerger : public Processor
{
public:
	virtual Processor*  newProcessor() { return new MapsaDataMerger; }

	MapsaDataMerger();

	/** Called at the begin of the job before anything is read.
	 * Use to initialize the processor, e.g. book histograms.
	 */
	virtual void init();

	/** Called for every run.
	 */
	virtual void processRunHeader( LCRunHeader* run );

	/** Called for every event - the working horse.
	 */
	virtual void processEvent( LCEvent * evt );

	virtual void check( LCEvent * evt );

	/** Called after data processing for clean up.
	 */
	virtual void end();


protected:
	/// REF tracker data collection name
	std::string _colHitData;
	/// REF
	std::string _colTelClusterData;
	std::string _colRefClusterData;
	/// REF cut data file
	/// ROOT output filename
	std::string _rootOutputFile;
	std::string _mapsaFile;
	std::vector<int> _includedSensorIds;

	TFile* _infile;
	TFile* _file;
	TTree* _tree;
	TTree* _mpaTreeIn;
	std::vector<MpaData> _mpaData;
	std::vector<PlainRippleCounter> _counterIn;
	std::vector<PlainMemoryNoProcessing> _noProcessingIn;
	TelescopeData _telData;
	TelescopeHits _telHits;

private:
	size_t _currentMpaEventNumber;
	void fillHits(LCCollection* col);
	void fillClusters(LCCollection* col);

	void bookHistos();

#if defined(USE_AIDA) || defined(MARLIN_USE_AIDA)
#endif
};

#endif



