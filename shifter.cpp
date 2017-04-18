
#include <TFile.h>
#include <TTree.h>
#include <TGraph.h>
#include <iostream>
#include <string>
#include <vector>
#include "datastructures.h"
#include <cassert>
#include <sstream>
#include <algorithm>

int main(int argc, char* argv[])
{
	if(argc != 5) {
		std::cerr << "Usage: " << argv[0] << " ROOT-IN ROOT-OUT REFSHIFT TELSHIFT" << std::endl;
		return 1;
	}
	TFile* in = new TFile(argv[1], "readonly");
	if(!in || in->IsZombie()) {
		std::cerr << argv[0] << ": cannot open input file." << std::endl;
		return 1;
	}
	TFile* out = new TFile(argv[2], "recreate");
	if(!out || out->IsZombie()) {
		std::cerr << argv[0] << ": cannot open output file." << std::endl;
		return 1;
	}
	int telshift = std::stoi(argv[4]);
	int refshift = std::stoi(argv[3]);

	TTree* intree = nullptr;
	in->GetObject("data", intree);
	if(!intree) {
		std::cerr << "Root tree 'data' not found in file. Possibly old format?" << std::endl;
		return 1;
	}
	std::vector<std::string> mpaNames;
	std::vector<int> mpaIndex;
	std::vector<MpaData*> mpaData;
	std::vector<MpaData*> mpaDataOut;
	std::vector<TBranch*> mpaBranches;
	for(size_t mpa = 1; mpa <= 6; ++mpa) {
		std::ostringstream name;
		name << "mpa_" << mpa;
		if(intree->FindBranch(name.str().c_str())) {
			mpaNames.push_back(name.str());
			mpaData.push_back(new MpaData);
			mpaDataOut.push_back(new MpaData);
			mpaIndex.push_back(mpa);
		}
	}
	auto tree = new TTree("data", "Shifted measurement data");
	for(size_t i = 0; i < mpaData.size(); ++i) {
		intree->SetBranchAddress(mpaNames[i].c_str(), &mpaData[i]);
		tree->Branch(mpaNames[i].c_str(), &mpaDataOut[i]);
		mpaBranches.push_back(intree->FindBranch(mpaNames[i].c_str()));
	}
	TelescopeData* telData = new TelescopeData;
	TelescopeData* telDataOut = new TelescopeData;
	intree->SetBranchAddress("telescope", &telData);
	tree->Branch("telescope", &telDataOut);
	auto telBranch = intree->FindBranch("telescope");
	int minShift = std::min(telshift, refshift);
	int maxShift = std::max(telshift, refshift);
	const auto numEvents = intree->GetEntries();
	const auto maxEvents = std::min(numEvents + minShift, numEvents);
	const auto initialEvent = std::max(0, maxShift);
	for(size_t evt = initialEvent; evt < maxEvents; ++evt) {
		if((evt-initialEvent) % 100 == 0) {
			std::cout << "\r\x1b[K Processing event "
			          << evt-initialEvent << " of " << maxEvents-initialEvent
				  << std::flush;
		}
		for(size_t i = 0; i < mpaData.size(); ++i) {
			mpaBranches[i]->GetEntry(evt);
			mpaDataOut[i] = mpaData[i];
		}
		telBranch->GetEntry(evt - telshift);
		for(size_t i = 0; i < 6; ++i) {
			auto pi = &telData->p1 + i;
			auto po = &telDataOut->p1 + i;
			po->x.ResizeTo(pi->x.GetNoElements());
			po->y.ResizeTo(pi->y.GetNoElements());
			*po = *pi;
		}
		telBranch->GetEntry(evt - refshift);
		telDataOut->ref.x.ResizeTo(telData->ref.x.GetNoElements());
		telDataOut->ref.y.ResizeTo(telData->ref.y.GetNoElements());
		telDataOut->ref = telData->ref;
		tree->Fill();
	}
	std::cout << "\r\x1b[K Processing event "
	          << maxEvents-initialEvent << " of " << maxEvents-initialEvent
		  << "\nSaving ROOT file ..."
		  << std::flush;
	out->Write();
	out->Close();
	std::cout << " done!" << std::endl;
	return 0;
}
