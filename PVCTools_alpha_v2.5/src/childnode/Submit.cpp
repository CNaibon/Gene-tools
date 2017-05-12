//
// Created by liujiajun on 2017/4/21.
//

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Submit and calculate VCF results.
//If you need to customize, you should modify the [bamlist] and [falist] in the working directory.
//Command: ./Submit <-w WorkPath>
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Submit.h"
#include "Environment.h"

using namespace std;

int Submit(int argc, char *argv[])
{
    long StartTime = time((time_t*)NULL);
    printf("start time = %ld\n", StartTime);

    char PATH_SAMTOOLS[CMD_NUM];
    GetToolsPath(PATH_SAMTOOLS, "-samtools");
    char PATH_BCFTOOLS[CMD_NUM];
    GetToolsPath(PATH_BCFTOOLS, "-bcftools");
    char PATH_GATK[CMD_NUM];
    GetToolsPath(PATH_GATK, "-gatk");
    char PATH_FREEBAYES[CMD_NUM];
    GetToolsPath(PATH_FREEBAYES, "-freebayes");

    vector<string> ChrName;
    vector<string> SampleName;
    //Command string.
    char ShellCommand[CMD_NUM];
    char *Buffer = NULL;
    size_t Len = FILE_LINE;

    char PathWork[CMD_NUM];
    char Queue[CMD_NUM] = "fatnode";
    char Span[CMD_NUM] = "20";
    string Tool = "samtools";

    for (int i = 0; i < argc; i++)
    {
        string cmd = argv[i];
        if (cmd == "-w")
        {
            snprintf(PathWork, sizeof(PathWork), "%s", argv[i + 1]);
        }
        if (cmd == "-q")
        {
            snprintf(Queue, sizeof(Queue), "%s", argv[i + 1]);
        }
        if (cmd == "-Span")
        {
            snprintf(Span, sizeof(Span), "%s", argv[i + 1]);
        }
        if (cmd == "-T")
        {
            Tool = argv[i + 1];
        }
    }

    //Import BAM list, if you need to customize the list, you should modify the [bamlist], fill in the need to split the BAM file.
    FILE *fp_bam;
    snprintf(ShellCommand, sizeof(ShellCommand), "%s/bamlist", PathWork);
    if ((fp_bam = fopen(ShellCommand, "r")) == NULL)
        exit(-1);
    getline(&Buffer, &Len, fp_bam);
    while (!feof(fp_bam))
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
            SampleName.push_back(Buffer);
        }
        getline(&Buffer, &Len, fp_bam);
    }
    fclose(fp_bam);

    //Import FA list, if you need to customize the list, you should modify the [falist], fill in the need to split the FA file.
    snprintf(ShellCommand, sizeof(ShellCommand), "%s/falist", PathWork);
    FILE *fp_fa;
    if ((fp_fa = fopen(ShellCommand, "r")) == NULL)
        exit(-1);
    getline(&Buffer, &Len, fp_fa);
    while (!feof(fp_fa))
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
        getline(&Buffer, &Len, fp_fa);
    }
    fclose(fp_fa);

    int FileNumber[ChrName.size()];


    //Each chromosome is counted.
#pragma omp parallel for
    for (int i = 0; i < (int)ChrName.size(); ++i)
    {
        char Command[CMD_NUM];
        char *SubBuffer = NULL;
        size_t SubLen = FILE_LINE;

        snprintf(Command, sizeof(Command), "touch %s_tmp", ChrName[i].c_str());
        system(Command);

        FILE *fp_sp;
        //Count the current number of files.
        snprintf(Command, sizeof(Command), "ls -l %s/fa/%s |grep \"^-\"|wc -l > %s_tmp ", PathWork, ChrName[i].c_str(), ChrName[i].c_str());
        system(Command);
        snprintf(Command, sizeof(Command), "%s_tmp", ChrName[i].c_str());
        if ((fp_sp = fopen(Command, "r")) == NULL)
            exit(-1);
        getline(&SubBuffer, &SubLen, fp_sp);
        //The number of files after bisection.
        FileNumber[i] = atoi(SubBuffer);

        snprintf(Command, sizeof(Command), "rm -rf %s_tmp ", ChrName[i].c_str());
        system(Command);

    }

    //Create the relevant directory.
    snprintf(ShellCommand, sizeof(ShellCommand), "mkdir -p %s/out", PathWork);
    system(ShellCommand);
    snprintf(ShellCommand, sizeof(ShellCommand), "mkdir -p %s/err", PathWork);
    system(ShellCommand);
    snprintf(ShellCommand, sizeof(ShellCommand), "mkdir -p %s/sub_script", PathWork);
    system(ShellCommand);
    snprintf(ShellCommand, sizeof(ShellCommand), "mkdir -p %s/vcf", PathWork);
    system(ShellCommand);

#pragma omp parallel for
    for (int i = 0; i < (int)ChrName.size(); i++)
    {
        char Command[CMD_NUM];
        snprintf(Command, sizeof(Command), "mkdir -p %s/vcf/%s", PathWork, ChrName[i].c_str());
        system(Command);
        //Write the commit script.
        for (int k = 0; k < FileNumber[i]; k++)
        {
            FILE *fp_sh;
            snprintf(Command, sizeof(Command), "%s/sub_script/%s_%d_%s.sh", PathWork, ChrName[i].c_str(), k, Tool.c_str());
            if ((fp_sh = fopen(Command, "w")) == NULL)
                exit(-1);
            snprintf(Command, sizeof(Command), "#BSUB -q %s\n", Queue);
            fputs(Command, fp_sh);
            snprintf(Command, sizeof(Command), "#BSUB -J %s_%d_%s\n", ChrName[i].c_str(), k, Tool.c_str());
            fputs(Command, fp_sh);
            snprintf(Command, sizeof(Command), "#BSUB -o %s/out/%s_%d_%s.out\n", PathWork, ChrName[i].c_str(), k, Tool.c_str());
            fputs(Command, fp_sh);
            snprintf(Command, sizeof(Command), "#BSUB -e %s/err/%s_%d_%s.err\n", PathWork, ChrName[i].c_str(), k, Tool.c_str());
            fputs(Command, fp_sh);
            snprintf(Command, sizeof(Command), "#BSUB -n 1\n");
            fputs(Command, fp_sh);
            snprintf(Command, sizeof(Command), "#BSUB -R \"span[ptile=%s]\"\n", Span);
            fputs(Command, fp_sh);

            if (Tool == "samtools")
            {
                snprintf(Command, sizeof(Command), "%s mpileup -u -t DP,AD,ADF -f ", PATH_SAMTOOLS);
                fputs(Command, fp_sh);
                snprintf(Command, sizeof(Command), "%s/fa/%s/%s_%d.fa ", PathWork, ChrName[i].c_str(), ChrName[i].c_str(), k);
                fputs(Command, fp_sh);
                //Write a number of sample small copies.
                for (int n = 0; n < (int)SampleName.size(); n++)
                {
                    snprintf(Command, sizeof(Command), "mkdir -p %s/sample/%s/%s_%s", PathWork, SampleName[n].c_str(), SampleName[n].c_str(), ChrName[i].c_str());
                    system(Command);

                    snprintf(Command, sizeof(Command), "touch %s/sample/%s/%s_%s/%s_%s_%d.bam", PathWork, SampleName[n].c_str(), SampleName[n].c_str(), ChrName[i].c_str(), SampleName[n].c_str(), ChrName[i].c_str(),k);
                    system(Command);

                    snprintf(Command, sizeof(Command), "%s/sample/%s/%s_%s/%s_%s_%d.bam ", PathWork, SampleName[n].c_str(), SampleName[n].c_str(), ChrName[i].c_str(), SampleName[n].c_str(), ChrName[i].c_str(), k);
                    fputs(Command, fp_sh);
                }
                snprintf(Command, sizeof(Command), "| %s call -vmO v -o %s/vcf/%s/%s_%d.var.flt.vcf ", PATH_BCFTOOLS, PathWork, ChrName[i].c_str(), ChrName[i].c_str(), k);
                fputs(Command, fp_sh);
            }
            else if (Tool == "gatk")
            {
                snprintf(Command, sizeof(Command), "java -jar %s -T HaplotypeCaller ", PATH_GATK);
                fputs(Command, fp_sh);
                snprintf(Command, sizeof(Command), "-R %s/fa/%s/%s_%d.fa -nct 1 ", PathWork, ChrName[i].c_str(), ChrName[i].c_str(), k);
                fputs(Command, fp_sh);
                for (int n = 0; n < (int)SampleName.size(); n++)
                {
                    snprintf(Command, sizeof(Command), "mkdir -p %s/sample/%s/%s_%s", PathWork, SampleName[n].c_str(), SampleName[n].c_str(), ChrName[i].c_str());
                    system(Command);

                    snprintf(Command, sizeof(Command), "touch %s/sample/%s/%s_%s/%s_%s_%d.bam", PathWork, SampleName[n].c_str(), SampleName[n].c_str(), ChrName[i].c_str(), SampleName[n].c_str(), ChrName[i].c_str(),k);
                    system(Command);

                    snprintf(Command, sizeof(Command), "-I %s/sample/%s/%s_%s/%s_%s_%d.bam ", PathWork, SampleName[n].c_str(), SampleName[n].c_str(), ChrName[i].c_str(), SampleName[n].c_str(), ChrName[i].c_str(), k);
                    fputs(Command, fp_sh);
                }
                snprintf(Command, sizeof(Command), "-o %s/vcf/%s/%s_%d.vcf ", PathWork, ChrName[i].c_str(), ChrName[i].c_str(), k);
                fputs(Command, fp_sh);
            }
            else if (Tool == "freebayes")
            {
                snprintf(Command, sizeof(Command), "/usr/bin/time -f \"%E\" %s --strict-vcf ", PATH_FREEBAYES);
                fputs(Command, fp_sh);
                snprintf(Command, sizeof(Command), "-f %s/fa/%s/%s_%d.fa ", PathWork, ChrName[i].c_str(), ChrName[i].c_str(), k);
                fputs(Command, fp_sh);
                for (int n = 0; n < (int)SampleName.size(); n++)
                {
                    snprintf(Command, sizeof(Command), "mkdir -p %s/sample/%s/%s_%s", PathWork, SampleName[n].c_str(), SampleName[n].c_str(), ChrName[i].c_str());
                    system(Command);

                    snprintf(Command, sizeof(Command), "touch %s/sample/%s/%s_%s/%s_%s_%d.bam", PathWork, SampleName[n].c_str(), SampleName[n].c_str(), ChrName[i].c_str(), SampleName[n].c_str(), ChrName[i].c_str(),k);
                    system(Command);

                    snprintf(Command, sizeof(Command), "-b %s/sample/%s/%s_%s/%s_%s_%d.bam ", PathWork, SampleName[n].c_str(), SampleName[n].c_str(), ChrName[i].c_str(), SampleName[n].c_str(), ChrName[i].c_str(), k);
                    fputs(Command, fp_sh);
                }
                snprintf(Command, sizeof(Command), "-v %s/vcf/%s/%s_%d.vcf ", PathWork, ChrName[i].c_str(), ChrName[i].c_str(), k);
                fputs(Command, fp_sh);
            }

            fclose(fp_sh);

            // Submit.
            snprintf(Command, sizeof(Command), "bsub < %s/sub_script/%s_%d_%s.sh", PathWork, ChrName[i].c_str(), k, Tool.c_str());
            system(Command);
        }
    }
    printf("The VCF task has been submitted to each node.\n");
    printf("Wait for the VCF task to complete the VCF statistics program after all the calculations have been completed.\n");

    long FinishTime = time((time_t*)NULL);
    printf("finish time = %ld\n", FinishTime);
    long RunningTime = FinishTime - StartTime;
    printf("running time = %ld\n", RunningTime);

    return 0;
}