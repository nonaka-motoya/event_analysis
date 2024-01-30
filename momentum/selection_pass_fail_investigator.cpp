/// @file selection_pass_fail_investigator.cpp
/// @brief Investigate events that pass or fail in the event selection for neutrino candidate.
/// @details At first, will check the events p < 100 GeV with 50plates but p > 200 GeV with 100 plates
/// @note note
/// @author Motoya Nonaka
/// @date 11th, Jan, 2024
#include <iostream>
#include <iterator>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_map>

/// @struct Track
/// @brief Structure for uniquely indentifying tracks.
/// @note Note
struct Track {
	int event_id;
	int track_id;
	int plate_id;
	double momentum;
	int npl;
	int p_true;
	int pdg_id;

	Track(int event_id, int track_id, int plate_id, double momentum, int npl) :event_id(event_id), track_id(track_id), plate_id(plate_id), momentum(momentum), npl(npl) {}

	void SetPTrue(int p_true) {
		this->p_true = p_true;
	}

	void SetPdgId(int pdg_id) {
		this->pdg_id = pdg_id;
	}

	bool operator==(const Track& rhs) const {
		return (event_id == rhs.event_id and track_id == rhs.track_id and plate_id == rhs.plate_id);
	}

	bool operator<(const Track& rhs) const {
		return event_id < rhs.event_id;
	}

	void Print() {
		std::cout << "Event ID: " << event_id << "\tTrack ID: " << track_id << "\tPDG ID: " << pdg_id << "\tPlate ID: " << plate_id << "\tNpl: " << npl << "\tMomentum: " << momentum << "\tP_true: " << p_true << std::endl;
	}
};

// Global variables
std::vector<Track> tracks_over200_with_100plates; // Track whose momentum is smaller than 100 GeV with 50 plates.
std::unordered_map<int, int> event_count_map;


/// @fn PrintUsage
/// @brief Print usage of this code
/// @return void
/// @note Note
void PrintUsage() {
	std::cerr << "Usage: " << std::endl; std::cerr << "./selection_pass_fail_investigator -S <Vertex file with 50 plates> -L <Vertex file with 100 plates>" << std::endl;
	return;
}

/// @fn sortTracks
/// @brief Sort tracks with event ID.
/// @return void
/// @Refer
/// 	- tracks_over200_with_100plates
/// @note Note
void sortTracks() {
	std::sort(tracks_over200_with_100plates.begin(), tracks_over200_with_100plates.end());
}

/// @fn pushTracksOver200With100plates
/// @brief Search tracks whose momentum is larger than 200 GeV with 100 plates.
/// @param filename Filename of the vertex file.
/// @return void
/// @Refer
/// 	- tracks_over200_with_100plates
/// @noteNote
void pushTracksOver200With100plates(std::string filename) {
	
	std::ifstream inputFile(filename);

	if (!inputFile) {
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
			event_count_map[event_id]++;
			if (p_reco > 200) {
				Track t(event_id, seg_id_first, plate_id_first, p_reco, npl);
				t.SetPdgId(pdg_id);
				t.SetPTrue(p_true);
				tracks_over200_with_100plates.push_back(t);
			}
		}
	}

	inputFile.close();

	// Sort tracks_over200_with_100plates with event_id.
	sortTracks();

	return;
}

void SearchTrackFailureMomentumSelection(std::string vtx_file_50plates) {
	
	std::ifstream inputFile(vtx_file_50plates);

	if (!inputFile) {
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
			if (p_reco < 100) {
				Track t(event_id, seg_id_first, plate_id_first, p_reco, npl);
				t.SetPdgId(pdg_id);
				t.SetPTrue(p_true);
				// Binary search
				auto iter = std::lower_bound(tracks_over200_with_100plates.begin(), tracks_over200_with_100plates.end(), t);
				if (*iter == t) {
					std::cout << "====================" << std::endl;
					std::cout << "50 plates:\t";
					t.Print();
					std::cout << "100 plates:\t";
					iter->Print();
				}
			}
		}
	}
}

/// @fn printTracks
/// @brief Print all tracks info
/// @return void
/// @noteNote
void printTracks() {
	for (auto track: tracks_over200_with_100plates) {
		track.Print();
	}
}

/// @fn PrintNumberOfEvents
/// @brief Print number of events
/// @return void
/// @refer
/// 	- event_count_map
/// @note Note
void PrintNumberOfEvents() {
	std::cout << std::endl;
	std::cout << "Number of all events: " << event_count_map.size() << std::endl;
	return;
}


int main(int argc, char** argv) {
	if (argc < 2) {
		std::cerr << "Error: Argument missing!" << std::endl;
		PrintUsage();
		exit(1);
	}

	std::string vtx_file_50plates;
	std::string vtx_file_100plates;
	for (int i=0; i<argc; i++) {
		std::string arg = argv[i];
		if (arg[0] == '-' and i+1 < argc) {
			if (arg == "-S") {
				vtx_file_50plates = argv[i+1];
				i++;
			} else if (arg == "-L") {
				vtx_file_100plates = argv[i+1];
				i++;
			} else {
				std::cerr << "Error: Invalid arugment!" << std::endl;
				PrintUsage();
				exit(1);
			}
		}
	}

	pushTracksOver200With100plates(vtx_file_100plates);
	SearchTrackFailureMomentumSelection(vtx_file_50plates);
	PrintNumberOfEvents();
	
	return 0;

}
