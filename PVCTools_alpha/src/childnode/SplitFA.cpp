//
// Created by liujiajun on 2017/4/21.
//

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//The source gene sequence fa is split into small portions according to the included chromosomes
//		all.fa ------> all_Chr01.fa all_Chr02.fa ………….all_ChrN.fa
//
//Command:  ./SplitFA	<-w WorkPath> <-fa FAPath>
////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "SplitFA.h"
#include "Environment.h"

using namespace std;

//The FA file is divided into chromosomes and obtained after cutting the chromosome name.
void Fa_Chr(char *workpath,char *fa_path)
{
    string Line;
    char FileName[CMD_NUM];
    char ShellCommand[CMD_NUM];

    //Create the FA directory.
    snprintf(ShellCommand, sizeof(ShellCommand), "mkdir -p %s/fa", workpath);
    system(ShellCommand);

    ifstream fp;
    ofstream fp_tmp;
    fp.open(fa_path,ios::in);
    getline(fp,Line);
    //Extract the sequence of each chromosome
    while (!fp.eof())
    {
        strncpy(FileName, &Line[1] ,sizeof(FileName) - 4);
        for (int i = 0; i < (int)strlen(FileName); i++)
        {
            if(FileName[i] == ' ' || FileName[i] == '\t' || FileName[i] == '\n')
            {
                FileName[i] = '\0';
                break;
            }
        }
        snprintf(ShellCommand, sizeof(ShellCommand), "%s/fa/%s.fa", workpath, FileName);
        snprintf(FileName, sizeof(FileName), "%s", ShellCommand);
        snprintf(ShellCommand, sizeof(ShellCommand), "touch %s", FileName);
        system(ShellCommand);
        fp_tmp.open(FileName, ios::out);
        fp_tmp<<Line<<endl;
        getline(fp,Line);
        while (!fp.eof() && Line.at(0) != '>')
        {
            fp_tmp<<Line<<endl;
            getline(fp,Line);
        }
        fp_tmp.close();
    }
    fp.close();
}

int SplitFA(int argc,char *argv[])
{
    long StartTime = time((time_t*)NULL);
    printf("SplitFA start time = %ld\n", StartTime);

    char ShellCommand[CMD_NUM];

    char PathWork[CMD_NUM];
    char PathFA[CMD_NUM];

    for (int i = 0; i < argc; i++)
    {
        string cmd = argv[i];
        if (cmd == "-w")
        {
            snprintf(PathWork, sizeof(PathWork), "%s", argv[i + 1]);
            if (PathWork[strlen(PathWork) - 1] == '/') PathWork[strlen(PathWork) - 1] = '\0';
        }
        if (cmd == "-fa")  snprintf(PathFA, sizeof(PathFA), "%s", argv[i + 1]);
    }
    snprintf(ShellCommand, sizeof(ShellCommand), "mkdir -p %s", PathWork);
    system(ShellCommand);
    //Generates an empty [falist] for import.
    snprintf(ShellCommand, sizeof(ShellCommand), "touch %s/falist", PathWork);
    system(ShellCommand);

    //Cut the FA file.
    Fa_Chr(PathWork, PathFA);

    long FinishTime = time((time_t*)NULL);
    printf("SplitFA finish time = %ld\n", FinishTime);
    long RunningTime = FinishTime - StartTime;
    printf("SplitFA running time = %ld\n", RunningTime);

    return 0;
}

