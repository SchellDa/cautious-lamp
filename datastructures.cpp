
#include "datastructures.h"
#include <math.h>

ClassImp(Conditionals)
ClassImp(RippleCounter)
ClassImp(MemoryNoProcessing)
ClassImp(MpaData)
ClassImp(TelescopePlaneClusters)
ClassImp(TelescopeData)

Conditionals::Conditionals() :
 threshold(NAN), voltage(NAN), current(NAN),
 temperature(NAN), z_pos(NAN), angle(NAN)
{
}

RippleCounter::RippleCounter() :
 header(0)
{
}

MemoryNoProcessing::MemoryNoProcessing() :
 numEvents(0), corrupt(0)
{
}

MpaData::MpaData() :
 counter(), noProcessing()
{
}

TelescopePlaneClusters::TelescopePlaneClusters()
 : x(), y()
{
}

TelescopeData::TelescopeData()
{
}

