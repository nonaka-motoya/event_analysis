/**
*	@file		fake_hadron.cpp
*	@brief		vertex fileを読み込んでevent selectionでfakeとなるhadronを出力する
*	@author		Motoya Nonaka
*	@date		2nd Nov 2023
*/

#include <iostream>
#include <cmath>
#include <fstream>
#include <sstream>
#include <exception>
#include <stdexcept>


/**
*	@fn			PrintUsage
*	@brief		プログラムの使用方法を出力する
*	@return		void
*/
void PrintUsage() {
	printf("Usage: \n");
	printf("./fake_hadron -I <input vertex file> -O <outout file>");

	return;
}

/**
*	@fn			Run
*	@brief
*	@param[in]	input_file
*	@param[in]	output_file
*	@return void
*/
void Run(std::string input_file, std::string output_file) {
	
	std::ifstream inputFile(input_file);
	std::ofstream outputFile(output_file);

	if (!inputFile.is_open()) {
		throw std::runtime_error("Cannot open the file");
	}

	std::string type_name; // 1ry_vtx or 1ry_track.
	// Variables for tracks.
	int plate_id_first, seg_id_first, plate_id_last, npl, pdg_id, event_id;
	float x_first, y_first, p_true, p_reco;

	std::string line_buf;
	while (std::getline(inputFile, line_buf)) {
		std::istringstream iss(line_buf);
		iss >> type_name;

		if (type_name == "1ry_trk") {
			iss >> plate_id_first >> seg_id_first >> x_first >> y_first >> plate_id_last >> npl >> pdg_id >> p_true >> p_reco >> event_id;
			if (p_reco > 200 and std::sqrt(x_first*x_first + y_first*y_first) > 0.005 and npl >= 10) {
				outputFile << type_name << "\t" << plate_id_first << "\t" << seg_id_first << "\t" << x_first << "\t" << y_first << "\t" << plate_id_last << "\t" << npl << "\t" << pdg_id << "\t" << p_true << "\t" << p_reco << "\t" << event_id << std::endl;
			}
		}
	}

	inputFile.close();
	outputFile.close();
}

int main(int argc, char** argv) {
	if (argc < 2) {
		std::cout << "Argument missig." << std::endl;
		PrintUsage();
		std::cout << "Exit." << std::endl;
		exit(1);
	}

	std::string input_vertex_file;
	std::string output_file;

	for (int i=1; i<argc; i+=2) {
		if (std::string(argv[i]) == "-I") input_vertex_file = std::string(argv[i+1]);
		else if (std::string(argv[i]) == "-O") output_file = std::string(argv[i+1]);
		else {
			std::cout << "Invalid argument." << std::endl;
			PrintUsage();
			std::cout << "Exit." << std::endl;
		}
	}
	
	try {
		Run(input_vertex_file, output_file);
	} catch(const std::exception& e) {
		std::cerr << "Caught exeption: " << e.what() << std::endl;
	}

	return 0;
}
