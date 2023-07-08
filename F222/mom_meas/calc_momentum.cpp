#include <cerrno>
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

#include <EdbDataSet.h>


// To specify track uniquely
struct Event {
	int event_id;
	int plate_id;
	int seg_id;
};

// Global variables.
std::vector<Event> events;

// To sort Event structure.
bool compareEventId(const Event& event, const Event& target_event_id) {
	return event.event_id < target_event_id.event_id;
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

	std::string type_name;
	int plate_id_first, seg_id_first, plate_id_last, npl, pdg_id, event_id;
	float x_first, y_first, p_true, p_reco;


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

			Event event;
			event.event_id = event_id;
			event.plate_id = plate_id_first;
			event.seg_id = seg_id_first;
			events.push_back(event);
		} else {
			continue;
		}
	}

	// Sort for binary search.
	std::sort(events.begin(), events.end(), compareEventId);
}


void ReadLinkedTracks(std::string linked_tracks_file) {
	// Extrach event ID.
	std::string pattern = "evt_(\\d+)_pl";
	std::regex regex(pattern);
	std::smatch match;
	std::string event_id;

	if (std::regex_search(linked_tracks_file, match, regex) && match.size() > 1) {
		event_id = match.str(1);
	} else {
		std::cerr << "Error: Couldn't extract event ID from the filename." << std::endl;
	}

	TString cut = Form("s.eMCEvt==%d", std::stoi(event_id)+100000);

	EdbDataProc* dproc = new EdbDataProc;
	EdbPVRec* pvr = new EdbPVRec;

	dproc -> ReadTracksTree(*pvr, linked_tracks_file.c_str(), cut);
	int ntrk = pvr -> Ntracks();
	
	Event event;
	event.event_id = std::stoi(event_id);
	std::vector<Event>::iterator iter_lower = std::lower_bound(events.begin(), events.end(), event, compareEventId);
	std::vector<Event>::iterator iter_upper = std::upper_bound(events.begin(), events.end(), event, compareEventId);
	int idx_lower = std::distance(events.begin(), iter_lower);
	int idx_upper = std::distance(events.begin(), iter_upper);


	for (int i=idx_lower; i<idx_upper; i++) {
		int eve = events[i].event_id;
		int track_id = events[i].seg_id;
		int plate_id = events[i].plate_id;

		for (int itrk=0; itrk<ntrk; itrk++) {
			EdbTrackP* track = pvr -> GetTrack(itrk);
			if (track -> GetSegmentFirst() -> ID() == track_id and track -> GetSegmentFirst() -> ScanID().GetPlate() == plate_id) {
				std::cout << track -> GetSegmentFirst() -> ID() << std::endl;
			}
		}
	}

}

int main(int argc, char** argv) {
	ReadVertexFile("./input_files/vtx_info_nuall_00010-00039_p500_numucc_v20230706.txt");

	ReadLinkedTracks("/data/FASER/fasernu-pilot-run/MDC_rec_tmp/20230402_nuall_00010-00019_p300/evt_21581_pl107_527/linked_tracks.root");

	return 0;
}
