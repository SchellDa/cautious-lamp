
#include <TFile.h>
#include <TTree.h>
#include <TGraph.h>
#include <TH2F.h>
#include <iostream>
#include <string>
#include <vector>
#include "datastructures.h"
#include <cassert>
#include <sstream>
#include <algorithm>
#include <deque>
#include <TImage.h>
#include <TCanvas.h>
#include <TLine.h>

void genTimecorrelation(int shift, TBranch* mpaBranch, TBranch* telBranch, MpaData* mpa, TelescopeData* tel, TH2D* hist, bool swapAxes);
void writeHistogramPanel(std::string canvasName, std::string fileName, std::string descr, std::vector<TH2D*> histograms, bool logz, bool swapAxes);

#define SHIFT_MIN -20
#define SHIFT_MAX 20

#define TIMECORRELATION_WINDOW_SIZE 15000
#define TIMECORRELATION_STEP_SIZE 10

#define MPA_RES_X 16
#define MPA_RES_Y 3
#define MIMOSA_RES_Y 576
#define MIMOSA_RES_X 1152
#define PIXEL_RES_X 52
#define PIXEL_RES_Y 80

#define MPA_DYN_X(swap) (swap?MPA_RES_Y:MPA_RES_X)
#define MPA_DYN_Y(swap) (swap?MPA_RES_X:MPA_RES_Y)
#define MIMOSA_DYN_X(swap) (swap?MIMOSA_RES_Y:MIMOSA_RES_X)
#define MIMOSA_DYN_Y(swap) (swap?MIMOSA_RES_X:MIMOSA_RES_Y)
#define PIXEL_DYN_X(swap) (swap?PIXEL_RES_Y:PIXEL_RES_X)
#define PIXEL_DYN_Y(swap) (swap?PIXEL_RES_X:PIXEL_RES_Y)

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
	if(argc != 4) {
		std::cerr << "Usage: " << argv[0] << " ROOT-IN IMAGE-OUT (swap|normal)" << std::endl;
		return 1;
	}
	TFile* in = new TFile(argv[1], "readonly");
	if(!in || in->IsZombie()) {
		std::cerr << argv[0] << ": cannot open input file." << std::endl;
		return 1;
	}
//	TFile* out = new TFile(argv[2], "recreate");
//	if(!out || out->IsZombie()) {
//		std::cerr << argv[0] << ": cannot open output file." << std::endl;
//		return 1;
//	}
	bool swapAxes = false;
	if(std::string(argv[3]) == "swap") {
		swapAxes = true;
	} else if(std::string(argv[3]) != "normal") {
		std::cerr << argv[0] << ": Unknown axes mode '" << argv[3] << "'. Must be swap or normal." << std::endl;
		return 1;
	}
	if(swapAxes) {
		std::cout << "MPA axes will be swapped w.r.t. REF and Tel" << std::endl;
	} else {
		std::cout << "MPA axes will be oriented like REF and Tel" << std::endl;
	}

	TTree* tree = nullptr;
	in->GetObject("data", tree);
	if(!tree) {
		std::cerr << "Root tree 'data' not found in file. Possibly old format?" << std::endl;
		return 1;
	}
	std::vector<MpaData*> mpaData;
	TelescopeData* telData = new TelescopeData;
	tree->SetBranchAddress("telescope", &telData);
	std::vector<std::string> mpaNames;
	std::vector<int> mpaIndex;
	for(size_t mpa = 1; mpa <= 6; ++mpa) {
		std::ostringstream name;
		name << "mpa_" << mpa;
		if(tree->FindBranch(name.str().c_str())) {
			mpaNames.push_back(name.str());
			mpaData.push_back(new MpaData);
			mpaIndex.push_back(mpa);
		}
	}
	for(size_t i = 0; i < mpaData.size(); ++i) {
		tree->SetBranchAddress(mpaNames[i].c_str(), &mpaData[i]);
	}
	std::cout << "Ready to correlate!" << std::endl;
	std::vector<TH2D*> timecorrelations;
	int nTimebins = tree->GetEntries() / TIMECORRELATION_STEP_SIZE;
	if(nTimebins == 0) {
		std::cout << "Not enough entries! Aborting..." << std::endl;
		return 2;
	}
	int xmax = MIMOSA_DYN_X(swapAxes) / 10 + MPA_RES_X;
	int xmin = PIXEL_DYN_X(swapAxes) + MPA_RES_X;
	for(int shift = SHIFT_MIN; shift <= SHIFT_MAX; ++shift) {
		std::ostringstream xlab;
		xlab << "Event No. * " << TIMECORRELATION_STEP_SIZE;
		std::ostringstream title;
		title << "Timecorrelation for shift " << shift;
		std::ostringstream name;
		name << "timecor_shift_";
		if(shift < 0) {
			name << "m" << -shift;
		} else {
			name << shift;
		}
		auto hist = new TH2D(name.str().c_str(),
				title.str().c_str(),
				nTimebins, 0, nTimebins,
		                MPA_RES_X*8, -xmin, xmax);
		hist->GetXaxis()->SetTitle(xlab.str().c_str());
		hist->GetYaxis()->SetTitle("Residual");
		timecorrelations.push_back(hist);
	}
	std::vector<TBranch*> mpaBranches(mpaNames.size());
	for(size_t i = 0; i < mpaNames.size(); ++i) {
		mpaBranches[i] = tree->FindBranch(mpaNames[i].c_str());
	}
	TBranch* telBranch = tree->FindBranch("telescope");
	for(int shift = SHIFT_MIN, idx = 0; shift <= SHIFT_MAX; ++shift, idx++) {
		std::cout << "\r\x1b[K Generate histogram for shift "
		          << shift
			  << std::flush;
		genTimecorrelation(shift, mpaBranches[0], telBranch, mpaData[0], telData, timecorrelations[idx], swapAxes);
	}
	std::cout << " done! \n Writing histograms ... " << std::flush;
	writeHistogramPanel("overview", argv[2], "Overview", timecorrelations, true, swapAxes);
	in->Close();
	std::cout << " done!" << std::endl;
//	out->Write();
//	out->Close();
	return 0;
}


void genTimecorrelation(int shift, TBranch* mpaBranch, TBranch* telBranch, MpaData* mpa, TelescopeData* tel, TH2D* hist, bool swapAxes)
{
	std::deque<std::vector<double>> window;
	int curTimebin = 0;
	const size_t maxEvents = std::min(mpaBranch->GetEntries() + shift, mpaBranch->GetEntries());
	const auto initialEvent = std::max(0, shift);
	for(size_t evt = initialEvent, processedEvents = 0; evt < maxEvents; ++evt, ++processedEvents) {
		mpaBranch->GetEntry(evt);
		telBranch->GetEntry(evt - shift);
		std::vector<double> currentResiduals;
		int numHits = 0;
		int mx = 0;
		for(size_t px = 0; px < 48; px++) {
			if(mpa->counter.pixels[px] != 1) {
				continue;
			}
			mx = getPixelX(px);
			if(mx == 0 || mx == 15) {
				continue;
			}
			++numHits;
			if(numHits >= 2) {
				break;
			}
		}
		if(numHits == 1) {
			for(size_t plane = 0; plane < 2; ++plane) {
				auto td = (&tel->p3);
				double factor = 0.1;
				if(plane == 1) {
					td = (&tel->ref);
					factor = -1.0;
				}
				int tx = 0;
				if(td->x.GetNoElements() != 1) {
					continue;
				}
				tx = swapAxes? td->y[0] : td->x[0];
				currentResiduals.push_back(static_cast<double>(tx - mx + MPA_RES_X)*factor);
			}
		}
		window.push_back(currentResiduals);
		while(window.size() > TIMECORRELATION_WINDOW_SIZE) {
			window.pop_front();
		}
		if(processedEvents % TIMECORRELATION_STEP_SIZE == 0) {
			for(const auto& residuals: window) {
				for(const auto& res: residuals) {
					hist->Fill(curTimebin, res);
				}
			}
			curTimebin++;
		}
	}
}

void writeHistogramPanel(std::string canvasName, std::string fileName, std::string descr, std::vector<TH2D*> histograms, bool logz, bool swapAxes)
{
	size_t num_x = sqrt(histograms.size());
	size_t num_y = histograms.size()/num_x;
	int ymax = MIMOSA_DYN_X(swapAxes) / 10 + MPA_RES_X;
	int ymin = PIXEL_DYN_X(swapAxes) + MPA_RES_X;
	int fullCorX = TIMECORRELATION_WINDOW_SIZE / TIMECORRELATION_STEP_SIZE;
	if(num_x * num_y < histograms.size()) {
		num_y++;
	}
	auto canvas = new TCanvas(canvasName.c_str(), descr.c_str(), num_x*400, num_y*300);
	canvas->Divide(num_x, num_y);
	for(size_t i = 0; i < histograms.size(); ++i) {
		canvas->cd(i+1);
		if(logz) {
			gPad->SetLogz();
		}
		histograms[i]->Draw("colz");
		TLine *line = new TLine(fullCorX, -ymin, fullCorX, ymax);
		line->SetLineColor(kBlue);
		line->SetLineWidth(3);
		line->Draw();
	}
	canvas->Write();
	if(fileName.size()) {
		auto img = TImage::Create();
		img->FromPad(canvas);
		img->WriteImage(fileName.c_str());
		delete img;
	}
}
