/**
*	@file		calc_momentum.cpp
*	@brief		vertex fileにreconstructed momentumを詰める
*	@author		Motoya Nonaka
*	@date		18th Oct 2023
*/

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

#include "EdbEDAUtil.h"
#include "FnuMomCoord.hpp"

/**
*	@struct		Track
*	@brief		To specify track uniquely
*/
struct Track {
	int event_id;		// Event ID
	int plate_id;		// Plate of first segment
	int seg_id;			// Segment ID of first segment
	double x_first;		// X of first segment
	double y_first;		// Y of first segment
	int plate_id_last;	// Plate of last segment
	int npl;			// Number of plates
	int pdg_id;			// PDG ID
	double p_true;		// Truth momenum
	double p_reco;		// Reconstructed momentum
	int ivertex;		// Index of the vertex
};


/**
*	@struct		Vertex
*	@brief		To specify vertex uniquely
*/
struct Vertex {
	int area_id;		// Area ID
	double vx;
	double vy;
	int plate;
	int ntrk;
	int ivertex;		//Index of the vertex
};

// Global variables.
std::vector<Track> tracks;
std::vector<Vertex> verteces;
std::vector<std::string> files;
std::vector<std::string> invalid_files;
FnuMomCoord mc; // For momentum measurement.
EdbDataProc* dproc;
EdbPVRec* pvr;


// To sort Track structure.
bool compareTrackId(const Track& track, const Track& target_track) {
	return track.event_id < target_track.event_id;
}

// Sort with ivertex.
template<typename T>
bool compareIVertex(const T& struct1, const T& struct2) {
	return struct1.ivertex < struct2.ivertex;
}

/*
*	@fn			ExtractEventID
*	@param[in]	str		文字列
*/
int ExtractEventID(std::string str, std::string pattern="evt_(\\d+)_") {
	std::regex regex(pattern);
	std::smatch match;
	std::string event_id;

	if (std::regex_search(str, match, regex) && match.size() > 1) {
		event_id = match.str(1);
	} else {
		std::cerr << "Error: Couldn't extract Track ID from the filename." << std::endl;
		exit(1);
	}

	return std::stoi(event_id);
}

/*
*	@fn			compareFile
*	@brief		To sort path of linked_tracks.root with event ID.
*	@param[in]
*	@param[in]
*	@return		bool
*/
bool CompareFile(const std::string& lhs, std::string& rhs) {
	int lhs_event_id = ExtractEventID(lhs);
	int rhs_event_id = ExtractEventID(rhs);
	return lhs_event_id < rhs_event_id;
}

/**
*	@fn			ReadVertexFile
*	@brief		Vertex fileを読み込んで構造体に詰める。
*	@param[in]	vtx_file	Vertex fileのパス
*	@return		void
*/
void ReadVertexFile(std::string vtx_file) {
	std::ifstream ifs(vtx_file);

	if (ifs.fail()) {
		std::cerr << "Failed to open this file." << std::endl;
		exit(1);
	}

	std::string type_name; // 1ry_vtx or 1ry_track.

	// Variables for tracks.
	int plate_id_first, seg_id_first, plate_id_last, npl, pdg_id, event_id;
	float x_first, y_first, p_true, p_reco;

	// Variables for veteces.
	int area_id, plate_id, ntrk;
	double vx, vy;

	// For the matching between vertex and tracks;
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
	//std::sort(tracks.begin(), tracks.end(), compareTrackId);
	std::sort(tracks.begin(), tracks.end(), compareIVertex<Track>);
}

/**
*	@fn			ReadFilePath
*	@breif		リストファイルからlinked_tracks.rootのパスを読み、filesに詰める
*	@param[in]	ltlists		linked_tracks.rootのパスが書かれているテキストファイルのパス
*	@par		Modify
*		- files
*	@return		void
*/
void ReadFilePath(std::string ltlists) {
	std::ifstream ifs(ltlists);

	// Error handling
	if (ifs.fail()) {
		std::cerr << "Error! Could not open the file." << std::endl;
		exit(1);
	}

	std::string path;
	while(std::getline(ifs, path)) {
		files.push_back(path);
	}

	std::sort(files.begin(), files.end(), CompareFile);

	return;
}

bool IsTrack(EdbTrackP* track, int event_id, int plate_id, int seg_id) {
	int nseg = track -> N();
	for (int i=0; i<nseg; i++) {
		EdbSegP* seg = track -> GetSegment(i);
		if ((seg -> MCEvt()%100000)==event_id%100000 and seg -> ScanID().GetPlate() == plate_id and seg -> ID() == seg_id) return true;
	}

	return false;
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
	//std::cout << nentries << " entries." << std::endl;

	if (nentries == 0) {
		invalid_files.push_back(input_files);
		file -> Close();
		return false;
	}
	file -> Close();
	return true;
}


/*
*	@fn			FillMomentum
*	@param[in]	event_id	探すevent_id
*	@param[in]	start	tracksの最初のindex
*	@param[in]	end		tracksの最初のindex
*	@par		Refer
*	@return		void
*/
void FillMomentum(int event_id, int start, int end) {
	for (int i=-1; i<6; i++) {

		int ev = i * 1000000 + event_id;
		//event idにマッチするpathを取得する
		auto iter_file = std::find_if(files.begin(), files.end(),
								   [&](std::string str) {
									return ExtractEventID(str) == ev;
								   }
		);
		if (iter_file == files.end()) continue;	// File not found.
		
		std::string file = *iter_file;
		std::cout << file << std::endl;
		
		TString cut = Form("s.eMCEvt==%d", event_id);
		dproc -> ReadTracksTree(*pvr, file.c_str(), cut);
		int ntrk = pvr -> Ntracks();

		for (int j=0; j<ntrk; j++) {
			EdbTrackP* track = pvr->GetTrack(j);
			for (int k=start; k<end; k++) {
				tracks[k];
				int track_id = tracks[k].seg_id;
				int plate_id = tracks[k].plate_id;
				if (IsTrack(track, event_id, plate_id, track_id)) {
					double mom = mc.CalcMomentum(track, 0);
					tracks[k].p_reco = mom;
				}
			}
		}
	}

	//for (Track track: v_trks) {
	//	std::cout << "1ry_trk" << "\t" << track.plate_id << "\t" << track.seg_id << "\t" << track.x_first << "\t" << track.y_first << "\t" << track.plate_id_last << "\t" << track.npl << "\t" << track.pdg_id << "\t" << track.p_true << "\t" << track.p_reco << "\t" << track.event_id << std::endl;
	//}
	//std::cout << std::endl;

	if (pvr->eTracks) pvr->eTracks->Clear();

	return;
}


// Old version.
/*
void CalcMomentum(std::string linked_tracks_file) {
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

	if (pvr->eTracks) pvr->eTracks->Clear();

	if (!IsFileValid(linked_tracks_file)) return;

	dproc -> ReadTracksTree(*pvr, linked_tracks_file.c_str(), cut);
	int ntrk = pvr -> Ntracks();
	
	Track track;
	track.event_id = std::stoi(event_id);
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
				std::cout << "Measured." << std::endl;
				tracks[i].plate_id_last = track -> GetSegmentLast() -> ScanID().GetPlate();
				tracks[i].npl = track -> Npl();
				//mc.DrawMomGraphCoord(track, c, "test");
			}
		}
	}


}
*/

void WriteVertexFile(std::string output_file) {
	std::cout << "Writing ..." << std::endl;
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

	std::cout << "Done." << std::endl;
	return;
	
}

void WriteVertexFileIntoRootFile(std::string output_file) {
	TFile* file = new TFile(output_file.c_str(), "RECREATE");
	TTree* tree_vertex = new TTree("vertex", "vertex");
	TTree* tree_track = new TTree("track", "track");

	TString type;
	int ivertex;
	// For verteces.
	int area_id, v_plate, ntrk;
	double vx, vy;

	// For tracks.
	int plate_id, seg_id, plate_id_last, npl, pdg_id, event_id;
	double x, y, p_true, p_rec;

	Vertex vertex;

	tree_vertex -> Branch("ivertex", &ivertex);
	tree_vertex -> Branch("area_id", &area_id);
	tree_vertex -> Branch("vx", &vx);
	tree_vertex -> Branch("vy", &vy);
	tree_vertex -> Branch("plate", &v_plate);
	tree_vertex -> Branch("ntrk", &ntrk);

	tree_track -> Branch("ivertex", &ivertex);
	tree_track -> Branch("event_id", &event_id);
	tree_track -> Branch("plate_id", &plate_id);
	tree_track -> Branch("seg_id", &seg_id);
	tree_track -> Branch("x", &x);
	tree_track -> Branch("y", &y);
	tree_track -> Branch("plate_id_last", &plate_id_last);
	tree_track -> Branch("npl", &npl);
	tree_track -> Branch("pdg_id", &pdg_id);
	tree_track -> Branch("p_true", &p_true);
	tree_track -> Branch("p_rec", &p_rec);

	std::sort(tracks.begin(), tracks.end(), compareIVertex<Track>);
	std::sort(verteces.begin(), verteces.end(), compareIVertex<Vertex>);

	for (int i=0; i<verteces.size(); i++) {
		vertex = verteces[i];
		ivertex = vertex.ivertex;
		area_id = vertex.area_id;
		vx = vertex.vx;
		vy = vertex.vy;
		v_plate = vertex.plate;
		ntrk = vertex.ntrk;
		
		tree_vertex -> Fill();

	  	Track trk_buf;
	  	trk_buf.ivertex = ivertex;
		std::vector<Track>::iterator iter_lower = std::lower_bound(tracks.begin(), tracks.end(), trk_buf, compareIVertex<Track>);
		std::vector<Track>::iterator iter_upper = std::upper_bound(tracks.begin(), tracks.end(), trk_buf, compareIVertex<Track>);
		int idx_lower = std::distance(tracks.begin(), iter_lower);
		int idx_upper = std::distance(tracks.begin(), iter_upper);

		for (int j=idx_lower; j<idx_upper; j++) {
			Track track = tracks[j];
			event_id = track.event_id;
			plate_id = track.plate_id;
			seg_id = track.seg_id;
			x = track.x_first;
			y = track.y_first;
			plate_id_last = track.plate_id_last;
			npl = track.npl;
			pdg_id = track.pdg_id;
			p_true = track.p_true;
			p_rec = track.p_reco;
			
			tree_track -> Fill();
		}
	}

	tree_vertex -> Write();
	tree_track -> Write();
	file -> Close();

	return;
}

/**
*	@fn			Run
*	@brief		運動量測定を実行する関数
*	@return void
*	@detail
*	はじめに運動量測定クラスの設定を行う。次にltlistsを1行ずつ読み、一つのlinked_tracks.rootファイルごとに運動量測定を行う。
*/
void Run(const char* par_file="../par/MC_plate_1_100.txt") {
	// Setup for momenum measurement.
	std::cout << "Read par file for momentum measurement." << std::endl;
	mc.ReadParFile(par_file);

	// Loop for the vertex.
	for (Vertex vertex: verteces) {
		int ivertex = vertex.ivertex;

	  	Track trk_buf;
	  	trk_buf.ivertex = ivertex;
		std::vector<Track>::iterator iter_lower = std::lower_bound(tracks.begin(), tracks.end(), trk_buf, compareIVertex<Track>);
		std::vector<Track>::iterator iter_upper = std::upper_bound(tracks.begin(), tracks.end(), trk_buf, compareIVertex<Track>);
		int idx_lower = std::distance(tracks.begin(), iter_lower);
		int idx_upper = std::distance(tracks.begin(), iter_upper);

		int event_id = tracks[idx_lower].event_id;
		FillMomentum(event_id, idx_lower, idx_upper);
	}

	return;
}

/**
*	@fn			Run
*	@brief		運動量測定を実行する関数
*	@param[in]	ltlists		linked_tracks.rootのパスが列挙されたファイルのパス
*	@param[in]	par_file	運動量測定の際のパラメータファイル
*	@return void
*	@detail
*	はじめに運動量測定クラスの設定を行う。次にltlistsを1行ずつ読み、一つのlinked_tracks.rootファイルごとに運動量測定を行う。
*/
// Old version.
/*
void Run(std::string ltlists, const char* par_file="../par/MC_plate_1_100.txt") {
	std::ifstream ifs(ltlists);

	if (ifs.fail()) {
		std::cerr << "Error! Could not open the file." << std::endl;
		exit(1);
	}

	// For the momentum measurement.
	mc.ReadParFile(par_file);

	std::string path;
	while(std::getline(ifs, path)) {
		CalcMomentum(path);
	}

	mc.ShowPar();

	std::cout << "Invalid files: " << std::endl;
	for (auto file: invalid_files) {
		std::cout << file << std::endl;
	}
}
*/

int main(int argc, char** argv) {
	char* input_vertex_file;
	char* input_list;
	char* output_vertex_file;
	char* par_file = nullptr;

	// Read arguments
	// -V: Path of input vertex file
	// -I: Path of list file of linked_tracks.root
	// -O: Path of output vertex file
	// -P: Path of parameter file for momentum measurement
	for (int i=1; i<argc; i+=2) {
		if (std::string(argv[i]) == "-V") input_vertex_file = argv[i+1];
		else if (std::string(argv[i]) == "-I") input_list = argv[i+1];
		else if (std::string(argv[i]) == "-O") output_vertex_file = argv[i+1];
		else if (std::string(argv[i]) == "-P") par_file = argv[i+1];
	}

	ReadVertexFile(input_vertex_file);
	ReadFilePath(input_list);

	dproc = new EdbDataProc;
	pvr = new EdbPVRec;

	Run(par_file);

	/*
	if (par_file == nullptr) {
		Run(input_list);
	} else {
		Run(input_list, par_file);
	}
	*/

	WriteVertexFile(output_vertex_file);
	
	return 0;
}
