#ifndef ALIBAVA_DATA_MERGER_H
#define ALIBAVA_DATA_MERGER_H

// marlin includes
#include "marlin/Processor.h"
//#include "lcio.h"

// ROOT includes
#include <TFile.h>
#include <TTree.h>

// AIDA includes
#if defined(USE_AIDA) || defined(MARLIN_USE_AIDA)
#include <AIDA/IHistogram2D.h>
#include <AIDA/IProfile1D.h>
#endif

#include "datastructures.h"
#include "AsciiRoot.h"

using namespace lcio;
using namespace marlin;

class AlibavaDataMerger : public Processor {

public:
    
    virtual Processor* newProcessor() { return new AlibavaDataMerger; }

    AlibavaDataMerger();

    /** Called at the begin of the job before anything is read.
     * Use to initialize the processor, e.g. book histograms.
     */
    virtual void init();
    
    /** Called for every run.
     */
    virtual void processRunHeader( LCRunHeader* run );
    
    /** Called for every event - the working horse.
     */
    virtual void processEvent( LCEvent *evt );
    
    //virtual void check( LCEvent *evt );
    
    /** Called after data processing for clean up.
     */
    virtual void end();


protected:
    
    std::string _colHitData;
    std::string _colTelClusterData;
    std::string _colRefClusterData;

    std::string _alibavaFile;
    std::string _rootOutputFile;

    std::vector<int> _includedSensorIds;

    AsciiRoot _alibavaIn;
    TFile* _file;
    TTree* _alibavaTreeIn;
    TTree* _tree;
    
    TelescopeData _telData;
    TelescopeHits _telHits;
    AlibavaData _aliData;

private:

    void fillHits(LCCollection*);

#if defined(USE_AIDA) || defined(MARLIN_USE_AIDA)
#endif

};


#endif
