#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <algorithm>
#include <math.h>

#include <TFile.h>
#include <TTree.h>
#include <TCanvas.h>
#include <TImage.h>
#include <TH2F.h>
//#include <TProfile.h>

#include "boost/program_options.hpp"

#include "datastructures.h"

#define MIMOSA_N_X 1152
#define MIMOSA_N_Y 576
#define FEI4_N_X 80
#define FEI4_N_Y 336
#define ALIBAVA_N 256

void correlateAlibavaToTelescope(AlibavaData* aliData, 
				 TelescopePlaneClusters* planeData, 
				 TH2F* corX, TH2F* corY);
void correlateAlibavaToReference(AlibavaData* aliData, 
				 TelescopePlaneClusters* planeData, 
				 TH2F* corX, TH2F* corY);
void correlateTelescopeToReference(TelescopePlaneClusters* planeData,
				   TelescopePlaneClusters* refData, 
				   TH2F* corX, TH2F* corY);
void timing(AlibavaData* aliData, TH2F* timing);

int main(int argc, char** argv){

	// parse command line arguments to BOOST
	namespace po = boost::program_options;
	po::options_description desc("Allowed options");
	desc.add_options()
		("help,h", "Print help message")
		("file,f", po::value<std::string>()->required(),
		 "Define input ROOT file")
		("out,o", po::value<std::string>()->default_value("out"),
		 "Define ROOT output file name");
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);

	if ( vm.count("help") ) {
		std::cout << desc << "\n";
		return 1;
	}
	po::notify(vm);
	
	// Read input file 
	TFile *inFile = new TFile(vm["file"].as<std::string>().c_str(), "READ");
	TTree *inTree = nullptr;
	inFile->GetObject("data", inTree);
	
	if(!inTree) 
	{
		std::cerr << "ROOT tree not found in file" << std::endl;
	}

	// Initialize data container and link them to their corresponding 
	// branch
	TelescopeData *telData = new TelescopeData;
	AlibavaData *aliData = new AlibavaData;
	inTree->SetBranchAddress("telescope", &telData);
	inTree->SetBranchAddress("alibava", &aliData);

	// Initialize output file and also create and switch to first folder
	TFile *oFile = new TFile((vm["out"].as<std::string>()+".root").c_str(), 
				 "RECREATE");
	oFile->mkdir("ali_tel_cor");
	oFile->cd("ali_tel_cor");

	// Prepare all histograms
	std::vector<TH2F*> aliTelX;
	std::vector<TH2F*> aliTelY;

	std::vector<TH2F*> telRefX;
	std::vector<TH2F*> telRefY;
	
	for(size_t iPlane = 0; iPlane < 6; ++iPlane)
	{
		std::ostringstream name;
		std::ostringstream descr;
		name << "plane_" << iPlane << "_";
		descr << "Plane " << iPlane << " <-> Alibava correlation in ";

		auto *corX = new TH2F((name.str()+"X").c_str(), 
				      (descr.str()+"X").c_str(),
				      ALIBAVA_N, 0, ALIBAVA_N, 
				      MIMOSA_N_X, 0, MIMOSA_N_X);
		auto *corY = new TH2F((name.str()+"Y").c_str(), 
				      (descr.str()+"Y").c_str(),				   
				      ALIBAVA_N, 0, ALIBAVA_N,
				      MIMOSA_N_Y, 0, MIMOSA_N_Y);
		aliTelX.push_back(corX);
		aliTelY.push_back(corY);
	}
	
	oFile->mkdir("ali_ref_cor");
	oFile->cd("ali_ref_cor");

	auto *aliRefX = new TH2F("Ref_Y", 
			      "Ref_Y <-> Alibava_X",
			      ALIBAVA_N, 0, ALIBAVA_N, 
			      FEI4_N_Y, 0, FEI4_N_Y);
	auto *aliRefY = new TH2F("Ref_X", 
			      "Ref_X <-> Alibava_Y",				   
			      ALIBAVA_N, 0, ALIBAVA_N,
			      FEI4_N_X, 0, FEI4_N_X);

	oFile->mkdir("tel_ref_cor");
	oFile->cd("tel_ref_cor");

	for(size_t iPlane = 0; iPlane < 6; ++iPlane)
	{
		std::ostringstream name;
		std::ostringstream descr;
		name << "planeRef_" << iPlane << "_";
		descr << "Plane " << iPlane << " <-> Ref correlation in ";

		auto *corX = new TH2F((name.str()+"X").c_str(), 
				      (descr.str()+"X").c_str(),
				      FEI4_N_Y, 0, FEI4_N_Y, 
				      MIMOSA_N_X, 0, MIMOSA_N_X);
		auto *corY = new TH2F((name.str()+"Y").c_str(), 
				      (descr.str()+"Y").c_str(),				   
				      FEI4_N_X, 0, FEI4_N_X,
				      MIMOSA_N_Y, 0, MIMOSA_N_Y);
		telRefX.push_back(corX);
		telRefY.push_back(corY);
	}
	
	oFile->mkdir("timing");
	oFile->cd("timing");

	auto *timeSignal = new TH2F("Timing", "Cluster signal <-> Timing", 
				    100, 0, 100, 100, 0, 100);
	
	// Loop over all events and fill the histograms
	for(int iEvt = 0; iEvt < inTree->GetEntries(); ++iEvt)
	{
		inTree->GetEvent(iEvt);
		auto ref = (&telData->p1) + 6;

		for(size_t telIdx = 0; telIdx < 6; ++telIdx) {
			auto td = (&telData->p1) + telIdx;
			correlateAlibavaToTelescope(aliData, td, 
						    aliTelX[telIdx], 
						    aliTelY[telIdx]);
			correlateTelescopeToReference(td, ref, 
						      telRefX[telIdx],
						      telRefY[telIdx]);
		}

		// Reference
		correlateAlibavaToReference(aliData, ref,
					    aliRefX, aliRefY);		

		timing(aliData, timeSignal);
	}

	//
	// Correlation factor not used during DQM
	//
	/*
	// Get correlation factor
	std::vector<Double_t> corFactX;
	std::ofstream corFact;
	corFact.open((vm["out"].as<std::string>()+"_cor.csv").c_str());
	for(const auto* iPlot : aliTelX){
		corFact << iPlot->GetCorrelationFactor() << ", ";
	}
	corFact << std::endl;
	for(const auto* iPlot : aliTelY){
		corFact << iPlot->GetCorrelationFactor() << ", ";
	}
	corFact.close();
	*/

	// Create summery plot
	TCanvas *canvas = new TCanvas("c1", "Correlation Plots", 1920, 1080);
	canvas->Divide(3,5);

	for(size_t telIdx = 0; telIdx < 6; ++telIdx){
		canvas->cd(telIdx+1);
		aliTelX[telIdx]->Draw("colz");
	}
	canvas->cd(7);
	aliRefX->Draw("colz");
	

	auto img = TImage::Create();
	img->FromPad(canvas);
	img->WriteImage((vm["out"].as<std::string>()+".png").c_str());
	delete img;

	inFile->Close();
	oFile->Write();
	oFile->Close();

	return 0;
}

void correlateAlibavaToTelescope(AlibavaData* aliData, 
				 TelescopePlaneClusters* planeData, 
				 TH2F* corX, TH2F* corY)
{
	for(int iEvt = 0; iEvt < planeData->x.GetNoElements(); ++iEvt)
	{
		for(int tp = 0; tp < planeData->x.GetNoElements(); ++tp) {
			for(int ap = 0; ap < aliData->center.GetNoElements(); ++ap) 
			{
				corX->Fill(aliData->center[ap], planeData->x[tp]);
				corY->Fill(aliData->center[ap], planeData->y[tp]);
			}
		}
	}
}

void correlateAlibavaToReference(AlibavaData* aliData, 
				 TelescopePlaneClusters* planeData, 
				 TH2F* corX, TH2F* corY)
{

	std::vector<int> mask = {0};

	/*
	std::vector<int> mask = {1, 117, 118, 119, 120, 128, 129, 130, 131, 155,
				 167, 168, 141, 142, 253, 254, 255};
	*/

	for(int iEvt = 0; iEvt < planeData->x.GetNoElements(); ++iEvt)
	{
		for(int tp = 0; tp < planeData->x.GetNoElements(); ++tp) 
		{
			for(int ap = 0; ap < aliData->center.GetNoElements(); ++ap) 
			{
				if ( std::find(mask.begin(), mask.end(), 
					       static_cast<int>(aliData->center[ap])) != mask.end() )
				{
					continue;
				}
				corX->Fill(aliData->center[ap], planeData->y[tp]);
				corY->Fill(aliData->center[ap], planeData->x[tp]);
			}
		}
	}
}

void correlateTelescopeToReference(TelescopePlaneClusters* planeData, 
				   TelescopePlaneClusters* refData, 
				   TH2F* corX, TH2F* corY)
{
	for(int iEvt = 0; iEvt < planeData->x.GetNoElements(); ++iEvt)
	{
		for(int tp = 0; tp < planeData->x.GetNoElements(); ++tp) {
			for(int rp = 0; rp < refData->x.GetNoElements(); ++rp) {
				corX->Fill(refData->y[rp], planeData->x[tp]);
				corY->Fill(refData->x[rp], planeData->y[tp]);
			}
		}
	}
}
 
void timing(AlibavaData* aliData, TH2F* timing){
	
	for(int ap = 0; ap < aliData->center.GetNoElements(); ++ap) {
		timing->Fill(aliData->time[ap], fabs(aliData->clusterSignal[ap]));
	}

	timing->ProfileX();
	
}
