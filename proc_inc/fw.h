//
// Created by KimByoungGook on 2021-03-10.
//

#ifndef FW_H
#define FW_H


#define FORK_POLICY     0
#define FORK_SERVICE    1
#define FORK_COUNT      2

#define MAX_DBFILE_INDEX  99999
#define MAX_DBFILE_SIZE   41943040 //40MB

/** 0 : POLICY
 *  1 : SERVICE
 * **/
#define MAX_FW_PROCESS  2

typedef struct
{

    pid_t parentPid;
    pid_t Pid[MAX_FW_PROCESS];
    int   nForkIdx[MAX_FW_PROCESS];
    int   nLastFileIdx;
    unsigned int   nCfgMaxFileSize;
    int   nCfgMaxFileIndex;
    time_t nCfgLastModify;
    FILE* Fp;

    char szPolicyFilePath[256 +1];
    char szFileName[32+1];
    char szFileFullPath[256 +1];

}_DAP_PROC_FW_INFO;

int                g_nForkIdx;
int                g_ChildExitFlag;
_DAP_PROC_FW_INFO  g_stProcFwInfo;

int fw_FwInit(void);
void fw_SigHandler(void);
void fw_HandleSignal(int sid);
void fw_SigchldHandler(int param_Sig);
int fw_ParentExit(void);
int fw_ChildExit(void);
int fw_GetForkIdx(pid_t param_FindPid);

int fw_MainTask(void);
int fw_ReloadCfgFile(void);
int fw_ForkWork(void);
int fw_GetLastFileName(int param_ForkIdx, char* param_FileName, int param_FileNameSize);
int fw_GetLastFileIndex( char* param_FileName );
//int fw_WriteDataFile(int param_ForkIdx, char* param_Buffer, int param_BufferSize, FILE* param_Fp);
int fw_WriteDataFile(int param_ForkIdx, _DAP_QUEUE_BUF* param_Buffer, int param_BufferSize, FILE* param_Fp);
int fw_GetNextFileSize(int param_ForkIdx);



#endif //FW_H
