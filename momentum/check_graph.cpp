#include <iostream>
#include <fstream>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <regex>
#include <vector>

#include <TFile.h>
#include <TTree.h>
#include <TString.h>
#include <TCanvas.h>

#include <EdbDataSet.h>

#include "FnuMomCoord.hpp"


// To specify track uniquely
struct Track {
	int event_id;
	int plate_id;
	int seg_id;
	double x_first;
	double y_first;
	int plate_id_last;
	int npl;
	int pdg_id;
	double p_true;
	double p_reco;
	int ivertex;
};


// To specify vertex uniquely.
struct Vertex {
	int area_id;
	double vx;
	double vy;
	int plate;
	int ntrk;
	int ivertex;
};

// Global variables.
std::vector<Track> tracks;
std::vector<Vertex> verteces;
std::vector<std::string> invalid_files;
FnuMomCoord mc; // For momentum measurement.
TCanvas* c;


// To sort Track structure.
bool compareTrackId(const Track& track, const Track& target_track) {
	return track.event_id < target_track.event_id;
}

// Sort with ivertex.
template<typename T>
bool compareIVertex(const T& struct1, const T& struct2) {
	return struct1.ivertex < struct2.ivertex;
}


bool IsFileValid(std::string input_files) {
	TFile* file = new TFile(input_files.c_str(), "READ");
	if (!file->IsOpen() or file->IsZombie()) {
		invalid_files.push_back(input_files);
		file -> Close();
		return false;
	}
	TTree* tree = (TTree*) file -> Get("tracks");

	int nentries = tree -> GetEntries();
	std::cout << nentries << " entries." << std::endl;

	if (nentries == 0) {
		invalid_files.push_back(input_files);
		file -> Close();
		return false;
	}
	file -> Close();
	return true;
}


bool IsTrack(EdbTrackP* track, int event_id, int plate_id, int seg_id) {
	int nseg = track -> N();
	for (int i=0; i<nseg; i++) {
		EdbSegP* seg = track -> GetSegment(i);
		if ((seg -> MCEvt()%100000)==event_id%100000 and seg -> ScanID().GetPlate() == plate_id and seg -> ID() == seg_id) return true;
	}

	return false;
}

void ReadVertexFile(std::string vtx_file) {
	std::ifstream ifs(vtx_file);

	if (ifs.fail()) {
		std::cerr << "Failed to open this file." << std::endl;
		exit(1);
	}

	std::string type_name; // 1ry_vtx or 1ry_track.

	// For tracks.
	int plate_id_first, seg_id_first, plate_id_last, npl, pdg_id, event_id;
	float x_first, y_first, p_true, p_reco;

	// For veteces.
	int area_id, plate_id, ntrk;
	double vx, vy;

	// For the matching of vertex and tracks;
	int ivertex = -1;


	std::string line_buf;
	while (std::getline(ifs, line_buf)) {

		// Format input data.
		auto it = std::unique(line_buf.begin(),line_buf.end(),
            [](char const &lhs, char const &rhs) {
                return (lhs == rhs) && (lhs == ' ');
		});
		//line_buf.erase(it,line_buf.end());

		std::istringstream iss(line_buf);
		iss >> type_name;
		if (type_name == "1ry_trk") {
			iss >> plate_id_first >> seg_id_first >> x_first >> y_first >> plate_id_last >> npl >> pdg_id >> p_true >> p_reco >> event_id;

			Track track;
			track.event_id = event_id % 100000;
			track.plate_id = plate_id_first;
			track.seg_id = seg_id_first;
			track.ivertex = ivertex;
			track.x_first = x_first;
			track.y_first = y_first;
			track.plate_id_last = plate_id_last;
			track.npl = npl;
			track.pdg_id = pdg_id;
			track.p_true = p_true;
			track.p_reco = -999;
			track.ivertex = ivertex;

			tracks.push_back(track);
		} else if (type_name == "1ry_vtx") {
			ivertex ++;
			iss >> area_id >> vx >> vy >> plate_id >> ntrk;

			Vertex vertex;
			vertex.area_id = area_id;
			vertex.vx = vx;
			vertex.vy = vy;
			vertex.plate = plate_id;
			vertex.ntrk = ntrk;
			vertex.ivertex = ivertex;

			verteces.push_back(vertex);
		}
	}

	// Sort for binary search.
	std::sort(tracks.begin(), tracks.end(), compareTrackId);

	std::cout << verteces.size() << " verteces are read." << std::endl;
	std::cout << tracks.size() << " tracks are read." << std::endl;
}

void PrintMomGraph(std::string linked_tracks_file) {

	// Extrach Track ID.
	std::string pattern = "evt_(\\d+)_";
	std::regex regex(pattern);
	std::smatch match;
	std::string event_id;

	if (std::regex_search(linked_tracks_file, match, regex) && match.size() > 1) {
		event_id = match.str(1);
	} else {
		std::cerr << "Error: Couldn't extract Track ID from the filename." << std::endl;
		exit(1);
	}

	TString cut = Form("(s.eMCEvt%%100000)==%d", std::stoi(event_id)%100000);

	EdbDataProc* dproc = new EdbDataProc;
	EdbPVRec* pvr = new EdbPVRec;

	if (!IsFileValid(linked_tracks_file)) return;

	dproc -> ReadTracksTree(*pvr, linked_tracks_file.c_str(), cut);
	int ntrk = pvr -> Ntracks();
	
	Track track;
	track.event_id = std::stoi(event_id)%100000;
	std::vector<Track>::iterator iter_lower = std::lower_bound(tracks.begin(), tracks.end(), track, compareTrackId);
	std::vector<Track>::iterator iter_upper = std::upper_bound(tracks.begin(), tracks.end(), track, compareTrackId);
	int idx_lower = std::distance(tracks.begin(), iter_lower);
	int idx_upper = std::distance(tracks.begin(), iter_upper);


	for (int i=idx_lower; i<idx_upper; i++) {
		Track trk_buf = tracks[i];
		int eve = trk_buf.event_id;
		int track_id = trk_buf.seg_id;
		int plate_id = trk_buf.plate_id;

		for (int itrk=0; itrk<ntrk; itrk++) {
			EdbTrackP* track = pvr -> GetTrack(itrk);
			if (IsTrack(track, eve, plate_id, track_id)) {
				tracks[i].p_reco = mc.CalcMomentum(track, 0);
				tracks[i].plate_id_last = track -> GetSegmentLast() -> ScanID().GetPlate();
				tracks[i].npl = track -> Npl();
				mc.DrawMomGraphCoord(track, c, "mom_graph");
				std::cout << "Measured." << std::endl;
			}
		}
	}
}

void Run(std::string ltlists, const char* par_file="../par/MC_plate_1_100.txt") {
	
	std::ifstream ifs(ltlists);

	if (ifs.fail()) {
		std::cerr << "Error! Could not open the file." << std::endl;
		exit(1);
	}

	// For the momentum measurement.
	mc.ReadParFile(par_file);

	c = new TCanvas("c");
	c -> Print("mom_graph.pdf[");
	std::string path;
	while(std::getline(ifs, path)) {
		PrintMomGraph(path);
	}

	mc.ShowPar();

	std::cout << "Invalid files: " << std::endl;
	for (auto file: invalid_files) {
		std::cout << file << std::endl;
	}
	c -> Print("mom_graph.pdf]");
}


int main(int argc, char** argv) {

	char* input_vertex_file;
	char* input_list;
	char* par_file;
	
	for (int i=1; i<argc; i+=2) {
		if (std::string(argv[i]) == "-V") input_vertex_file = argv[i+1];
		else if (std::string(argv[i]) == "-I") input_list = argv[i+1];
		else if (std::string(argv[i]) == "-P") par_file = argv[i+1];
	}

	ReadVertexFile(input_vertex_file);
	
	if (par_file == nullptr) {
		Run(input_list);
	} else {
		Run(input_list, par_file);
	}
}
