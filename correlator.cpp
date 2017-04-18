
#include <TFile.h>
#include <TTree.h>
#include <TH2F.h>
#include <iostream>
#include <string>
#include <vector>
#include "datastructures.h"
#include <cassert>
#include <sstream>
#include <algorithm>

void correlateMpaToTelescope(MpaData* mpa, TelescopePlaneClusters* tel, TH2D* histX, TH2D* histY);
void correlateTelescopeToTelescope(TelescopePlaneClusters* tel1, TelescopePlaneClusters* tel2,
                                   TH2D* histX, TH2D* histY);

#define MPA_RES_X 16
#define MPA_RES_Y 3
#define MIMOSA_RES_Y 576
#define MIMOSA_RES_X 1152
#define PIXEL_RES_X 52
#define PIXEL_RES_Y 80
#define CORRELATION_SLICE_SIZE 50000

int getPixelY(int index)
{
	return 2 - index/16;
}

int getPixelX(int index)
{
	int py = getPixelY(index);
	return ((py==1)?15:0) + index%16 * ((py==1)?-1:1);
}

int main(int argc, char* argv[])
{
	if(argc != 3 && argc != 4) {
		std::cerr << "Usage: " << argv[0] << " ROOT-IN ROOT-OUT [NUM-EVENTS]" << std::endl;
		return 1;
	}
	TFile* in = new TFile(argv[1], "readonly");
	TFile* out = new TFile(argv[2], "recreate");
	if(!in || in->IsZombie()) {
		std::cerr << argv[0] << ": cannot open input file." << std::endl;
		return 1;
	}
	if(!out || out->IsZombie()) {
		std::cerr << argv[0] << ": cannot open output file." << std::endl;
		return 1;
	}
	Long64_t numEvents = 0;
	if(argc == 4) {
		numEvents = std::stoi(argv[3]);
	}
	TTree* tree = nullptr;
	in->GetObject("data", tree);
	if(!tree) {
		assert(0);
		return 1;
	}
	std::vector<MpaData*> mpaData;
	TelescopeData* telData = new TelescopeData;
	tree->SetBranchAddress("telescope", &telData);
	std::vector<std::string> mpaNames;
	for(size_t mpa = 1; mpa <= 6; ++mpa) {
		std::ostringstream name;
		name << "mpa_" << mpa;
		if(tree->FindBranch(name.str().c_str())) {
			mpaNames.push_back(name.str());
			mpaData.push_back(new MpaData);
		}
	}
	for(size_t i = 0; i < mpaData.size(); ++i) {
		tree->SetBranchAddress(mpaNames[i].c_str(), &mpaData[i]);
	}
	
	std::cout << "Ready to correlate!" << std::endl;
	std::vector<TH2D*> corhists;
	for(size_t mpaIdx = 0; mpaIdx < mpaData.size(); ++mpaIdx) {
		auto mpaName = mpaNames[mpaIdx];
		out->mkdir(mpaName.c_str());
		out->cd(mpaName.c_str());
		for(size_t telIdx = 0; telIdx < 6; ++telIdx) {
			std::ostringstream name;
			name << "plane_" << telIdx << "_";
			std::ostringstream descr;
			descr << "MPA to Plane " << telIdx << " Correlation in ";
			auto xcor = new TH2D((name.str()+"X").c_str(),
			                     (descr.str()+"X").c_str(),
			                     MPA_RES_X, 0, MPA_RES_X,
					     MIMOSA_RES_X, 0, MIMOSA_RES_X);
			auto ycor = new TH2D((name.str()+"Y").c_str(),
			                     (descr.str()+"Y").c_str(),
			                     MPA_RES_Y, 0, MPA_RES_Y,
					     MIMOSA_RES_Y, 0, MIMOSA_RES_Y);
			// todo: set axis labels
			corhists.push_back(xcor);
			corhists.push_back(ycor);
		}
		auto xcor = new TH2D("ref_x",
		                     "MPA to Reference Correlation in X",
		                     MPA_RES_X, 0, MPA_RES_X,
				     PIXEL_RES_X, 0, PIXEL_RES_X);
		auto ycor = new TH2D("ref_y",
		                     "MPA to Reference Correlation in Y",
		                     MPA_RES_Y, 0, MPA_RES_Y,
				     PIXEL_RES_Y, 0, PIXEL_RES_Y);
		// todo: set axis labels
		corhists.push_back(xcor);
		corhists.push_back(ycor);
	}
	out->mkdir("ref_tel_cor");
	out->cd("ref_tel_cor");
	std::vector<TH2D*> refhists(2);
	refhists[0] = new TH2D("ref_plane_6_x",
	                       "Ref to Plane 6 Correlation in X",
			       PIXEL_RES_X, 0, PIXEL_RES_X,
			       MIMOSA_RES_X, 0, MIMOSA_RES_X);
	refhists[1] = new TH2D("ref_plane_6_y",
	                       "Ref to Plane 6 Correlation in Y",
			       PIXEL_RES_Y, 0, PIXEL_RES_Y,
			       MIMOSA_RES_Y, 0, MIMOSA_RES_Y);
	refhists[0]->GetXaxis()->SetTitle("Reference X");
	refhists[0]->GetYaxis()->SetTitle("Plane 6 X");
	refhists[1]->GetXaxis()->SetTitle("Reference Y");
	refhists[1]->GetYaxis()->SetTitle("Plane 6 Y");
	std::vector<TH2D*> slicehists(tree->GetEntries() / CORRELATION_SLICE_SIZE + 1);
	out->mkdir("slices");
	out->cd("slices");
	for(size_t sliceNum = 0; sliceNum < slicehists.size(); ++sliceNum) {
		std::ostringstream name, descr;
		name << "slice_from_evt_" << sliceNum*CORRELATION_SLICE_SIZE;
		descr << "Slice from " << sliceNum*CORRELATION_SLICE_SIZE
		      << " to " << (sliceNum+1)*CORRELATION_SLICE_SIZE;
		slicehists[sliceNum] = new TH2D(name.str().c_str(),
		                                descr.str().c_str(),
						MPA_RES_X, 0, MPA_RES_X,
						MIMOSA_RES_X, 0, MIMOSA_RES_X);
		std::string xname(mpaNames[0]);
		xname += " X";
		slicehists[sliceNum]->GetXaxis()->SetTitle(xname.c_str());
		slicehists[sliceNum]->GetYaxis()->SetTitle("Telescope Plane 3 X");
	}
	out->mkdir("hitmaps");
	out->cd("hitmaps");
	std::vector<TH2D*> hitmaps;
	for(size_t mpaIdx = 0; mpaIdx < mpaData.size(); ++mpaIdx) {
		auto mpaName = mpaNames[mpaIdx];
		auto hitmap = new TH2D((std::string("hitmap_"+mpaName)).c_str(), mpaName.c_str(),
			MPA_RES_X, 0, MPA_RES_X,
			MPA_RES_Y, 0, MPA_RES_Y);
		hitmaps.push_back(hitmap);
	}
	if(numEvents <= 0) {
		numEvents = tree->GetEntries();
	}
	numEvents = std::min(numEvents, tree->GetEntries());
	for(int i = 0; i < numEvents; ++i) {
		int currentSlice = i / CORRELATION_SLICE_SIZE;
		assert(currentSlice < slicehists.size());
		if(i % 1 == 0) {
			std::cout << "\r\x1b[K Processing event "
			          << i << " of " << numEvents
				  << "  Slice " << currentSlice + 1 << " of " << slicehists.size()
				  << std::flush;
		}
		tree->GetEntry(i);
		size_t histIdx = 0;
		for(size_t mpaIdx = 0; mpaIdx < mpaData.size(); ++mpaIdx) {
			auto md = mpaData[mpaIdx];
			for(size_t telIdx = 0; telIdx < 7; ++telIdx) {
				auto td = (&telData->p1) + telIdx;
				correlateMpaToTelescope(md, td, corhists[histIdx], corhists[histIdx+1]);
				histIdx += 2;
			}
			for(size_t px = 0; px < 48; px++) {
				if(md->counter.pixels[px] > 0) {
					hitmaps[mpaIdx]->Fill(getPixelX(px), getPixelY(px));
				}
			}
		}
		correlateMpaToTelescope(mpaData[0], &telData->p3, slicehists[currentSlice], nullptr);
		correlateTelescopeToTelescope(&telData->ref, &telData->p6, refhists[0], refhists[1]);
	}
	std::cout << "\r\x1b[K Processing " << numEvents << " events done!\n"
	          << "Generate summary images..."
		  << std::endl;
	in->Close();
	out->Write();
	out->Close();
	return 0;
}

void correlateMpaToTelescope(MpaData* mpa, TelescopePlaneClusters* tel, TH2D* histX, TH2D* histY)
{
	for(int tp = 0; tp < tel->x.GetNoElements(); ++tp) {
		for(size_t mp = 0; mp < 48; ++mp) {
			if(mpa->counter.pixels[mp] == 0) {
				continue;
			}
			int pixel_y = getPixelY(mp);
			int pixel_x = getPixelX(mp);
			if(histX) {
				histX->Fill(pixel_x, tel->x[tp]);
			}
			if(histY) {
				histY->Fill(pixel_y, tel->y[tp]);
			}
		}
	}
}

void correlateTelescopeToTelescope(TelescopePlaneClusters* tel1, TelescopePlaneClusters* tel2,
                                   TH2D* histX, TH2D* histY)
{
	for(int tp1 = 0; tp1 < tel1->x.GetNoElements(); ++tp1) {
		for(int tp2 = 0; tp2 < tel2->x.GetNoElements(); ++tp2) {
			if(histX) {
				histX->Fill(tel1->x[tp1], tel2->x[tp2]);
			}
			if(histY) {
				histY->Fill(tel1->y[tp1], tel2->y[tp2]);
			}
		}
	}
}
