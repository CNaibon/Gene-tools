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

int SegmentFA(int argc,char *argv[])
{
    long StartTime = time((time_t*)NULL);
    printf("start time = %ld\n", StartTime);

    vector<string> ChrName;
    FILE *fp_Tmp;
    //Command string
    char ShellCommand[CMD_NUM];
    char *Buffer = NULL;
    size_t Len = FILE_LINE;

    //Create a shell command to call the result log file
    system("touch tmp");

    long Limit = 0;
    int SplitNumber;
    char PathWork[CMD_NUM];

    for (int i = 0; i < argc; i++)
    {
        string cmd = argv[i];
        if (cmd == "-w")
        {
            snprintf(PathWork, sizeof(PathWork), "%s", argv[i + 1]);
        }
        if (cmd == "-n")
        {
            SplitNumber = atoi(argv[i + 1]);
        }
        if (cmd == "-lm")
        {
            Limit = atol(argv[i + 1]);
        }
    }

    FILE *fp_FA;
    if(Limit == 0)
    {
        //Import FA list. If you need to customize the list, you should modify [falist], fill in the need to split the fa files.
        snprintf(ShellCommand, sizeof(ShellCommand), "%s/falist", PathWork);
        if ((fp_FA = fopen(ShellCommand, "r")) == NULL)
            exit(-1);
        getline(&Buffer, &Len, fp_FA);
        while (!feof(fp_FA))
        {
            if (strlen(Buffer) != 0)
            {
                for (int i = (int)strlen(Buffer) - 1; i > 0; i--)
                {
                    if (Buffer[i] == '.')
                    {
                        Buffer[i] = '\0';
                        break;
                    }
                }
                ChrName.push_back(Buffer);
            }
            getline(&Buffer, &Len, fp_FA);
        }
        fclose(fp_FA);
    }
    else
    {
        snprintf(ShellCommand, sizeof(ShellCommand), "%s/falist", PathWork);
        if ((fp_FA = fopen(ShellCommand, "w")) == NULL)
            exit(-1);
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
                if (FAFile[i] == '.')
                {
                    Suffix = FAFile + i + 1;
                }
            }
            if (Suffix != "fa") continue;
            //Call the command to query the FA file size.
            snprintf(ShellCommand, sizeof(ShellCommand), "du -h --block-size=M %s/fa/%s > tmp", PathWork, ptr->d_name);
            system(ShellCommand);
            if ((fp_Tmp = fopen("tmp", "r")) == NULL)
                exit(-1);
            getline(&Buffer, &Len, fp_Tmp);
            fclose(fp_Tmp);
            long FASize = atol(Buffer);
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

    int FileNumber[ChrName.size()];

    //Each chromosome is divided
    for (int i = 0; i < (int)ChrName.size(); ++i)
    {
        snprintf(ShellCommand, sizeof(ShellCommand), "du -h --block-size=M %s/fa/%s.fa > tmp ", PathWork, ChrName[i].c_str());
        system(ShellCommand);
        if ((fp_Tmp = fopen("tmp", "r")) == NULL)
            exit(-1);
        getline(&Buffer, &Len, fp_Tmp);
        fclose(fp_Tmp);
        long FASize = atol(Buffer);
        //Calculate the size of each post split.
        long fa_split_piece_size = FASize / SplitNumber;

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
        snprintf(ShellCommand, sizeof(ShellCommand), "ls -l %s/fa/%s |grep \"^-\"|wc -l > tmp ", PathWork, ChrName[i].c_str());
        system(ShellCommand);
        if ((fp_Tmp = fopen("tmp", "r")) == NULL)
            exit(-1);
        getline(&Buffer, &Len, fp_Tmp);
        //The number of files after bisection
        FileNumber[i] = atoi(Buffer);
        fclose(fp_Tmp);
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
        printf("FA files rename is done.\n");
    }
    remove("tmp");

    long FinishTime = time((time_t*)NULL);
    printf("finish time = %ld\n", FinishTime);
    long RunningTime = FinishTime - StartTime;
    printf("running time = %ld\n", RunningTime);

    return 0;
}