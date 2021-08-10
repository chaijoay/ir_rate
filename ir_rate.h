///
///
/// FACILITY    : rating 3 sources of ir cdr(TAP, NRTRDE and SCP) and output to one common format
///
/// FILE NAME   : ir_rate.h
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
///     1.0         15-May-2019     First Version
///     1.1.0       17-Sep-2019     Load Balance by 10 processes regarding last digit of imsi
///                                 Obsoletes backup feature, Add keep state, flushes logState and purge old data feature
///
///
#ifndef __IR_RATE_H__
#define __IR_RATE_H__

#ifdef  __cplusplus
    extern "C" {
#endif
#include <ftw.h>

#define _APP_NAME_              "ir_rate"
#define _APP_VERS_              "1.1"

#define     TYPE_TAP            "TAP"
#define     TYPE_NRT            "NRT"
#define     TYPE_SCP            "SCP"

#define     TYPE_GPRS           "18"
#define     TYPE_VOICE_MO       "20"
#define     TYPE_SMS_MO         "21"
#define     TYPE_VOICE_MT       "30"
#define     TYPE_SMS_MT         "31"

#define     STATE_SUFF          ".proclist"
#define     ALERT_SUFF          ".alrt"

//#define     TYPE_CFWD           "22"    // obsoleted
//#define     TYPE_VOICE_MO_CF    "25"    // obsoleted
//#define     TYPE_CFWD_CF        "26"    // obsoleted
//#define     TYPE_CFWD_CF        "45"    // obsoleted

// ----- INI Parameters -----
// All Section
typedef enum {
    E_INPUT = 0,
    E_OUTPUT,
    E_COMMON,
    E_DBCONN,
    E_NOF_SECTION
} E_INI_SECTION;


typedef enum {
    // INPUT Section
    E_TAP_INP_DIR = 0,
    E_TAP_FPREF,
    E_TAP_FSUFF,
    E_NRT_INP_DIR,
    E_NRT_FPREF,
    E_NRT_FSUFF,
    E_SCP_INP_DIR,
    E_SCP_FPREF,
    E_SCP_FSUFF,
    E_RTB_INP_DIR,
    E_RTB_FPREF,
    E_RTB_FSUFF,
    E_NOF_PAR_INPUT
} E_INI_INPUT_SEC;

typedef enum {
    // OUTPUT Section
    E_OUT_DIR = 0,
    E_OUT_FPREF,
    E_OUT_FSUFF,
    E_NOF_PAR_OUTPUT
} E_INI_OUTPUT_SEC;

typedef enum {
    // COMMON Section
    E_DEF_IDD_ACC = 0,
    E_REJ_INVALID,
    E_REJ_OUT_DIR,
    E_ALRT_IMSI,
    E_ALRT_IMSI_DIR,
    E_ALRT_PMN,
    E_ALRT_PMN_DIR,
    E_ALRT_DB,
    E_ALRT_DB_DIR,
    E_TMP_DIR,
    E_STATE_DIR,
    E_KEEP_STATE_DAY,
    E_SKIP_OLD_FILE,
    E_LOG_DIR,
    E_LOG_LEVEL,
    E_SLEEP_SEC,
    E_NOF_PAR_COMMON
} E_INI_COMMON_SEC;

typedef enum {
    // DB Section
    E_SUB_USER = 0,
    E_SUB_PASSWORD,
    E_SUB_DB_SID,
    E_SFF_USER,
    E_SFF_PASSWORD,
    E_SFF_DB_SID,
    E_PPI_USER,
    E_PPI_PASSWORD,
    E_PPI_DB_SID,
    E_RETRY_COUNT,
    E_RETRY_WAIT,
    E_NOF_PAR_DBCONN
} E_INI_DBCONN_SEC;

typedef struct _ir_common_ {
    char call_type[3];              // 01: 18 - GPRS, 20 - Voice MO, 21 - SMS MO, 25 - Voice MO CF, 22 - Call Forward, 26 - Call Forward CF, 45 - Call Forward CF, 30 - Voice MT, 31 - SMS MT
    char imsi[16];                  // 02: IMSI
    char st_call_date[9];           // 03: Start Call Date (format YYYYMMDD)
    char st_call_time[7];           // 04: Start Call Time (format HHMMSS)
    char duration[21];              // 05: Duration of call in second
    char called_no[25];             // 06: Calling Number / Called Number / APN
    char charge[21];                // 07: Charge amount + VAT +Markup  unit in "SDR" (decimal point is 3 position  ex. 256.789 => 256789)
    char pmn[11];                   // 08: PMN Code
    char proc_dtm[15];              // 09: Start Call Date (format YYYYMMDDHH24MISS)
    char volume[21];                // 10: Volume (Byte)
    char chrg_type[2];              // 11: 1 - Postpaid, 0 - Prepaid
    char company_name[4];           // 12: AIS, AWN
    char chrg_one_tariff[21];       // 13: Charge amount calculate from One Tariff unit in 'Baht' (decimal point is 3 position ex 256.789 => 256789)
    char th_st_call_dtm[18];        // 14: Thai date time (home)  (format yyyymmdd hh24:MM:ss)
    char called_no_type[6];         // 15: Called number flag (Thai, Local, IDD)
    char risk_no_flg[2];            // 16: Called number is risk number (Y - Risk, N - Normal)
    char risk_no_id[21];            // 17: Risk Number Pattern ID (running seq in db table)
    char billing_sys[5];            // 18: Billing System Flag (BOS, IRB, RTB, AVT, INS)
    char start_dtm[18];             // 19: Call Start date time  ( format yyyymmdd hh24:MM:ss )
    char stop_dtm[18];              // 20: Call Stop date time  ( format yyyymmdd hh24:MM:ss )
    char chrg_id[11];               // 21: Charging Id
    char utc_time[7];               // 22: UTC offset
    char total_call_evt_dur[21];    // 23: Call duration in second
    char ori_rec_type[21];          // 24: Original Record Type (Event Type ID-Sub Event Type ID-Modifier) (TAP)
    char mobile_no[21];             // 25: Mobile Number (Ano)
    char imei[21];                  // 26: IMEI
    char ori_source[4];             // 27: TAP, NRT, SCP
    char ori_filename[100];         // 28: Original input file name
    char country_code[5];           // 29: Country code
    char pmn_name[50];              // 30: PMN Name
    char roam_country[50];          // 31: Roaming Country
    char roam_region[50];           // 32: Roaming Region
} ST_IR_COMMON;

typedef enum {
    E_EVT_TYPE_ID = 0,              // 00
    E_SUB_EVT_TYPE,                 // 01
    E_MODIFIER,                     // 02
    E_TAP_IMSI,                     // 03
    E_ST_DATE,                      // 04
    E_ST_TIME,                      // 05
    E_TAP_CHRG_TIME,                // 06
    E_CALLED_NO,                    // 07
    E_AMOUNT,                       // 08
    E_PMN_CODE,                     // 09
    E_CREATED_DTM,                  // 10
    E_TAP_CHRG_ID,                  // 11
    E_UTC_TIME,                     // 12
    E_CUST_TYPE,                    // 13
    E_TAP_IMEI,                     // 14
    NOF_TAP_FLD                     // 15
} E_TAP_FLD;
//
// The following NRT format is an output of asn1conv_nrtrde.exe which is provided by eFIT - HPE ERM
//
typedef enum {
    E_EVT_CODE = 0,                 // 00
    E_SND_PMN,                      // 01
    E_REC_PMN,                      // 02
    E_SEQ_NO,                       // 03
    E_FILE_TIME,                    // 04
    E_UTC_OFFSET,                   // 05
    E_DECODE_TIME,                  // 06
    E_NRT_IMSI,                     // 07
    E_NRT_IMEI,                     // 08
    E_CALL_ST_TIME,                 // 09
    E_ROAM_UTC,                     // 10
    E_DURATION,                     // 11
    E_CAUSE_FOR_TERM,               // 12
    E_TELE_SRV_CODE,                // 13
    E_BEAR_SRV_CODE,                // 14
    E_SUPP_SRV_CODE,                // 15
    E_DIALLED_DIGITS,               // 16
    E_CONN_NO,                      // 17
    E_CALL_NO,                      // 18
    E_3RD_PART_NO,                  // 19
    E_REC_ENT_ID,                   // 20
    E_CALL_REF,                     // 21
    E_CHRG_AMNT,                    // 22
    E_SRV_NET,                      // 23
    E_UNUSED,                       // 24
    E_APN_NI,                       // 25
    E_APN_OI,                       // 26
    E_VOL_IN,                       // 27
    E_VOL_OUT,                      // 28
    E_SGSN,                         // 29
    E_GGSN,                         // 30
    E_NRT_CHRG_ID,                  // 31
    NOF_NRT_FLD                     // 32
} E_NRT_FLD;

typedef enum {
    EA_CDR_ID_01 = 0,               // 000
    EA_CDR_SUB_ID_01,               // 001
    EA_CDR_TYPE,                    // 002
    EA_SPLIT_CDR_REAS,              // 003
    EA_CDR_BAT_ID,                  // 004
    EA_SRC_REC_LINE_NO,             // 005
    EA_SRC_CDR_ID,                  // 006
    EA_SRC_CDR_NO,                  // 007
    EA_STATUS,                      // 008
    EA_RE_RATING_TIMES,             // 009
    EA_CREATE_DATE,                 // 010
    EA_START_DATE,                  // 011
    EA_END_DATE,                    // 012
    EA_CUST_LOC_START_DATE,         // 013
    EA_CUST_LOC_END_DATE,           // 014
    EA_STD_EVT_TYPE_ID,             // 015
    EA_EVT_SRC_CAT,                 // 016
    EA_OBJ_TYPE,                    // 017
    EA_OBJ_ID,                      // 018
    EA_OWNER_CUST_ID,               // 019
    EA_DEFAULT_ACCT_ID,             // 020
    EA_PRI_IDENT,                   // 021
    EA_BILL_CYC_ID_01,              // 022
    EA_SVC_CAT,                     // 023
    EA_USG_SVC_TYPE,                // 024
    EA_SESS_ID,                     // 025
    EA_RESLT_CODE,                  // 026
    EA_RESLT_REAS,                  // 027
    EA_BE_ID,                       // 028
    EA_HOT_SEQ,                     // 029
    EA_CP_ID,                       // 030
    EA_RECPT_NUM,                   // 031
    EA_USG_MEAS_ID,                 // 032
    EA_ACT_USG,                     // 033
    EA_RATE_USG,                    // 034
    EA_SVC_UNIT_TYPE,               // 035
    EA_USG_MEAS_ID2,                // 036
    EA_ACT_USG2,                    // 037
    EA_RATE_USG2,                   // 038
    EA_SVC_UNIT_TYPE2,              // 039
    EA_DEBIT_AMT,                   // 040
    EA_RESVD_042,                   // 041
    EA_DEBIT_FROM_PRE,              // 042
    EA_DEBIT_FROM_ADVC_PRE,         // 043
    EA_DEBIT_FROM_POST,             // 044
    EA_DEBIT_FROM_ADVC_POST,        // 045
    EA_DEBIT_FROM_CREDIT_POST,      // 046
    EA_TOTAL_TAX,                   // 047
    EA_FREE_UNIT_AMT_OF_TIMES,      // 048
    EA_FREE_UNIT_AMT_OF_DUR,        // 049
    EA_FREE_UNIT_AMT_OF_FLUX,       // 050
    EA_ACCT_ID_01,                  // 051
    EA_ACCT_BAL_ID_01,              // 052
    EA_BAL_TYPE_01,                 // 053
    EA_CUR_BAL_01,                  // 054
    EA_CHG_BAL_01,                  // 055
    EA_CRNCY_ID_01,                 // 056
    EA_OPER_TYPE_01,                // 057
    EA_ACCT_ID_02,                  // 058
    EA_ACCT_BAL_ID_02,              // 059
    EA_BAL_TYPE_02,                 // 060
    EA_CUR_BAL_02,                  // 061
    EA_CHG_BAL_02,                  // 062
    EA_CRNCY_ID_02,                 // 063
    EA_OPER_TYPE_02,                // 064
    EA_ACCT_ID_03,                  // 065
    EA_ACCT_BAL_ID_03,              // 066
    EA_BAL_TYPE_03,                 // 067
    EA_CUR_BAL_03,                  // 068
    EA_CHG_BAL_03,                  // 069
    EA_CRNCY_ID_03,                 // 070
    EA_OPER_TYPE_03,                // 071
    EA_ACCT_ID_04,                  // 072
    EA_ACCT_BAL_ID_04,              // 073
    EA_BAL_TYPE_04,                 // 074
    EA_CUR_BAL_04,                  // 075
    EA_CHG_BAL_04,                  // 076
    EA_CRNCY_ID_04,                 // 077
    EA_OPER_TYPE_04,                // 078
    EA_ACCT_ID_05,                  // 079
    EA_ACCT_BAL_ID_05,              // 080
    EA_BAL_TYPE_05,                 // 081
    EA_CUR_BAL_05,                  // 082
    EA_CHG_BAL_05,                  // 083
    EA_CRNCY_ID_05,                 // 084
    EA_OPER_TYPE_05,                // 085
    EA_ACCT_ID_06,                  // 086
    EA_ACCT_BAL_ID_06,              // 087
    EA_BAL_TYPE_06,                 // 088
    EA_CUR_BAL_06,                  // 089
    EA_CHG_BAL_06,                  // 090
    EA_CRNCY_ID_06,                 // 091
    EA_OPER_TYPE_06,                // 092
    EA_ACCT_ID_07,                  // 093
    EA_ACCT_BAL_ID_07,              // 094
    EA_BAL_TYPE_07,                 // 095
    EA_CUR_BAL_07,                  // 096
    EA_CHG_BAL_07,                  // 097
    EA_CRNCY_ID_07,                 // 098
    EA_OPER_TYPE_07,                // 099
    EA_ACCT_ID_08,                  // 100
    EA_ACCT_BAL_ID_08,              // 101
    EA_BAL_TYPE_08,                 // 102
    EA_CUR_BAL_08,                  // 103
    EA_CHG_BAL_08,                  // 104
    EA_CRNCY_ID_08,                 // 105
    EA_OPER_TYPE_08,                // 106
    EA_ACCT_ID_09,                  // 107
    EA_ACCT_BAL_ID_09,              // 108
    EA_BAL_TYPE_09,                 // 109
    EA_CUR_BAL_09,                  // 110
    EA_CHG_BAL_09,                  // 111
    EA_CRNCY_ID_09,                 // 112
    EA_OPER_TYPE_09,                // 113
    EA_ACCT_ID_10,                  // 114
    EA_ACCT_BAL_ID_10,              // 115
    EA_BAL_TYPE_10,                 // 116
    EA_CUR_BAL_10,                  // 117
    EA_CHG_BAL_10,                  // 118
    EA_CRNCY_ID_10,                 // 119
    EA_OPER_TYPE_10,                // 120
    EA_FU_OWN_TYPE_01,              // 121
    EA_FU_OWN_ID_01,                // 122
    EA_FREE_UNIT_ID_01,             // 123
    EA_FREE_UNIT_TYPE_01,           // 124
    EA_CUR_AMT_01,                  // 125
    EA_CHG_AMT_01,                  // 126
    EA_FU_MEAS_ID_01,               // 127
    EA_OPER_TYPE_11,                // 128
    EA_FU_OWN_TYPE_02,              // 129
    EA_FU_OWN_ID_02,                // 130
    EA_FREE_UNIT_ID_02,             // 131
    EA_FREE_UNIT_TYPE_02,           // 132
    EA_CUR_AMT_02,                  // 133
    EA_CHG_AMT_02,                  // 134
    EA_FU_MEAS_ID_02,               // 135
    EA_OPER_TYPE_12,                // 136
    EA_FU_OWN_TYPE_03,              // 137
    EA_FU_OWN_ID_03,                // 138
    EA_FREE_UNIT_ID_03,             // 139
    EA_FREE_UNIT_TYPE_03,           // 140
    EA_CUR_AMT_03,                  // 141
    EA_CHG_AMT_03,                  // 142
    EA_FU_MEAS_ID_03,               // 143
    EA_OPER_TYPE_13,                // 144
    EA_FU_OWN_TYPE_04,              // 145
    EA_FU_OWN_ID_04,                // 146
    EA_FREE_UNIT_ID_04,             // 147
    EA_FREE_UNIT_TYPE_04,           // 148
    EA_CUR_AMT_04,                  // 149
    EA_CHG_AMT_04,                  // 150
    EA_FU_MEAS_ID_04,               // 151
    EA_OPER_TYPE_14,                // 152
    EA_FU_OWN_TYPE_05,              // 153
    EA_FU_OWN_ID_05,                // 154
    EA_FREE_UNIT_ID_05,             // 155
    EA_FREE_UNIT_TYPE_05,           // 156
    EA_CUR_AMT_05,                  // 157
    EA_CHG_AMT_05,                  // 158
    EA_FU_MEAS_ID_05,               // 159
    EA_OPER_TYPE_15,                // 160
    EA_FU_OWN_TYPE_06,              // 161
    EA_FU_OWN_ID_06,                // 162
    EA_FREE_UNIT_ID_06,             // 163
    EA_FREE_UNIT_TYPE_06,           // 164
    EA_CUR_AMT_06,                  // 165
    EA_CHG_AMT_06,                  // 166
    EA_FU_MEAS_ID_06,               // 167
    EA_OPER_TYPE_16,                // 168
    EA_FU_OWN_TYPE_07,              // 169
    EA_FU_OWN_ID_07,                // 170
    EA_FREE_UNIT_ID_07,             // 171
    EA_FREE_UNIT_TYPE_07,           // 172
    EA_CUR_AMT_07,                  // 173
    EA_CHG_AMT_07,                  // 174
    EA_FU_MEAS_ID_07,               // 175
    EA_OPER_TYPE_17,                // 176
    EA_FU_OWN_TYPE_08,              // 177
    EA_FU_OWN_ID_08,                // 178
    EA_FREE_UNIT_ID_08,             // 179
    EA_FREE_UNIT_TYPE_08,           // 180
    EA_CUR_AMT_08,                  // 181
    EA_CHG_AMT_08,                  // 182
    EA_FU_MEAS_ID_08,               // 183
    EA_OPER_TYPE_18,                // 184
    EA_FU_OWN_TYPE_09,              // 185
    EA_FU_OWN_ID_09,                // 186
    EA_FREE_UNIT_ID_09,             // 187
    EA_FREE_UNIT_TYPE_09,           // 188
    EA_CUR_AMT_09,                  // 189
    EA_CHG_AMT_09,                  // 190
    EA_FU_MEAS_ID_09,               // 191
    EA_OPER_TYPE_19,                // 192
    EA_FU_OWN_TYPE_10,              // 193
    EA_FU_OWN_ID_10,                // 194
    EA_FREE_UNIT_ID_10,             // 195
    EA_FREE_UNIT_TYPE_10,           // 196
    EA_CUR_AMT_10,                  // 197
    EA_CHG_AMT_10,                  // 198
    EA_FU_MEAS_ID_10,               // 199
    EA_OPER_TYPE_20,                // 200
    EA_ACCT_ID_11,                  // 201
    EA_ACCT_BAL_ID_11,              // 202
    EA_BAL_TYPE_11,                 // 203
    EA_BONUS_AMT_01,                // 204
    EA_CUR_BAL_11,                  // 205
    EA_CRNCY_ID_11,                 // 206
    EA_OPER_TYPE_21,                // 207
    EA_ACCT_ID_12,                  // 208
    EA_ACCT_BAL_ID_12,              // 209
    EA_BAL_TYPE_12,                 // 210
    EA_BONUS_AMT_02,                // 211
    EA_CUR_BAL_12,                  // 212
    EA_CRNCY_ID_12,                 // 213
    EA_OPER_TYPE_22,                // 214
    EA_ACCT_ID_13,                  // 215
    EA_ACCT_BAL_ID_13,              // 216
    EA_BAL_TYPE_13,                 // 217
    EA_BONUS_AMT_03,                // 218
    EA_CUR_BAL_13,                  // 219
    EA_CRNCY_ID_13,                 // 220
    EA_OPER_TYPE_23,                // 221
    EA_ACCT_ID_14,                  // 222
    EA_ACCT_BAL_ID_14,              // 223
    EA_BAL_TYPE_14,                 // 224
    EA_BONUS_AMT_04,                // 225
    EA_CUR_BAL_14,                  // 226
    EA_CRNCY_ID_14,                 // 227
    EA_OPER_TYPE_24,                // 228
    EA_ACCT_ID_15,                  // 229
    EA_ACCT_BAL_ID_15,              // 230
    EA_BAL_TYPE_15,                 // 231
    EA_BONUS_AMT_05,                // 232
    EA_CUR_BAL_15,                  // 233
    EA_CRNCY_ID_15,                 // 234
    EA_OPER_TYPE_25,                // 235
    EA_ACCT_ID_16,                  // 236
    EA_ACCT_BAL_ID_16,              // 237
    EA_BAL_TYPE_16,                 // 238
    EA_BONUS_AMT_06,                // 239
    EA_CUR_BAL_16,                  // 240
    EA_CRNCY_ID_16,                 // 241
    EA_OPER_TYPE_26,                // 242
    EA_ACCT_ID_17,                  // 243
    EA_ACCT_BAL_ID_17,              // 244
    EA_BAL_TYPE_17,                 // 245
    EA_BONUS_AMT_07,                // 246
    EA_CUR_BAL_17,                  // 247
    EA_CRNCY_ID_17,                 // 248
    EA_OPER_TYPE_27,                // 249
    EA_ACCT_ID_18,                  // 250
    EA_ACCT_BAL_ID_18,              // 251
    EA_BAL_TYPE_18,                 // 252
    EA_BONUS_AMT_08,                // 253
    EA_CUR_BAL_18,                  // 254
    EA_CRNCY_ID_18,                 // 255
    EA_OPER_TYPE_28,                // 256
    EA_ACCT_ID_19,                  // 257
    EA_ACCT_BAL_ID_19,              // 258
    EA_BAL_TYPE_19,                 // 259
    EA_BONUS_AMT_09,                // 260
    EA_CUR_BAL_19,                  // 261
    EA_CRNCY_ID_19,                 // 262
    EA_OPER_TYPE_29,                // 263
    EA_ACCT_ID_20,                  // 264
    EA_ACCT_BAL_ID_20,              // 265
    EA_BAL_TYPE_20,                 // 266
    EA_BONUS_AMT_10,                // 267
    EA_CUR_BAL_20,                  // 268
    EA_CRNCY_ID_20,                 // 269
    EA_OPER_TYPE_30,                // 270
    EA_FU_OWN_TYPE_11,              // 271
    EA_FU_OWN_ID_11,                // 272
    EA_FREE_UNIT_TYPE_11,           // 273
    EA_FREE_UNIT_ID_11,             // 274
    EA_BONUS_AMT_11,                // 275
    EA_CUR_AMT_11,                  // 276
    EA_FU_MEAS_ID_11,               // 277
    EA_OPER_TYPE_31,                // 278
    EA_FU_OWN_TYPE_12,              // 279
    EA_FU_OWN_ID_12,                // 280
    EA_FREE_UNIT_TYPE_12,           // 281
    EA_FREE_UNIT_ID_12,             // 282
    EA_BONUS_AMT_12,                // 283
    EA_CUR_AMT_12,                  // 284
    EA_FU_MEAS_ID_12,               // 285
    EA_OPER_TYPE_32,                // 286
    EA_FU_OWN_TYPE_13,              // 287
    EA_FU_OWN_ID_13,                // 288
    EA_FREE_UNIT_TYPE_13,           // 289
    EA_FREE_UNIT_ID_13,             // 290
    EA_BONUS_AMT_13,                // 291
    EA_CUR_AMT_13,                  // 292
    EA_FU_MEAS_ID_13,               // 293
    EA_OPER_TYPE_33,                // 294
    EA_FU_OWN_TYPE_14,              // 295
    EA_FU_OWN_ID_14,                // 296
    EA_FREE_UNIT_TYPE_14,           // 297
    EA_FREE_UNIT_ID_14,             // 298
    EA_BONUS_AMT_14,                // 299
    EA_CUR_AMT_14,                  // 300
    EA_FU_MEAS_ID_14,               // 301
    EA_OPER_TYPE_34,                // 302
    EA_FU_OWN_TYPE_15,              // 303
    EA_FU_OWN_ID_15,                // 304
    EA_FREE_UNIT_TYPE_15,           // 305
    EA_FREE_UNIT_ID_15,             // 306
    EA_BONUS_AMT_15,                // 307
    EA_CUR_AMT_15,                  // 308
    EA_FU_MEAS_ID_15,               // 309
    EA_OPER_TYPE_35,                // 310
    EA_FU_OWN_TYPE_16,              // 311
    EA_FU_OWN_ID_16,                // 312
    EA_FREE_UNIT_TYPE_16,           // 313
    EA_FREE_UNIT_ID_16,             // 314
    EA_BONUS_AMT_16,                // 315
    EA_CUR_AMT_16,                  // 316
    EA_FU_MEAS_ID_16,               // 317
    EA_OPER_TYPE_36,                // 318
    EA_FU_OWN_TYPE_17,              // 319
    EA_FU_OWN_ID_17,                // 320
    EA_FREE_UNIT_TYPE_17,           // 321
    EA_FREE_UNIT_ID_17,             // 322
    EA_BONUS_AMT_17,                // 323
    EA_CUR_AMT_17,                  // 324
    EA_FU_MEAS_ID_17,               // 325
    EA_OPER_TYPE_37,                // 326
    EA_FU_OWN_TYPE_18,              // 327
    EA_FU_OWN_ID_18,                // 328
    EA_FREE_UNIT_TYPE_18,           // 329
    EA_FREE_UNIT_ID_18,             // 330
    EA_BONUS_AMT_18,                // 331
    EA_CUR_AMT_18,                  // 332
    EA_FU_MEAS_ID_18,               // 333
    EA_OPER_TYPE_38,                // 334
    EA_FU_OWN_TYPE_19,              // 335
    EA_FU_OWN_ID_19,                // 336
    EA_FREE_UNIT_TYPE_19,           // 337
    EA_FREE_UNIT_ID_19,             // 338
    EA_BONUS_AMT_19,                // 339
    EA_CUR_AMT_19,                  // 340
    EA_FU_MEAS_ID_19,               // 341
    EA_OPER_TYPE_39,                // 342
    EA_FU_OWN_TYPE_20,              // 343
    EA_FU_OWN_ID_20,                // 344
    EA_FREE_UNIT_TYPE_20,           // 345
    EA_FREE_UNIT_ID_20,             // 346
    EA_BONUS_AMT_20,                // 347
    EA_CUR_AMT_20,                  // 348
    EA_FU_MEAS_ID_20,               // 349
    EA_OPER_TYPE_40,                // 350
    EA_CLLG_PARTY_NUM,              // 351
    EA_CLLD_PARTY_NUM,              // 352
    EA_CLLG_PARTY_IMSI,             // 353
    EA_CLLD_PARTY_IMSI,             // 354
    EA_DIALED_NUM,                  // 355
    EA_ORIG_CLLD_PARTY,             // 356
    EA_SVC_FLOW,                    // 357
    EA_CALL_FW_IND,                 // 358
    EA_CLLG_RM_INFO,                // 359
    EA_CLLG_CELLID,                 // 360
    EA_CLLD_RM_INFO,                // 361
    EA_CLLD_CELLID,                 // 362
    EA_TIMESTAMP_OFSSP,             // 363
    EA_TIMEZONE_OFSSP,              // 364
    EA_BRER_CAP,                    // 365
    EA_CHG_TIME,                    // 366
    EA_WAIT_DUR,                    // 367
    EA_TERM_REAS,                   // 368
    EA_CALL_REF_NUM,                // 369
    EA_IMEI,                        // 370
    EA_ACCESS_PREF,                 // 371
    EA_ROUTING_PREF,                // 372
    EA_REDIRECT_PARTY_ID,           // 373
    EA_MSC_ADDR,                    // 374
    EA_BRAND_ID,                    // 375
    EA_MAIN_OFFRNG_ID,              // 376
    EA_CHG_PARTY_NUM,               // 377
    EA_CHG_PARTY_IND,               // 378
    EA_PAY_TYPE,                    // 379
    EA_CHG_TYPE,                    // 380
    EA_CALL_TYPE,                   // 381
    EA_RM_STATE,                    // 382
    EA_CLLG_HM_CNTRY_CODE,          // 383
    EA_CLLG_HM_AREA_NUM,            // 384
    EA_CLLG_HM_NET_CODE,            // 385
    EA_CLLG_RM_CNTRY_CODE,          // 386
    EA_CLLG_RM_AREA_NUM,            // 387
    EA_CLLG_RM_NET_CODE,            // 388
    EA_CLLD_HM_CNTRY_CODE,          // 389
    EA_CLLD_HM_AREA_NUM,            // 390
    EA_CLLD_HM_NET_CODE,            // 391
    EA_CLLD_RM_CNTRY_CODE,          // 392
    EA_CLLD_RM_AREA_NUM,            // 393
    EA_CLLD_RM_NET_CODE,            // 394
    EA_SVC_TYPE,                    // 395
    EA_HOT_LINE_IND,                // 396
    EA_HM_ZONEID,                   // 397
    EA_SPECIAL_ZONEID,              // 398
    EA_NP_FLG,                      // 399
    EA_NP_PREF,                     // 400
    EA_CLLG_CUGNO,                  // 401
    EA_CLLD_CUGNO,                  // 402
    EA_OPP_NUM_TYPE,                // 403
    EA_CLLG_NET_TYPE,               // 404
    EA_CLLD_NET_TYPE,               // 405
    EA_CLLG_VPN_TOPGRP_NUM,         // 406
    EA_CLLG_VPN_GRP_NUM,            // 407
    EA_CLLG_VPN_SHORT_NUM,          // 408
    EA_CLLD_VPN_TOPGRP_NUM,         // 409
    EA_CLLD_VPN_GRP_NUM,            // 410
    EA_CLLD_VPN_SHORT_NUM,          // 411
    EA_GRP_CALL_TYPE,               // 412
    EA_ONLINE_CHG_FLG,              // 413
    EA_STARTTIME_OF_BILLCYC,        // 414
    EA_LAST_EFF_OFFRNG,             // 415
    EA_DT_DISCNT,                   // 416
    EA_OPP_MAIN_OFFRNG_ID,          // 417
    EA_MAIN_BAL_INFO,               // 418
    EA_CHG_BAL_INFO,                // 419
    EA_CHG_FREE_UNIT_INFO,          // 420
    EA_USR_STATE,                   // 421
    EA_GRP_PAYFLG,                  // 422
    EA_RM_ZONEID,                   // 423
    EA_PRI_OFFR_CHG_AMT,            // 424
    EA_ORIG_IOI,                    // 425
    EA_TERM_IOI,                    // 426
    EA_IMS_CHG_IDENT,               // 427
    EA_RESVD_429,                   // 428
    EA_RECPT_CP_ID,                 // 429
    EA_CDR_ORIG_PROC_TIME,          // 430
    EA_CHG_CODE_01,                 // 431
    EA_SVC_START_GRADE_01,          // 432
    EA_SVC_GRADE_01,                // 433
    EA_SVC_USG_01,                  // 434
    EA_CRNCY_ID_21,                 // 435
    EA_ORI_FEE_WO_FU_01,            // 436
    EA_DISCNT_FEE_BY_FU_01,         // 437
    EA_FINAL_FEE_01,                // 438
    EA_FREE_UNIT_ID_21,             // 439
    EA_FREE_UNIT_TYPE_21,           // 440
    EA_FREE_UNIT_AMT_01,            // 441
    EA_TARIFF_CODE_01,              // 442
    EA_RATING_UNIT_TYPE_01,         // 443
    EA_RATING_UNIT_ID_01,           // 444
    EA_RATING_AMT_01,               // 445
    EA_DISCNT_TYPE_01,              // 446
    EA_DISCNT_AMT_01,               // 447
    EA_TARIFF_CODE_02,              // 448
    EA_RATE_INFO_01,                // 449
    EA_RATING_UNIT_TYPE_02,         // 450
    EA_RATING_UNIT_ID_02,           // 451
    EA_RATING_AMT_02,               // 452
    EA_DISCNT_TYPE_02,              // 453
    EA_DISCNT_AMT_02,               // 454
    EA_TARIFF_CODE_03,              // 455
    EA_RATE_INFO_02,                // 456
    EA_RATING_UNIT_TYPE_03,         // 457
    EA_RATING_UNIT_ID_03,           // 458
    EA_RATING_AMT_03,               // 459
    EA_DISCNT_TYPE_03,              // 460
    EA_DISCNT_AMT_03,               // 461
    EA_TARIFF_CODE_04,              // 462
    EA_RATE_INFO_03,                // 463
    EA_RATING_UNIT_TYPE_04,         // 464
    EA_RATING_UNIT_ID_04,           // 465
    EA_RATING_AMT_04,               // 466
    EA_DISCNT_TYPE_04,              // 467
    EA_DISCNT_AMT_04,               // 468
    EA_TARIFF_CODE_05,              // 469
    EA_RATE_INFO_04,                // 470
    EA_RATING_UNIT_TYPE_05,         // 471
    EA_RATING_UNIT_ID_05,           // 472
    EA_RATING_AMT_05,               // 473
    EA_DISCNT_TYPE_05,              // 474
    EA_DISCNT_AMT_05,               // 475
    EA_TARIFF_CODE_06,              // 476
    EA_RATE_INFO_05,                // 477
    EA_TIME_TARIFF_CHNG_01,         // 478
    EA_TTC_END_TIME_01,             // 479
    EA_CHG_CODE_02,                 // 480
    EA_SVC_START_GRADE_02,          // 481
    EA_SVC_GRADE_02,                // 482
    EA_SVC_USG_02,                  // 483
    EA_CRNCY_ID_22,                 // 484
    EA_ORI_FEE_WO_FU_02,            // 485
    EA_DISCNT_FEE_BY_FU_02,         // 486
    EA_FINAL_FEE_02,                // 487
    EA_FREE_UNIT_ID_22,             // 488
    EA_FREE_UNIT_TYPE_22,           // 489
    EA_FREE_UNIT_AMT_02,            // 490
    EA_TARIFF_CODE_07,              // 491
    EA_RATING_UNIT_TYPE_06,         // 492
    EA_RATING_UNIT_ID_06,           // 493
    EA_RATING_AMT_06,               // 494
    EA_DISCNT_TYPE_06,              // 495
    EA_DISCNT_AMT_06,               // 496
    EA_TARIFF_CODE_08,              // 497
    EA_RATE_INFO_06,                // 498
    EA_RATING_UNIT_TYPE_07,         // 499
    EA_RATING_UNIT_ID_07,           // 500
    EA_RATING_AMT_07,               // 501
    EA_DISCNT_TYPE_07,              // 502
    EA_DISCNT_AMT_07,               // 503
    EA_TARIFF_CODE_09,              // 504
    EA_RATE_INFO_07,                // 505
    EA_RATING_UNIT_TYPE_08,         // 506
    EA_RATING_UNIT_ID_08,           // 507
    EA_RATING_AMT_08,               // 508
    EA_DISCNT_TYPE_08,              // 509
    EA_DISCNT_AMT_08,               // 510
    EA_TARIFF_CODE_10,              // 511
    EA_RATE_INFO_08,                // 512
    EA_RATING_UNIT_TYPE_09,         // 513
    EA_RATING_UNIT_ID_09,           // 514
    EA_RATING_AMT_09,               // 515
    EA_DISCNT_TYPE_09,              // 516
    EA_DISCNT_AMT_09,               // 517
    EA_TARIFF_CODE_11,              // 518
    EA_RATE_INFO_09,                // 519
    EA_RATING_UNIT_TYPE_10,         // 520
    EA_RATING_UNIT_ID_10,           // 521
    EA_RATING_AMT_10,               // 522
    EA_DISCNT_TYPE_10,              // 523
    EA_DISCNT_AMT_10,               // 524
    EA_TARIFF_CODE_12,              // 525
    EA_RATE_INFO_10,                // 526
    EA_TIME_TARIFF_CHNG_02,         // 527
    EA_TTC_END_TIME_02,             // 528
    EA_CHG_CODE_03,                 // 529
    EA_SVC_START_GRADE_03,          // 530
    EA_SVC_GRADE_03,                // 531
    EA_SVC_USG_03,                  // 532
    EA_CRNCY_ID_23,                 // 533
    EA_ORI_FEE_WO_FU_03,            // 534
    EA_DISCNT_FEE_BY_FU_03,         // 535
    EA_FINAL_FEE_03,                // 536
    EA_FREE_UNIT_ID_23,             // 537
    EA_FREE_UNIT_TYPE_23,           // 538
    EA_FREE_UNIT_AMT_03,            // 539
    EA_TARIFF_CODE_13,              // 540
    EA_RATING_UNIT_TYPE_11,         // 541
    EA_RATING_UNIT_ID_11,           // 542
    EA_RATING_AMT_11,               // 543
    EA_DISCNT_TYPE_11,              // 544
    EA_DISCNT_AMT_11,               // 545
    EA_TARIFF_CODE_14,              // 546
    EA_RATE_INFO_11,                // 547
    EA_RATING_UNIT_TYPE_12,         // 548
    EA_RATING_UNIT_ID_12,           // 549
    EA_RATING_AMT_12,               // 550
    EA_DISCNT_TYPE_12,              // 551
    EA_DISCNT_AMT_12,               // 552
    EA_TARIFF_CODE_15,              // 553
    EA_RATE_INFO_12,                // 554
    EA_RATING_UNIT_TYPE_13,         // 555
    EA_RATING_UNIT_ID_13,           // 556
    EA_RATING_AMT_13,               // 557
    EA_DISCNT_TYPE_13,              // 558
    EA_DISCNT_AMT_13,               // 559
    EA_TARIFF_CODE_16,              // 560
    EA_RATE_INFO_13,                // 561
    EA_RATING_UNIT_TYPE_14,         // 562
    EA_RATING_UNIT_ID_14,           // 563
    EA_RATING_AMT_14,               // 564
    EA_DISCNT_TYPE_14,              // 565
    EA_DISCNT_AMT_14,               // 566
    EA_TARIFF_CODE_17,              // 567
    EA_RATE_INFO_14,                // 568
    EA_RATING_UNIT_TYPE_15,         // 569
    EA_RATING_UNIT_ID_15,           // 570
    EA_RATING_AMT_15,               // 571
    EA_DISCNT_TYPE_15,              // 572
    EA_DISCNT_AMT_15,               // 573
    EA_TARIFF_CODE_18,              // 574
    EA_RATE_INFO_15,                // 575
    EA_TIME_TARIFF_CHNG_03,         // 576
    EA_TTC_END_TIME_03,             // 577
    EA_CHG_CODE_04,                 // 578
    EA_SVC_START_GRADE_04,          // 579
    EA_SVC_GRADE_04,                // 580
    EA_SVC_USG_04,                  // 581
    EA_CRNCY_ID_24,                 // 582
    EA_ORI_FEE_WO_FU_04,            // 583
    EA_DISCNT_FEE_BY_FU_04,         // 584
    EA_FINAL_FEE_04,                // 585
    EA_FREE_UNIT_ID_24,             // 586
    EA_FREE_UNIT_TYPE_24,           // 587
    EA_FREE_UNIT_AMT_04,            // 588
    EA_TARIFF_CODE_19,              // 589
    EA_RATING_UNIT_TYPE_16,         // 590
    EA_RATING_UNIT_ID_16,           // 591
    EA_RATING_AMT_16,               // 592
    EA_DISCNT_TYPE_16,              // 593
    EA_DISCNT_AMT_16,               // 594
    EA_TARIFF_CODE_20,              // 595
    EA_RATE_INFO_16,                // 596
    EA_RATING_UNIT_TYPE_17,         // 597
    EA_RATING_UNIT_ID_17,           // 598
    EA_RATING_AMT_17,               // 599
    EA_DISCNT_TYPE_17,              // 600
    EA_DISCNT_AMT_17,               // 601
    EA_TARIFF_CODE_21,              // 602
    EA_RATE_INFO_17,                // 603
    EA_RATING_UNIT_TYPE_18,         // 604
    EA_RATING_UNIT_ID_18,           // 605
    EA_RATING_AMT_18,               // 606
    EA_DISCNT_TYPE_18,              // 607
    EA_DISCNT_AMT_18,               // 608
    EA_TARIFF_CODE_22,              // 609
    EA_RATE_INFO_18,                // 610
    EA_RATING_UNIT_TYPE_19,         // 611
    EA_RATING_UNIT_ID_19,           // 612
    EA_RATING_AMT_19,               // 613
    EA_DISCNT_TYPE_19,              // 614
    EA_DISCNT_AMT_19,               // 615
    EA_TARIFF_CODE_23,              // 616
    EA_RATE_INFO_19,                // 617
    EA_RATING_UNIT_TYPE_20,         // 618
    EA_RATING_UNIT_ID_20,           // 619
    EA_RATING_AMT_20,               // 620
    EA_DISCNT_TYPE_20,              // 621
    EA_DISCNT_AMT_20,               // 622
    EA_TARIFF_CODE_24,              // 623
    EA_RATE_INFO_20,                // 624
    EA_TIME_TARIFF_CHNG_04,         // 625
    EA_TTC_END_TIME_04,             // 626
    EA_CHG_CODE_05,                 // 627
    EA_SVC_START_GRADE_05,          // 628
    EA_SVC_GRADE_05,                // 629
    EA_SVC_USG_05,                  // 630
    EA_CRNCY_ID_25,                 // 631
    EA_ORI_FEE_WO_FU_05,            // 632
    EA_DISCNT_FEE_BY_FU_05,         // 633
    EA_FINAL_FEE_05,                // 634
    EA_FREE_UNIT_ID_25,             // 635
    EA_FREE_UNIT_TYPE_25,           // 636
    EA_FREE_UNIT_AMT_05,            // 637
    EA_TARIFF_CODE_25,              // 638
    EA_RATING_UNIT_TYPE_21,         // 639
    EA_RATING_UNIT_ID_21,           // 640
    EA_RATING_AMT_21,               // 641
    EA_DISCNT_TYPE_21,              // 642
    EA_DISCNT_AMT_21,               // 643
    EA_TARIFF_CODE_26,              // 644
    EA_RATE_INFO_21,                // 645
    EA_RATING_UNIT_TYPE_22,         // 646
    EA_RATING_UNIT_ID_22,           // 647
    EA_RATING_AMT_22,               // 648
    EA_DISCNT_TYPE_22,              // 649
    EA_DISCNT_AMT_22,               // 650
    EA_TARIFF_CODE_27,              // 651
    EA_RATE_INFO_22,                // 652
    EA_RATING_UNIT_TYPE_23,         // 653
    EA_RATING_UNIT_ID_23,           // 654
    EA_RATING_AMT_23,               // 655
    EA_DISCNT_TYPE_23,              // 656
    EA_DISCNT_AMT_23,               // 657
    EA_TARIFF_CODE_28,              // 658
    EA_RATE_INFO_23,                // 659
    EA_RATING_UNIT_TYPE_24,         // 660
    EA_RATING_UNIT_ID_24,           // 661
    EA_RATING_AMT_24,               // 662
    EA_DISCNT_TYPE_24,              // 663
    EA_DISCNT_AMT_24,               // 664
    EA_TARIFF_CODE_29,              // 665
    EA_RATE_INFO_24,                // 666
    EA_RATING_UNIT_TYPE_25,         // 667
    EA_RATING_UNIT_ID_25,           // 668
    EA_RATING_AMT_25,               // 669
    EA_DISCNT_TYPE_25,              // 670
    EA_DISCNT_AMT_25,               // 671
    EA_TARIFF_CODE_30,              // 672
    EA_RATE_INFO_25,                // 673
    EA_TIME_TARIFF_CHNG_05,         // 674
    EA_TTC_END_TIME_05,             // 675
    EA_SERIAL_NO_01,                // 676
    EA_CDR_ID_02,                   // 677
    EA_CDR_SUB_ID_02,               // 678
    EA_PAY_ACCT_ID_01,              // 679
    EA_PAY_OBJ_ID_01,               // 680
    EA_PAY_OBJ_TYPE_01,             // 681
    EA_PAY_OBJ_TYPE_ID_01,          // 682
    EA_CHG_CODE_06,                 // 683
    EA_OFFRNG_ID_01,                // 684
    EA_PLAN_ID_01,                  // 685
    EA_DEDUCT_CHG_AMT_01,           // 686
    EA_TAX_CODE1_01,                // 687
    EA_INCSV_TAX_FLG1_01,           // 688
    EA_TAX_AMT1_01,                 // 689
    EA_TAX_CODE2_01,                // 690
    EA_INCSV_TAX_FLG2_01,           // 691
    EA_TAX_AMT2_01,                 // 692
    EA_TAX_CODE3_01,                // 693
    EA_INCSV_TAX_FLG3_01,           // 694
    EA_TAX_AMT3_01,                 // 695
    EA_CHG_CRNCY_ID_01,             // 696
    EA_PAY_CRNCY_ID_01,             // 697
    EA_PAY_FLG_01,                  // 698
    EA_CUST_ID_01,                  // 699
    EA_BILL_CYC_ID_02,              // 700
    EA_CP_BILL_CYC_ID_01,           // 701
    EA_SERIAL_NO_02,                // 702
    EA_CDR_ID_03,                   // 703
    EA_CDR_SUB_ID_03,               // 704
    EA_PAY_ACCT_ID_02,              // 705
    EA_PAY_OBJ_ID_02,               // 706
    EA_PAY_OBJ_TYPE_02,             // 707
    EA_PAY_OBJ_TYPE_ID_02,          // 708
    EA_CHG_CODE_07,                 // 709
    EA_OFFRNG_ID_02,                // 710
    EA_PLAN_ID_02,                  // 711
    EA_DEDUCT_CHG_AMT_02,           // 712
    EA_TAX_CODE1_02,                // 713
    EA_INCSV_TAX_FLG1_02,           // 714
    EA_TAX_AMT1_02,                 // 715
    EA_TAX_CODE2_02,                // 716
    EA_INCSV_TAX_FLG2_02,           // 717
    EA_TAX_AMT2_02,                 // 718
    EA_TAX_CODE3_02,                // 719
    EA_INCSV_TAX_FLG3_02,           // 720
    EA_TAX_AMT3_02,                 // 721
    EA_CHG_CRNCY_ID_02,             // 722
    EA_PAY_CRNCY_ID_02,             // 723
    EA_PAY_FLG_02,                  // 724
    EA_CUST_ID_02,                  // 725
    EA_BILL_CYC_ID_03,              // 726
    EA_CP_BILL_CYC_ID_02,           // 727
    EA_SERIAL_NO_03,                // 728
    EA_CDR_ID_04,                   // 729
    EA_CDR_SUB_ID_04,               // 730
    EA_PAY_ACCT_ID_03,              // 731
    EA_PAY_OBJ_ID_03,               // 732
    EA_PAY_OBJ_TYPE_03,             // 733
    EA_PAY_OBJ_TYPE_ID_03,          // 734
    EA_CHG_CODE_08,                 // 735
    EA_OFFRNG_ID_03,                // 736
    EA_PLAN_ID_03,                  // 737
    EA_DEDUCT_CHG_AMT_03,           // 738
    EA_TAX_CODE1_03,                // 739
    EA_INCSV_TAX_FLG1_03,           // 740
    EA_TAX_AMT1_03,                 // 741
    EA_TAX_CODE2_03,                // 742
    EA_INCSV_TAX_FLG2_03,           // 743
    EA_TAX_AMT2_03,                 // 744
    EA_TAX_CODE3_03,                // 745
    EA_INCSV_TAX_FLG3_03,           // 746
    EA_TAX_AMT3_03,                 // 747
    EA_CHG_CRNCY_ID_03,             // 748
    EA_PAY_CRNCY_ID_03,             // 749
    EA_PAY_FLG_03,                  // 750
    EA_CUST_ID_03,                  // 751
    EA_BILL_CYC_ID_04,              // 752
    EA_CP_BILL_CYC_ID_03,           // 753
    EA_SERIAL_NO_04,                // 754
    EA_CDR_ID_05,                   // 755
    EA_CDR_SUB_ID_05,               // 756
    EA_PAY_ACCT_ID_04,              // 757
    EA_PAY_OBJ_ID_04,               // 758
    EA_PAY_OBJ_TYPE_04,             // 759
    EA_PAY_OBJ_TYPE_ID_04,          // 760
    EA_CHG_CODE_09,                 // 761
    EA_OFFRNG_ID_04,                // 762
    EA_PLAN_ID_04,                  // 763
    EA_DEDUCT_CHG_AMT_04,           // 764
    EA_TAX_CODE1_04,                // 765
    EA_INCSV_TAX_FLG1_04,           // 766
    EA_TAX_AMT1_04,                 // 767
    EA_TAX_CODE2_04,                // 768
    EA_INCSV_TAX_FLG2_04,           // 769
    EA_TAX_AMT2_04,                 // 770
    EA_TAX_CODE3_04,                // 771
    EA_INCSV_TAX_FLG3_04,           // 772
    EA_TAX_AMT3_04,                 // 773
    EA_CHG_CRNCY_ID_04,             // 774
    EA_PAY_CRNCY_ID_04,             // 775
    EA_PAY_FLG_04,                  // 776
    EA_CUST_ID_04,                  // 777
    EA_BILL_CYC_ID_05,              // 778
    EA_CP_BILL_CYC_ID_04,           // 779
    EA_SERIAL_NO_05,                // 780
    EA_CDR_ID_06,                   // 781
    EA_CDR_SUB_ID_06,               // 782
    EA_PAY_ACCT_ID_05,              // 783
    EA_PAY_OBJ_ID_05,               // 784
    EA_PAY_OBJ_TYPE_05,             // 785
    EA_PAY_OBJ_TYPE_ID_05,          // 786
    EA_CHG_CODE_10,                 // 787
    EA_OFFRNG_ID_05,                // 788
    EA_PLAN_ID_05,                  // 789
    EA_DEDUCT_CHG_AMT_05,           // 790
    EA_TAX_CODE1_05,                // 791
    EA_INCSV_TAX_FLG1_05,           // 792
    EA_TAX_AMT1_05,                 // 793
    EA_TAX_CODE2_05,                // 794
    EA_INCSV_TAX_FLG2_05,           // 795
    EA_TAX_AMT2_05,                 // 796
    EA_TAX_CODE3_05,                // 797
    EA_INCSV_TAX_FLG3_05,           // 798
    EA_TAX_AMT3_05,                 // 799
    EA_CHG_CRNCY_ID_05,             // 800
    EA_PAY_CRNCY_ID_05,             // 801
    EA_PAY_FLG_05,                  // 802
    EA_CUST_ID_05,                  // 803
    EA_BILL_CYC_ID_06,              // 804
    EA_CP_BILL_CYC_ID_05,           // 805
    EA_BRER_SVC_CODE,               // 806
    EA_TELE_SVC_CODE,               // 807
    EA_TRNSF_CAP,                   // 808
    EA_USR_INFO_LAYER1_PROT,        // 809
    EA_OVER_DRAFT_AMT,              // 810
    EA_OFFER_SEQID,                 // 811
    EA_LOC_NUM,                     // 812
    EA_CLLG_RM_CNTRY_CODE_VIR,      // 813
    EA_PRVNC_NAME_DOME,             // 814
    EA_DEST_PRVNC_NAME_DOME,        // 815
    EA_DEST_CNTRY_NAME_IDD,         // 816
    EA_RM_CNTRY_NAME_IR,            // 817
    EA_RM_PLMN_CODE_IR,             // 818
    EA_RM_NET_NAME_IR,              // 819
    EA_DEST_NET_NAME_DOME,          // 820
    EA_CONV_ACT_USG,                // 821
    EA_CONV_RATE_USG,               // 822
    EA_RM_TIME,                     // 823
    NOF_SCP_FLD                     // 824
} E_SCP_FLD;    // Avatar 824 field, avg byte per field = 31, est. record size = 25,600 bytes (25 kB)

typedef enum {
    E_RDUMMY = 0,
    NOF_RTB_FLD
} E_RTB_FLD;


int     buildSnapFile(const char *snapfile);
int     _chkTapFile(const char *fpath, const struct stat *info, int typeflag, struct FTW *ftwbuf);
int     _chkNrtFile(const char *fpath, const struct stat *info, int typeflag, struct FTW *ftwbuf);
int     _chkScpFile(const char *fpath, const struct stat *info, int typeflag, struct FTW *ftwbuf);
int     _chkRtbFile(const char *fpath, const struct stat *info, int typeflag, struct FTW *ftwbuf);
int     chkSnapVsState(const char *snap);
void    procSynFiles(const char *dir, const char *fname, const char *ir_type, long cont_pos);
int     olderThan(int day, const char *sdir, const char *fname);
int     (*verifyField)(char *pbuf[], int bsize, const char *fname, char *err_msg);
int     verifyInpFieldTap(char *pbuf[], int bsize, const char *fname, char *err_msg);
int     verifyInpFieldNrt(char *pbuf[], int bsize, const char *fname, char *err_msg);
int     verifyInpFieldScp(char *pbuf[], int bsize, const char *fname, char *err_msg);
int     verifyInpFieldRtb(char *pbuf[], int bsize, const char *fname, char *err_msg);
int     calcOneTariff();
int     calcDurCharge(int duration, int one_tariff);
int     wrtOutIrCommon(const char *odir, const char *ir_type, const char *file_dtm, FILE **ofp);
int     wrtOutReject(const char *odir, const char *fname, FILE **ofp, const char *record);
int     wrtAlrtMismatchImsi(const char *odir, const char *fname, FILE **ofp, const char *imsi);
int     wrtAlrtNotExactPmn(const char *odir, const char *fname, FILE **ofp, const char *pmn, const char *chrg_type);
int     wrtAlrtDbConnFail(const char *odir, const char *fname, const char *dbsvr);

int     manageMapTab();

int     logState(const char *dir, const char *file_name, const char *ir_type);
void    clearOldState();
void    purgeOldData(const char *old_state);
int     readConfig(int argc, char *argv[]);
void    logHeader();
void    printUsage();
int     validateIni();
int     _ini_callback(const char *section, const char *key, const char *value, void *userdata);
void    makeIni();


#ifdef  __cplusplus
    }
#endif


#endif  // __IR_RATE_H__

