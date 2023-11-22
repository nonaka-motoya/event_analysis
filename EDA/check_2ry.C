/**
*	@file		check_2ry.C
*	@brief		hadronに2ry hadronがいないかをチェックするマクロ
*	@author		Motoya Nonaka
*	@date		12th Oct 2023
*	@note		実行方法: root -l check_2ry <dir name> <event_id> <track_id>
*/


/**
*	@fn
*	@brief		trackの2ry trackを探してEDA displayで表示する
*	@param[in]	filePath	linked_tracks.rootのパス
*	@param[in]	event_id	Event ID
*	@param[in]	track_id	Track ID
*	@return		bool	Displayで表示するトラックがいればtrueを返す
*/
bool ReadLinkedTracks(std::string filePath, int event_id, int track_id) {
	bool isTrack = false;

	std::cout << filePath << std::endl;
	char* file = new char[filePath.size()+1];
	std::strcpy(file, filePath.c_str());
				
	EdbEDA* eda = new EdbEDA(file, 100, "1", kFALSE);

	EdbEDATrackSet* ts = eda -> GetTrackSet("TS");
	int nbase = ts -> NBase();
	ts -> ClearTracks();

	EdbTrackP* primary_track;
	for (int i=0; i<nbase; i++) {
		EdbTrackP* track = (EdbTrackP*) ts -> GetTrackBase(i);
		if (track -> GetSegmentFirst() -> MCEvt()%100000==event_id%100000 and track -> GetSegmentFirst() -> ID() == track_id) {
			isTrack = true;
			ts -> AddTrack(track);
			primary_track = track;
		}
	}

	if (isTrack) {
		EdbSegP* last_seg = primary_track -> GetSegmentLast();
		for (int i=0; i<nbase; i++) {
			EdbTrackP* track = (EdbTrackP*) ts -> GetTrackBase(i);
			if (track == primary_track) continue;
			EdbSegP* first_seg = track -> GetSegmentFirst();
			if ((first_seg->PID() - last_seg->PID())<5 and (first_seg->PID() - last_seg->PID())>=0 and EdbEDAUtil::CalcDmin(first_seg, last_seg) < 20) {
				ts -> AddTrack(track);
			}
		}
	}

	if (isTrack) {
		eda->Run();
		return true;
	}

	delete eda;
	std::cout << "Not fount track." << std::endl;
	return false;
}


/**
 *	@fn
 *	@brief		event_idとtrack_idが一致するイベントの入ったlinked_tracks.rootを探す
 *	@param[in]	directory	イベントを探すディレクトリ
 *	@param[in]	event_id	Event ID
 *	@param[in]	trac_id		Event ID
 *	@return		void
 */
void ReadFiles(char* directory, int event_id, int track_id) {
	TSystemDirectory dir(directory, directory);
	TList* fileList = dir.GetListOfFiles();

	if (!fileList) {
		std::cerr << "Error: Unable to retrieve file list from directory " << directory << std::endl;
        return;
	}

	std::regex pattern("evt_(\\d+)");
	TIter fileIter(fileList);
	while(TObject* obj = fileIter.Next()) {
		TSystemFile* file = (TSystemFile*) obj;
		TString fileName = file -> GetName();

		std::string fileNameStr = fileName.Data();
		std::smatch matches;

		if (std::regex_search(fileNameStr, matches, pattern)) {
			std::string numberStr = matches[1].str();
			int eventIdFromFileName = std::stoi(numberStr);

			if (eventIdFromFileName%100000 == event_id%100000) {

				std::string filePath = std::string(directory);
				filePath += fileNameStr;
				filePath += "/linked_tracks.root";

				if (ReadLinkedTracks(filePath, event_id, track_id)) {
					return;
				}
			}
		}
	}
}

/**
*	@fn
*	@brief		main関数
*/
void check_2ry(char* directory, int event_id, int track_id) {
	ReadFiles(directory, event_id, track_id);
}