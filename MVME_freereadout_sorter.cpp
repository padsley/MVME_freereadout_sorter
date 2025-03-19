#include "MVME_freereadout_sorter.hpp"



int main(int argc, char *argv[])//to make it so I can use g++ to compile the sodding thing if I need to
{
    std::string filename = "";
    double corrtime = 0.0;
    std::string treename = "event0";
    
    //uncomment the below lines to troubleshoot reading the input arguments - can't do with the VerboseFlag because I haven't read the VerboseFlag yet!
    /*
     *   std::cout << "argc: " << argc << std::endl;
     *   for(int i=0;i<argc;i++)
     *       std::cout << "argv[" << i << "]: " << argv[i] << std::endl;
     */
    
    if(argc<3)
    {
        fprintf(stderr,"too few arguments: usage mcerr [-v] <filename for raw MVME ROOT file> <correlation time in us> [<treename>]\n");
        exit(1);
    }
    else if(argc==3 && strcmp(argv[1],"-v")!=0)
    {
        std::cout << "Using ROOT file: " << argv[1] << std::endl;
        std::cout << "With correlation time: " << atof(argv[2]) << " us" << std::endl;
        
        //         TFile *fInput = TFile::Open(argv[1]);
        TString InputName(argv[1]);
        MVME_freereadout_sorter(InputName, atof(argv[2]), "event0");
    }
    else if(argc==4 && strcmp(argv[1],"-v")==0)
    {
        VerboseFlag = true;
        std::cout << "Using ROOT file: " << argv[2] << std::endl;
        std::cout << "With correlation time: " << atof(argv[3]) << " us" << std::endl;
        
        TString InputName(argv[2]);
        MVME_freereadout_sorter(InputName, atof(argv[3]), "event0");
    }
    else if(argc==5 && strcmp(argv[1],"-v")==0)
    {
        VerboseFlag = true;
        std::cout << "Using ROOT file: " << argv[2] << std::endl;
        std::cout << "With correlation time: " << atof(argv[3]) << " us" << std::endl;
        std::cout << "With tree name: " << argv[4] << std::endl;
        
        TString InputName(argv[2]);
        MVME_freereadout_sorter(InputName, atof(argv[3]), treename);
    }
    else
    {
        fprintf(stderr,"other error: usage mcerr [-v] <filename for raw MVME ROOT file> <correlation time in us> [<treename>]\n");
        exit(1);
    }
    
    return 0;
}

void MVME_freereadout_sorter(TString InputName, double CorrelationTime = 2, std::string treename = "event0")
{
    std::cout << "Correlation time is " << CorrelationTime << " us" << std::endl;
    //correlation time given in microseconds but we want it in time bins
    //time bins for the 16 MHz VME clock are 62.5 ns
    CorrelationTime *= 1000;//convert us to ns
    CorrelationTime /= 62.5;//convert from ns to time bins/clock cycles
    //correlation times are 0->pow(2,30) (around 1 billion) and then it resets so there's a strange wraparound thing which has to be accounted for in the code
    std::cout << "Correlation time corresponds to " << CorrelationTime << " bins" << std::endl;
    
    if(VerboseFlag)
        std::cout << "Input file: " << InputName.Data() << std::endl;
    
    TFile *fInput = TFile::Open(InputName.Data());
    
    TString OutputName = InputName;
    OutputName.ReplaceAll("raw","correlated");
    if(VerboseFlag)
        std::cout << "Output file: " << OutputName.Data() << std::endl;
    
    TFile *fOutput = new TFile(OutputName.Data(),"RECREATE");
    TTree *oak = new TTree("MVMECorrelatedData","Tree to store offline-correlated MVME data using TimeStamps for correlations");
    int EventMultiplicity = 0;
    std::vector<int> DAQChannel;
    std::vector<double> AmplitudeValue;
    std::vector<double> TimeValue;//relative to timestamp?
    std::vector<double> TimeStamp;
    oak->Branch("EventMultiplicity",&EventMultiplicity);
    oak->Branch("DAQChannel",&DAQChannel);
    oak->Branch("AmplitudeValue",&AmplitudeValue);
    oak->Branch("TimeValue",&TimeValue);
    oak->Branch("TimeStamp",&TimeStamp);
    
    TH1F *hCorrelations = new TH1F("hCorrelations","Correlation Time Histogram",20000,-10000,10000);
    
    if(VerboseFlag)
        fInput->ls();
    
    if(fInput->IsOpen())
    {
        TTree *trin = (TTree*)fInput->Get(treename.c_str());
        trin->SetMakeClass(1);
        
        //         if(VerboseFlag)
        //             trin->Print();
        
        long nentries = trin->GetEntries();
        std::cout << "Number of Entries: " << nentries << std::endl;
        //         std::cout << trin->GetBranch("mdpp32_scp.mdpp32_scp_module_timestamp[1]")->GetAddress() << std::endl;
        
        //This part has to be modified because I can't be bothered to do it automatically :)
        //need to set the right branch names and have the right number of stuff to read out the channels to then assemble into events
        unsigned int fUniqueID_mdpp0;     TBranch *b_mdpp32_0_fUniqueID = 0;
        unsigned int fBits_mdpp0;         TBranch *b_mdpp32_0_fBits = 0;
        TString fName_mdpp0;              TBranch *b_mdpp32_0_fName = 0;
        TString fTitle_mdpp0;             TBranch *b_mdpp32_0_fTitle = 0;
        
        double mdpp32_0_amplitude[32];    TBranch *b_mdpp32_0_amplitude = 0;
        double mdpp32_0_channel_time[32]; TBranch *b_mdpp32_0_channel_time = 0;
        double mdpp32_0_trigger_time[32]; TBranch *b_mdpp32_0_trigger_time = 0;
        double mdpp32_0_timestamp[1];     TBranch *b_mdpp32_0_timestamp = 0;
        
        trin->SetBranchAddress("fUniqueID",&fUniqueID_mdpp0,&b_mdpp32_0_fUniqueID);
        trin->SetBranchAddress("fBits",&fBits_mdpp0,&b_mdpp32_0_fBits);
        trin->SetBranchAddress("fName",&fName_mdpp0,&b_mdpp32_0_fName);
        trin->SetBranchAddress("fTitle",&fTitle_mdpp0,&b_mdpp32_0_fTitle);
        
        //turn off branches for the porpoises of speed
        trin->SetBranchStatus("fUniqueID",0);
        trin->SetBranchStatus("fBits",0);
        trin->SetBranchStatus("fName",0);
        trin->SetBranchStatus("fTitle",0);
        
        
        trin->SetBranchAddress("mdpp32_scp_0_amplitude[32]",&mdpp32_0_amplitude,&b_mdpp32_0_amplitude);
        trin->SetBranchStatus("mdpp32_scp_0_amplitude[32]",0);
        trin->SetBranchAddress("mdpp32_scp_0_channel_time[32]",&mdpp32_0_channel_time,&b_mdpp32_0_channel_time);
        trin->SetBranchStatus("mdpp32_scp_0_channel_time[32]",0);
        trin->SetBranchAddress("mdpp32_scp_0_trigger_time[2]",&mdpp32_0_trigger_time,&b_mdpp32_0_trigger_time);
        trin->SetBranchStatus("mdpp32_scp_0_trigger_time[2]",0);
        trin->SetBranchAddress("mdpp32_scp_0_module_timestamp[1]",mdpp32_0_timestamp,&b_mdpp32_0_timestamp);
        //         if(VerboseFlag)std::cout  << "Checking branch status: " << trin->GetBranchStatus("mdpp32_scp.mdpp32_scp_module_timestamp[1]") << std::endl;
        
        //mdpp1
        unsigned int fUniqueID_mdpp1;     TBranch *b_mdpp32_1_fUniqueID = 0;
        unsigned int fBits_mdpp1;         TBranch *b_mdpp32_1_fBits = 0;
        TString fName_mdpp1;              TBranch *b_mdpp32_1_fName = 0;
        TString fTitle_mdpp1;             TBranch *b_mdpp32_1_fTitle = 0;
        
        double mdpp32_1_amplitude[32];    TBranch *b_mdpp32_1_amplitude = 0;
        double mdpp32_1_channel_time[32]; TBranch *b_mdpp32_1_channel_time = 0;
        double mdpp32_1_trigger_time[32]; TBranch *b_mdpp32_1_trigger_time = 0;
        double mdpp32_1_timestamp[1];     TBranch *b_mdpp32_1_timestamp = 0;
        
        trin->SetBranchAddress("mdpp32_scp_1_amplitude[32]",mdpp32_1_amplitude,&b_mdpp32_1_amplitude);
        trin->SetBranchStatus("mdpp32_scp_1_amplitude[32]",0);
        trin->SetBranchAddress("mdpp32_scp_1_channel_time[32]",mdpp32_1_channel_time,&b_mdpp32_1_channel_time);
        trin->SetBranchStatus("mdpp32_scp_1_channel_time[32]",0);
        trin->SetBranchAddress("mdpp32_scp_1_trigger_time[2]",mdpp32_1_trigger_time,&b_mdpp32_1_trigger_time);
        trin->SetBranchStatus("mdpp32_scp_1_trigger_time[2]",0);
        trin->SetBranchAddress("mdpp32_scp_1_module_timestamp[1]",mdpp32_1_timestamp,&b_mdpp32_1_timestamp);
        trin->SetBranchStatus("mdpp32_scp_1_module_timestamp[1]",1);
        //         if(VerboseFlag)std::cout  << "Checking branch status: " << trin->GetBranchStatus("mdpp32_scp_1_module_timestamp[1]") << std::endl;
        
        //mdpp2
        unsigned int fUniqueID_mdpp2;     TBranch *b_mdpp32_2_fUniqueID = 0;
        unsigned int fBits_mdpp2;         TBranch *b_mdpp32_2_fBits = 0;
        TString fName_mdpp2;              TBranch *b_mdpp32_2_fName = 0;
        TString fTitle_mdpp2;             TBranch *b_mdpp32_2_fTitle = 0;
        
        double mdpp32_2_amplitude[32];    TBranch *b_mdpp32_2_amplitude = 0;
        double mdpp32_2_channel_time[32]; TBranch *b_mdpp32_2_channel_time = 0;
        double mdpp32_2_trigger_time[32]; TBranch *b_mdpp32_2_trigger_time = 0;
        double mdpp32_2_timestamp[1];     TBranch *b_mdpp32_2_timestamp = 0;
        
        trin->SetBranchAddress("mdpp32_scp_2_amplitude[32]",mdpp32_2_amplitude,&b_mdpp32_2_amplitude);
        trin->SetBranchStatus("mdpp32_scp_2_amplitude[32]",0);
        trin->SetBranchAddress("mdpp32_scp_2_channel_time[32]",mdpp32_2_channel_time,&b_mdpp32_2_channel_time);
        trin->SetBranchStatus("mdpp32_scp_2_channel_time[32]",0);
        trin->SetBranchAddress("mdpp32_scp_2_trigger_time[2]",mdpp32_2_trigger_time,&b_mdpp32_2_trigger_time);
        trin->SetBranchStatus("mdpp32_scp_2_trigger_time[2]",0);
        trin->SetBranchAddress("mdpp32_scp_2_module_timestamp[1]",mdpp32_2_timestamp,&b_mdpp32_2_timestamp);
        //         if(VerboseFlag)std::cout  << "Checking branch status: " << trin->GetBranchStatus("mdpp32_scp_2_module_timestamp[1]") << std::endl;
        
        if(VerboseFlag)
            std::cout << "Number of Entries: " << nentries << std::endl;
        
        int CountOfOutTimeEvents = 0;
        
        long i = 0;
        double TimeOfFirstEventToMatchAgainst = 0;
        while(i<nentries)
            //         while(i<8889)
            //         for(Long64_t i=0; i<10000;i++)
            //         while (i<2984)
        {
            if(VerboseFlag)
                std::cout << "Processing event " << i << std::endl;
            else if(i%10000 == 0)
            {
                std::cout << "[";
                int pos = 50 * i/nentries;
                for(int kk=0;kk<50;kk++)
                {
                    if(kk<pos)std::cout << "=";
                    else if (kk == pos) std::cout << ">";
                    else std::cout << " ";
                }
                std::cout << "]" << int(float(i)/float(nentries)*100.0) << " %\r" << std::flush;
            }
            //                 std::cout << "Processing event " << i << std::endl;
            
            std::vector<Long64_t> ListOfEvents;//make a vector to store the list of the events which are part of this entry so I can do stuff with them
            std::vector<int> ListOfModules;
            std::vector<double> ListOfTimestamps;
            
            double PreviousTimestamp = 0;
            
            if(ListOfEvents.size()!=0)std::cout << "not got an empty array!" << std::endl;
            if(ListOfModules.size()!=0)std::cout << "not got an empty array!" << std::endl;
            if(ListOfTimestamps.size()!=0)std::cout << "not got an empty array!" << std::endl;
            
            //clear out the vectors for the TTree correlated event filling
            DAQChannel.clear();
            AmplitudeValue.clear();
            TimeValue.clear();
            TimeStamp.clear();
            
            //turn off the branches that aren't the timestamps to try to keep things moving
            if(trin->GetBranchStatus("mdpp32_scp_0_amplitude[32]")==1)trin->SetBranchStatus("mdpp32_scp_0_amplitude[32]",0);
            if(trin->GetBranchStatus("mdpp32_scp_0_channel_time[32]")==1)trin->SetBranchStatus("mdpp32_scp_0_channel_time[32]",0);
            if(trin->GetBranchStatus("mdpp32_scp_0_trigger_time[2]")==1)trin->SetBranchStatus("mdpp32_scp_0_trigger_time[2]",0);
            if(trin->GetBranchStatus("mdpp32_scp_1_amplitude[32]")==1)trin->SetBranchStatus("mdpp32_scp_1_amplitude[32]",0);
            if(trin->GetBranchStatus("mdpp32_scp_1_channel_time[32]")==1)trin->SetBranchStatus("mdpp32_scp_1_channel_time[32]",0);
            if(trin->GetBranchStatus("mdpp32_scp_1_trigger_time[2]")==1)trin->SetBranchStatus("mdpp32_scp_1_trigger_time[2]",0);
            if(trin->GetBranchStatus("mdpp32_scp_2_amplitude[32]")==1)trin->SetBranchStatus("mdpp32_scp_2_amplitude[32]",0);
            if(trin->GetBranchStatus("mdpp32_scp_2_channel_time[32]")==1)trin->SetBranchStatus("mdpp32_scp_2_channel_time[32]",0);
            if(trin->GetBranchStatus("mdpp32_scp_2_trigger_time[2]")==1)trin->SetBranchStatus("mdpp32_scp_2_trigger_time[2]",0);
            
            double FirstTimestamp = 0;
            TimeOfFirstEventToMatchAgainst = 0;
            
            mdpp32_0_timestamp[0] = 0;
            mdpp32_1_timestamp[0] = 0;
            mdpp32_2_timestamp[0] = 0;
            
            for(int m=0;m<32;m++)
            {
                mdpp32_0_amplitude[m] = 0; mdpp32_0_channel_time[m] = 0;
                mdpp32_1_amplitude[m] = 0; mdpp32_1_channel_time[m] = 0;
                mdpp32_2_amplitude[m] = 0; mdpp32_2_channel_time[m] = 0;
            }
            
            trin->GetEntry(i);
            
            if(mdpp32_0_timestamp[0]!=0 && !std::isnan(mdpp32_0_timestamp[0]))//mdpp0 for initial triggering event
            {
                if(VerboseFlag)std::cout << mdpp32_0_timestamp[0] - TimeOfFirstEventToMatchAgainst << std::endl;
                
                if(TimeOfFirstEventToMatchAgainst == 0)TimeOfFirstEventToMatchAgainst = mdpp32_0_timestamp[0];
                else if(mdpp32_0_timestamp[0] - TimeOfFirstEventToMatchAgainst < CorrelationTime)
                {
                    if(VerboseFlag)std::cout << "Found an event in mdpp0 " << i << std::endl;
                    
                    //                     std::cout << "Check list of events: " << std::find(ListOfEvents.begin(), ListOfEvents.end(), i) << std::endl;
                    
                    if(std::find(ListOfEvents.begin(), ListOfEvents.end(), i) == ListOfEvents.end())//== not != -> == only add the event if it isn't already in the list of events, != only add the event if it already is in the list!
                    {
                        ListOfEvents.push_back(i);
                        ListOfModules.push_back(0);
                        ListOfTimestamps.push_back(mdpp32_0_timestamp[0]);
                    }
                    
                    double InitialTime_mdpp0 = mdpp32_0_timestamp[0];
                    TimeOfFirstEventToMatchAgainst = mdpp32_0_timestamp[0];
                    
                    bool WrapAroundEvent = false;
                    
                    if(InitialTime_mdpp0 < PreviousTimestamp)
                    {
                        if(VerboseFlag)
                        {
                            std::cout << "Current Timestamp is below the previous timestamp!" << std::endl;
                            std::cout << "PreviousTimestamp: " << PreviousTimestamp << "\t InitialTime: " << InitialTime_mdpp0 << "\t Difference: " << PreviousTimestamp - InitialTime_mdpp0 << std::endl << std::endl;
                            WrapAroundEvent = true;
                            
                            if(abs(InitialTime_mdpp0 - PreviousTimestamp + pow(2,30)) < CorrelationTime)std::cout << "BUT we found that it's correlated around the end of the clock cycle: " << InitialTime_mdpp0 - PreviousTimestamp + pow(2,30) << " is within the correlation time " << CorrelationTime << std::endl;
                        }
                        CountOfOutTimeEvents++;
                    }
                    
                    PreviousTimestamp = InitialTime_mdpp0;
                    
                    long j = i;
                    //checking for module 1 correlated events
                    bool ReadForCorrelatedTimestamps = true;
                    while(j<nentries && ReadForCorrelatedTimestamps)
                    {
                        if(VerboseFlag)std::cout << "Checking entry for mdpp0 correlation: " << j << std::endl;
                        trin->GetEntry(j);
                        
                        if(mdpp32_0_timestamp[0]!=0 && !std::isnan(mdpp32_0_timestamp[0]) && j>i)//possible correlation with module 0
                        {
                            if(mdpp32_0_timestamp[0] - InitialTime_mdpp0<0)
                            {
                                if(VerboseFlag)std::cout << "Probably got a wraparound event!" << std::endl;
                                
                                if((mdpp32_0_timestamp[0] - InitialTime_mdpp0<CorrelationTime && mdpp32_0_timestamp[0] - InitialTime_mdpp0>0) || (mdpp32_0_timestamp[0] - InitialTime_mdpp0 + pow(2,30)<CorrelationTime))//check if within correlation time
                                {
                                    if(VerboseFlag)std::cout << "Found a matching event (" << j << ") between mdpp0 and event (" << i << ") mdpp0 with timestamp difference: " << mdpp32_0_timestamp[0] - InitialTime_mdpp0 << std::endl;
                                    
                                    if(std::find(ListOfEvents.begin(), ListOfEvents.end(), j) == ListOfEvents.end())
                                    {
                                        ListOfEvents.push_back(j);
                                        ListOfModules.push_back(0);
                                        ListOfTimestamps.push_back(mdpp32_0_timestamp[0]);
                                    }
                                }
                                else if(mdpp32_0_timestamp[0] - InitialTime_mdpp0 > CorrelationTime)
                                    ReadForCorrelatedTimestamps = false;
                                
                            }
                            
                            if(mdpp32_1_timestamp[0]!=0 && !std::isnan(mdpp32_1_timestamp[0]))//possible correlation with module 1
                            {
                                if((mdpp32_1_timestamp[0] - InitialTime_mdpp0<CorrelationTime && mdpp32_1_timestamp[0] - InitialTime_mdpp0>0) || (mdpp32_2_timestamp[0] - InitialTime_mdpp0 + pow(2,30)<CorrelationTime))//check if within correlation time
                                {
                                    if(VerboseFlag)std::cout << "Found a matching event (" << j << ") between mdpp0 and event (" << i << ") mdpp1 with timestamp difference: " << mdpp32_1_timestamp[0] - InitialTime_mdpp0 << std::endl;
                                    
                                    if(std::find(ListOfEvents.begin(), ListOfEvents.end(), j) == ListOfEvents.end())
                                    {
                                        ListOfEvents.push_back(j);
                                        ListOfModules.push_back(1);
                                        ListOfTimestamps.push_back(mdpp32_1_timestamp[0]);
                                    }
                                }
                                else if(mdpp32_1_timestamp[0] - InitialTime_mdpp0 > CorrelationTime)
                                    ReadForCorrelatedTimestamps = false;
                            }
                            
                            if(mdpp32_2_timestamp[0]!=0 && !std::isnan(mdpp32_2_timestamp[0]))//possible correlation with module 2
                            {
                                if((mdpp32_2_timestamp[0] - InitialTime_mdpp0<CorrelationTime && mdpp32_0_timestamp[0] - InitialTime_mdpp0>0) || (mdpp32_2_timestamp[0] - InitialTime_mdpp0 + pow(2,30)<CorrelationTime))//check if within correlation time
                                {
                                    if(VerboseFlag)std::cout << "Found a matching event (" << j << ") between mdpp0 and event (" << i << ") mdpp2 with timestamp difference: " << mdpp32_2_timestamp[0] - InitialTime_mdpp0 << std::endl;
                                    
                                    if(std::find(ListOfEvents.begin(), ListOfEvents.end(), j) == ListOfEvents.end())
                                    {
                                        ListOfEvents.push_back(j);
                                        ListOfModules.push_back(2);
                                        ListOfTimestamps.push_back(mdpp32_2_timestamp[0]);
                                    }
                                }
                                else if(mdpp32_2_timestamp[0] - InitialTime_mdpp0 > CorrelationTime)
                                    ReadForCorrelatedTimestamps = false;
                            }
                            j++;//increment to the next event
                        }//end of the loop looking for matching events in the next few triggers
                    }//end of test statement to see if future events match the timestamp in that detector
                    //                 else
                    //                     ReadForCorrelatedTimestamps = false;
                }//end of condition checking if within correlation time
            }//end of condition checking if there's a valid hit in mdpp0
            //             
            if(mdpp32_1_timestamp[0]!=0 && !std::isnan(mdpp32_1_timestamp[0]))//MDPP1 for the initial triggering event
            {
                if(VerboseFlag)std::cout << mdpp32_1_timestamp[0] - TimeOfFirstEventToMatchAgainst << std::endl;
                if(TimeOfFirstEventToMatchAgainst == 0)TimeOfFirstEventToMatchAgainst = mdpp32_1_timestamp[0];
                else if(mdpp32_1_timestamp[0] - TimeOfFirstEventToMatchAgainst < CorrelationTime)
                    if(mdpp32_1_timestamp[0] - TimeOfFirstEventToMatchAgainst < CorrelationTime)
                    {
                        if(VerboseFlag)std::cout << "Found an event in mdpp1 " << i << std::endl;
                        
                        if(std::find(ListOfEvents.begin(), ListOfEvents.end(), i) == ListOfEvents.end())
                        {
                            ListOfEvents.push_back(i);
                            ListOfModules.push_back(1);
                            ListOfTimestamps.push_back(mdpp32_1_timestamp[0]);
                        }
                        
                        double InitialTime_mdpp1 = mdpp32_1_timestamp[0];
                        TimeOfFirstEventToMatchAgainst = mdpp32_1_timestamp[0];
                        
                        bool WrapAroundEvent = false;
                        
                        if(InitialTime_mdpp1 < PreviousTimestamp)
                        {
                            if(VerboseFlag)
                            {
                                std::cout << "Current Timestamp is below the previous timestamp!" << std::endl;
                                std::cout << "PreviousTimestamp: " << PreviousTimestamp << "\t InitialTime: " << InitialTime_mdpp1 << "\t Difference: " << PreviousTimestamp - InitialTime_mdpp1 << std::endl << std::endl;
                                WrapAroundEvent = true;
                                
                                if(abs(InitialTime_mdpp1 - PreviousTimestamp + pow(2,30)) < CorrelationTime)std::cout << "BUT we found that it's correlated around the end of the clock cycle: " << InitialTime_mdpp1 - PreviousTimestamp + pow(2,30) << " is within the correlation time " << CorrelationTime << std::endl;
                            }
                            CountOfOutTimeEvents++;
                        }
                        
                        PreviousTimestamp = InitialTime_mdpp1;
                        
                        long j = i;
                        //checking for module 1 correlated events
                        bool ReadForCorrelatedTimestamps = true;
                        while(j<nentries && ReadForCorrelatedTimestamps)
                        {
                            if(VerboseFlag)std::cout << "Checking entry for mdpp1 correlation: " << j << std::endl;
                            trin->GetEntry(j);
                            
                            if(mdpp32_0_timestamp[0]!=0 && !std::isnan(mdpp32_0_timestamp[0]))//possible correlation with module 0
                            {
                                if(((mdpp32_0_timestamp[0] - InitialTime_mdpp1<CorrelationTime && mdpp32_0_timestamp[0] - InitialTime_mdpp1>0)) || ((mdpp32_0_timestamp[0] - InitialTime_mdpp1 + pow(2,30)<CorrelationTime)))//check if within correlation time
                                {
                                    if(VerboseFlag)std::cout << "Found a matching event (" << j << ") between mdpp1 and event (" << i << ") mdpp0 with timestamp difference: " << mdpp32_0_timestamp[0] - InitialTime_mdpp1 << std::endl;
                                    
                                    
                                    if(std::find(ListOfEvents.begin(), ListOfEvents.end(), j) == ListOfEvents.end())
                                    {
                                        ListOfEvents.push_back(j);
                                        ListOfModules.push_back(0);
                                        ListOfTimestamps.push_back(mdpp32_0_timestamp[0]);
                                    }
                                }
                                else if(mdpp32_0_timestamp[0] - InitialTime_mdpp1 > CorrelationTime)
                                    ReadForCorrelatedTimestamps = false;
                                
                            }
                            
                            if(mdpp32_1_timestamp[0]!=0 && !std::isnan(mdpp32_1_timestamp[0]) && j>i)//possible correlation with module 1
                            {
                                if(((mdpp32_1_timestamp[0] - InitialTime_mdpp1<CorrelationTime && mdpp32_1_timestamp[0] - InitialTime_mdpp1>0)) || (mdpp32_1_timestamp[0] - InitialTime_mdpp1 + pow(2,30)<CorrelationTime))//check if within correlation time
                                {
                                    if(VerboseFlag)std::cout << "Found a matching event (" << j << ") between mdpp1 and event (" << i << ") mdpp1 with timestamp difference: " << mdpp32_1_timestamp[0] - InitialTime_mdpp1 << std::endl;
                                    
                                    if(std::find(ListOfEvents.begin(), ListOfEvents.end(), j) == ListOfEvents.end())
                                    {
                                        ListOfEvents.push_back(j);
                                        ListOfModules.push_back(1);
                                        ListOfTimestamps.push_back(mdpp32_1_timestamp[0]);
                                    }
                                }
                                else if(mdpp32_1_timestamp[0] - InitialTime_mdpp1 > CorrelationTime)
                                    ReadForCorrelatedTimestamps = false;
                            }
                            
                            if(mdpp32_2_timestamp[0]!=0 && !std::isnan(mdpp32_2_timestamp[0]))//possible correlation with module 2
                            {
                                if((mdpp32_2_timestamp[0] - InitialTime_mdpp1<CorrelationTime && mdpp32_0_timestamp[0] - InitialTime_mdpp1>0) || (mdpp32_2_timestamp[0] - InitialTime_mdpp1 + pow(2,30)<CorrelationTime))//check if within correlation time
                                {
                                    if(VerboseFlag)std::cout << "Found a matching event (" << j << ") between mdpp1 and event (" << i << ") mdpp2 with timestamp difference: " << mdpp32_2_timestamp[0] - InitialTime_mdpp1 << std::endl;
                                    
                                    if(std::find(ListOfEvents.begin(), ListOfEvents.end(), j) == ListOfEvents.end())
                                    {
                                        ListOfEvents.push_back(j);
                                        ListOfModules.push_back(2);
                                        ListOfTimestamps.push_back(mdpp32_2_timestamp[0]);
                                    }
                                }
                                else if(mdpp32_2_timestamp[0] - InitialTime_mdpp1 > CorrelationTime)
                                    ReadForCorrelatedTimestamps = false;
                            }
                            j++;//increment to the next event
                        }//end of the loop looking for matching events in the next few triggers
                    }
                    //                 else
                    //                     ReadForCorrelatedTimestamps = false;
            }
            
            if(mdpp32_2_timestamp[0]!=0 && !std::isnan(mdpp32_2_timestamp[0]))//MDPP1 for the initial triggering event
            {
                if(VerboseFlag)std::cout << mdpp32_2_timestamp[0] - TimeOfFirstEventToMatchAgainst << std::endl;
                if(TimeOfFirstEventToMatchAgainst == 0)TimeOfFirstEventToMatchAgainst = mdpp32_2_timestamp[0];
                else if(mdpp32_2_timestamp[0] - TimeOfFirstEventToMatchAgainst < CorrelationTime)
                    if(mdpp32_2_timestamp[0] - TimeOfFirstEventToMatchAgainst < CorrelationTime)
                    {
                        if(VerboseFlag)std::cout << "Found an event in mdpp2 " << i << std::endl;
                        
                        if(std::find(ListOfEvents.begin(), ListOfEvents.end(), i) == ListOfEvents.end())
                        {                    
                            ListOfEvents.push_back(i);
                            ListOfModules.push_back(2);
                            ListOfTimestamps.push_back(mdpp32_2_timestamp[0]);
                        }
                        
                        double InitialTime_mdpp2 = mdpp32_2_timestamp[0];
                        TimeOfFirstEventToMatchAgainst = mdpp32_2_timestamp[0];
                        
                        if(InitialTime_mdpp2 < PreviousTimestamp)
                        {
                            if(VerboseFlag)
                            {
                                std::cout << "Current Timestamp is below the previous timestamp!" << std::endl;
                                std::cout << "PreviousTimestamp: " << PreviousTimestamp << "\t InitialTime: " << InitialTime_mdpp2 << "\t Difference: " << PreviousTimestamp - InitialTime_mdpp2 << std::endl << std::endl;
                                
                                if(abs(InitialTime_mdpp2 - PreviousTimestamp + pow(2,30)) < CorrelationTime)std::cout << "BUT we found that it's correlated around the end of the clock cycle: " << InitialTime_mdpp2 - PreviousTimestamp + pow(2,30) << " is within the correlation time " << CorrelationTime << std::endl;
                            }
                            
                            CountOfOutTimeEvents++;
                        }
                        
                        PreviousTimestamp = InitialTime_mdpp2;
                        
                        long j = i;
                        //loop over timing values for mdpp2
                        bool ReadForCorrelatedTimestamps = true;
                        while(j<nentries && ReadForCorrelatedTimestamps)
                        {
                            if(VerboseFlag)std::cout << "Checking entry for mdpp2 correlation: " << j << std::endl;
                            trin->GetEntry(j);
                            
                            if(mdpp32_0_timestamp[0]!=0 && !std::isnan(mdpp32_0_timestamp[0]))//possible correlation with module 0
                            {
                                if((mdpp32_0_timestamp[0] - InitialTime_mdpp2<CorrelationTime && mdpp32_0_timestamp[0] - InitialTime_mdpp2>0) || (mdpp32_0_timestamp[0] - InitialTime_mdpp2 + pow(2,30)<CorrelationTime))//check if within correlation time
                                {
                                    if(VerboseFlag)std::cout << "Found a matching event (" << j << ") between mdpp0 and event (" << i << ") mdpp1 with timestamp difference: " << mdpp32_0_timestamp[0] - InitialTime_mdpp2 << std::endl;
                                    
                                    if(std::find(ListOfEvents.begin(), ListOfEvents.end(), j) == ListOfEvents.end())
                                    {
                                        ListOfEvents.push_back(j);
                                        ListOfModules.push_back(0);
                                        ListOfTimestamps.push_back(mdpp32_0_timestamp[0]);
                                    }
                                }
                                else if(mdpp32_0_timestamp[0] - InitialTime_mdpp2 > CorrelationTime)
                                    ReadForCorrelatedTimestamps = false;
                            }
                            
                            if(mdpp32_1_timestamp[0]!=0 && !std::isnan(mdpp32_1_timestamp[0]))//possible correlation with module 1
                            {
                                if((mdpp32_1_timestamp[0] - InitialTime_mdpp2<CorrelationTime && mdpp32_1_timestamp[0] - InitialTime_mdpp2>0) || (mdpp32_1_timestamp[0] - InitialTime_mdpp2 + pow(2,30)<CorrelationTime))//check if within correlation time
                                {
                                    if(VerboseFlag)std::cout << "Found a matching event (" << j << ") between mdpp2 and event (" << i << ") mdpp1 with timestamp difference: " << mdpp32_1_timestamp[0] - InitialTime_mdpp2 << std::endl;
                                    
                                    if(std::find(ListOfEvents.begin(), ListOfEvents.end(), j) == ListOfEvents.end())
                                    {
                                        ListOfEvents.push_back(j);
                                        ListOfModules.push_back(1);
                                        ListOfTimestamps.push_back(mdpp32_1_timestamp[0]);
                                    }
                                }
                                else if(mdpp32_1_timestamp[0] - InitialTime_mdpp2 > CorrelationTime)
                                    ReadForCorrelatedTimestamps = false;
                            }
                            
                            if(mdpp32_2_timestamp[0]!=0 && !std::isnan(mdpp32_2_timestamp[0]) && j>i)//possible correlation with module 2
                            {
                                if((mdpp32_2_timestamp[0] - InitialTime_mdpp2<CorrelationTime && mdpp32_2_timestamp[0] - InitialTime_mdpp2>0) || (mdpp32_2_timestamp[0] - InitialTime_mdpp2 + pow(2,30)<CorrelationTime))//check if within correlation time
                                {
                                    if(VerboseFlag)std::cout << "Found a matching event (" << j << ") between mdpp2 and event (" << i << ") mdpp2 with timestamp difference: " << mdpp32_2_timestamp[0] - InitialTime_mdpp2 << std::endl;
                                    
                                    if(std::find(ListOfEvents.begin(), ListOfEvents.end(), j) == ListOfEvents.end())
                                    {
                                        ListOfEvents.push_back(j);
                                        ListOfModules.push_back(2);
                                        ListOfTimestamps.push_back(mdpp32_2_timestamp[0]);
                                    }
                                }
                                else if(mdpp32_2_timestamp[0] - InitialTime_mdpp2 > CorrelationTime)
                                    ReadForCorrelatedTimestamps = false;
                            }
                            j++;
                        }//end of the loop looking for matching events
                    }
                    //                 else
                    //                     ReadForCorrelatedTimestamps = false;
            }
            
            if(VerboseFlag)std::cout << "Checked for matching timestamps - now to fill the array with this TTree information" << std::endl;    
            if(ListOfEvents.size()!=ListOfModules.size() || ListOfEvents.size()!=ListOfTimestamps.size())std::cout << "Something is wrong!" << std::endl;
            for(unsigned int k = 0;k<ListOfEvents.size();k++)
            {
                if(VerboseFlag)std::cout << "k: " << k << "\tEvent: " << ListOfEvents.at(k) << "\tModule: " << ListOfModules.at(k) << std::endl;
                
                //                 if(ListOfModules.at(k)==0)
                //                 {
                trin->SetBranchStatus("mdpp32_scp_0_amplitude[32]",1);
                trin->SetBranchStatus("mdpp32_scp_0_channel_time[32]",1);
                trin->SetBranchStatus("mdpp32_scp_0_trigger_time[2]",1);
                //                 }
                //                 if(ListOfModules.at(k)==1)
                //                 {
                trin->SetBranchStatus("mdpp32_scp_1_amplitude[32]",1);
                trin->SetBranchStatus("mdpp32_scp_1_channel_time[32]",1);
                trin->SetBranchStatus("mdpp32_scp_1_trigger_time[2]",1);
                //                 }
                // //                 if(ListOfModules.at(k)==2)
                //                 {
                trin->SetBranchStatus("mdpp32_scp_2_amplitude[32]",1);
                trin->SetBranchStatus("mdpp32_scp_2_channel_time[32]",1);
                trin->SetBranchStatus("mdpp32_scp_2_trigger_time[2]",1);
                // //                 }
                
                trin->GetEntry(ListOfEvents.at(k));
                //                 if(k>0)
                {
                    for(unsigned int kk=k+1;kk<ListOfEvents.size();kk++)
                    {
                        if(VerboseFlag)
                            std::cout << "Entry: " << k << "\tEvent: " << ListOfEvents.at(k) << "\tModule: " << ListOfModules.at(k) << "\tTimeStamp: " << ListOfTimestamps.at(k) << "\tEntry: " << kk << "\tEvent: " << ListOfEvents.at(kk) << "\tModule: " << ListOfModules.at(kk) << "\tTimeStamp: " << ListOfTimestamps.at(kk) << "\t TimeStampDifference: " << ListOfTimestamps.at(kk)-ListOfTimestamps.at(k) << std::endl;
                        hCorrelations->Fill(ListOfTimestamps.at(k)-ListOfTimestamps.at(0));
                    }
                }
                
                for(int chan = 0;chan<32;chan++)
                {
                    //                     std::cout << "channel: " << chan << "\tAmplitude: " << mdpp32_0_amplitude[chan] << "\tTime: " << mdpp32_0_channel_time[chan] << std::endl;
                    //                     std::cout << "channel: " << chan << "\tAmplitude: " << mdpp32_1_amplitude[chan] << "\tTime: " << mdpp32_1_channel_time[chan] << std::endl;
                    //                     std::cout << "channel: " << chan << "\tAmplitude: " << mdpp32_2_amplitude[chan] << "\tTime: " << mdpp32_2_channel_time[chan] << std::endl;
                    
                    if((mdpp32_0_timestamp[0]!=0 &&!std::isnan(mdpp32_0_amplitude[chan]) && mdpp32_0_amplitude[chan] > 0))
                    {
                        DAQChannel.push_back(chan);
                        AmplitudeValue.push_back(mdpp32_0_amplitude[chan]);
                        TimeValue.push_back(mdpp32_0_channel_time[chan]);
                        TimeStamp.push_back(mdpp32_0_timestamp[0]);
                        //                         if(VerboseFlag)std::cout << "channel: " << chan << "\tAmplitude: " << mdpp32_0_amplitude[chan] << "\tTime: " << mdpp32_0_channel_time[chan] << std::endl;
                    }
                    if((mdpp32_1_timestamp[0]!=0 && !std::isnan(mdpp32_1_amplitude[chan]) && mdpp32_1_amplitude[chan] > 0))
                    {
                        DAQChannel.push_back(32+chan);
                        AmplitudeValue.push_back(mdpp32_1_amplitude[chan]);
                        TimeValue.push_back(mdpp32_1_channel_time[chan]);
                        TimeStamp.push_back(mdpp32_1_timestamp[0]);
                        //                         if(VerboseFlag)std::cout << "channel: " << chan << "\tAmplitude: " << mdpp32_1_amplitude[chan] << "\tTime: " << mdpp32_1_channel_time[chan] << std::endl;
                        
                    }
                    if((mdpp32_2_timestamp[0]!=0 && !std::isnan(mdpp32_2_amplitude[chan]) && mdpp32_2_amplitude[chan] > 0))
                    {
                        DAQChannel.push_back(64+chan);
                        AmplitudeValue.push_back(mdpp32_2_amplitude[chan]);
                        TimeValue.push_back(mdpp32_2_channel_time[chan]);
                        TimeStamp.push_back(mdpp32_2_timestamp[0]);
                        //                         if(VerboseFlag)std::cout << "channel: " << chan << "\tAmplitude: " << mdpp32_2_amplitude[chan] << "\tTime: " << mdpp32_2_channel_time[chan] << std::endl;
                    }
                }
                
                EventMultiplicity = (int)DAQChannel.size();
                oak->Fill();
            }
            
            
            if(ListOfEvents.size()>1 && ListOfEvents.at(ListOfEvents.size()-1) == i)i++;//if the correlated events are within the same event
            else if(ListOfEvents.size()>1)
            {
                if(VerboseFlag)std::cout << "Skipping to event " << ListOfEvents.at(ListOfEvents.size()-1) << std::endl;
                i = ListOfEvents.at(ListOfEvents.size()-1);//set the next event to the last event which wasn't included in the last set of timestamps?
            }
            else
            {
                if(VerboseFlag)std::cout << "Did not find a matching event" << std::endl;
                i++;
            }
            
            //make sure that the vectors are clear
            ListOfEvents.clear();
            ListOfModules.clear();
            ListOfTimestamps.clear();
        }//end of loop over the entries
        std::cout << "Finished processing the input ROOT TTree" << std::endl;
        
        std::cout << "Proportion of out-of-time events: " << (double)CountOfOutTimeEvents/nentries * 100. << std::endl;
    }//end of check that the input file is open
    else
        std::cout << "File not found" << std::endl;
    
    std::cout << "Checking in output file" << std::endl;
    fOutput->cd();
    std::cout << "Writing the correlations histogram" << std::endl;
    hCorrelations->Write();
    std::cout << "Writing the output ROOT TTree with correlated events (hopefully!)" << std::endl;
    oak->Write();

    fOutput->Close();
}
