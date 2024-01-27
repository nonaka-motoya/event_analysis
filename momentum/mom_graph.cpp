/// @file mom_graph.cpp
/// @brief Plot some graphs of the momentum measurement
/// @author Motoya Nonaka
/// @date 2024/01/26

#include <iostream>
#include <string>
#include <regex>

#include <TCanvas.h>
#include <TRint.h>
#include <TSystemDirectory.h>
#include <TList.h>

#include <EdbDataSet.h>

#include "FnuMomCoord.hpp"

// Global variables
EdbDataProc* dproc;
EdbPVRec* pvr;
TCanvas* c;
FnuMomCoord mc;

/// @fn PrintUsage()
/// @brief Print the usage of this program
/// @return void
void PrintUsage() {
    std::cout << "Usage: mom_graph [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -D, -dir <filename>    Set the Directory name you will see" << std::endl;
    std::cout << "  -P, -parfile <filename> Set the par file name" << std::endl;
    std::cout << "  -event <event_id>       Set the event ID" << std::endl;
    std::cout << "  -trid <track_id>        Set the track ID" << std::endl;
    std::cout << "  -plate <plate>          Set the plate number" << std::endl;
    return;
}

/// @fn DrawMomGraph(std::string filename, int event_id, int track_id, int plate)
/// @brief Draw the momentum graph  
/// @param[in] filename The name of the linked_tracks file
/// @param[in] event_id The event ID
/// @param[in] track_id The track ID
/// @param[in] plate The plate number
/// @return void
bool DrawMomGraph(std::string filename, int event_id, int track_id, int plate) {
    bool found_track = false;

    std::cout << "Reading " << filename << std::endl;
    try {
        dproc->ReadTracksTree(*pvr, filename.c_str(), "");
    } catch (...) {
        throw std::runtime_error("Cannot read the linked_tracks.root");
    }
    std::cout << "Searching the track with event_id = " << event_id << ", track_id = " << track_id << ", plate = " << plate << std::endl;
    
    int ntrk = pvr->Ntracks();
    for (int i=0; i<ntrk; i++) {
        EdbTrackP* track = pvr->GetTrack(i);
        int nseg = track->N();
        for (int j=0; j<nseg; j++) {
            EdbSegP* seg = track->GetSegment(j);
            if (seg->MCEvt()%100000 == event_id%100000 and seg->ID() == track_id and seg->ScanID().GetPlate() == plate) {
                found_track = true;
                break;
            }
        }
        if (found_track) {
            mc.CalcMomentum(track);
            mc.DrawMomGraphCoord(track, c, "mom_graph");
            return true;
        }
    }

    std::cout << "No track is found in file: " << filename << std::endl;
    return false;
}

int main(int argc, char** argv) {

    if (argc < 3) {
        std::cout << "Too few arguments" << std::endl;
        PrintUsage();
        exit(1);
    }

    // Read the arguments
    std::string dirname;
    std::string par_file;
    int event_id;
    int track_id;
    int plate;
    for (int i=1; i<argc; i++) {
        std::string arg = argv[i];
        if (arg[0] == '-') {
            if ((arg == "-D" or arg == "-dir") and i+1 < argc) {
                dirname = argv[i+1];
                i++;
            } else if ((arg == "-P" or arg == "-parfile") and i+1 < argc) {
                par_file = argv[i+1];
                i++;
            } else if (arg == "-event") {
                event_id = std::stoi(argv[i+1]);
                i++;
            } else if (arg == "-trid") {
                track_id = std::stoi(argv[i+1]);
                i++;
            } else if (arg == "-plate") {
                plate = std::stoi(argv[i+1]);
                i++;
            }
            else {
                std::cout << "Unknown option: " << arg << std::endl;
                PrintUsage();
                exit(1);
            }
        } else {
            std::cout << "Unknown option: " << arg << std::endl;
            PrintUsage();
            exit(1);
        }
    }

    // Read the linked_tracks.root
    dproc = new EdbDataProc;
    pvr = new EdbPVRec;
    c = new TCanvas("c");
    TRint app("app", 0, 0);

    // Initialize FnuMomCoord
    if (par_file != "") {
        mc.ReadParFile(par_file);
    } else {
        std::cout << "Warning: No par file is specified" << std::endl;
        mc.ReadParFile("../par/MC_plate_1_100.txt");
    }
    mc.ShowPar();

    // Loop for the directory
    TSystemDirectory dir(dirname.c_str(), dirname.c_str());
    TList* files = dir.GetListOfFiles();

    if (!files) {
        std::cout << "Error: Cannot open the directory: " << dirname << std::endl;
        exit(1);
    }

	std::regex pattern("evt_(\\d+)");
    TIter fileIter(files);
    bool found_event = false;
    while (TObject* obj = fileIter.Next()) {
        TSystemFile* file = (TSystemFile*)obj;
        std::string filename = file->GetName();

        if (pvr->eTracks) pvr->eTracks->Clear();

        std::smatch match;
        if (std::regex_search(filename, match, pattern)) {
            std::string evt_id = match[1];
            if (std::stoi(evt_id)%100000 == event_id%100000) {
                std::string filename = dirname + file->GetName() + "/linked_tracks.root";
                try {
                    if (DrawMomGraph(filename, event_id, track_id, plate)) {
                        found_event = true;
                        break;
                    }
                } catch (std::exception& e) {
                    std::cout << e.what() << std::endl;
                }
            }
        }
    }

    if (!found_event) {
        std::cout << "No event is found" << std::endl;
        return 1;
    }

    c->Draw();
    mc.ShowPar();
    app.Run();

    
    return 0;
}