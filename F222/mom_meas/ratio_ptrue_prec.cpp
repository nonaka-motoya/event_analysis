#include <iostream>
#include <fstream>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

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


// To sort Track structure.
bool compareTrackId(const Track& track, const Track& target_track) {
	return track.event_id < target_track.event_id;
}

// Sort with ivertex.
template<typename T>
bool compareIVertex(const T& struct1, const T& struct2) {
	return struct1.ivertex < struct2.ivertex;
}

// Global variables.
std::vector<Track> tracks;
std::vector<Vertex> verteces;


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
			track.p_reco = p_reco;
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
	
	// Sort with ivertex.
	std::sort(tracks.begin(), tracks.end(), compareIVertex<Track>);
	std::sort(verteces.begin(), verteces.end(), compareIVertex<Vertex>);
}

void calc_ratio() {
	
	for (int i=0; i<verteces.size(); i++) {
		Vertex vertex = verteces[i];
		int ivertex = vertex.ivertex;

	  	Track trk_buf;
	  	trk_buf.ivertex = ivertex;
		std::vector<Track>::iterator iter_lower = std::lower_bound(tracks.begin(), tracks.end(), trk_buf, compareIVertex<Track>);
		std::vector<Track>::iterator iter_upper = std::upper_bound(tracks.begin(), tracks.end(), trk_buf, compareIVertex<Track>);
		int idx_lower = std::distance(tracks.begin(), iter_lower);
		int idx_upper = std::distance(tracks.begin(), iter_upper);
		
		std::cout << "Event ID: " << tracks[idx_lower].event_id << std::endl;
		for (int j=idx_lower; j<idx_upper; j++) {
			Track track = tracks[j];
			std::cout << "PGD: " << track.pdg_id << "\tP_true: " << track.p_true << "\tP_rec: " << track.p_reco << std::endl;
		}
		std::cout << "======================================================================" << std::endl;
	}
}

int main(int argc, char** argv) {
	ReadVertexFile("./output/vtx_test.txt");
	calc_ratio();
	return 0;
}
