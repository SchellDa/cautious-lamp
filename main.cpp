
#include <TFile.h>
#include <boost/program_options.hpp>
#include <string>
#include <iterator>
#include <iostream>
#include <fstream>
#include "johannesexporter.h"

namespace po = boost::program_options;

std::string join_path(std::string a, std::string b);
std::string join_path(std::string a, std::string b, std::string c);
std::string join_path(std::string a, std::string b, std::string c, std::string d);
std::string search_lcio(std::string directory, std::string lcio);

int main(int argc, char* argv[])
{
	po::options_description generic("Generic Options");
	generic.add_options()
		("help,h", "Print help message")
		("config,c", po::value<std::string>(), "Config file to load")
	;
	po::options_description config("Configuration");
	config.add_options()
		("lcio-dir,d", po::value<std::string>()->default_value(""), "Path where LCIO files are stored")
		("mapsa-dir,m", po::value<std::string>()->default_value(""), "Path where MaPSA data files are stored")
		("output-dir,o", po::value<std::string>()->default_value(""), "Directory where output files are put")
		("mapsa-shift", po::value<int>()->default_value(0), "Shift MaPSA events by given distance")
		("num-events", po::value<int>()->default_value(0), "Export only given number of events. Set to 0 to export everything")
	;
	po::options_description hidden("");
	hidden.add_options()
		("input-lcio", po::value<std::string>(), "")
		("input-mapsa", po::value<std::string>(), "")
		("output-root", po::value<std::string>(), "")
	;
	po::options_description cmdline_options;
	cmdline_options.add(generic).add(config).add(hidden);
	po::options_description display_options;
	cmdline_options.add(generic).add(config);
	po::options_description configfile_options;
	configfile_options.add(config);
	po::positional_options_description p;
	p.add("input-lcio", 1);
	p.add("input-mapsa", 1);
	p.add("output-root", 1);

	po::variables_map vm;
	po::store(po::command_line_parser(argc, argv).options(cmdline_options).positional(p).run(), vm);
	if(vm.count("help")) {
		std::cout << "Usage: " << argv[0] << "[options] LCIO MAPSA OUTPUT\n"
		          << display_options << std::endl;
		return 0;
	}
	notify(vm);
	if(vm.count("config")) {
		std::ifstream file(vm["config"].as<std::string>());
		if(!file.good()) {
			std::cerr << argv[0] << ": Cannot open config file '" << vm["config"].as<std::string>() << "'!" << std::endl;
			return 1;
		}
		po::store(po::parse_config_file(file, configfile_options), vm);
		notify(vm);
	}
	if(!vm.count("input-lcio")) {
		std::cerr << argv[0] << ": LCIO input file not specified." << std::endl;
		return 1;
	}
	if(!vm.count("input-mapsa")) {
		std::cerr << argv[0] << ": MaPSA input file not specified." << std::endl;
		return 1;
	}
	if(!vm.count("output-root")) {
		std::cerr << argv[0] << ": ROOT output file not specified" << std::endl;
		return 1;
	}

	auto lcioFile = search_lcio(vm["lcio-dir"].as<std::string>(), vm["input-lcio"].as<std::string>());
	auto mapsaFile = search_lcio(vm["mapsa-dir"].as<std::string>(), vm["input-mapsa"].as<std::string>());
	auto rootFile = join_path(vm["output-dir"].as<std::string>(), vm["output-root"].as<std::string>());
	lcio::LCReader* lcReader = lcio::LCFactory::getInstance()->createLCReader();
	lcReader->open(lcioFile);
	JohannesExporter exporter(mapsaFile, rootFile, vm["mapsa-shift"].as<int>(), vm["num-events"].as<int>(), false);
	lcReader->registerLCEventListener(&exporter);
	lcReader->readStream();
	lcReader->close();
	
	return 0;
}

std::string join_path(std::string a, std::string b)
{
	if(b.size() == 0) {
		return a;
	}
	if(b[0] == '/') {
		return b;
	}
	if(a.size() == 0) {
		return b;
	}
	if(a[a.size()-1] == '/') {
		return a+b;
	}
	return a+'/'+b;
}
std::string join_path(std::string a, std::string b, std::string c)
{
	return join_path(join_path(a, b), c);
}
std::string join_path(std::string a, std::string b, std::string c, std::string d)
{
	return join_path(join_path(a, b, c), d);
}

std::string search_lcio(std::string directory, std::string lcio)
{
	assert(lcio.size() > 0);
	std::ifstream file(lcio);
	if(file.good()) {
		return lcio;
	}
	return join_path(directory, lcio);
}
