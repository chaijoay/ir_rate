///
///
/// FACILITY    : rating 3 sources of ir cdr(TAP, NRTRDE and SCP) and output to one common format
///
/// FILE NAME   : ir_rate.c
///
/// AUTHOR      : Thanakorn Nitipiromchai
///
/// CREATE DATE : 15-May-2019
///
/// CURRENT VERSION NO : 1.0
///
/// LAST RELEASE DATE  : 15-May-2019
///
/// MODIFICATION HISTORY :
///     1.0     15-May-2019     First Version
///
///
#define _XOPEN_SOURCE           700         // Required under GLIBC for nftw()
#define _POSIX_C_SOURCE         200809L
#define _XOPEN_SOURCE_EXTENDED  1

#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <dirent.h>
#include <libgen.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>

#include "minIni.h"
#include "procsig.h"
#include "ir_rate.h"
#include "strlogutl.h"
#include "ir_rate_dbu.h"

#define  TMPSUF     "._tmp"

char gszAppName[SIZE_ITEM_S];
char gszIniFile[SIZE_FULL_NAME];
char gszToday[SIZE_DATE_ONLY+1];
char gszOutFname[SIZE_ITEM_L];

// char *pbuf_rec[NOF_TAP_FLD];
// char *pbuf_nrt[NOF_NRT_FLD];
// char *pbuf_scp[NOF_SCP_FLD];

//char            *pbuf_rec[SIZE_ITEM_M];
char            *pbuf_rec[SIZE_BUFF];

ST_IR_COMMON    gIrCommon;
FILE    *gfpSnap;
int     gnSnapCnt;
int     gnLenPreTap;
int     gnLenSufTap;
int     gnLenPreNrt;
int     gnLenSufNrt;
int     gnLenPreScp;
int     gnLenSufScp;
int     gnLenPreRtb;
int     gnLenSufRtb;
short   gnFileSeq = 0;
time_t  gzLastTimeT = 0;

const char gszIniStrSection[E_NOF_SECTION][SIZE_ITEM_T] = {
    "INPUT",
    "OUTPUT",
    "COMMON",
    "DB_CONNECTION"
};

const char gszIniStrInput[E_NOF_PAR_INPUT][SIZE_ITEM_T] = {
    "TAP_INPUT_DIR",
    "TAP_FILE_PREFIX",
    "TAP_FILE_SUFFIX",
    "NRT_INPUT_DIR",
    "NRT_FILE_PREFIX",
    "NRT_FILE_SUFFIX",
    "SCP_INPUT_DIR",
    "SCP_FILE_PREFIX",
    "SCP_FILE_SUFFIX",
    "RTB_INPUT_DIR",
    "RTB_FILE_PREFIX",
    "RTB_FILE_SUFFIX"
};

const char gszIniStrOutput[E_NOF_PAR_OUTPUT][SIZE_ITEM_T] = {
    "OUTPUT_DIR",
    "OUT_FILE_PREFIX",
    "OUT_FILE_SUFFIX"
};

const char gszIniStrCommon[E_NOF_PAR_COMMON][SIZE_ITEM_T] = {
    "DEF_IDD_ACC",
    "REJ_INVALID",
    "REJ_OUT_DIR",
    "ALRT_IMSI_NO_MOB",
    "ALRT_IMSI_DIR",
    "ALRT_PMN_MISMATCH",
    "ALRT_PMN_DIR",
    "ALRT_DBCON_FAIL",
    "ALRT_DBCON_DIR",
    "TMP_DIR",
    "BACKUP",
    "BACKUP_DIR",
    "LOG_DIR",
    "LOG_LEVEL",
    "SLEEP_SECOND"
};

const char gszIniStrDbConn[E_NOF_PAR_DBCONN][SIZE_ITEM_T] = {
    "SUB_USER_NAME",
    "SUB_PASSWORD",
    "SUB_DB_SID",
    "SFF_USER_NAME",
    "SFF_PASSWORD",
    "SFF_DB_SID",
    "PPI_USER_NAME",
    "PPI_PASSWORD",
    "PPI_DB_SID",
    "RETRY_COUNT",
    "RETRY_WAIT"
};

char gszIniParInput[E_NOF_PAR_INPUT][SIZE_ITEM_L];
char gszIniParOutput[E_NOF_PAR_OUTPUT][SIZE_ITEM_L];
char gszIniParCommon[E_NOF_PAR_COMMON][SIZE_ITEM_L];
char gszIniParDbConn[E_NOF_PAR_DBCONN][SIZE_ITEM_L];

int main(int argc, char *argv[])
{
    FILE *ifp = NULL;
    char szSnap[SIZE_ITEM_L], snp_line[SIZE_BUFF];
    int retryBldSnap = 3, nInpFileCntDay = 0, nInpFileCntRnd = 0;
    time_t t_bat_start = 0, t_bat_stop = 0;

    memset(gszAppName, 0x00, sizeof(gszAppName));
    memset(gszIniFile, 0x00, sizeof(gszIniFile));
    memset(gszToday, 0x00, sizeof(gszToday));

    // 1. read ini file
    if ( readConfig(argc, argv) != SUCCESS ) {
        return EXIT_FAILURE;
    }

    if ( procLock(gszAppName, E_CHK) != SUCCESS ) {
        fprintf(stderr, "another instance of %s is running\n", gszAppName);
        return EXIT_FAILURE;
    }

    if ( handleSignal() != SUCCESS ) {
        fprintf(stderr, "init handle signal failed: %s\n", getSigInfoStr());
        return EXIT_FAILURE;
    }

    if ( startLogging(gszIniParCommon[E_LOG_DIR], gszAppName, atoi(gszIniParCommon[E_LOG_LEVEL])) != SUCCESS ) {
       return EXIT_FAILURE;
    }

    if ( validateIni() == FAILED ) {
        return EXIT_FAILURE;
    }
    logHeader();
    
    char ir_file[SIZE_ITEM_L];  memset(ir_file, 0x00, sizeof(ir_file));
    char ir_type[10];           memset(ir_type, 0x00, sizeof(ir_type));
    long cont_pos = 0L;

    cont_pos = checkPoint(NULL, ir_file, ir_type, gszIniParCommon[E_TMP_DIR], gszAppName, E_CHK);

    strcpy(gszToday, getSysDTM(DTM_DATE_ONLY));
    // Main processing loop
    while ( TRUE ) {

        procLock(gszAppName, E_SET);
        
        if ( isTerminated() == TRUE ) {
            break;
        }

        // main process flow:
        // 1. build snapshot file -> list all files to be processed.
        // 2. connect to dbs (and also retry if any)
        // 3. recognise and reformat file according to source type (TAP, NRT or SCP)
        // 4. rating and fill additional field to complete common format
        // 5. write out common rated common format to further merging process
        // 6. disconnect from dbs to release resouce then give a process to rest (sleep)
        // 7. start over from step 1
        
        memset(szSnap, 0x00, sizeof(szSnap));
        sprintf(szSnap, "%s/%s.snap", gszIniParCommon[E_TMP_DIR], gszAppName);
        if ( cont_pos <= 0 ) {  // skip buildsnap if it need to process from last time check point
            if ( buildSnapFile(szSnap) != SUCCESS ) {
                if ( --retryBldSnap <= 0 ) {
                    fprintf(stderr, "retry build snap exceeded\n");
                    break;
                }
                sleep(10);
                continue;
            }
            retryBldSnap = 3;
        }

        if ( gnSnapCnt > 0 || cont_pos > 0 ) {
#if 1
            if ( manageMapTab() != SUCCESS ) {
                break;
            }
#endif
            if ( (ifp = fopen(szSnap, "r")) == NULL ) {
                writeLog(LOG_SYS, "unable to open %s for reading (%s)", szSnap, strerror(errno));
                break;
            }
            else {
#if 1
                if ( connectDbSff(gszIniParDbConn[E_SFF_USER], gszIniParDbConn[E_SFF_PASSWORD], gszIniParDbConn[E_SFF_DB_SID], atoi(gszIniParDbConn[E_RETRY_COUNT]), atoi(gszIniParDbConn[E_RETRY_WAIT])) != SUCCESS ) {
                    if ( *gszIniParCommon[E_ALRT_DB] == 'Y' ) {
                        wrtAlrtDbConnFail(gszIniParCommon[E_ALRT_DB_DIR], gszToday, gszIniParDbConn[E_SFF_DB_SID]);
                    }
                    break;
                }
#endif
#if 1
                if ( connectDbPpi(gszIniParDbConn[E_PPI_USER], gszIniParDbConn[E_PPI_PASSWORD], gszIniParDbConn[E_PPI_DB_SID], atoi(gszIniParDbConn[E_RETRY_COUNT]), atoi(gszIniParDbConn[E_RETRY_WAIT])) != SUCCESS ) {
                    writeLog(LOG_ERR, "connectDbPpi failed");
                    if ( *gszIniParCommon[E_ALRT_DB] == 'Y' ) {
                        wrtAlrtDbConnFail(gszIniParCommon[E_ALRT_DB_DIR], gszToday, gszIniParDbConn[E_PPI_DB_SID]);
                    }
                    break;
                }
#endif

                if ( cont_pos > 0 ) {   // continue from last time first 
                    writeLog(LOG_INF, "continue process %s from last time", ir_file);
                    procSynFiles(dirname(ir_file), basename(ir_file), ir_type, cont_pos);
                    cont_pos = 0;
                    continue;           // back to build snap to continue normal loop
                }

                nInpFileCntRnd = 0;
                t_bat_start = time(NULL);
                while ( fgets(snp_line, sizeof(snp_line), ifp) ) {
                    
                    if ( isTerminated() == TRUE ) {
                        break;
                    }

                    trimStr(snp_line);  // snap record format => <path>|<filename>
                    char sdir[SIZE_ITEM_M], sfname[SIZE_ITEM_M], irtype[10];
                    memset(sdir, 0x00, sizeof(sdir));
                    memset(sfname, 0x00, sizeof(sfname));
                    memset(irtype, 0x00, sizeof(irtype));

                    getTokenItem(snp_line, 1, '|', irtype);
                    getTokenItem(snp_line, 2, '|', sdir);
                    getTokenItem(snp_line, 3, '|', sfname);

                    procSynFiles(sdir, sfname, irtype, 0L);

                    nInpFileCntDay++;
                    nInpFileCntRnd++;
                }
                t_bat_stop = time(NULL);
                writeLog(LOG_INF, "total processed files for this round=%d round_time_used=%d sec", nInpFileCntRnd, (t_bat_stop - t_bat_start));

                fclose(ifp);

                disconnSff(gszIniParDbConn[E_SFF_DB_SID]);
                disconnPpi(gszIniParDbConn[E_PPI_DB_SID]);

            }
        }

        if ( isTerminated() == TRUE ) {
            break;
        }
        else {
            writeLog(LOG_INF, "sleep %s sec", gszIniParCommon[E_SLEEP_SEC]);
            sleep(atoi(gszIniParCommon[E_SLEEP_SEC]));
        }

        if ( strcmp(gszToday, getSysDTM(DTM_DATE_ONLY)) ) {
            writeLog(LOG_INF, "total processed files for today=%d", nInpFileCntDay);
            strcpy(gszToday, getSysDTM(DTM_DATE_ONLY));
            manageLogFile();
            nInpFileCntDay = 0;
        }

    }
    procLock(gszAppName, E_CLR);
    freeTab();
    writeLog(LOG_INF, "%s", getSigInfoStr());
    writeLog(LOG_INF, "------- %s process completely stop -------", _APP_NAME_);
    stopLogging();

    return EXIT_SUCCESS;

}

int buildSnapFile(const char *snapfile)
{
    char cmd[SIZE_BUFF];
    gnSnapCnt = 0;

    gnLenPreTap = strlen(gszIniParInput[E_TAP_FPREF]);
    gnLenSufTap = strlen(gszIniParInput[E_TAP_FSUFF]);
    gnLenPreNrt = strlen(gszIniParInput[E_NRT_FPREF]);
    gnLenSufNrt = strlen(gszIniParInput[E_NRT_FSUFF]);
    gnLenPreScp = strlen(gszIniParInput[E_SCP_FPREF]);
    gnLenSufScp = strlen(gszIniParInput[E_SCP_FSUFF]);
    gnLenPreRtb = strlen(gszIniParInput[E_RTB_FPREF]);
    gnLenSufRtb = strlen(gszIniParInput[E_RTB_FSUFF]);

    // open snap file for writing
    if ( (gfpSnap = fopen(snapfile, "w")) == NULL ) {
        writeLog(LOG_SYS, "unable to open %s for writing: %s\n", snapfile, strerror(errno));
        return FAILED;
    }

    // recursively walk through directories and file and check matching
    if ( *gszIniParInput[E_TAP_INP_DIR] != '\0' ) {
        writeLog(LOG_INF, "scaning sync file in directory %s", gszIniParInput[E_TAP_INP_DIR]);
        if ( nftw(gszIniParInput[E_TAP_INP_DIR], _chkTapFile, 32, FTW_DEPTH) ) {
            writeLog(LOG_SYS, "unable to read path %s: %s\n", gszIniParInput[E_TAP_INP_DIR], strerror(errno));
            fclose(gfpSnap);
            gfpSnap = NULL;
            return FAILED;
        }
    }

    // recursively walk through directories and file and check matching
    if ( *gszIniParInput[E_NRT_INP_DIR] != '\0' ) {
        writeLog(LOG_INF, "scaning sync file in directory %s", gszIniParInput[E_NRT_INP_DIR]);
        if ( nftw(gszIniParInput[E_NRT_INP_DIR], _chkNrtFile, 32, FTW_DEPTH) ) {
            writeLog(LOG_SYS, "unable to read path %s: %s\n", gszIniParInput[E_NRT_INP_DIR], strerror(errno));
            fclose(gfpSnap);
            gfpSnap = NULL;
            return FAILED;
        }
    }

    // recursively walk through directories and file and check matching
    if ( *gszIniParInput[E_SCP_INP_DIR] != '\0' ) {
        writeLog(LOG_INF, "scaning sync file in directory %s", gszIniParInput[E_SCP_INP_DIR]);
        if ( nftw(gszIniParInput[E_SCP_INP_DIR], _chkScpFile, 32, FTW_DEPTH) ) {
            writeLog(LOG_SYS, "unable to read path %s: %s\n", gszIniParInput[E_SCP_INP_DIR], strerror(errno));
            fclose(gfpSnap);
            gfpSnap = NULL;
            return FAILED;
        }
    }

    // recursively walk through directories and file and check matching
    if ( *gszIniParInput[E_RTB_INP_DIR] != '\0' ) {
        writeLog(LOG_INF, "scaning sync file in directory %s", gszIniParInput[E_RTB_INP_DIR]);
        if ( nftw(gszIniParInput[E_RTB_INP_DIR], _chkRtbFile, 32, FTW_DEPTH) ) {
            writeLog(LOG_SYS, "unable to read path %s: %s\n", gszIniParInput[E_RTB_INP_DIR], strerror(errno));
            fclose(gfpSnap);
            gfpSnap = NULL;
            return FAILED;
        }
    }

    fclose(gfpSnap);
    gfpSnap = NULL;

    // if there are sync files then sort the snap file
    if ( gnSnapCnt > 0 ) {
        memset(cmd, 0x00, sizeof(cmd));
        sprintf(cmd, "sort -T %s -u %s > %s.tmp 2>/dev/null", gszIniParCommon[E_TMP_DIR], snapfile, snapfile);
writeLog(LOG_DB3, "buildSnapFile cmd '%s'", cmd);
        if ( system(cmd) != SUCCESS ) {
            writeLog(LOG_SYS, "cannot sort file %s (%s)", snapfile, strerror(errno));
            sprintf(cmd, "rm -f %s %s.tmp", snapfile, snapfile);
            system(cmd);
            return FAILED;
        }
        sprintf(cmd, "mv %s.tmp %s 2>/dev/null", snapfile, snapfile);
writeLog(LOG_DB3, "buildSnapFile cmd '%s'", cmd);
        system(cmd);
    }
    else {
        writeLog(LOG_INF, "no input file");
    }

    return SUCCESS;

}

int _chkTapFile(const char *fpath, const struct stat *info, int typeflag, struct FTW *ftwbuf)
{

    const char *fname = fpath + ftwbuf->base;
    int fname_len = strlen(fname);
    char path_only[SIZE_ITEM_L];

    if ( typeflag != FTW_F && typeflag != FTW_SL && typeflag != FTW_SLN )
        return 0;

    if ( strncmp(fname, gszIniParInput[E_TAP_FPREF], gnLenPreTap) != 0 ) {
        return 0;
    }

    if ( strcmp(fname + (fname_len - gnLenSufTap), gszIniParInput[E_TAP_FSUFF]) != 0 ) {
        return 0;
    }

    if ( !(info->st_mode & (S_IRUSR|S_IRGRP|S_IROTH)) ) {
        writeLog(LOG_WRN, "no read permission for %s skipped", fname);
        return 0;
    }

    memset(path_only, 0x00, sizeof(path_only));
    strncpy(path_only, fpath, ftwbuf->base - 1);

    gnSnapCnt++;
    fprintf(gfpSnap, "%s|%s|%s\n", gszIniParInput[E_TAP_FPREF], path_only, fname);    // write snap output format -> <IR_TYPE>|<DIR>|<FILE>
    return 0;

}

int _chkNrtFile(const char *fpath, const struct stat *info, int typeflag, struct FTW *ftwbuf)
{

    const char *fname = fpath + ftwbuf->base;
    int fname_len = strlen(fname);
    char path_only[SIZE_ITEM_L];

    if ( typeflag != FTW_F && typeflag != FTW_SL && typeflag != FTW_SLN )
        return 0;

    if ( strncmp(fname, gszIniParInput[E_NRT_FPREF], gnLenPreNrt) != 0 ) {
        return 0;
    }

    if ( strcmp(fname + (fname_len - gnLenSufNrt), gszIniParInput[E_NRT_FSUFF]) != 0 ) {
        return 0;
    }

    if ( !(info->st_mode & (S_IRUSR|S_IRGRP|S_IROTH)) ) {
        writeLog(LOG_WRN, "no read permission for %s skipped", fname);
        return 0;
    }

    memset(path_only, 0x00, sizeof(path_only));
    strncpy(path_only, fpath, ftwbuf->base - 1);

    gnSnapCnt++;
    fprintf(gfpSnap, "%s|%s|%s\n", gszIniParInput[E_NRT_FPREF], path_only, fname);  // write snap output format -> <IR_TYPE>|<DIR>|<FILE>
    return 0;

}

int _chkScpFile(const char *fpath, const struct stat *info, int typeflag, struct FTW *ftwbuf)
{

    const char *fname = fpath + ftwbuf->base;
    int fname_len = strlen(fname);
    char path_only[SIZE_ITEM_L];

    if ( typeflag != FTW_F && typeflag != FTW_SL && typeflag != FTW_SLN )
        return 0;

    if ( strncmp(fname, gszIniParInput[E_SCP_FPREF], gnLenPreScp) != 0 ) {
        return 0;
    }

    if ( strcmp(fname + (fname_len - gnLenSufScp), gszIniParInput[E_SCP_FSUFF]) != 0 ) {
        return 0;
    }

    if ( !(info->st_mode & (S_IRUSR|S_IRGRP|S_IROTH)) ) {
        writeLog(LOG_WRN, "no read permission for %s skipped", fname);
        return 0;
    }

    memset(path_only, 0x00, sizeof(path_only));
    strncpy(path_only, fpath, ftwbuf->base - 1);

    gnSnapCnt++;
    fprintf(gfpSnap, "%s|%s|%s\n", gszIniParInput[E_SCP_FPREF], path_only, fname);    // write snap output format -> <IR_TYPE>|<DIR>|<FILE>
    return 0;

}

int _chkRtbFile(const char *fpath, const struct stat *info, int typeflag, struct FTW *ftwbuf)
{

    const char *fname = fpath + ftwbuf->base;
    int fname_len = strlen(fname);
    char path_only[SIZE_ITEM_L];

    if ( typeflag != FTW_F && typeflag != FTW_SL && typeflag != FTW_SLN )
        return 0;

    if ( strncmp(fname, gszIniParInput[E_RTB_FPREF], gnLenPreRtb) != 0 ) {
        return 0;
    }

    if ( strcmp(fname + (fname_len - gnLenSufRtb), gszIniParInput[E_RTB_FSUFF]) != 0 ) {
        return 0;
    }

    if ( !(info->st_mode & (S_IRUSR|S_IRGRP|S_IROTH)) ) {
        writeLog(LOG_WRN, "no read permission for %s skipped", fname);
        return 0;
    }

    memset(path_only, 0x00, sizeof(path_only));
    strncpy(path_only, fpath, ftwbuf->base - 1);

    gnSnapCnt++;
    fprintf(gfpSnap, "%s|%s|%s\n", gszIniParInput[E_RTB_FPREF], path_only, fname);    // write snap output format -> <IR_TYPE>|<DIR>|<FILE>
    return 0;

}

int readConfig(int argc, char *argv[])
{

    char appPath[SIZE_ITEM_L];
    int key, i;

    memset(gszIniFile, 0x00, sizeof(gszIniFile));
    memset(gszAppName, 0x00, sizeof(gszAppName));

    memset(gszIniParInput,  0x00, sizeof(gszIniParInput));
    memset(gszIniParOutput, 0x00, sizeof(gszIniParOutput));
    memset(gszIniParCommon, 0x00, sizeof(gszIniParCommon));
    memset(gszIniParDbConn, 0x00, sizeof(gszIniParDbConn));

    strcpy(appPath, argv[0]);
    char *p = strrchr(appPath, '/');
    *p = '\0';

    for ( i = 1; i < argc; i++ ) {
        if ( strcmp(argv[i], "-n") == 0 ) {     // specified ini file
            strcpy(gszIniFile, argv[++i]);
        }
        else if ( strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0 ) {
            printUsage();
            return FAILED;
        }
        else if ( strcmp(argv[i], "-mkini") == 0 ) {
            makeIni();
            return FAILED;
        }
    }

    if ( gszIniFile[0] == '\0' ) {
        sprintf(gszIniFile, "%s/%s.ini", appPath, _APP_NAME_);
    }
    sprintf(gszAppName, "%s", _APP_NAME_);

    if ( access(gszIniFile, F_OK|R_OK) != SUCCESS ) {
        fprintf(stderr, "unable to access ini file %s (%s)\n", gszIniFile, strerror(errno));
        return FAILED;
    }

    // Read config of INPUT Section
    for ( key = 0; key < E_NOF_PAR_INPUT; key++ ) {
        ini_gets(gszIniStrSection[E_INPUT], gszIniStrInput[key], "NA", gszIniParInput[key], sizeof(gszIniParInput[key]), gszIniFile);
    }

    // Read config of OUTPUT Section
    for ( key = 0; key < E_NOF_PAR_OUTPUT; key++ ) {
        ini_gets(gszIniStrSection[E_OUTPUT], gszIniStrOutput[key], "NA", gszIniParOutput[key], sizeof(gszIniParOutput[key]), gszIniFile);
    }

    // Read config of COMMON Section
    for ( key = 0; key < E_NOF_PAR_COMMON; key++ ) {
        ini_gets(gszIniStrSection[E_COMMON], gszIniStrCommon[key], "NA", gszIniParCommon[key], sizeof(gszIniParCommon[key]), gszIniFile);
    }

    // Read config of DB Connection Section
    for ( key = 0; key < E_NOF_PAR_DBCONN; key++ ) {
        ini_gets(gszIniStrSection[E_DBCONN], gszIniStrDbConn[key], "NA", gszIniParDbConn[key], sizeof(gszIniParDbConn[key]), gszIniFile);
    }

    return SUCCESS;

}

void logHeader()
{
    writeLog(LOG_INF, "---- Start %s (v%s) with following parameters ----", _APP_NAME_, _APP_VERS_);
    // print out all ini file
    ini_browse(_ini_callback, NULL, gszIniFile);
}

void printUsage()
{
    fprintf(stderr, "\nusage: %s version %s\n", _APP_NAME_, _APP_VERS_);
    fprintf(stderr, "\trating and output common format for TAP, NRT and SCP\n\n");
    fprintf(stderr, "%s.exe [-n <ini_file>] [-mkini]\n", _APP_NAME_);
    fprintf(stderr, "\tini_file\tto specify ini file other than default ini\n");
    fprintf(stderr, "\t-mkini\t\tto create ini template\n");
    fprintf(stderr, "\n");

}

int validateIni()
{
    int result = SUCCESS;

    // ----- Input Section -----
    if ( *gszIniParInput[E_TAP_INP_DIR] != '\0' ) {
        if ( access(gszIniParInput[E_TAP_INP_DIR], F_OK|R_OK) != SUCCESS ) {
            result = FAILED;
            fprintf(stderr, "unable to access %s %s (%s)\n", gszIniStrInput[E_TAP_INP_DIR], gszIniParInput[E_TAP_INP_DIR], strerror(errno));
        }
    }
    if ( *gszIniParInput[E_NRT_INP_DIR] != '\0' ) {
        if ( access(gszIniParInput[E_NRT_INP_DIR], F_OK|R_OK) != SUCCESS ) {
            result = FAILED;
            fprintf(stderr, "unable to access %s %s (%s)\n", gszIniStrInput[E_NRT_INP_DIR], gszIniParInput[E_NRT_INP_DIR], strerror(errno));
        }
    }
    if ( *gszIniParInput[E_SCP_INP_DIR] != '\0' ) {
        if ( access(gszIniParInput[E_SCP_INP_DIR], F_OK|R_OK) != SUCCESS ) {
            result = FAILED;
            fprintf(stderr, "unable to access %s %s (%s)\n", gszIniStrInput[E_SCP_INP_DIR], gszIniParInput[E_SCP_INP_DIR], strerror(errno));
        }
    }
    if ( *gszIniParInput[E_RTB_INP_DIR] != '\0' ) {
        if ( access(gszIniParInput[E_RTB_INP_DIR], F_OK|R_OK) != SUCCESS ) {
            result = FAILED;
            fprintf(stderr, "unable to access %s %s (%s)\n", gszIniStrInput[E_RTB_INP_DIR], gszIniParInput[E_RTB_INP_DIR], strerror(errno));
        }
    }

    // ----- Output Section -----
    if ( access(gszIniParOutput[E_OUT_DIR], F_OK|R_OK) != SUCCESS ) {
        result = FAILED;
        fprintf(stderr, "unable to access %s %s (%s)\n", gszIniStrOutput[E_OUT_DIR], gszIniParOutput[E_OUT_DIR], strerror(errno));
    }

    // ----- Common Section -----
    if ( *gszIniParCommon[E_DEF_IDD_ACC] == '\0' || strcmp(gszIniParCommon[E_DEF_IDD_ACC], "NA") == 0 ) {
        result = FAILED;
        fprintf(stderr, "invalid %s '%s'\n", gszIniStrCommon[E_DEF_IDD_ACC], gszIniParCommon[E_DEF_IDD_ACC]);
    }
    if ( *gszIniParCommon[E_REJ_INVALID] == 'Y' || *gszIniParCommon[E_REJ_INVALID] == 'y' ) {
        strcpy(gszIniParCommon[E_REJ_INVALID], "Y");
        if ( access(gszIniParCommon[E_REJ_OUT_DIR], F_OK|R_OK) != SUCCESS ) {
            result = FAILED;
            fprintf(stderr, "unable to access %s %s (%s)\n", gszIniStrCommon[E_REJ_OUT_DIR], gszIniParCommon[E_REJ_OUT_DIR], strerror(errno));
        }
    }
    if ( *gszIniParCommon[E_ALRT_IMSI] == 'Y' || *gszIniParCommon[E_ALRT_IMSI] == 'y' ) {
        strcpy(gszIniParCommon[E_ALRT_IMSI], "Y");
        if ( access(gszIniParCommon[E_ALRT_IMSI_DIR], F_OK|R_OK) != SUCCESS ) {
            result = FAILED;
            fprintf(stderr, "unable to access %s %s (%s)\n", gszIniStrCommon[E_ALRT_IMSI_DIR], gszIniParCommon[E_ALRT_IMSI_DIR], strerror(errno));
        }
    }
    if ( *gszIniParCommon[E_ALRT_PMN] == 'Y' || *gszIniParCommon[E_ALRT_PMN] == 'y' ) {
        strcpy(gszIniParCommon[E_ALRT_PMN], "Y");
        if ( access(gszIniParCommon[E_ALRT_PMN_DIR], F_OK|R_OK) != SUCCESS ) {
            result = FAILED;
            fprintf(stderr, "unable to access %s %s (%s)\n", gszIniStrCommon[E_ALRT_PMN_DIR], gszIniParCommon[E_ALRT_PMN_DIR], strerror(errno));
        }
    }
    if ( *gszIniParCommon[E_ALRT_DB] == 'Y' || *gszIniParCommon[E_ALRT_DB] == 'y' ) {
        strcpy(gszIniParCommon[E_ALRT_DB], "Y");
        if ( access(gszIniParCommon[E_ALRT_DB_DIR], F_OK|R_OK) != SUCCESS ) {
            result = FAILED;
            fprintf(stderr, "unable to access %s %s (%s)\n", gszIniStrCommon[E_ALRT_DB_DIR], gszIniParCommon[E_ALRT_DB_DIR], strerror(errno));
        }
    }
    if ( access(gszIniParCommon[E_TMP_DIR], F_OK|R_OK) != SUCCESS ) {
        result = FAILED;
        fprintf(stderr, "unable to access %s %s (%s)\n", gszIniStrCommon[E_TMP_DIR], gszIniParCommon[E_TMP_DIR], strerror(errno));
    }
    if ( *gszIniParCommon[E_BCKUP] == 'Y' || *gszIniParCommon[E_BCKUP] == 'y' ) {
        strcpy(gszIniParCommon[E_BCKUP], "Y");
        if ( access(gszIniParCommon[E_BCKUP_DIR], F_OK|R_OK) != SUCCESS ) {
            result = FAILED;
            fprintf(stderr, "unable to access %s %s (%s)\n", gszIniStrCommon[E_BCKUP_DIR], gszIniParCommon[E_BCKUP_DIR], strerror(errno));
        }
    }
    if ( access(gszIniParCommon[E_LOG_DIR], F_OK|R_OK) != SUCCESS ) {
        result = FAILED;
        fprintf(stderr, "unable to access %s %s (%s)\n", gszIniStrCommon[E_LOG_DIR], gszIniParCommon[E_LOG_DIR], strerror(errno));
    }
    if ( atoi(gszIniParCommon[E_SLEEP_SEC]) <= 0 ) {
        result = FAILED;
        fprintf(stderr, "%s must be > 0 (%s)\n", gszIniStrCommon[E_SLEEP_SEC], gszIniParCommon[E_SLEEP_SEC]);
    }

    // ----- Db Connection Section -----
    if ( *gszIniParDbConn[E_SUB_USER] == '\0' || strcmp(gszIniParDbConn[E_SUB_USER], "NA") == 0 ) {
        result = FAILED;
        fprintf(stderr, "invalid %s '%s'\n", gszIniStrDbConn[E_SUB_USER], gszIniParDbConn[E_SUB_USER]);
    }
    if ( *gszIniParDbConn[E_SUB_PASSWORD] == '\0' || strcmp(gszIniParDbConn[E_SUB_PASSWORD], "NA") == 0 ) {
        result = FAILED;
        fprintf(stderr, "invalid %s '%s'\n", gszIniStrDbConn[E_SUB_PASSWORD], gszIniParDbConn[E_SUB_PASSWORD]);
    }
    if ( *gszIniParDbConn[E_SUB_DB_SID] == '\0' || strcmp(gszIniParDbConn[E_SUB_DB_SID], "NA") == 0 ) {
        result = FAILED;
        fprintf(stderr, "invalid %s '%s'\n", gszIniStrDbConn[E_SUB_DB_SID], gszIniParDbConn[E_SUB_DB_SID]);
    }
    if ( *gszIniParDbConn[E_SFF_USER] == '\0' || strcmp(gszIniParDbConn[E_SFF_USER], "NA") == 0 ) {
        result = FAILED;
        fprintf(stderr, "invalid %s '%s'\n", gszIniStrDbConn[E_SFF_USER], gszIniParDbConn[E_SFF_USER]);
    }
    if ( *gszIniParDbConn[E_SFF_PASSWORD] == '\0' || strcmp(gszIniParDbConn[E_SFF_PASSWORD], "NA") == 0 ) {
        result = FAILED;
        fprintf(stderr, "invalid %s '%s'\n", gszIniStrDbConn[E_SFF_PASSWORD], gszIniParDbConn[E_SFF_PASSWORD]);
    }
    if ( *gszIniParDbConn[E_SFF_DB_SID] == '\0' || strcmp(gszIniParDbConn[E_SFF_DB_SID], "NA") == 0 ) {
        result = FAILED;
        fprintf(stderr, "invalid %s '%s'\n", gszIniStrDbConn[E_SFF_DB_SID], gszIniParDbConn[E_SFF_DB_SID]);
    }
    if ( *gszIniParDbConn[E_PPI_USER] == '\0' || strcmp(gszIniParDbConn[E_PPI_USER], "NA") == 0 ) {
        result = FAILED;
        fprintf(stderr, "invalid %s '%s'\n", gszIniStrDbConn[E_PPI_USER], gszIniParDbConn[E_PPI_USER]);
    }
    if ( *gszIniParDbConn[E_PPI_PASSWORD] == '\0' || strcmp(gszIniParDbConn[E_PPI_PASSWORD], "NA") == 0 ) {
        result = FAILED;
        fprintf(stderr, "invalid %s '%s'\n", gszIniStrDbConn[E_PPI_PASSWORD], gszIniParDbConn[E_PPI_PASSWORD]);
    }
    if ( *gszIniParDbConn[E_PPI_DB_SID] == '\0' || strcmp(gszIniParDbConn[E_PPI_DB_SID], "NA") == 0 ) {
        result = FAILED;
        fprintf(stderr, "invalid %s '%s'\n", gszIniStrDbConn[E_PPI_DB_SID], gszIniParDbConn[E_PPI_DB_SID]);
    }
    if ( atoi(gszIniParDbConn[E_RETRY_COUNT]) <= 0 ) {
        result = FAILED;
        fprintf(stderr, "%s must be > 0 (%s)\n", gszIniStrDbConn[E_RETRY_COUNT], gszIniParDbConn[E_RETRY_COUNT]);
    }
    if ( atoi(gszIniParDbConn[E_RETRY_WAIT]) <= 0 ) {
        result = FAILED;
        fprintf(stderr, "%s must be > 0 (%s)\n", gszIniStrDbConn[E_RETRY_WAIT], gszIniParDbConn[E_RETRY_WAIT]);
    }
    return result;

}

int _ini_callback(const char *section, const char *key, const char *value, void *userdata)
{
    if ( strstr(key, "PASSWORD") ) {
        writeLog(LOG_INF, "[%s]\t%s = ********", section, key);
    }
    else {
        writeLog(LOG_INF, "[%s]\t%s = %s", section, key, value);
    }
    return 1;
}

void procSynFiles(const char *dir, const char *fname, const char *ir_type, long cont_pos)
{

    FILE *ifp_ir = NULL, *ofp_ir = NULL, *ofp_rej = NULL;
    FILE *ofp_imsi = NULL, *ofp_pmn = NULL;
    char full_ir_name[SIZE_ITEM_L], ofile_dtm[SIZE_DATE_TIME+1];
    char read_rec[SIZE_BUFF_20X], read_rec_ori[SIZE_BUFF_20X], cSep;
    char rej_msg[SIZE_BUFF_20X];
    int ir_num_field = 0, parse_field_cnt = 0;
    int rate_result = 0, line_cnt = 0;
    int cntr_imsi = 0, cntr_pmn = 0, cntr_wrt = 0, cntr_rej = 0;
    time_t t_start = 0, t_stop = 0;

    memset(full_ir_name, 0x00, sizeof(full_ir_name));
    memset(read_rec, 0x00, sizeof(read_rec));
    memset(ofile_dtm, 0x00, sizeof(ofile_dtm));
    strcpy(ofile_dtm, getSysDTM(DTM_DATE_TIME));

    sprintf(full_ir_name, "%s/%s", dir, fname);
    if ( (ifp_ir = fopen(full_ir_name, "r")) == NULL ) {
        writeLog(LOG_SYS, "unable to open read %s (%s)", full_ir_name, strerror(errno));
        return;
    }
    else {
        writeLog(LOG_INF, "processing file %s", fname);

        t_start = time(NULL);
        if ( cont_pos > 0 ) {
            fseek(ifp_ir, cont_pos, SEEK_SET);
        }
        while ( fgets(read_rec, sizeof(read_rec), ifp_ir) ) {

            memset(pbuf_rec, 0x00, sizeof(pbuf_rec));
            memset(&gIrCommon, 0x00, sizeof(gIrCommon));
            trimStr(read_rec);
            memset(read_rec_ori, 0x00, sizeof(read_rec_ori));
            
            // safe original read record for later use, since getTokenAll modifies input string.
            strcpy(read_rec_ori, read_rec);
            line_cnt++;

            if ( strcmp(ir_type, gszIniParInput[E_TAP_FPREF]) == 0 ) {
                ir_num_field = NOF_TAP_FLD;
                cSep = '#';
                verifyField = verifyInpFieldTap;
            }
            else if ( strcmp(ir_type, gszIniParInput[E_NRT_FPREF]) == 0 ) {
                ir_num_field = NOF_NRT_FLD;
                cSep = '|';
                verifyField = verifyInpFieldNrt;
            }
            else if ( strcmp(ir_type, gszIniParInput[E_SCP_FPREF]) == 0 ) {
                ir_num_field = 800;  /* NOF_SCP_FLD; */
                cSep = '|';
                verifyField = verifyInpFieldScp;
            }
            else {
                ir_num_field = NOF_RTB_FLD;
                cSep = '|';
                verifyField = verifyInpFieldRtb;
            }

            // parse field
            if ( (parse_field_cnt = getTokenAll(pbuf_rec, ir_num_field, read_rec, cSep)) < ir_num_field ) {
                if ( *gszIniParCommon[E_REJ_INVALID] == 'Y' ) {
                    memset(rej_msg, 0x00, sizeof(rej_msg));
                    sprintf(rej_msg, "invalid num field %d expected %d | %s", parse_field_cnt, ir_num_field, read_rec_ori);
                    wrtOutReject(gszIniParCommon[E_REJ_OUT_DIR], fname, &ofp_rej, rej_msg);
                    cntr_rej++;
                }
                continue;
            }

            // do validation and ratinge here ...
            memset(rej_msg, 0x00, sizeof(rej_msg));
            if ( !verifyField(pbuf_rec, ir_num_field, fname, rej_msg) ) {
                if ( *gszIniParCommon[E_REJ_INVALID] == 'Y' ) {
                    sprintf(rej_msg, "%s | %s", rej_msg, read_rec_ori);
                    wrtOutReject(gszIniParCommon[E_REJ_OUT_DIR], fname, &ofp_rej, rej_msg);
                    cntr_rej++;
                }
                continue;
            }

            // do rating cdr
            rate_result = calcOneTariff();
            if ( (rate_result & BIT_MIS_IMSI) && *gszIniParCommon[E_ALRT_IMSI] == 'Y' ) {
//printf("rate_result = '%d'\n", rate_result);
                wrtAlrtMismatchImsi(gszIniParCommon[E_ALRT_IMSI_DIR], gszToday, &ofp_imsi, gIrCommon.imsi);
                cntr_imsi++;
            }
            else if ( (rate_result & BIT_MIS_PMN) && *gszIniParCommon[E_ALRT_PMN] == 'Y' ) {
                wrtAlrtNotExactPmn(gszIniParCommon[E_ALRT_PMN_DIR], gszToday, &ofp_pmn, gIrCommon.pmn, gIrCommon.chrg_type);
                cntr_pmn++;
            }
            getPmnInfo(gIrCommon.pmn, gIrCommon.pmn_name, gIrCommon.roam_country, gIrCommon.roam_region);

            if ( wrtOutIrCommon(gszIniParOutput[E_OUT_DIR], ir_type, ofile_dtm, &ofp_ir) == SUCCESS ) {
                cntr_wrt++;
            }
            else {
            }
            
            if ( (line_cnt % 5000) == 0 && line_cnt > 0 ) {
                writeLog(LOG_INF, "%10d records have been processed", line_cnt);
                checkPoint(&ifp_ir, full_ir_name, (char*)ir_type, gszIniParCommon[E_TMP_DIR], gszAppName, E_SET);
            }
            
            if ( isTerminated() == TRUE ) {
                checkPoint(&ifp_ir, full_ir_name, (char*)ir_type, gszIniParCommon[E_TMP_DIR], gszAppName, E_SET);
                break;
            }

        }
        if ( isTerminated() != TRUE ) {
            // clear check point in case whole file has been processed
            checkPoint(NULL, "", "", gszIniParCommon[E_TMP_DIR], gszAppName, E_CLR);
        }
        t_stop = time(NULL);
        
        if ( ifp_ir   != NULL ) fclose(ifp_ir);
        if ( ofp_rej  != NULL ) fclose(ofp_rej);
        if ( ofp_imsi != NULL ) fclose(ofp_imsi);
        if ( ofp_pmn  != NULL ) fclose(ofp_pmn);
        if ( ofp_ir   != NULL ) {
            char cmd[SIZE_FULL_NAME];   memset(cmd, 0x00, sizeof(cmd));
            fclose(ofp_ir);
            writeLog(LOG_INF, "processed %s -> %s", fname, basename(gszOutFname));
            sprintf(cmd, "mv %s%s %s", gszOutFname, TMPSUF, gszOutFname);
            system(cmd);
        }
        
        writeLog(LOG_INF, "%s done, common=%d not_found(imsi=%d, pmn=%d) reject=%d total=%d file_time_used=%d sec", fname, cntr_wrt, cntr_imsi, cntr_pmn, cntr_rej, line_cnt, (t_stop - t_start));

        if ( *gszIniParCommon[E_BCKUP] == 'Y' ) {
            char cmd[SIZE_ITEM_L];
            memset(cmd, 0x00, sizeof(cmd));
            sprintf(cmd, "cp -p %s %s", full_ir_name, gszIniParCommon[E_BCKUP_DIR]);
            system(cmd);
        }
#if 1
        unlink(full_ir_name);
#endif
    }

}

int verifyInpFieldTap(char *pbuf[], int bsize, const char *fname, char *err_msg)
{
    char hh[3], mm[3], ss[3], start_dtm[SIZE_DATE_TIME+1];
    char yyyy[5], MM[3], dd[3];
    time_t t_dtm;

    if ( *pbuf[E_EVT_TYPE_ID] != '2' ) {    // reject head/trailer record, data record always be 2xx
        return FALSE;
    }
    
    // set default
    strcpy(gIrCommon.volume, "0");
    strcpy(gIrCommon.duration,  "0");
    strcpy(gIrCommon.total_call_evt_dur, "0");
    
    if ( atoi(gszIniParCommon[E_LOG_LEVEL]) >= LOG_DB3 ) {
        int i; char _rec[SIZE_BUFF_2X]; memset(_rec, 0x00, sizeof(_rec));
        for ( i=0; i<bsize; i++ ) {
            if ( i == 0 )
                sprintf(_rec, "'%s'", pbuf[i]);
            else
                sprintf(_rec, "%s, '%s'", _rec, pbuf[i]);
        }
        writeLog(LOG_DB3, "%s", _rec);
    }

    if ( getCallType(pbuf[E_EVT_TYPE_ID], pbuf[E_SUB_EVT_TYPE], pbuf[E_MODIFIER], gIrCommon.call_type, gIrCommon.chrg_type, gIrCommon.company_name) != SUCCESS ) {
        sprintf(err_msg, "unable to map call type ET(%s), SUB_ET(%s), MOD(%s)", pbuf[E_EVT_TYPE_ID], pbuf[E_SUB_EVT_TYPE], pbuf[E_MODIFIER]);
        return FALSE;
    }

    strcpy(gIrCommon.imsi, pbuf[E_TAP_IMSI]);
    memset(yyyy, 0x00, sizeof(yyyy)); memset(MM, 0x00, sizeof(MM)); memset(dd, 0x00, sizeof(dd));
    strncpy(dd, pbuf[E_ST_DATE], 2);
    strncpy(MM, pbuf[E_ST_DATE]+2, 2);
    strncpy(yyyy, pbuf[E_ST_DATE]+4, 4);
    sprintf(gIrCommon.st_call_date, "%s%s%s", yyyy, MM, dd);
    strcpy(gIrCommon.st_call_time,  pbuf[E_ST_TIME]);

    if ( strcmp(gIrCommon.call_type, TYPE_GPRS) != 0 ) {     // Not GPRS
        sprintf(gIrCommon.duration, "%d", atoi(pbuf[E_TAP_CHRG_TIME]));
    }
    else {  // GPRS
        sprintf(gIrCommon.volume, "%d", atoi(pbuf[E_TAP_CHRG_TIME]));
    }
    sprintf(gIrCommon.total_call_evt_dur, "%d", atoi(pbuf[E_TAP_CHRG_TIME]));

    strcpy(gIrCommon.called_no,       pbuf[E_CALLED_NO]);
    sprintf(gIrCommon.charge, "%.3f", (atol(pbuf[E_AMOUNT])/1000.0));
    strToUpper(gIrCommon.pmn,         pbuf[E_PMN_CODE]);
    strcpy(gIrCommon.proc_dtm,        pbuf[E_CREATED_DTM]);
    strcpy(gIrCommon.chrg_id,         pbuf[E_TAP_CHRG_ID]);
    //strcpy(gIrCommon.utc_time,        pbuf[E_UTC_TIME]);
    sprintf(gIrCommon.utc_time, "%04d", (int)(atof(pbuf[E_UTC_TIME]) * 100));
    strcpy(gIrCommon.imei,            pbuf[E_TAP_IMEI]);
    strcpy(gIrCommon.ori_source,      TYPE_TAP);
    strcpy(gIrCommon.ori_filename,    fname);
    sprintf(gIrCommon.ori_rec_type, "%s-%s-%s", pbuf[E_EVT_TYPE_ID], pbuf[E_SUB_EVT_TYPE], pbuf[E_MODIFIER]);

    // start date time
    memset(hh, 0x00, sizeof(hh)); memset(mm, 0x00, sizeof(mm)); memset(ss, 0x00, sizeof(ss));
    strncpy(hh, pbuf[E_ST_TIME], 2);
    strncpy(mm, pbuf[E_ST_TIME]+2, 2);
    strncpy(ss, pbuf[E_ST_TIME]+4, 2);
    sprintf(gIrCommon.start_dtm, "%s %s:%s:%s", gIrCommon.st_call_date, hh, mm, ss);

    // stop date time
    memset(start_dtm, 0x00, sizeof(start_dtm));
    sprintf(start_dtm, "%s%s",  gIrCommon.st_call_date, pbuf[E_ST_TIME]);
    if ( strcmp(gIrCommon.call_type, TYPE_VOICE_MO) == 0 ||
         strcmp(gIrCommon.call_type, TYPE_VOICE_MT) == 0 ) {    // Only Voice
        t_dtm = dateStr2TimeT(start_dtm) + atoi(pbuf[E_TAP_CHRG_TIME]);
        strcpy(gIrCommon.stop_dtm, getDateTimeT(&t_dtm, DTM_DATE_TIMEF));
    }
    // thailand date time (home datetime)
    // utc in format of (-)n[.n]    eg. 9, 5.3, -10.5
    strcpy(gIrCommon.th_st_call_dtm, getThDTM(start_dtm, atof(pbuf[E_UTC_TIME]), DTM_DATE_TIMEF));


    // The following fields will be calculated and assigned by another function (calcOneTariff).
    //  // gIrCommon.chrg_one_tariff    // this field is from mapping/rating
    //  // gIrCommon.called_no_type     // this field is from mapping/rating
    //  // gIrCommon.risk_no_flg        // this field is from mapping/rating
    //  // gIrCommon.risk_no_id         // this field is from mapping/rating
    //  // gIrCommon.billing_sys        // this field is from mapping/rating
    //  // gIrCommon.country_code       // this field is from mapping/rating
    //  // gIrCommon.mobile_no          // this field is from mapping/rating

    return TRUE;
}

int verifyInpFieldNrt(char *pbuf[], int bsize, const char *fname, char *err_msg)
{
    char hh[3], mm[3], ss[3], start_dtm[SIZE_DATE_TIME+1];
    time_t t_dtm;
    
    // set default
    strcpy(gIrCommon.volume, "0");
    strcpy(gIrCommon.duration,  "0");
    strcpy(gIrCommon.total_call_evt_dur, "0");
    
    if ( atoi(gszIniParCommon[E_LOG_LEVEL]) >= LOG_DB3 ) {
        int i; char _rec[SIZE_BUFF_2X]; memset(_rec, 0x00, sizeof(_rec));
        for ( i=0; i<bsize; i++ ) {
            if ( i == 0 )
                sprintf(_rec, "'%s'", pbuf[i]);
            else
                sprintf(_rec, "%s, '%s'", _rec, pbuf[i]);
        }
        writeLog(LOG_DB3, "%s", _rec);
    }

    if ( *pbuf[E_EVT_CODE] == '2' ) {               // GPRS
        strcpy(gIrCommon.call_type, TYPE_GPRS);
        sprintf(gIrCommon.volume, "%ld", (atol(pbuf[E_VOL_IN]) + atol(pbuf[E_VOL_OUT])));
    }
    else if ( *pbuf[E_EVT_CODE] == '0' ) {          // MOC
        if ( *pbuf[E_TELE_SRV_CODE] == '2' ) {      // SMS
            strcpy(gIrCommon.call_type, TYPE_SMS_MO);
        }
        else {
            strcpy(gIrCommon.call_type, TYPE_VOICE_MO);
            strcpy(gIrCommon.duration,  pbuf[E_DURATION]);
            strcpy(gIrCommon.total_call_evt_dur, pbuf[E_DURATION]);
        }
        strcpy(gIrCommon.called_no, pbuf[E_DIALLED_DIGITS]);
    }
    else {  // MTC
        if ( *pbuf[E_TELE_SRV_CODE] == '2' ) {      // SMS
            strcpy(gIrCommon.call_type, TYPE_SMS_MT);
        }
        else {
            strcpy(gIrCommon.call_type, TYPE_VOICE_MT);
            strcpy(gIrCommon.duration,  pbuf[E_DURATION]);
            strcpy(gIrCommon.total_call_evt_dur, pbuf[E_DURATION]);
        }
        strcpy(gIrCommon.called_no, pbuf[E_CALL_NO]);
    }

    strcpy(gIrCommon.imsi,          pbuf[E_NRT_IMSI]);
    strncpy(gIrCommon.st_call_date, pbuf[E_CALL_ST_TIME], SIZE_DATE_ONLY);
    strncpy(gIrCommon.st_call_time, pbuf[E_CALL_ST_TIME]+SIZE_DATE_ONLY, SIZE_TIME_ONLY);
    strcpy(gIrCommon.charge,        pbuf[E_CHRG_AMNT]);
    strcpy(gIrCommon.pmn,           pbuf[E_SND_PMN]);
    strcpy(gIrCommon.proc_dtm,      getSysDTM(DTM_DATE_TIME));
    strcpy(gIrCommon.company_name,  "AWN");

    // start date time
    memset(hh, 0x00, sizeof(hh)); memset(mm, 0x00, sizeof(mm)); memset(ss, 0x00, sizeof(ss));
    strncpy(hh, gIrCommon.st_call_time, 2);
    strncpy(mm, gIrCommon.st_call_time+2, 2);
    strncpy(ss, gIrCommon.st_call_time+4, 2);
    sprintf(gIrCommon.start_dtm, "%s %s:%s:%s", gIrCommon.st_call_date, hh, mm, ss);

    // stop date time
    memset(start_dtm, 0x00, sizeof(start_dtm));
    strcpy(start_dtm, pbuf[E_CALL_ST_TIME]);
    t_dtm = dateStr2TimeT(start_dtm) + atoi(pbuf[E_DURATION]);
    strcpy(gIrCommon.stop_dtm, getDateTimeT(&t_dtm, DTM_DATE_TIMEF));

    // thailand date time (home datetime)
    // utc in format of <+->NNNN    eg +0900, +0530, -1050
    char hhh[4], new_utc[7];
    memset(hhh, 0x00, sizeof(hhh)); memset(mm, 0x00, sizeof(mm)); memset(new_utc, 0x00, sizeof(new_utc));
    strncpy(hhh, pbuf[E_ROAM_UTC], 3);
    strncpy(mm,  pbuf[E_ROAM_UTC] + 3, 2);
    sprintf(new_utc, "%s.%s", hhh, mm);
    strcpy(gIrCommon.th_st_call_dtm, getThDTM(start_dtm, atof(new_utc), DTM_DATE_TIMEF));

    strcpy(gIrCommon.chrg_id, pbuf[E_NRT_CHRG_ID]);
    strcpy(gIrCommon.utc_time, pbuf[E_ROAM_UTC]);

    if ( *pbuf[E_TELE_SRV_CODE] != '\0' )
        strcpy(gIrCommon.ori_rec_type, pbuf[E_TELE_SRV_CODE]);
    else
        strcpy(gIrCommon.ori_rec_type, pbuf[E_BEAR_SRV_CODE]);

    strcpy(gIrCommon.imei, pbuf[E_NRT_IMEI]);
    strcpy(gIrCommon.ori_source, TYPE_NRT);
    strcpy(gIrCommon.ori_filename, fname);

    // The following fields will be calculated and assigned by another function (calcOneTariff).
    // strcpy(gIrCommon.chrg_type,          pbuf[]);    // from imsi
    // strcpy(gIrCommon.chrg_one_tariff,    pbuf[]);
    // strcpy(gIrCommon.called_no_type,     pbuf[]);
    // strcpy(gIrCommon.risk_no_flg,        pbuf[]);
    // strcpy(gIrCommon.risk_no_id,         pbuf[]);
    // strcpy(gIrCommon.billing_sys,        pbuf[]);
    // strcpy(gIrCommon.country_code,       pbuf[]);
    // strcpy(gIrCommon.mobile_no,          pbuf[]);

    return TRUE;

}

int verifyInpFieldScp(char *pbuf[], int bsize, const char *fname, char *err_msg)
{
    char hh[3], mm[3], ss[3], start_dtm[SIZE_DATE_TIME+1];
    time_t t_dtm;
    
    if ( atoi(gszIniParCommon[E_LOG_LEVEL]) >= LOG_DB3 ) {
        int i; char _rec[SIZE_BUFF_2X]; memset(_rec, 0x00, sizeof(_rec));
        for ( i=0; i<bsize; i++ ) {
            if ( i == 0 )
                sprintf(_rec, "'%s'", pbuf[i]);
            else
                sprintf(_rec, "%s, '%s'", _rec, pbuf[i]);
        }
        writeLog(LOG_DB3, "%s", _rec);
    }
    
    if ( *pbuf[EA_SVC_CAT] == '5' ) {   // GPRS
        strcpy(gIrCommon.call_type, TYPE_GPRS);
    }
    else if ( *pbuf[EA_SVC_CAT] == '1' ) {  // Voice
        if ( *pbuf[EA_SVC_FLOW] == '2' ) {  // MTC
            strcpy(gIrCommon.call_type, TYPE_VOICE_MT);
            strcpy(gIrCommon.called_no, pbuf[EA_CLLG_PARTY_NUM]);   //  A_No
        }
        else {  // MOC: EA_SVC_FLOW == 1 or 3
            strcpy(gIrCommon.call_type, TYPE_VOICE_MO);
            strcpy(gIrCommon.called_no, pbuf[EA_CLLD_PARTY_NUM]);   //  B_No
        }
        if ( strcmp(pbuf[EA_USG_MEAS_ID], "1004") == 0 ) {    // Usage_Measure_id = 1004 => Actual_usage * 60
            sprintf(gIrCommon.duration, "%d", (atoi(pbuf[EA_ACT_USG]) * 60));
        }
        else {  // Usage_Measure_id = 1003 => did not convert
            strcpy(gIrCommon.duration, pbuf[EA_ACT_USG]);
        }
    }
    else if ( *pbuf[EA_SVC_CAT] == '2' || *pbuf[EA_SVC_CAT] == '3' ) {  // SMS
        if ( *pbuf[EA_SVC_FLOW] == '2' ) {  // MTC
            strcpy(gIrCommon.call_type, TYPE_SMS_MT);
        }
        else {  // MOC
            strcpy(gIrCommon.call_type, TYPE_SMS_MO);
        }
    }
    else {
        writeLog(LOG_ERR, "unknown service category %s expected 1,2,3 or 5", pbuf[EA_SVC_CAT]);
        return FALSE;
    }

    strcpy(gIrCommon.imsi,          pbuf[EA_CLLG_PARTY_IMSI]);
    strncpy(gIrCommon.st_call_date, pbuf[EA_START_DATE], SIZE_DATE_ONLY);
    strncpy(gIrCommon.st_call_time, pbuf[EA_START_DATE]+SIZE_DATE_ONLY, SIZE_TIME_ONLY);
    strcpy(gIrCommon.pmn,           pbuf[EA_RM_PLMN_CODE_IR]);
    strcpy(gIrCommon.proc_dtm,      getSysDTM(DTM_DATE_TIME));
    strcpy(gIrCommon.company_name,  "AWN");

    // start date time
    memset(hh, 0x00, sizeof(hh)); memset(mm, 0x00, sizeof(mm)); memset(ss, 0x00, sizeof(ss));
    strncpy(hh, gIrCommon.st_call_time, 2);
    strncpy(mm, gIrCommon.st_call_time+2, 2);
    strncpy(ss, gIrCommon.st_call_time+4, 2);
    sprintf(gIrCommon.start_dtm, "%s %s:%s:%s", gIrCommon.st_call_date, hh, mm, ss);

    // stop date time
    memset(start_dtm, 0x00, sizeof(start_dtm));
    strcpy(start_dtm, pbuf[E_CALL_ST_TIME]);
    t_dtm = dateStr2TimeT(start_dtm) + atoi(pbuf[E_DURATION]);
    strcpy(gIrCommon.stop_dtm, getDateTimeT(&t_dtm, DTM_DATE_TIMEF));

    // thailand date time (home datetime)
    //strcpy(gIrCommon.th_st_call_dtm, getThDTM(start_dtm, atof(pbuf[E_ROAM_UTC]), DTM_DATE_TIMEF));
    
    sprintf(gIrCommon.chrg_one_tariff, "%.3f", (float)(atoi(pbuf[EA_DEDUCT_CHG_AMT_01])/1000));
    
    if ( strcmp(pbuf[EA_USG_MEAS_ID], "1004") == 0 ) {    // Usage_Measure_id = 1004 => Actual_usage * 60
        sprintf(gIrCommon.total_call_evt_dur, "%d", (atoi(pbuf[EA_RATE_USG]) * 60));
    }
    else {  // Usage_Measure_id = 1003 => did not convert
        strcpy(gIrCommon.total_call_evt_dur, pbuf[EA_RATE_USG]);
    }
    
    strcpy(gIrCommon.ori_rec_type, pbuf[EA_CDR_TYPE]);
    strcpy(gIrCommon.mobile_no, pbuf[EA_PRI_IDENT]);
    strcpy(gIrCommon.imei, pbuf[EA_IMEI]);
    
    strcpy(gIrCommon.ori_source, TYPE_SCP);
    strcpy(gIrCommon.ori_filename, fname);

    // strcpy(gIrCommon.chrg_type,          pbuf[]);
    // strcpy(gIrCommon.called_no_type,     pbuf[]);
    // strcpy(gIrCommon.risk_no_flg,        pbuf[]);
    // strcpy(gIrCommon.risk_no_id,         pbuf[]);
    // strcpy(gIrCommon.billing_sys,        pbuf[]);
    // strcpy(gIrCommon.start_dtm,          pbuf[]);
    // strcpy(gIrCommon.stop_dtm,           pbuf[]);
    // strcpy(gIrCommon.chrg_id,            pbuf[]);
    // strcpy(gIrCommon.utc_time,           pbuf[]);
    // strcpy(gIrCommon.country_code,       pbuf[]);

    return TRUE;

}

int verifyInpFieldRtb(char *pbuf[], int bsize, const char *fname, char *err_msg)
{
    char hh[3], mm[3], ss[3], start_dtm[SIZE_DATE_TIME+1];
    time_t t_dtm;
    
    if ( atoi(gszIniParCommon[E_LOG_LEVEL]) >= LOG_DB3 ) {
        int i; char _rec[SIZE_BUFF_2X]; memset(_rec, 0x00, sizeof(_rec));
        for ( i=0; i<bsize; i++ ) {
            if ( i == 0 )
                sprintf(_rec, "'%s'", pbuf[i]);
            else
                sprintf(_rec, "%s, '%s'", _rec, pbuf[i]);
        }
        writeLog(LOG_DB3, "%s", _rec);
    }

    // strcpy(gIrCommon.call_type,          pbuf[]);
    // strcpy(gIrCommon.imsi,               pbuf[]);
    // strcpy(gIrCommon.st_call_date,       pbuf[]);
    // strcpy(gIrCommon.st_call_time,       pbuf[]);
    // strcpy(gIrCommon.duration,           pbuf[]);
    // strcpy(gIrCommon.called_no,          pbuf[]);
    // strcpy(gIrCommon.charge,             pbuf[]);
    // strcpy(gIrCommon.pmn,                pbuf[]);
    // strcpy(gIrCommon.proc_dtm,           pbuf[]);
    // strcpy(gIrCommon.volume,             pbuf[]);
    // strcpy(gIrCommon.chrg_type,          pbuf[]);
    // strcpy(gIrCommon.company_name,       pbuf[]);
    // strcpy(gIrCommon.chrg_one_tariff,    pbuf[]);
    // strcpy(gIrCommon.th_st_call_dtm,     pbuf[]);
    // strcpy(gIrCommon.called_no_type,     pbuf[]);
    // strcpy(gIrCommon.risk_no_flg,        pbuf[]);
    // strcpy(gIrCommon.risk_no_id,         pbuf[]);
    // strcpy(gIrCommon.billing_sys,        pbuf[]);
    // strcpy(gIrCommon.start_dtm,          pbuf[]);
    // strcpy(gIrCommon.stop_dtm,           pbuf[]);
    // strcpy(gIrCommon.chrg_id,            pbuf[]);
    // strcpy(gIrCommon.utc_time,           pbuf[]);
    // strcpy(gIrCommon.total_call_evt_dur, pbuf[]);
    // strcpy(gIrCommon.ori_rec_type,       pbuf[]);
    // strcpy(gIrCommon.mobile_no,          pbuf[]);
    // strcpy(gIrCommon.imei,               pbuf[]);
    // strcpy(gIrCommon.ori_source,         TYPE_SCP);
    // strcpy(gIrCommon.ori_filename,       fname);
    // strcpy(gIrCommon.country_code,       pbuf[]);

    return TRUE;

}

int calcOneTariff()  // need to fixed
{
    int result = SUCCESS;
    int one_charge = 0, tmp_one_tariff = 0;
    char tmpChgType[SIZE_CHRG_TYPE];
    ONETARIFF_TAB onetariff;

    // get mobile number from imsi, if not found, set mis_imsi bit for later write alert.
    memset(tmpChgType, 0x00, sizeof(tmpChgType));
    memset(gIrCommon.mobile_no, 0x00, sizeof(gIrCommon.mobile_no));
    memset(gIrCommon.billing_sys, 0x00, sizeof(gIrCommon.billing_sys));
    //int ret = getMobileNo(gIrCommon.imsi, gIrCommon.mobile_no, tmpChgType, gIrCommon.billing_sys);
//printf("> ret = '%d' result = '%d'\n", ret, result);
    if ( getMobileNo(gIrCommon.imsi, gIrCommon.mobile_no, tmpChgType, gIrCommon.billing_sys) != SUCCESS ) {
        //writeLog(LOG_WRN, "calcOneTariff: not found mobile_no for imsi(%s)", gIrCommon.imsi);
        result |= BIT_MIS_IMSI;
    }
//printf(">> ret = '%d' result = '%d'\n", ret, result);

    if ( *gIrCommon.chrg_type == '\0' ) {   // For NRT
        strcpy(gIrCommon.chrg_type, tmpChgType);
    }

    // get one tariff, if not exactly found, set mis_pmn bit for later write alert.
    memset(&onetariff, 0x00, sizeof(onetariff));
    strcpy(onetariff.charge_type, gIrCommon.chrg_type);
// printf("gIrCommon.pmn (%s)\n", gIrCommon.pmn);
    if ( getOneTariff(E_MATCH, gIrCommon.pmn, &onetariff) != SUCCESS ) {

        if ( *gIrCommon.chrg_type != '\0' ) {   // No need to log in case of empty charge_type since there is no null charge_type in db
            writeLog(LOG_WRN, "calcOneTariff: no exact match pmn(%s) charge_type(%s)", gIrCommon.pmn, gIrCommon.chrg_type);
        }
        result |= BIT_MIS_PMN;
//printf(">>> ret = '%d' result = '%d'\n", ret, result);
        memset(&onetariff, 0x00, sizeof(onetariff));
        strcpy(onetariff.charge_type, gIrCommon.chrg_type);

        if ( getOneTariff(E_AVG_PMN, gIrCommon.pmn, &onetariff) != SUCCESS ) {

            if ( *gIrCommon.chrg_type != '\0' ) {   // No need to log in case of empty charge_type since there is no null charge_type in db
                writeLog(LOG_WRN, "calcOneTariff: no match avg pmn(%.3s) charge_type(%s)", gIrCommon.pmn, gIrCommon.chrg_type);
            }
            memset(&onetariff, 0x00, sizeof(onetariff));
            strcpy(onetariff.charge_type, gIrCommon.chrg_type);
            getOneTariff(E_AVG_ALL, gIrCommon.pmn, &onetariff);

        }
    }
// printf("\nOUT: pmn(%s), ct(%s), cc(%s), id(%s), gm(%d), gr(%d)\n", onetariff.pmn, onetariff.charge_type, onetariff.country_code, onetariff.idd_acc_code, onetariff.gprs_min, onetariff.gprs_round);
// int i = 0;
// for(i=0; i<NOF_ONE_TARIFF; i++) { printf("\t%02d\t'%010d'\n", i, onetariff.tariff[i]); }
// printf("\n");
    // rating
    if ( strcmp(gIrCommon.call_type, TYPE_GPRS) == 0 ) {    // E_GPRS_CHR (18)
        long volume = atol(gIrCommon.volume);
        one_charge = onetariff.tariff[E_GPRS_CHR];

        if ( onetariff.gprs_min > 0 && onetariff.gprs_round > 0 && volume > 0 ) {
            if ( (int)(volume/KBYTE) <= onetariff.gprs_min ) {
                //sprintf(gIrCommon.chrg_one_tariff, "%.3f", (float)(one_charge * onetariff.gprs_min));
                tmp_one_tariff = (one_charge * onetariff.gprs_min);
            }
            else {
                //sprintf(gIrCommon.chrg_one_tariff, "%.3f", (ceil((volume/KBYTE)/onetariff.gprs_round) * onetariff.gprs_round * one_charge));
                tmp_one_tariff = (ceil(((float)volume/KBYTE)/onetariff.gprs_round) * onetariff.gprs_round * one_charge);
            }
        }
        else {
            //sprintf(gIrCommon.chrg_one_tariff, "%.3f", ceil(volume/KBYTE) * one_charge);
            tmp_one_tariff = (ceil((float)volume/KBYTE) * one_charge);
        }
    }
    else if ( strcmp(gIrCommon.call_type, TYPE_SMS_MO) == 0 ) {     // E_SMSO_CHR (21)
        //one_charge = onetariff.tariff[E_SMSO_CHR];
        //sprintf(gIrCommon.chrg_one_tariff, "%.3f", (float)(one_charge/10));    // make it satang by divide by ten
        tmp_one_tariff = onetariff.tariff[E_SMSO_CHR];
    }
    else if ( strcmp(gIrCommon.call_type, TYPE_VOICE_MT) == 0 ) {   // E_MTC_CHR (30)
        one_charge = onetariff.tariff[E_MTC_CHR];
        //sprintf(gIrCommon.chrg_one_tariff, "%.3f", (ceil(gIrCommon.duration/60) * one_charge));
        tmp_one_tariff = calcDurCharge(atoi(gIrCommon.duration), one_charge);
    }
    else if ( strcmp(gIrCommon.call_type, TYPE_SMS_MT) == 0 ) {     // E_SMST_CHR (31)
        //one_charge = onetariff.tariff[E_SMST_CHR];
        //sprintf(gIrCommon.chrg_one_tariff, "%.3f", (float)(one_charge/10));  // make it satang by divide by ten
        tmp_one_tariff = onetariff.tariff[E_SMST_CHR];
    }
    else {
        char call_num[25];
        int acc_len = strlen(gszIniParCommon[E_DEF_IDD_ACC]);
        int idd_len = strlen(onetariff.idd_acc_code);
        int cntry_len = strlen(onetariff.country_code);
        memset(call_num, 0x00, sizeof(call_num));

        if ( strncmp(gIrCommon.called_no, gszIniParCommon[E_DEF_IDD_ACC], acc_len) == 0 ) {
            strcpy(call_num, gIrCommon.called_no+acc_len);
        }
        else if ( strncmp(gIrCommon.called_no, onetariff.idd_acc_code, idd_len) == 0 ) {
            strcpy(call_num, gIrCommon.called_no+idd_len);
        }
        else {
            strcpy(call_num, gIrCommon.called_no);
        }

        if ( strncmp(call_num, "66", 2) == 0 ) {
            one_charge = onetariff.tariff[E_MOC_CHR_TO_TH];
            strcpy(gIrCommon.country_code, "66");
            strcpy(gIrCommon.called_no_type, "Thai");
        }
        else if ( strncmp(call_num, onetariff.country_code, cntry_len) == 0 ) {
            one_charge = onetariff.tariff[E_MOC_CHR_LOCAL];
            strcpy(gIrCommon.country_code, onetariff.country_code);
            strcpy(gIrCommon.called_no_type, "Local");
        }
        else {
            one_charge = onetariff.tariff[E_MOC_CHR_INTER];
            strcpy(gIrCommon.called_no_type, "IDD");
            getCountryCode(call_num, gIrCommon.country_code);
        }

        strcpy(gIrCommon.risk_no_flg, "N");
        int risk_id;
        if ( isRiskBno(call_num, &risk_id) == SUCCESS ) {
            sprintf(gIrCommon.risk_no_id, "%d", risk_id);
            strcpy(gIrCommon.risk_no_flg, "Y");
        }

        //sprintf(gIrCommon.chrg_one_tariff, "%.3f", (ceil(atoi(gIrCommon.duration)/60) * one_charge));
        tmp_one_tariff = calcDurCharge(atoi(gIrCommon.duration), one_charge);
    }
    sprintf(gIrCommon.chrg_one_tariff, "%.0f", (float)(tmp_one_tariff/10));    // make it satang by divide by ten

    return result;

}

int calcDurCharge(int duration, int one_tariff)
{
    double integ = 0, fract = 0;

    if ( !duration || !one_tariff )
        return 0;
    
    if ( duration <= 60 ) {
        return one_tariff;
    }

    // charge is calculated using one_tariff multiply by duration block (6 sec per block and round up)
    fract = modf(((double)duration/6), &integ);
    if ( fract > 0.0)   // if fraction is greater than zero, integral need to plus one as round up for one block
        integ++;        // round to next unit

    // one_tariff is devided by ten because its a rate per minute, so a minute contains 10 blocks
    return (rint(integ * (one_tariff/10)));
}

int wrtOutIrCommon(const char *odir, const char *ir_type, const char *file_dtm, FILE **ofp)
{
    char full_irfile[SIZE_ITEM_L];
    if ( *ofp == NULL ) {
        ( ++gnFileSeq > 999 ? gnFileSeq = 0 : gnFileSeq );
        memset(gszOutFname, 0x00, sizeof(gszOutFname));
        memset(full_irfile, 0x00, sizeof(full_irfile));
        sprintf(gszOutFname, "%s/%s_%s_%s_%03d%s", odir, gszIniParOutput[E_OUT_FPREF], ir_type, file_dtm, gnFileSeq, gszIniParOutput[E_OUT_FSUFF]);
        sprintf(full_irfile, "%s%s", gszOutFname, TMPSUF);
        if ( (*ofp = fopen(full_irfile, "a")) == NULL ) {
            writeLog(LOG_SYS, "unable to open append %s (%s)", full_irfile, strerror(errno));
            return FAILED;
        }
    }

    fprintf(*ofp, "%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s\n",
                gIrCommon.call_type, gIrCommon.imsi, gIrCommon.st_call_date, gIrCommon.st_call_time,
                gIrCommon.duration, gIrCommon.called_no, gIrCommon.charge, gIrCommon.pmn, gIrCommon.proc_dtm,
                gIrCommon.volume, gIrCommon.chrg_type, gIrCommon.company_name, gIrCommon.chrg_one_tariff,
                gIrCommon.th_st_call_dtm, gIrCommon.called_no_type, gIrCommon.risk_no_flg, gIrCommon.risk_no_id,
                gIrCommon.billing_sys, gIrCommon.start_dtm, gIrCommon.stop_dtm, gIrCommon.chrg_id, gIrCommon.utc_time,
                gIrCommon.total_call_evt_dur, gIrCommon.ori_rec_type, gIrCommon.mobile_no, gIrCommon.imei,
                gIrCommon.ori_source, gIrCommon.ori_filename, gIrCommon.country_code,
                gIrCommon.pmn_name, gIrCommon.roam_country, gIrCommon.roam_region);
    return SUCCESS;

}

int wrtOutReject(const char *odir, const char *fname, FILE **ofp, const char *record)
{

    char full_rejfile[SIZE_ITEM_L];
    if ( *ofp == NULL ) {
        sprintf(full_rejfile, "%s/%s.REJ", odir, fname);
        if ( (*ofp = fopen(full_rejfile, "a")) == NULL ) {
            writeLog(LOG_SYS, "unable to open append %s (%s)", full_rejfile, strerror(errno));
            return FAILED;
        }
    }

    fprintf(*ofp, "%s\n", record);
    return SUCCESS;

}

int wrtAlrtMismatchImsi(const char *odir, const char *fname, FILE **ofp, const char *imsi)
{

    char full_imsifile[SIZE_ITEM_L];
    if ( *ofp == NULL ) {
        sprintf(full_imsifile, "%s/%s_imsi_%s%s", odir, gszAppName, fname, ALERT_SUFF);
        if ( (*ofp = fopen(full_imsifile, "a")) == NULL ) {
            writeLog(LOG_SYS, "unable to open append %s (%s)", full_imsifile, strerror(errno));
            return FAILED;
        }
    }

    fprintf(*ofp, "%s\n", imsi);
    return SUCCESS;

}

int wrtAlrtNotExactPmn(const char *odir, const char *fname, FILE **ofp, const char *pmn, const char *chrg_type)
{

    char full_pmnfile[SIZE_ITEM_L];
    if ( *ofp == NULL ) {
        sprintf(full_pmnfile, "%s/%s_pmn_%s%s", odir, gszAppName, fname, ALERT_SUFF);
        if ( (*ofp = fopen(full_pmnfile, "a")) == NULL ) {
            writeLog(LOG_SYS, "unable open append %s (%s)", full_pmnfile, strerror(errno));
            return FAILED;
        }
    }

    fprintf(*ofp, "%s|%s\n", pmn, chrg_type);
    return SUCCESS;

}

int wrtAlrtDbConnFail(const char *odir, const char *fname, const char *dbsvr)
{
    char full_dbconfile[SIZE_ITEM_L];
    FILE *ofp = NULL;

    sprintf(full_dbconfile, "%s/%s_dbcon_%s%s", odir, gszAppName, fname, ALERT_SUFF);
    if ( (ofp = fopen(full_dbconfile, "a")) == NULL ) {
        writeLog(LOG_SYS, "unable to open append %s (%s)", full_dbconfile, strerror(errno));
        return FAILED;
    }
    fprintf(ofp, "%s %s\n", getSysDTM(DTM_DATE_TIME_FULL), dbsvr);
    fclose(ofp);
    writeLog(LOG_INF, "db connection alert file is created: %s", fname);

    return SUCCESS;
    
}

int manageMapTab()
{
    time_t curr_time = time(NULL);
    int reload_hour = 4 * 60 * 60;  // reload every 4 hours
    int result = SUCCESS;

    if ( (curr_time - gzLastTimeT) > reload_hour || gzLastTimeT == 0 ) {
        if ( connectDbSub(gszIniParDbConn[E_SUB_USER], gszIniParDbConn[E_SUB_PASSWORD], gszIniParDbConn[E_SUB_DB_SID], atoi(gszIniParDbConn[E_RETRY_COUNT]), atoi(gszIniParDbConn[E_RETRY_WAIT])) != SUCCESS ) {
            writeLog(LOG_ERR, "connectDbSub failed");
            if ( *gszIniParCommon[E_ALRT_DB] == 'Y' ) {
                wrtAlrtDbConnFail(gszIniParCommon[E_ALRT_DB_DIR], gszToday, gszIniParDbConn[E_SUB_DB_SID]);
            }
            return FAILED;
        }
        writeLog(LOG_INF, "loading db tables ...");
        result = loadTables();
        gzLastTimeT = time(NULL);
        disconnSub(gszIniParDbConn[E_SUB_DB_SID]);
    }
    return result;
}

void makeIni()
{

    int key;
    char cmd[SIZE_ITEM_S];
    char tmp_ini[SIZE_ITEM_S];
    char tmp_itm[SIZE_ITEM_S];

    sprintf(tmp_ini, "./%s_XXXXXX", _APP_NAME_);
    mkstemp(tmp_ini);
    strcpy(tmp_itm, "<place_holder>");

    // Write config of INPUT Section
    for ( key = 0; key < E_NOF_PAR_INPUT; key++ ) {
        ini_puts(gszIniStrSection[E_INPUT], gszIniStrInput[key], tmp_itm, tmp_ini);
    }

    // Write config of OUTPUT Section
    for ( key = 0; key < E_NOF_PAR_OUTPUT; key++ ) {
        ini_puts(gszIniStrSection[E_OUTPUT], gszIniStrOutput[key], tmp_itm, tmp_ini);
    }

    // Write config of COMMON Section
    for ( key = 0; key < E_NOF_PAR_COMMON; key++ ) {
        ini_puts(gszIniStrSection[E_COMMON], gszIniStrCommon[key], tmp_itm, tmp_ini);
    }

    // Write config of BACKUP Section
    for ( key = 0; key < E_NOF_PAR_DBCONN; key++ ) {
        ini_puts(gszIniStrSection[E_DBCONN], gszIniStrDbConn[key], tmp_itm, tmp_ini);
    }

    sprintf(cmd, "mv %s %s.ini", tmp_ini, tmp_ini);
    system(cmd);
    fprintf(stderr, "ini template file is %s.ini\n", tmp_ini);

}
