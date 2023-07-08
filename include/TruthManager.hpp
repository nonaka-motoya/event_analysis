#ifndef TRUTHMANAGER_H_
#define TRUTHMANAGER_H_

#include <map>

#include <EdbDataSet.h>

class TruthManager {
  private:
    std::map<int, std::vector<int>> uniqueID_; // <MCEvt, vector<MC track ID>>
	bool is_read_hadron_;

  public:
    TruthManager();
    TruthManager(std::string input_file_path);

	void SetReadHadron() { is_read_hadron_ = true; }
    void ReadTruthFile(std::string path);

    bool IsTrack(EdbTrackP* track);

    bool IsNumu(int event_id);

	void PrintUniqueID();
};

#endif
