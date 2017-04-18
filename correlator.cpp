
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
#include <TImage.h>
#include <TCanvas.h>

void correlateMpaToTelescope(MpaData* mpa, TelescopePlaneClusters* tel, TH2D* histX, TH2D* histY, bool swapAxes);
void correlateTelescopeToTelescope(TelescopePlaneClusters* tel1, TelescopePlaneClusters* tel2,
                                   TH2D* histX, TH2D* histY);
void shiftScan(TTree* tree, std::vector<std::string> mpaNames, const std::vector<MpaData*>& mpaData, TelescopeData* telData, TFile* out, bool swapAxes);
void shiftRefScan(TTree* tree, std::vector<std::string> mpaNames, const std::vector<MpaData*>& mpaData, TelescopeData* telData, TFile* out, bool swapAxes);
void writeHistogramPanel(std::string canvasName, std::string fileName, std::string descr, std::vector<TH2D*> histograms);

#define TEL_SHIFT_MIN -9
#define TEL_SHIFT_MAX 3
#define REF_SHIFT_MIN -9
#define REF_SHIFT_MAX 3

#define MPA_RES_X 16
#define MPA_RES_Y 3
#define MIMOSA_RES_Y 576
#define MIMOSA_RES_X 1152
#define PIXEL_RES_X 52
#define PIXEL_RES_Y 80
#define CORRELATION_SLICE_SIZE 25000

#define MPA_DYN_X(swap) (swap?MPA_RES_Y:MPA_RES_X)
#define MPA_DYN_Y(swap) (swap?MPA_RES_X:MPA_RES_Y)
#define MIMOSA_DYN_X(swap) (swap?MIMOSA_RES_Y:MIMOSA_RES_X)
#define MIMOSA_DYN_Y(swap) (swap?MIMOSA_RES_X:MIMOSA_RES_Y)
#define PIXEL_DYN_X(swap) (swap?PIXEL_RES_Y:PIXEL_RES_X)
#define PIXEL_DYN_Y(swap) (swap?PIXEL_RES_X:PIXEL_RES_Y)

std::string g_outputBaseName;

std::string getFilename(std::string suffix)
{
	return g_outputBaseName + "_" + suffix;
}

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
	if(argc != 4 && argc != 5) {
		std::cerr << "Usage: " << argv[0] << " ROOT-IN ROOT-OUT (swap|normal) [SHIFT]" << std::endl;
		return 1;
	}
	TFile* in = new TFile(argv[1], "readonly");
	g_outputBaseName = argv[2];
	if(!in || in->IsZombie()) {
		std::cerr << argv[0] << ": cannot open input file." << std::endl;
		return 1;
	}
	TFile* out = new TFile(argv[2], "recreate");
	if(!out || out->IsZombie()) {
		std::cerr << argv[0] << ": cannot open output file." << std::endl;
		return 1;
	}
	Long64_t numEvents = 0;
	//if(argc >= 4) {
	//	numEvents = std::stoi(argv[3]);
	//}
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
	bool calculateShifts = true;
	int shift = 0;
	if(argc >= 5) {
		calculateShifts = false;
		shift = std::stoi(argv[4]);
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
	out->mkdir("summary");
	if(calculateShifts) {
		shiftScan(tree, mpaNames, mpaData, telData, out, swapAxes);
		shiftRefScan(tree, mpaNames, mpaData, telData, out, swapAxes);
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
			auto xcor = new TH2D((name.str()+"Y").c_str(),
			                     (descr.str()+"Y").c_str(),
			                     MPA_RES_X, 0, MPA_RES_X,
					     MIMOSA_DYN_X(swapAxes), 0, MIMOSA_DYN_X(swapAxes));
			auto ycor = new TH2D((name.str()+"X").c_str(),
			                     (descr.str()+"X").c_str(),
			                     MPA_RES_Y, 0, MPA_RES_Y,
					     MIMOSA_DYN_Y(swapAxes), 0, MIMOSA_DYN_Y(swapAxes));
			// todo: set axis labels
			corhists.push_back(xcor);
			corhists.push_back(ycor);
		}
		auto xcor = new TH2D("ref_y",
		                     "MPA X to Reference Y Correlation",
		                     MPA_RES_X, 0, MPA_RES_X,
				     PIXEL_DYN_X(swapAxes), 0, PIXEL_DYN_X(swapAxes));
		auto ycor = new TH2D("ref_x",
		                     "MPA Y to Reference X Correlation",
		                     MPA_RES_Y, 0, MPA_RES_Y,
				     PIXEL_DYN_Y(swapAxes), 0, PIXEL_DYN_Y(swapAxes));
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
	std::vector<TH2D*> telslicehists(tree->GetEntries() / CORRELATION_SLICE_SIZE + 1);
	std::vector<TH2D*> refslicehists(tree->GetEntries() / CORRELATION_SLICE_SIZE + 1);
	std::vector<TH2D*> reftelslicehists(tree->GetEntries() / CORRELATION_SLICE_SIZE + 1);
	out->mkdir("correlation_slices");
	out->cd("correlation_slices");
	for(size_t sliceNum = 0; sliceNum < telslicehists.size(); ++sliceNum) {
		std::ostringstream name, descr;
		name << "slice_from_evt_" << sliceNum*CORRELATION_SLICE_SIZE;
		descr << "Slice from " << sliceNum*CORRELATION_SLICE_SIZE
		      << " to " << (sliceNum+1)*CORRELATION_SLICE_SIZE;
		telslicehists[sliceNum] = new TH2D((name.str()+"_MPA_TEL").c_str(),
		                                   descr.str().c_str(),
		   				   MPA_RES_X, 0, MPA_RES_X,
		   				   MIMOSA_DYN_X(swapAxes), 0, MIMOSA_DYN_X(swapAxes));
		refslicehists[sliceNum] = new TH2D((name.str()+"_MPA_REF").c_str(),
		                                   descr.str().c_str(),
		   				   MPA_RES_X, 0, MPA_RES_X,
		   				   PIXEL_DYN_X(swapAxes), 0, PIXEL_DYN_X(swapAxes));
		reftelslicehists[sliceNum] = new TH2D((name.str()+"_REF_TEL").c_str(),
		                                      descr.str().c_str(),
		   				      PIXEL_RES_X, 0, PIXEL_RES_X,
		   				      MIMOSA_RES_X, 0, MIMOSA_RES_X);
//		std::string xname(mpaNames[0]);
//		xname += " X";
//		slicehists[sliceNum]->GetXaxis()->SetTitle(xname.c_str());
//		slicehists[sliceNum]->GetYaxis()->SetTitle("Telescope Plane 3 Y");
	}
	out->mkdir("residual");
	out->cd("residual");
	struct residual_t {
		TH1D* x[7];
		TH1D* y[7];
	};
	std::vector<residual_t> residuals(mpaNames.size());
	for(size_t mpa = 0; mpa < mpaNames.size(); ++mpa) {
		std::string dir("residual/mpa_");
		dir += std::to_string(mpa);
		out->mkdir(dir.c_str());
		out->cd(dir.c_str());
		for(size_t i = 0; i < 6; ++i) {
			int xwidth = MIMOSA_DYN_X(swapAxes) + MPA_RES_X;
			int ywidth = MIMOSA_DYN_Y(swapAxes) + MPA_RES_Y;
			residuals[mpa].x[i] = new TH1D((std::string("mpa_x_plane_") + std::to_string(i)).c_str(), "", MPA_RES_X*10, -xwidth, xwidth);
			residuals[mpa].y[i] = new TH1D((std::string("mpa_y_plane_") + std::to_string(i)).c_str(), "", MPA_RES_Y*50, -ywidth, ywidth);
		}
		int xwidth = PIXEL_DYN_X(swapAxes) + MPA_RES_X;
		int ywidth = PIXEL_DYN_Y(swapAxes) + MPA_RES_Y;
		residuals[mpa].x[6] = new TH1D("mpa_x_ref", "", MPA_RES_X*10, -xwidth, xwidth);
		residuals[mpa].y[6] = new TH1D("mpa_y_ref", "", MPA_RES_Y*10, -ywidth, ywidth);
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
	std::vector<TBranch*> mpaBranches(mpaNames.size());
	for(size_t i = 0; i < mpaNames.size(); ++i) {
		mpaBranches[i] = tree->FindBranch(mpaNames[i].c_str());
	}
	TBranch* telBranch = tree->FindBranch("telescope");
	int activatedPixels = 0;
	int processedEvents = 0;
	for(int i = std::max(0, shift); i < std::min(numEvents, numEvents-shift); ++i, ++processedEvents) {
		int currentSlice = i / CORRELATION_SLICE_SIZE;
		assert(currentSlice < telslicehists.size());
		if(i % 1 == 0) {
			std::cout << "\r\x1b[K Processing event "
			          << i << " of " << numEvents
				  << "  Slice " << currentSlice + 1 << " of " << telslicehists.size()
				  << std::flush;
		}
		for(const auto& branch: mpaBranches) {
			branch->GetEntry(i);
		}
		telBranch->GetEntry(i-shift);
		size_t histIdx = 0;
		for(size_t mpaIdx = 0; mpaIdx < mpaData.size(); ++mpaIdx) {
			auto md = mpaData[mpaIdx];
			for(size_t telIdx = 0; telIdx < 7; ++telIdx) {
				auto td = (&telData->p1) + telIdx;
				correlateMpaToTelescope(md, td, corhists[histIdx], corhists[histIdx+1], swapAxes);
				histIdx += 2;
				// Residual Plots
				int numMpaHits = 0;
				int mx = 0;
				int my = 0;
				int tx = 0;
				int ty = 0;
				double factor = (mpaIndex[mpaIdx] <= 3)? -1.0 : 1.0;
				for(size_t px = 0; px < 48; px++) {
					if(md->counter.pixels[px] != 1) {
						continue;
					}
					if(td->x.GetNoElements() != 1) {
						continue;
					}
					mx = getPixelX(px);
					my = getPixelY(px);
					if(mx == 0 || mx == 15) {
						continue;
					}
					for(int i = 0; i < td->x.GetNoElements(); ++i) {
						tx = swapAxes? td->y[i] : td->x[i];
						ty = swapAxes? td->x[i] : td->y[i];
					}
					++numMpaHits;
				}
				if(numMpaHits == 1) {
					residuals[mpaIdx].x[telIdx]->Fill(mx - tx * factor);
					residuals[mpaIdx].y[telIdx]->Fill(my - ty * factor);
				}
			}
			for(size_t px = 0; px < 48; px++) {
				if(md->counter.pixels[px] > 0) {
					hitmaps[mpaIdx]->Fill(getPixelX(px), getPixelY(px));
					++activatedPixels;
				}
			}
		}
		correlateMpaToTelescope(mpaData[0], &telData->p3, telslicehists[currentSlice], nullptr, swapAxes);
		correlateMpaToTelescope(mpaData[0], &telData->ref, refslicehists[currentSlice], nullptr, swapAxes);
		correlateTelescopeToTelescope(&telData->ref, &telData->p6, reftelslicehists[currentSlice], nullptr);
		correlateTelescopeToTelescope(&telData->ref, &telData->p6, refhists[0], refhists[1]);
	}
	double yield = static_cast<double>(activatedPixels)/static_cast<double>(processedEvents);
	std::cout << "\r\x1b[K Processing " << numEvents << " events done!\n"
	          << "I found " << activatedPixels << " pixel activations in all MPAs for " << processedEvents << "processed events\n"
		  << "That's a yield of " << yield*100 << "%,  estimate " << yield*100*0.35 << "% with REF\n\n"
	          << "Generate summary images..."
		  << std::endl;
	in->Close();
	out->Write();
	out->Close();
	return 0;
}

void shiftRefScan(TTree* tree, std::vector<std::string> mpaNames, const std::vector<MpaData*>& mpaData, TelescopeData* telData, TFile* out, bool swapAxes)
{
	assert(mpaNames.size() > 0);
	assert(out != nullptr);
	assert(tree != nullptr);
	int numEvents = std::min((Long64_t)20000, tree->GetEntries() - std::abs(REF_SHIFT_MAX) - std::abs(REF_SHIFT_MIN));
	std::vector<double> summaryShift;
	std::vector<double> summaryCoefficient;
	std::cout << "Calculate REF shifts..." << std::endl;;
	out->mkdir("shift_mpa_ref");
	out->cd("shift_mpa_ref");
	std::vector<TH2D*> histograms;
	std::vector<TBranch*> mpaBranches(mpaNames.size());
	for(size_t i = 0; i < mpaNames.size(); ++i) {
		mpaBranches[i] = tree->FindBranch(mpaNames[i].c_str());
	}
	TBranch* telBranch = tree->FindBranch("telescope");
	for(int shift = REF_SHIFT_MIN, idx = 0; shift <= REF_SHIFT_MAX; ++shift, ++idx) {
		std::cout << "\r\x1b[K for event offset "
		          << shift
			  << std::flush;
		std::string shift_name("shift_");
		if(shift < 0) {
			shift_name += "m";
		}
		shift_name += std::to_string(std::abs(shift));
		auto refcor = new TH2D((shift_name + "_correlation").c_str(), (std::string("MaPSA X <-> REF Y Correlation at Shift ") + std::to_string(shift)).c_str(),
				MPA_RES_X, 0, MPA_RES_X,
				PIXEL_DYN_X(swapAxes)/8, 0, PIXEL_DYN_X(swapAxes));
		histograms.push_back(refcor);
		for(int event = shift; event < numEvents; ++event) {
			std::vector<int> mpaPixels;
			for(auto branch: mpaBranches) {
				branch->GetEntry(event);
			}
			telBranch->GetEntry(event - shift);
			for(size_t mp = 0; mp < 48; ++mp) {
				if(mpaData[0]->counter.pixels[mp] == 0) {
					continue;
				}
				mpaPixels.push_back(getPixelX(mp));
			}
			for(auto mp: mpaPixels) {
				for(int tp = 0; tp < telData->ref.y.GetNoElements(); ++tp) {
					int telX = swapAxes? telData->ref.y[tp] : telData->ref.x[tp];
					refcor->Fill(mp, telX);
				}
			}
		}
	}
	out->cd("summary");
	writeHistogramPanel("ref_mpa_shifts", getFilename("ref_mpa_shift.png"), "Shifts between REF and MPA", histograms);
	std::cout << "\r\x1b[K done!" << std::endl;
}

void shiftScan(TTree* tree, std::vector<std::string> mpaNames, const std::vector<MpaData*>& mpaData, TelescopeData* telData, TFile* out, bool swapAxes)
{
	assert(mpaNames.size() > 0);
	assert(out != nullptr);
	assert(tree != nullptr);
	int maxShift = 5;
	int numEvents = std::min((Long64_t)20000, tree->GetEntries()-maxShift*2);
	std::vector<double> summaryShift;
	std::vector<double> summaryCoefficient;
	std::cout << "Calculate shifts..." << std::endl;;
	out->mkdir("shift_mpa_tel");
	out->cd("shift_mpa_tel");
	std::vector<TH2D*> histograms;
	std::vector<TBranch*> mpaBranches(mpaNames.size());
	for(size_t i = 0; i < mpaNames.size(); ++i) {
		mpaBranches[i] = tree->FindBranch(mpaNames[i].c_str());
	}
	TBranch* telBranch = tree->FindBranch("telescope");
	for(int shift = TEL_SHIFT_MIN, idx = 0; shift <= TEL_SHIFT_MAX; ++shift, ++idx) {
		std::cout << "\r\x1b[K for event offset "
		          << shift
			  << std::flush;
		std::string shift_name("shift_");
		if(shift < 0) {
			shift_name += "m";
		}
		shift_name += std::to_string(std::abs(shift));
		auto xcor = new TH2D((shift_name + "_correlation").c_str(), (std::string("MaPSA X <-> Plane 3 Y Correlation at Shift ") + std::to_string(shift)).c_str(),
				MPA_RES_X, 0, MPA_RES_X,
				MIMOSA_DYN_X(swapAxes)/8, 0, MIMOSA_DYN_X(swapAxes));
		histograms.push_back(xcor);
		auto res = new TH1D((shift_name + "_residual").c_str(), (std::string("MaPSA X <-> Plane 3 Y Residual Shift ") + std::to_string(shift)).c_str(),
				MIMOSA_DYN_X(swapAxes), 0, MIMOSA_DYN_X(swapAxes));
		for(int event = shift; event < numEvents; ++event) {
			std::vector<int> mpaPixels;
			for(auto branch: mpaBranches) {
				branch->GetEntry(event);
			}
			telBranch->GetEntry(event - shift);
			for(size_t mp = 0; mp < 48; ++mp) {
				if(mpaData[0]->counter.pixels[mp] == 0) {
					continue;
				}
				mpaPixels.push_back(getPixelX(mp));
			}
			for(auto mp: mpaPixels) {
				for(int tp = 0; tp < telData->p6.y.GetNoElements(); ++tp) {
					int telX = swapAxes? telData->p6.y[tp] : telData->p6.x[tp];
					xcor->Fill(mp, telX);
					res->Fill(telX - mp);
				}
			}
		}
		summaryShift.push_back(static_cast<double>(shift));
		summaryCoefficient.push_back(res->GetRMS());
	}
	auto summary = new TGraph(summaryShift.size(), &(summaryShift[0]), &(summaryCoefficient[0]));
	summary->SetName("summary");
	summary->Write();
	out->cd("summary");
	writeHistogramPanel("tel_mpa_shifts", getFilename("tel_mpa_shifts.png"), "Shifts between Telescope and MPA", histograms);
	std::cout << "\r\x1b[K done!" << std::endl;
}

void correlateMpaToTelescope(MpaData* mpa, TelescopePlaneClusters* tel, TH2D* histX, TH2D* histY, bool swapAxes)
{
	for(int tp = 0; tp < tel->x.GetNoElements(); ++tp) {
		for(size_t mp = 0; mp < 48; ++mp) {
			if(mpa->counter.pixels[mp] == 0) {
				continue;
			}
			int pixel_y = getPixelY(mp);
			int pixel_x = getPixelX(mp);
			int telX = swapAxes? tel->y[tp] : tel->x[tp];
			int telY = swapAxes? tel->x[tp] : tel->y[tp];
			if(histX) {
				histX->Fill(pixel_x, telX);
			}
			if(histY) {
				histY->Fill(pixel_y, telY);
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

void writeHistogramPanel(std::string canvasName, std::string fileName, std::string descr, std::vector<TH2D*> histograms)
{
	size_t num_x = sqrt(histograms.size());
	size_t num_y = histograms.size()/num_x;
	if(num_x * num_y < histograms.size()) {
		num_y++;
	}
	auto canvas = new TCanvas(canvasName.c_str(), descr.c_str(), num_x*400, num_y*300);
	canvas->Divide(num_x, num_y);
	for(size_t i = 0; i < histograms.size(); ++i) {
		canvas->cd(i+1);
		gPad->SetLogz();
		histograms[i]->Draw("colz");
	}
	canvas->Write();
	if(fileName.size()) {
		auto img = TImage::Create();
		img->FromPad(canvas);
		img->WriteImage(fileName.c_str());
		delete img;
	}
}
