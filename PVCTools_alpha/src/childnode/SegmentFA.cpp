//
// Created by liujiajun on 2017/4/21.
//

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Each chromosome FA file is divided into small portions according to the split number.
//			all_Chr01.fa -----> all_Chr01_1.fa all_Chr01_2.fa all_Chr01_3.fa ……………….all_Chr01_N.fa
//
//Command:     ./SegmentFA <-w WorkPath> <-n SplitNumber> [-lm Size]
////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "SegmentFA.h"

using namespace std;

int AddReserve(char *tar_file, char *from_file, int line_number)
{
    if(line_number == 0) return 0;
    ifstream infile;
    ofstream outfile;
    infile.open(from_file,ios::in);
    outfile.open(tar_file, ios::out|ios::app);
    string buff;
    getline(infile,buff);
    for(int i = 0; i < line_number && infile.eof() == 0;)
    {
        if(buff[0] != '>')
        {
            outfile<<buff<<endl;
            i++;
        }
        getline(infile,buff);
    }
    infile.close();
    outfile.close();
    return 0;
}

int SegmentFA(int argc,char *argv[])
{
    long StartTime = time((time_t*)NULL);
    printf("SegmentFA start time = %ld\n", StartTime);

    string PATH_SAMTOOLS;
    GetToolsPath(argv[0], PATH_SAMTOOLS, "-samtools");
    vector<string> ChrName;
    ifstream fp_Tmp;
    //Command string
    char ShellCommand[CMD_NUM];
    string Buffer;
    long Limit = 0;
    int SplitNumber;
    double Reserve = 0;
    char PathWork[CMD_NUM];

    //Create a shell command to call the result log file
    system("touch tmp");

    for (int i = 0; i < argc; i++)
    {
        string cmd = argv[i];
        if (cmd == "-w")
        {
            snprintf(PathWork, sizeof(PathWork), "%s", argv[i + 1]);
            if (PathWork[strlen(PathWork) - 1] == '/') PathWork[strlen(PathWork) - 1] = '\0';
        }
        if (cmd == "-n") SplitNumber = atoi(argv[i + 1]);
        if (cmd == "-lm") Limit = atol(argv[i + 1]);
        if (cmd == "-R") Reserve = atol(argv[i + 1]);
    }


    if(Limit == 0)
    {
        //Import FA list. If you need to customize the list, you should modify [falist], fill in the need to split the fa files.
        snprintf(ShellCommand, sizeof(ShellCommand), "%s/falist", PathWork);
        ifstream fp_fa;
        string strbuff;
        fp_fa.open(ShellCommand,ios::in);
        getline(fp_fa, Buffer);
        while (!fp_fa.eof())
        {
            if (Buffer.size() != 0)
            {
                int i = Buffer.rfind('.');
                strbuff = Buffer.substr(0,i);
                ChrName.push_back(strbuff.c_str());
            }
            getline(fp_fa, Buffer);
        }
        fp_fa.close();
    }
    else
    {
        FILE *fp_FA;
        snprintf(ShellCommand, sizeof(ShellCommand), "%s/falist", PathWork);
        if ((fp_FA = fopen(ShellCommand, "w")) == NULL) exit(-1);
        DIR * dir;
        struct dirent * ptr;
        char FAFile[CMD_NUM];
        snprintf(FAFile, sizeof(FAFile), "%s/fa",PathWork);
        dir = opendir(FAFile);
        while((ptr = readdir(dir)) != NULL)
        {
            if (ptr->d_type == 4) continue;
            strncpy(FAFile, ptr->d_name, sizeof(FAFile)-1);
            string Suffix;
            for (int i = (int)strlen(FAFile) - 1; i > 0; i--)
            {
                if (FAFile[i] == '.') Suffix = FAFile + i + 1;
            }
            if (Suffix != "fa") continue;
            //Call the command to query the FA file size.
            snprintf(ShellCommand, sizeof(ShellCommand), "du -h --block-size=M %s/fa/%s > tmp", PathWork, ptr->d_name);
            system(ShellCommand);
            fp_Tmp.open("tmp",ios::in);
            getline(fp_Tmp, Buffer);
            fp_Tmp.close();
            long FASize = atol(Buffer.c_str());
            if (FASize < Limit) continue;
            strncpy(FAFile, ptr->d_name, sizeof(FAFile)-1);
            fputs(FAFile, fp_FA);
            fputs("\n", fp_FA);
            for (int i = (int)strlen(FAFile) - 1; i > 0; i--)
            {
                if (FAFile[i] == '.')
                {
                    FAFile[i] = '\0';
                    break;
                }
            }
            ChrName.push_back(FAFile);
        }
        closedir(dir);
        fclose(fp_FA);
    }

    snprintf(ShellCommand, sizeof(ShellCommand), "sort %s/falist -o %s/falist", PathWork, PathWork);
    system(ShellCommand);

    int FileNumber[ChrName.size()];

    //Each chromosome is divided
    for (int i = 0; i < (int)ChrName.size(); ++i)
    {
        snprintf(ShellCommand, sizeof(ShellCommand), "du -h --block-size=M %s/fa/%s.fa > tmp ", PathWork, ChrName[i].c_str());
        system(ShellCommand);
        fp_Tmp.open("tmp",ios::in);
        getline(fp_Tmp, Buffer);
        fp_Tmp.close();
        long FASize = atol(Buffer.c_str());
        //Calculate the size of each post split.
        long fa_split_piece_size = FASize / SplitNumber;
        if (fa_split_piece_size == 0) fa_split_piece_size = 1;

        //Already exists before the chromosome processing data is deleted from the original data to prepare to write new data.
        snprintf(ShellCommand, sizeof(ShellCommand), "%s/fa/%s", PathWork, ChrName[i].c_str());
        DIR * fa_dir;
        fa_dir = opendir(ShellCommand);
        if (fa_dir != NULL)
        {
            closedir(fa_dir);
            snprintf(ShellCommand, sizeof(ShellCommand), "rm -rf %s/fa/%s", PathWork, ChrName[i].c_str());
            system(ShellCommand);
        }
        snprintf(ShellCommand, sizeof(ShellCommand), "mkdir -p %s/fa/%s", PathWork, ChrName[i].c_str());
        system(ShellCommand);

        printf("FA files cutting...\n");
        snprintf(ShellCommand, sizeof(ShellCommand), "split -C %ldM %s/fa/%s.fa %s_", fa_split_piece_size, PathWork, ChrName[i].c_str(), ChrName[i].c_str());
        system(ShellCommand);
        snprintf(ShellCommand, sizeof(ShellCommand), "mv %s_* %s/fa/%s", ChrName[i].c_str(), PathWork, ChrName[i].c_str());
        system(ShellCommand);
        printf("FA files cutting is done\n");

        //Count the current number of files
        snprintf(ShellCommand, sizeof(ShellCommand), "ls -l %s/fa/%s |grep \"^-\" |wc -l > tmp ", PathWork, ChrName[i].c_str());
        system(ShellCommand);
        fp_Tmp.open("tmp",ios::in);
        getline(fp_Tmp, Buffer);
        fp_Tmp.close();
        //The number of files after bisection
        FileNumber[i] = atoi(Buffer.c_str());
        printf("FA files renaming...\n");
        for (int n = 0; n < FileNumber[i]; n++)
        {
            char j = (char)('a' + n / 26);
            char k = (char)('a' + n % 26);
            snprintf(ShellCommand, sizeof(ShellCommand), "mv %s/fa/%s/%s_%c%c %s/fa/%s/%s_%d.fa", PathWork, ChrName[i].c_str(),
                    ChrName[i].c_str(), j, k, PathWork, ChrName[i].c_str(),
                    ChrName[i].c_str(), n);
            system(ShellCommand);

            //Add the header to all the FA files after the first file
            if (n > 0)
            {
                snprintf(ShellCommand, sizeof(ShellCommand), "sed -i \"1i>%s\" %s/fa/%s/%s_%d.fa", ChrName[i].c_str(), PathWork,
                        ChrName[i].c_str(), ChrName[i].c_str(), n);
                system(ShellCommand);
            }
        }
        if (Reserve != 0)
        {
            ifstream fp_chr;
            //Get the length of each line in the FA files.
            snprintf(ShellCommand, sizeof(ShellCommand), "wc -L %s/fa/%s.fa > tmp", PathWork, ChrName[i].c_str());
            system(ShellCommand);
            snprintf(ShellCommand, sizeof(ShellCommand), "tmp");
            fp_chr.open(ShellCommand,ios::in);
            getline(fp_chr, Buffer);
            int Maxlen_PreLine = atoi(Buffer.c_str());
            fp_chr.close();
            //Add the reserved value at the end of the segment FA file
            for (int n = 0; n < FileNumber[i] - 1; n++)
            {
                char tar_file[CMD_NUM];
                char from_file[CMD_NUM];
                snprintf(tar_file, sizeof(tar_file), "%s/fa/%s/%s_%d.fa", PathWork, ChrName[i].c_str(), ChrName[i].c_str(), n);
                snprintf(from_file, sizeof(from_file), "%s/fa/%s/%s_%d.fa", PathWork, ChrName[i].c_str(), ChrName[i].c_str(), n + 1);
                AddReserve(tar_file, from_file, (int)(ceil(Reserve / Maxlen_PreLine)));
            }
        }
        for (int n = 0; n < FileNumber[i]; n++)
        {
            snprintf(ShellCommand, sizeof(ShellCommand), "%s faidx %s/fa/%s/%s_%d.fa", PATH_SAMTOOLS.c_str(), PathWork, ChrName[i].c_str(), ChrName[i].c_str(),n);
            system(ShellCommand);
        }
        printf("FA files rename is done.\n");
    }
    remove("tmp");
    long FinishTime = time((time_t*)NULL);
    printf("SegmentFA finish time = %ld\n", FinishTime);
    long RunningTime = FinishTime - StartTime;
    printf("SegmentFA running time = %ld\n", RunningTime);
    return 0;
}