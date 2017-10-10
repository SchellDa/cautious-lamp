#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include <TObject.h>
#include <TVector.h>

class Conditionals {
public:
	Float_t threshold;
	Float_t voltage;
	Float_t current;
	Float_t temperature;
	Float_t z_pos;
	Float_t angle;
	Conditionals();
	virtual ~Conditionals() {}
	ClassDef(Conditionals, 1);
};

class RippleCounter {
public:
	UInt_t header;
	UShort_t pixels[48];
	RippleCounter();
	virtual ~RippleCounter() {}
	ClassDef(RippleCounter, 1);
};

class MemoryNoProcessing {
public:
	ULong64_t pixelMatrix[96];
	UShort_t bunchCrossingId[96];
	UShort_t header[96];
	UChar_t numEvents;
	UChar_t corrupt;
	MemoryNoProcessing();
	virtual ~MemoryNoProcessing() {}
	ClassDef(MemoryNoProcessing, 1);
};

struct PlainRippleCounter {
	UInt_t header;
	UShort_t pixels[48];
};

struct PlainMemoryNoProcessing {
	ULong64_t* pixelMatrix;
	UShort_t* bunchCrossingId;
	UShort_t* header;
	UChar_t* numEvents;
	UChar_t* corrupt;
};

class MpaData {
public:
	RippleCounter counter;
	MemoryNoProcessing noProcessing;
	MpaData();
	virtual ~MpaData() {}
	ClassDef(MpaData, 1);
};

class TelescopePlaneClusters {
public:
	TVectorF x;
	TVectorF y;
	TelescopePlaneClusters();
	virtual ~TelescopePlaneClusters() {}
	ClassDef(TelescopePlaneClusters, 1);
};

class PlaneHits {
public:
	TVectorF x;
	TVectorF y;
	TVectorF z;
	PlaneHits();
	virtual ~PlaneHits() {}
	ClassDef(PlaneHits, 1);
};

class TelescopeData {
public:
	TelescopePlaneClusters p1;
	TelescopePlaneClusters p2;
	TelescopePlaneClusters p3;
	TelescopePlaneClusters p4;
	TelescopePlaneClusters p5;
	TelescopePlaneClusters p6;
	TelescopePlaneClusters ref;
	TelescopeData();
	virtual ~TelescopeData() {}
	ClassDef(TelescopeData, 1);
};

class TelescopeHits {
public:
	PlaneHits p1;
	PlaneHits p2;
	PlaneHits p3;
	PlaneHits p4;
	PlaneHits p5;
	PlaneHits p6;
	PlaneHits ref;
	TelescopeHits();
	virtual ~TelescopeHits() {}
	ClassDef(TelescopeHits, 1);
};

class AlibavaData {
public:

    TVectorI event;
    TVectorI center;
    TVectorD clock;
    TVectorD time;
    TVectorD temp;
    TVectorD clusterSignal;

    AlibavaData();
    virtual ~AlibavaData() {};
    ClassDef(AlibavaData, 1);
};



#endif//DATA_STRUCTURES_H

