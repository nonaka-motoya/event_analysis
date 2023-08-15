#include <iostream>

#include <TObjArray.h>

#include <EdbDataSet.h>

#include "FnuMomCoord.hpp"

// Global variables.
FnuMomCoord mc;

void Init(std::string par_file="../par/MC_plate_1_100.txt") {
	mc.ReadParFile(par_file);
}

void FillMomentum(std::string input_file, std::string output_file="linked_tracks_measured_momentum.root") {
	EdbDataProc* dproc = new EdbDataProc;
	EdbPVRec* pvr = new EdbPVRec;

	dproc -> ReadTracksTree(*pvr, input_file.c_str(), "npl>=100");

	int ntrk = pvr -> Ntracks();

	TObjArray* selected = new TObjArray();
	for (int i=0; i<ntrk; i++) {

		if (i%1000 == 0) {
			std::cout << "\033[1A";
			std::cout << "\033[2K";
			std::cout << i << "/" << ntrk << " tracks are read." << std::endl;
		}

		EdbTrackP* track = pvr -> GetTrack(i);
		double momentum = mc.CalcMomentum(track, 0);
		track -> SetP(momentum);

		double angle_diff = mc.CalcTrackAngleDiffMax(track);
	
		if (angle_diff > 1.0) track -> SetFlag(-1);

		selected -> Add(track);
	}


	dproc -> MakeTracksTree(*selected, 0, 0, output_file.c_str());

	delete dproc;
	delete pvr;
}

int main(int argc, char** argv) {
	
	std::string input_list;
	std::string par_file;
	std::string output_file;

	for (int i=1; i<argc; i+=2) {
		if (std::string(argv[i]) == "-I") input_list = argv[i+1];
		else if (std::string(argv[i]) == "-O") output_file = argv[i+1];
		else if (std::string(argv[i]) == "-P") par_file = argv[i+1];
	}

	Init(par_file);
	FillMomentum(input_list, output_file);

	return 0;
}
