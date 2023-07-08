#include "TruthManager.hpp"

#include <string>
#include <fstream>
#include <vector>
#include <map>

#include <EdbDataSet.h>

TruthManager::TruthManager() : is_read_hadron_(false) {};

// ----------------------------------------------------

TruthManager::TruthManager(std::string input_file_path) {

  is_read_hadron_ = false;

  std::ifstream ifs;
	ifs.open(input_file_path);

	if (!ifs) {
		std::cerr << "Invalid file! Check the input file." << std::endl;
		exit(1);
	}

	std::string line_buf;
	while (std::getline(ifs, line_buf)) {
		ReadTruthFile(line_buf);
	}
};

// ----------------------------------------------------

void TruthManager::ReadTruthFile(std::string path) {

	std::cout << "Read " << path << std::endl;
	if (is_read_hadron_) std::cout << "2ry particles are also read." << std::endl;

	TFile* file = new TFile(path.c_str(), "READ");
	TTree* tree = (TTree*) file -> Get("m_NuMCTruth_tree");

	int event_id, pdg_id;
	float vz_decay;
	std::vector<int> *trackid_out_particle = 0;
	std::vector<int> *pdg_out_particle= 0;
	std::vector<int> *pdg_in_particle= 0;

	tree -> SetBranchAddress("m_event_id_MC", &event_id);
	tree -> SetBranchAddress("m_pdg_id", &pdg_id);
	tree -> SetBranchAddress("m_trackid_out_particle", &trackid_out_particle);
	tree -> SetBranchAddress("m_pdg_out_particle", &pdg_out_particle);
	tree -> SetBranchAddress("m_vz_decay", &vz_decay);
	tree -> SetBranchAddress("m_pdg_in_particle", &pdg_in_particle);

	// To adopt FEDRA format.
	int added_MCEvt = 100000;

	for (int i=0; i<tree->GetEntries(); i++) {
		tree -> GetEntry(i);

		std::vector<int> trackid_buf;
		// select 1ry particle and 2ry particle.
	    bool is_2ry = std::find(pdg_in_particle->begin(), pdg_in_particle->end(), 14) != pdg_in_particle->end() or std::find(pdg_in_particle->begin(), pdg_in_particle->end(), -14) != pdg_in_particle->end(); // check whether parent particle is numu.
		if (abs(pdg_id) == 14 or (is_read_hadron_ and is_2ry)) {
			// fill daughter particle.
			for (int j=0; j<trackid_out_particle->size(); j++) {
				trackid_buf.push_back(trackid_out_particle->at(j));
			}

			if (uniqueID_.find(event_id+added_MCEvt) == uniqueID_.end()) {
				uniqueID_[event_id+added_MCEvt] = trackid_buf;
			} else {
				uniqueID_[event_id+added_MCEvt].insert(uniqueID_[event_id+added_MCEvt].end(), trackid_buf.begin(), trackid_buf.end());
			}
		}


	}

	std::cout << event_id << "\t" << uniqueID_.end()->second.size() << std::endl;
	std::cout << uniqueID_.size() << " events are read." << std::endl;
}


// ----------------------------------------------------

bool TruthManager::IsTrack(EdbTrackP* track) {
	int event_id = track -> GetSegmentFirst() -> MCEvt();
	int track_id = track -> GetSegmentFirst() -> Volume();
	double z = track -> GetSegmentFirst() -> Z();

	//std::cout << "event id: " << event_id << "\ttrack id: " << track_id << std::endl;

	auto iter = uniqueID_.find(event_id);

	if (iter == uniqueID_.end()) {
		return false;
	} else {
		std::vector<int> v_trackid = iter -> second;
		if (std::find(v_trackid.begin(), v_trackid.end(), track_id) == v_trackid.end()) return false;
	}

	std::cout << "matching event ID, track ID: " << event_id << ", " << track_id << std::endl;
	return true;
}

// ----------------------------------------------------

bool TruthManager::IsNumu(int event_id) {

  auto iter = uniqueID_.find(event_id);

  if (iter == uniqueID_.end()) return false;

  return true;

}

// ----------------------------------------------------

void TruthManager::PrintUniqueID() {
	for (auto iter : uniqueID_) {
		std::cout << "Event ID:" << iter.first << "\tTrack ID:";
		for (auto track : iter.second) {
			std::cout << " " << track;
		}
		std::cout << std::endl;
	}

	return;
}

// ----------------------------------------------------

