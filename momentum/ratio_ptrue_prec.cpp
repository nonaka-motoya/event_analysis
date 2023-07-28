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
int nvertex;


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
	nvertex = 0;


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
			nvertex ++;
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

	std::cout << nvertex << " verteces are read." << std::endl;
}

void CalcRatio(std::string output_file = "./output/p_true_vs_p_rec.txt") {

	std::ofstream ofs(output_file);
	
	// To calculate the ratio.
	int num_p_true_over = 0;
	int num_p_true_under = 0;

	int num_p_true_over_p_rec_over = 0;
	int num_p_true_over_p_rec_under = 0;
	int num_p_true_under_p_rec_over = 0;
	int num_p_true_under_p_rec_under = 0;

	// For muon.
	int num_mu_p_true_over = 0;
	int num_mu_p_true_under = 0;

	int num_mu_p_true_over_p_rec_over = 0;
	int num_mu_p_true_over_p_rec_under = 0;
	int num_mu_p_true_under_p_rec_over = 0;
	int num_mu_p_true_under_p_rec_under = 0;

	// To check the event;
	bool is_p_true_over_200;
	bool is_p_rec_over_200;
	bool is_mu_p_true_over_200;
	bool is_mu_p_rec_over_200;

	bool is_this_event_valid; // Skip the event if it includes a track whose p=-999

	for (int i=0; i<verteces.size(); i++) {
		is_this_event_valid = true;
		
		Vertex vertex = verteces[i];
		int ivertex = vertex.ivertex;

	  	Track trk_buf;
	  	trk_buf.ivertex = ivertex;
		std::vector<Track>::iterator iter_lower = std::lower_bound(tracks.begin(), tracks.end(), trk_buf, compareIVertex<Track>);
		std::vector<Track>::iterator iter_upper = std::upper_bound(tracks.begin(), tracks.end(), trk_buf, compareIVertex<Track>);
		int idx_lower = std::distance(tracks.begin(), iter_lower);
		int idx_upper = std::distance(tracks.begin(), iter_upper);

		// Initialize the flags.
		is_p_true_over_200 = false;
		is_p_rec_over_200 = false;
		is_mu_p_true_over_200 = false;
		is_mu_p_rec_over_200 = false;
		
		ofs << "Event ID: " << tracks[idx_lower].event_id << std::endl;
		for (int j=idx_lower; j<idx_upper; j++) {
			Track track = tracks[j];
			ofs << "PGD: " << track.pdg_id << "\tNpl: " << track.npl << "\tP_true: " << track.p_true << "\tP_rec: " << track.p_reco << std::endl;

			if (track.p_reco == -999) is_this_event_valid = false;

			if (track.p_true > 200) is_p_true_over_200 = true;
			if (track.p_reco > 200) is_p_rec_over_200 = true;

			if (abs(track.pdg_id) == 13) {
				if (track.p_true > 200) is_mu_p_true_over_200 = true;
				if (track.p_reco > 200) is_mu_p_rec_over_200 = true;
			}
		}

		if (!is_this_event_valid) continue; // Skip this event if it includes a track whose p=-999

		// Check if p_true > 200.
		if (is_p_true_over_200) {
			num_p_true_over++;
			// Check if p_rec > 200.
			if (is_p_rec_over_200) {
				num_p_true_over_p_rec_over ++;
			} else {
				num_p_true_over_p_rec_under ++;
			}
		} else {
			num_p_true_under++;
			// Check if p_rec > 200.
			if (is_p_rec_over_200) {
				num_p_true_under_p_rec_over ++;
			} else {
				num_p_true_under_p_rec_under ++;
			}
		}

		// Check if muon's p_true > 200.
		if (is_mu_p_true_over_200) {
			num_mu_p_true_over ++;
			// Check if muon's p_rec > 200.
			if (is_mu_p_rec_over_200) {
				num_mu_p_true_over_p_rec_over ++;
			} else {
				num_mu_p_true_over_p_rec_under ++;
			}
		} else {
			num_mu_p_true_under ++;
			// Check if muon's p_rec > 200.
			if (is_mu_p_rec_over_200) {
				num_mu_p_true_under_p_rec_over ++;
			} else {
				num_mu_p_true_under_p_rec_under ++;
			}
		}

		ofs << "======================================================================" << std::endl;
	}

	ofs << std::endl << std::endl;


	ofs << std::endl << std::endl;

	ofs << "Number of events: " << nvertex << std::endl;
	ofs << "少なくとも1本p_true>200GeVのトラックがいるevent数: " << num_p_true_over << std::endl;
	ofs << "すべてのトラックがp_true<200GeVのevent数: " << num_p_true_under << std::endl;
	//ofs << "p_true>200GeVのmuonがいるevent数: " << num_mu_over_200 << std::endl;

	ofs << std::endl << std::endl;

	ofs << "| | P_rec > 200 | P_rec < 200 |" << std::endl;
	ofs << "| --- | --- | --- |" << std::endl;
	ofs << "| P_true > 200 | " << num_p_true_over_p_rec_over << "/" << num_p_true_over << "=" << (double) num_p_true_over_p_rec_over/num_p_true_over << " | " << num_p_true_over_p_rec_under << "/" <<  num_p_true_over << "=" << (double) num_p_true_over_p_rec_under/num_p_true_over << " |" << std::endl;
	ofs << "| P_true < 200 | " << num_p_true_under_p_rec_over << "/" << num_p_true_under << "=" << (double) num_p_true_under_p_rec_over/num_p_true_under << " | " << num_p_true_under_p_rec_under << "/" << num_p_true_under << "=" << (double) num_p_true_under_p_rec_under/num_p_true_under << " |" << std::endl;

	ofs << std::endl << std::endl;
	ofs << "Only muon tracks." << std::endl;
	ofs << "| | P_rec > 200 | P_rec < 200 |" << std::endl;
	ofs << "| --- | --- | --- |" << std::endl;
	ofs << "| P_true > 200 | " << num_mu_p_true_over_p_rec_over << "/" << num_mu_p_true_over << "=" << (double) num_mu_p_true_over_p_rec_over/num_mu_p_true_over << " | " << num_mu_p_true_over_p_rec_under << "/" << num_mu_p_true_over << "=" << (double) num_mu_p_true_over_p_rec_under/num_mu_p_true_over << " |" << std::endl;
	ofs << "| P_true < 200 | " << num_mu_p_true_under_p_rec_over << "/" << num_mu_p_true_under << "=" << (double) num_mu_p_true_under_p_rec_over/num_mu_p_true_under << " | " << num_mu_p_true_under_p_rec_under << "/" << num_mu_p_true_under << "=" << (double) num_mu_p_true_under_p_rec_under/num_mu_p_true_under << " |" << std::endl;
}

int main(int argc, char** argv) {

	char* input_vertex_file;
	char* output_file;

	for (int i=1; i<argc; i+=2) {
		if (std::string(argv[i]) == "-V") input_vertex_file = argv[i+1];
		else if (std::string(argv[i]) == "-O") output_file = argv[i+1];
	}

	ReadVertexFile(input_vertex_file);
	CalcRatio(output_file);

	return 0;
}
