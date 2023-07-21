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
	
	int num_miss_numu_event = 0;
	int num_misidentify_numu_event = 0;
	int num_miss_muon = 0;
	int num_over_200 = 0;
	int num_under_200 = 0;
	int num_mu_over_200 = 0;
	int A_A = 0;
	int B_B = 0;

	bool is_p_true_over_200;
	bool is_p_rec_over_200;

	bool is_mu_p_true_over_200;
	bool is_mu_p_rec_over_200;

	for (int i=0; i<verteces.size(); i++) {
		Vertex vertex = verteces[i];
		int ivertex = vertex.ivertex;

	  	Track trk_buf;
	  	trk_buf.ivertex = ivertex;
		std::vector<Track>::iterator iter_lower = std::lower_bound(tracks.begin(), tracks.end(), trk_buf, compareIVertex<Track>);
		std::vector<Track>::iterator iter_upper = std::upper_bound(tracks.begin(), tracks.end(), trk_buf, compareIVertex<Track>);
		int idx_lower = std::distance(tracks.begin(), iter_lower);
		int idx_upper = std::distance(tracks.begin(), iter_upper);

		is_p_true_over_200 = false;
		is_p_rec_over_200 = false;
		is_mu_p_true_over_200 = false;
		is_mu_p_rec_over_200 = false;
		
		ofs << "Event ID: " << tracks[idx_lower].event_id << std::endl;
		for (int j=idx_lower; j<idx_upper; j++) {
			Track track = tracks[j];
			ofs << "PGD: " << track.pdg_id << "\tNpl: " << track.npl << "\tP_true: " << track.p_true << "\tP_rec: " << track.p_reco << std::endl;


			if (track.npl < 10) {
				ofs << "Diverge!" << std::endl;
				continue;
			}
			if (track.p_true > 200) is_p_true_over_200 = true;
			if (track.p_reco > 200) is_p_rec_over_200 = true;
			if (abs(track.pdg_id) == 13 and track.p_true > 200) is_mu_p_true_over_200 = true;
			if (abs(track.pdg_id) == 13 and track.p_reco > 200) is_mu_p_rec_over_200 = true;
		}

		if (is_p_true_over_200) {
			num_over_200 ++;
		} else {
			num_under_200 ++;
		}

		if (is_mu_p_true_over_200) {
			num_mu_over_200 ++;
		}

		if (is_p_true_over_200 and !is_p_rec_over_200) {
			num_miss_numu_event++;
			ofs << "P_true > 200 but P_reco < 200." << std::endl;;
		}

		if (is_p_true_over_200 and is_p_rec_over_200) {
			A_A ++;
		}

		if (!is_p_true_over_200 and !is_p_rec_over_200) {
			B_B++;
		}

		if (!is_p_true_over_200 and is_p_rec_over_200) {
			ofs << "P_true < 200 but P_reco > 200." << std::endl;;
			num_misidentify_numu_event++;
		}

		if (is_mu_p_true_over_200 and !is_mu_p_rec_over_200) {
			ofs << "Muon P_true > 200 but P_reco < 200." << std::endl;;
			num_miss_muon ++;
		}
		ofs << "======================================================================" << std::endl;
	}

	ofs << std::endl << std::endl;

	ofs << "少なくとも1本p_true>200GeVのトラックいるのにp_rec>200GeVは1本もいないeventの割合: " << num_miss_numu_event << "/" << num_over_200 << "= " << (double)num_miss_numu_event/num_over_200 << std::endl;
	ofs << "すべてのトラックがp_true<200GeVなのにp_rec>200GeVのトラックがいる割合: " << num_misidentify_numu_event << "/" << num_under_200 << "= " << (double)num_misidentify_numu_event/num_under_200 << std::endl;
	ofs << "p_true>200GeVのmuonがいるのにp_rec<200GeVになる割合 : " << num_miss_muon << "/" << num_mu_over_200 << "= " << (double)num_miss_muon/num_mu_over_200 << std::endl;

	ofs << std::endl << std::endl;

	ofs << "Number of events: " << nvertex << std::endl;
	ofs << "少なくとも1本p_true>200GeVのトラックがいるevent数: " << num_over_200 << std::endl;
	ofs << "すべてのトラックがp_true<200GeVのevent数: " << num_under_200 << std::endl;
	ofs << "p_true>200GeVのmuonがいるevent数: " << num_mu_over_200 << std::endl;

	ofs << std::endl << std::endl;

	ofs << "| | P_true>200 | P_true<200 |" << std::endl;
	ofs << "| --- | --- | --- |" << std::endl;
	ofs << "| P_rec>200 | " << (double)A_A/num_over_200 << " | " << (double)num_misidentify_numu_event/num_under_200 << " |" << std::endl;
	ofs << "| P_rec<200 | " << (double)num_miss_numu_event/num_over_200 << " | " << (double)B_B/num_under_200 << " |" << std::endl;
}

int main(int argc, char** argv) {
	ReadVertexFile("./output/vtx_info_nuall_00010-00039_p500_numucc_v20230706_reconnected_measured_mometum_100plates.txt");
	CalcRatio("./output/p_true_vs_p_rec_100plates_reconnected.txt");
	return 0;
}
