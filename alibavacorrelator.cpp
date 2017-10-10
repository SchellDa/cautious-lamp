#include <iostream>

#include <TFile.h>
#include <TTree.h>
#include <TH2F.h>

#include "boost/program_options.hpp"

#include "datastructures.h"

#define MIMOSA_N_X 1152
#define MIMOSA_N_Y 576
#define ALIBAVA_N 256

int main(int argc, char** argv){

	namespace po = boost::program_options;
	po::options_description desc("Allowed options");
	desc.add_options()
		("help,h", "Print help message")
		("file,f", po::value<std::string>()->required(),
		 "Define input ROOT file")
		("out,o", po::value<std::string>()->default_value("out.root"),
		 "Define ROOT output file name");
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);

	if ( vm.count("help") ) {
		std::cout << desc << "\n";
		return 1;
	}
	po::notify(vm);
	
	// Read in
	TFile *inFile = new TFile(vm["file"].as<std::string>().c_str(), "READ");
	TTree *inTree = nullptr;
	inFile->GetObject("data", inTree);
	
	if(!inTree) 
	{
		std::cerr << "ROOT tree not found in file" << std::endl;
	}

	TelescopeData *telData = new TelescopeData;
	AlibavaData *aliData = new AlibavaData;
	inTree->SetBranchAddress("telescope", &telData);
	inTree->SetBranchAddress("alibava", &aliData);

	TFile *oFile = new TFile((vm["out"].as<std::string>()+".root").c_str(), 
				 "RECREATE");
	TH2F *corX = new TH2F("xcor", "X correlation", 
			      ALIBAVA_N, 0, ALIBAVA_N, 
			      MIMOSA_N_X, 0, MIMOSA_N_X);
	TH2F *corY = new TH2F("ycor", "Y correlation", 
			      ALIBAVA_N, 0, ALIBAVA_N,
			      MIMOSA_N_Y, 0, MIMOSA_N_Y);

	oFile->mkdir("ali_tel_cor");
	oFile->cd("ali_tel_cor");
	
	for(int tp = 0; tp < telData->p3.x.GetNoElements(); ++tp) {
		for(size_t ap = 0; ap < aliData->center.GetNoElements(); ++ap) {
			corX->Fill(telData->p3.x[tp], aliData->center[ap]);
			corY->Fill(telData->p3.y[tp], aliData->center[ap]);
		}
	}

	inFile->Close();
	oFile->Write();
	oFile->Close();

	return 0;
}
