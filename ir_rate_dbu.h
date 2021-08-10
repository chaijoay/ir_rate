///
///
/// FACILITY    : db utility for rating and mapping of ir cdr
///
/// FILE NAME   : ir_rate_dbu.h
///
/// AUTHOR      : Thanakorn Nitipiromchai
///
/// CREATE DATE : 15-May-2019
///
/// CURRENT VERSION NO : 1.1.2
///
/// LAST RELEASE DATE  : 21-Nov-2019
///
/// MODIFICATION HISTORY :
///     1.0         15-May-2019     First Version
///     1.1.0       17-Sep-2019     Load Balance by 10 processes regarding last digit of imsi
///                                 Obsoletes backup feature, Add keep state, flushes logState and purge old data feature
///     1.1.1       26-Sep-2019     Minor Change (IDD Access Code can be a list)
///     1.1.2       21-Nov-2019     fix state file checking
///     1.2.0       20-Jan-2020     Support IR SCP file format
///
///
#ifndef __IR_RATE_DBU_H__
#define __IR_RATE_DBU_H__

#ifdef  __cplusplus
    extern "C" {
#endif

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdlib.h>

// #include <sqlca.h>
// #include <sqlda.h>
// #include <sqlcpr.h>

#include "strlogutl.h"
#include "glb_str_def.h"


#define NOT_FOUND               1403
#define FETCH_NULL              1405
#define KEY_DUP                 -1
#define DEADLOCK                -60
#define FETCH_OUTOFSEQ          -1002

#define SIZE_GEN_STR            100
#define SIZE_SQL_STR            1024

#define SIZE_PMN_CODE           10
#define SIZE_COUNTRY_CODE       10
#define SIZE_CHRG_TYPE          2
#define SIZE_IDD_ACC_CODE       10
#define SIZE_BNO                50
#define SIZE_ANO                20

#define FETCH_ERR_ALLOW         20

#define     PREPAID             "0"
#define     POSTPAID            "1"
#define     HYBRID              "2"

#define     BIT_MIS_IMSI        0x001
#define     BIT_MIS_PMN         0x002

#ifdef	__cplusplus
extern "C" {
#endif

typedef enum {
    E_MTC_CHR = 0,
    E_GPRS_CHR,
    E_SMST_CHR,
    E_SMSO_CHR,
    E_MOC_CHR_TO_TH,
    E_MOC_CHR_LOCAL,
    E_MOC_CHR_INTER,
    NOF_ONE_TARIFF
} TARIFF_TYPE;

typedef enum {
    E_MATCH = 0,
    E_AVG_PMN,
    E_AVG_ALL
} PMN_TYPE;

// one_tariff_exact
// one_tariff_avg_pmn
// one_tariff_avg_all
typedef struct onetariff_tab {
    char  pmn[SIZE_PMN_CODE];           // key1
    char  charge_type[SIZE_CHRG_TYPE];  // key2
    char  country_code[SIZE_COUNTRY_CODE];
    char  idd_acc_code[SIZE_IDD_ACC_CODE];
    int   tariff[NOF_ONE_TARIFF];
    int   gprs_min;
    int   gprs_round;
    float gmt_offset;
} ONETARIFF_TAB;

// risk_bno
typedef struct risk_bno_tab {
    char bno[SIZE_BNO];    // key
    int  pat_id;
} RISK_BNO_TAB;

// call_type
typedef struct call_type_tab {
    char evt_type_id[5+1];          // key1
    char sub_evt_type_id[2+1];      // key2
    char modifier[5+1];             // key3
    char call_type[2+1];
    char chrg_type[SIZE_CHRG_TYPE];
    char company_name[3+1];
} CALL_TYPE_TAB;

typedef struct country_code {
    char country_code[SIZE_COUNTRY_CODE];
} COUNTRY_CODE_TAB;

typedef struct pmn_info {
    char pmn_code[SIZE_PMN_CODE];
    char pmn_name[50];
    char roam_country[50];
    char roam_region[50];
} PMN_INFO_TAB;

//int   connAllDb(char *szFrmUsr, char *szFrmPwd, char *szFrmSvr, char *szSffUsr, char *szSffPwd, char *szSffSvr);
int   connectDbSub(char *szDbUsr, char *szDbPwd, char *szDbSvr, int nRetryCnt, int nRetryWait);
int   connectDbSff(char *szDbUsr, char *szDbPwd, char *szDbSvr, int nRetryCnt, int nRetryWait);
int   connectDbPpi(char *szDbUsr, char *szDbPwd, char *szDbSvr, int nRetryCnt, int nRetryWait);
//void  discAllDb();
void  disconnSub(char *dbsvr);
void  disconnSff(char *dbsvr);
void  disconnPpi(char *dbsvr);

int   loadOneTariff();
int   loadOneTariffAvgPmn();
int   loadOneTariffAvgAll();
int   loadRiskBno();
int   loadCallType();
int   loadCountryCode();
int   loadPmnInfo();
int   loadTables();
void  freeTab();

int   cmpOneTariff(const void *ptr1, const void *ptr2);
int   cmpRiskBno(const void *ptr1, const void *ptr2);
int   cmpCallType(const void *ptr1, const void *ptr2);
int   cmpCountryCode(const void *ptr1, const void *ptr2);
int   cmpPmnInfo(const void *ptr1, const void *ptr2);

int   getOneTariff(int pmn_type, const char *pmn, ONETARIFF_TAB *one_out);
int   _getOneTariffExact(int pmn_type, const char *pmn, ONETARIFF_TAB *one_out);
int   getGmtOffset(const char *pmn, float *gmt_offset);
int   getCallType(const char *evt_type_id, const char *sub_evt_type_id, const char *modifier, char *call_type, char *chrg_type, char *company_name);
int   isRiskBno(const char *bno, int *pat_id);
int   _isRiskBnoExact(const char *bno, int *pat_id);
int   getMobileNo(const char *imsi, char *mobile, char *chrg_type, char *billsys);
int   getCountryCode(const char *bno, char *country_code);
int   _getCountryCodeExact(const char *bno, char *country_code);
int   getPmnInfo(const char *pmn_code, char *pmn_name, char *roam_country, char *roam_region);


#ifdef  __cplusplus
    }
#endif

#endif
