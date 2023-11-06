/**
*	@file		meas_mom_after_reconnect.cpp
*	@brief		繋ぎ直した後に運動量を測定して出力する
*	@author		Motoya Nonaka
*	@date		3rd Nov 2023
*/

#include <iostream>

#include <TString.h>

#include <EdbDataSet.h>

// Global variables.
EdbDataProc* dproc;
EdbPVRec* pvr;

/**
*	@fn			PrintUsage
*	@brief		プログラムの使用方法を出力する
*	@return		void
*/
void PrintUsage() {
	std::cout << "Usage: " << std::endl;
	std::cout << "./meas_mom_after_reconnect -E <event ID> -T <track ID> -P <Plate number> -F <linked_tracks.root path>" << std::endl;

	return;
}

/**
*	@fn			IsTrack
*	@brief		
*	@param[in]	track
*	@param[in]	event_id
*	@param[in]	plate_id
*	@param[in]	seg_id
*	@return		bool
*/
bool IsTrack(EdbTrackP* track, int event_id, int plate_id, int seg_id) {
	int nseg = track -> N();
	for (int i=0; i<nseg; i++) {
		EdbSegP* seg = track -> GetSegment(i);
		if ((seg -> MCEvt()%100000)==event_id%100000 and seg -> ScanID().GetPlate() == plate_id and seg -> ID() == seg_id) return true;
	}

	return false;
}

/**
*	@fn			ReadTracks
*	@brief		linked_tracks.rootを読み込む
*	@params[in]	path
*	@params[in]	event_id
*	@par		Modify
*		- pvr
*	@return void
*/
void ReadTracks(TString path, int event_id) {
	TString cut = Form("s.eMCEvt==%d", event_id);
	dproc->ReadTracksTree(*pvr, path, cut);

	return;
}

void ConnectTracks(int event_id, int track_id, int plate_id) {

	EdbTrackP* target_track;
	
	int ntrk = pvr->Ntracks();
	for (int i=0; i<ntrk; i++) {
		EdbTrackP* track = pvr->GetTrack(i);
		if (IsTrack(track, event_id, plate_id, track_id)) {
			target_track = track;
			break;
		}
	}

	for (int i=0; i<ntrk; i++) {
		EdbTrackP* track = pvr->GetTrack(i);
		std::cout << track->ID() << "\t" << track->ScanID().GetPlate() << std::endl;
	}
	std::cout << "Target track ID: " << std::endl;
	std::cout << target_track->ID() << "\t" << target_track->ScanID().GetPlate() << std::endl;

	return;
}

int main(int argc, char** argv) {
	if (argc < 3) {
		std::cerr << "Arguments missing." << std::endl;
		PrintUsage();
		std::cerr << "Exit." << std::endl;
	}

	int event_id, track_id, plate_id;
	TString file_path;
	for (int i=1; i<argc; i+=2) {
		if ((std::string(argv[i]) == "-E")) event_id = atoi(argv[i+1]);
		else if ((std::string(argv[i]) == "-T")) track_id = atoi(argv[i+1]);
		else if ((std::string(argv[i]) == "-P")) plate_id = atoi(argv[i+1]);
		else if ((std::string(argv[i]) == "-F")) file_path = TString(argv[i+1]);
		else {
			std::cerr << "Invalid argument." << std::endl;
			PrintUsage();
			std::cerr << "Exit." << std::endl;
			exit(1);
		}
	}

	dproc = new EdbDataProc;
	pvr = new EdbPVRec;

	ReadTracks(file_path, event_id);
	ConnectTracks(event_id, track_id, plate_id);

	return 0;
}
