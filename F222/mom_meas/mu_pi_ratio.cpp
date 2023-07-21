#include "Rtypes.h"
#include <iostream>
#include <string>
#include <fstream>
#include <regex>

#include <TH1.h>
#include <TString.h>
#include <TRint.h>
#include <TLegend.h>
#include <TStyle.h>

#include <EdbDataSet.h>

#include "Utils.hpp"


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

TH1D* mu_hist;
TH1D* pi_hist;
TH1D* mu_hist_after;
TH1D* pi_hist_after;

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

bool is_mu(EdbTrackP* track) {
	EdbSegP* first_seg = track -> GetSegmentFirst();
	if (abs(track -> Track()) == 13) return true;
	return false;
}

bool is_pi(EdbTrackP* track) {
	EdbSegP* first_seg = track -> GetSegmentFirst();
	if (abs(track -> Track()) == 211) return true;
	return false;
}

void fill_hist(std::string path) {
	std::string pattern = "evt_(\\d+)_";
	std::regex regex(pattern);
	std::smatch match;
	std::string event_id;

	if (std::regex_search(path, match, regex) && match.size() > 1) {
		event_id = match.str(1);
	} else {
		std::cerr << "Error: Couldn't extract Track ID from the filename." << std::endl;
		exit(1);
	}

	TString cut = Form("(s.eMCEvt%%100000)==%d", std::stoi(event_id));
	dproc -> ReadTracksTree(*pvr, path.c_str(), cut);

	int ntrk = pvr -> Ntracks();
	for (int i=0; i<pvr->Ntracks(); i++) {
		EdbTrackP* track = pvr -> GetTrack(i);
		
		if (is_mu(track)) {
			mu_hist -> Fill(track->Npl());
		} else if (is_pi(track)) {
			pi_hist -> Fill(track->Npl());
		}
	}
	
	
	pvr -> Clear();
}

void make_hist() {
	// Sort tracks and verteces with ivertex;
	std::sort(tracks.begin(), tracks.end(), compareIVertex<Track>);
	std::sort(verteces.begin(), verteces.end(), compareIVertex<Vertex>);


	for (auto vertex: verteces) {
		Track trk_buf;
		trk_buf.ivertex = vertex.ivertex;
		
		// binary search
		std::vector<Track>::iterator iter_lower = std::lower_bound(tracks.begin(), tracks.end(), trk_buf, compareIVertex<Track>);
		std::vector<Track>::iterator iter_upper = std::upper_bound(tracks.begin(), tracks.end(), trk_buf, compareIVertex<Track>);
		int idx_lower = std::distance(tracks.begin(), iter_lower);
		int idx_upper = std::distance(tracks.begin(), iter_upper);

		for (int i=idx_lower; i<idx_upper; i++) {
			Track track = tracks[i];

			if (abs(track.pdg_id) == 13) {
				mu_hist -> Fill(track.npl);
				if (track.npl < 70) std::cout << "Event ID: " << track.event_id << "\tNpl: " << track.npl << std::endl;
			} else if(abs(track.pdg_id) == 211) {
				pi_hist -> Fill(track.npl);
			}
		}
	}

	return;
}

void clear_events() {
	verteces.clear();
	tracks.clear();
	mu_hist -> Clear();
	pi_hist -> Clear();
}

int main(int argc, char** argv) {
	
	std::ifstream ifs("./input_files/LTList.txt.debug");
	if (ifs.fail()) {
		std::cerr << "Error! Could not open the file." << std::endl;
		exit(1);
	}

	mu_hist = new TH1D("mu hist", ";plates;counts", 770, 0, 770);
	pi_hist = new TH1D("pi hist", ";plates;counts", 770, 0, 770);
	dproc = new EdbDataProc;
	pvr = new EdbPVRec;
	TRint app("app", 0, 0);

	ReadVertexFile("./output/vtx_info_nuall_00010-00039_p500_numucc_v20230706_measured_mometum_100plates.txt");
	make_hist();
	clear_events();

	TH1* cum_mu_hist = mu_hist -> GetCumulative(kFALSE);
	TH1* cum_pi_hist = pi_hist -> GetCumulative(kFALSE);

	int number_of_muon = mu_hist -> GetEntries();
	int number_of_pion = pi_hist -> GetEntries();
	cum_mu_hist -> Scale(1./number_of_muon);
	cum_pi_hist -> Scale(1./number_of_pion);

	ReadVertexFile("./output/vtx_info_nuall_00010-00039_p500_numucc_v20230706_reconnected_measured_mometum_100plates.txt");
	make_hist();
	TH1* cum_mu_hist_after = mu_hist -> GetCumulative(kFALSE);
	TH1* cum_pi_hist_after = pi_hist -> GetCumulative(kFALSE);

	number_of_muon = mu_hist -> GetEntries();
	number_of_pion = pi_hist -> GetEntries();
	cum_mu_hist_after -> Scale(1./number_of_muon);
	cum_pi_hist_after -> Scale(1./number_of_pion);

	cum_mu_hist -> GetXaxis() -> SetRangeUser(0, 150);


	TLegend* legend = new TLegend(0.7, 0.7, 0.9, 0.9);
	legend -> AddEntry(cum_mu_hist, "Muon (before)");
	legend -> AddEntry(cum_mu_hist_after, "Muon (after)");
	legend -> AddEntry(cum_pi_hist, "Pion (before)");
	legend -> AddEntry(cum_pi_hist_after, "Pion (after)");


	cum_mu_hist -> SetLineStyle(3);
	cum_mu_hist -> Draw();
	cum_pi_hist -> SetLineColor(kRed);
	cum_pi_hist -> SetLineStyle(3);
	cum_pi_hist -> Draw("SAME");
	cum_mu_hist_after -> Draw("SAME");
	cum_pi_hist_after -> SetLineColor(kRed);
	cum_pi_hist_after -> Draw("SAME");

	legend -> Draw();

	gStyle -> SetOptStat(0);

	app.Run();
	return 0;
}
