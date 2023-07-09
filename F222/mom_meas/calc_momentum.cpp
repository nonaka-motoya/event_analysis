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

// DEBUG
//TCanvas* c = new TCanvas("c");

// To sort Track structure.
bool compareTrackId(const Track& track, const Track& target_track) {
	return track.event_id < target_track.event_id;
}

// Sort with ivertex.
template<typename T>
bool compareIVertex(const T& struct1, const T& struct2) {
	return struct1.ivertex < struct2.ivertex;
}


// Read MC files using list file.
void ReadMCFiles(std::string list_file) {
	std::ifstream ifs(list_file);

	if (ifs.fail()) {
		std::cerr << "Failed to open this file." << std::endl;
		exit(1);
	}

	std::string line_buf;
	while (std::getline(ifs, line_buf)) {
		std::cout << line_buf << std::endl;
	}
}

// Read a MC file.
void ReadMCFile(std::string MC_file) {
	TFile* file = new TFile(MC_file.c_str(), "READ");
	TTree* tree = (TTree*) file -> Get("");
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
			track.event_id = event_id;
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
}

bool IsTrack(EdbTrackP* track, int event_id, int plate_id, int seg_id) {
	event_id += 100000;
	int nseg = track -> N();
	for (int i=0; i<nseg; i++) {
		EdbSegP* seg = track -> GetSegment(i);
		if (seg -> MCEvt() == event_id and seg -> ScanID().GetPlate() == plate_id and seg -> ID() == seg_id) return true;
	}

	return false;
}


void CalcMomentum(std::string linked_tracks_file) {
	// Extrach Track ID.
	std::string pattern = "evt_(\\d+)_pl";
	std::regex regex(pattern);
	std::smatch match;
	std::string event_id;

	if (std::regex_search(linked_tracks_file, match, regex) && match.size() > 1) {
		event_id = match.str(1);
	} else {
		std::cerr << "Error: Couldn't extract Track ID from the filename." << std::endl;
		exit(1);
	}

	TString cut = Form("s.eMCEvt==%d", std::stoi(event_id)+100000);

	EdbDataProc* dproc = new EdbDataProc;
	EdbPVRec* pvr = new EdbPVRec;

	dproc -> ReadTracksTree(*pvr, linked_tracks_file.c_str(), cut);
	int ntrk = pvr -> Ntracks();
	
	Track track;
	track.event_id = std::stoi(event_id);
	std::vector<Track>::iterator iter_lower = std::lower_bound(tracks.begin(), tracks.end(), track, compareTrackId);
	std::vector<Track>::iterator iter_upper = std::upper_bound(tracks.begin(), tracks.end(), track, compareTrackId);
	int idx_lower = std::distance(tracks.begin(), iter_lower);
	int idx_upper = std::distance(tracks.begin(), iter_upper);

	// For the momentum measurement.
	FnuMomCoord mc;
	mc.ReadParFile("/home/mnonaka/environment/FASERSoft/FnuMomCoord/par/MC_plate_1_50.txt");
	mc.ShowPar();


	for (int i=idx_lower; i<idx_upper; i++) {
		Track trk_buf = tracks[i];
		int eve = trk_buf.event_id;
		int track_id = trk_buf.seg_id;
		int plate_id = trk_buf.plate_id;

		for (int itrk=0; itrk<ntrk; itrk++) {
			EdbTrackP* track = pvr -> GetTrack(itrk);
			if (IsTrack(track, eve, plate_id, track_id)) {
				tracks[i].p_reco = mc.CalcMomentum(track, 0);
				//mc.DrawMomGraphCoord(track, c, "test");
			}
		}
	}


}

void WriteVertexFile(std::string output_file) {
	std::ofstream ofs(output_file);

	// Sort tracks and verteces with ivertex;
	std::sort(tracks.begin(), tracks.end(), compareIVertex<Track>);
	std::sort(verteces.begin(), verteces.end(), compareIVertex<Vertex>);

	for (int i=0; i<verteces.size(); i++) {
		Vertex vertex = verteces[i];
		int ivertex = vertex.ivertex;

		ofs << "1ry_vtx\t" << vertex.area_id << "\t" << vertex.vx << "\t" << vertex.vy << "\t" << vertex.plate << "\t" << vertex.ntrk << std::endl;

	  	Track trk_buf;
	  	trk_buf.ivertex = ivertex;
		std::vector<Track>::iterator iter_lower = std::lower_bound(tracks.begin(), tracks.end(), trk_buf, compareIVertex<Track>);
		std::vector<Track>::iterator iter_upper = std::upper_bound(tracks.begin(), tracks.end(), trk_buf, compareIVertex<Track>);
		int idx_lower = std::distance(tracks.begin(), iter_lower);
		int idx_upper = std::distance(tracks.begin(), iter_upper);

		for (int j=idx_lower; j<idx_upper; j++) {
			Track track = tracks[j];
			ofs << "1ry_trk" << "\t" << track.plate_id << "\t" << track.seg_id << "\t" << track.x_first << "\t" << track.y_first << "\t" << track.plate_id_last << "\t" << track.npl << "\t" << track.pdg_id << "\t" << track.p_true << "\t" << track.p_reco << "\t" << track.event_id << std::endl;
		}
	}

	ofs.close();
	
}

void Run(std::string ltlists) {
	std::ifstream ifs(ltlists);

	if (ifs.fail()) {
		std::cerr << "Error! Could not open the file." << std::endl;
		exit(1);
	}

	//c -> Print("test.pdf[");
	std::string path;
	while(std::getline(ifs, path)) {
		CalcMomentum(path);
	}
	//c -> Print("test.pdf]");
}

int main(int argc, char** argv) {
	ReadVertexFile("./input_files/vtx_info_nuall_00010-00039_p500_numucc_v20230706.txt");

	//CalcMomentum("/data/FASER/fasernu-pilot-run/MDC_rec_tmp/20230402_nuall_00010-00019_p300/evt_21581_pl107_527/linked_tracks.root");
	Run("./input_files/LTList.txt");

	WriteVertexFile("./output/vtx_info_nuall_00010-00039_p500_numucc_v20230706_measured_mometum.txt");
	//WriteVertexFile("./output/vtx_test.txt");
	
	return 0;
}
