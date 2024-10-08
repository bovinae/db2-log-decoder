/******************************************************************************
**
** Source File Name: SQLUV
**
** (C) COPYRIGHT International Business Machines Corp. 1995,1997
** All Rights Reserved
** Licensed Materials - Property of IBM
**
** US Government Users Restricted Rights - Use, duplication or
** disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
**
** Function = Include File defining:
**              Vendor - Constants
**              Vendor - Data Structures
**              Vendor - Function Prototypes
**              Vendor - Labels for SQLCODES
**
*******************************************************************************/
#ifndef SQLUV_INCLUDED
#define SQLUV_INCLUDED

#include "sqlutil.h"
#include "sqlenv.h"
#include "db2ApiDf.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(DB2NT)
#if defined _WIN64
#pragma pack(8)
#else
#pragma pack(4)
#endif
#elif (defined(DB2AIX) && defined(__64BIT__))
#pragma options align=natural
#elif (defined(DB2AIX))
#pragma options align=power
#endif

/*
** external defines for vendor unload and load APIs
*/

/*
** sqluvtun() callerac values
*/
#define SQLU_CALLERAC_UNLOAD_INIT         0
#define SQLU_CALLERAC_UNLOAD_COMPLETE_OK  2
#define SQLU_CALLERAC_UNLOAD_INTERNAL     5
#define SQLU_CALLERAC_UNLOAD_ABORT        9

/*
** sqluvtld() callerac values
*/
#define SQLU_CALLERAC_LOAD_INIT         0
#define SQLU_CALLERAC_LOAD_RESTART      1
#define SQLU_CALLERAC_LOAD_COMPLETE_OK  2
#define SQLU_CALLERAC_LOAD_INTERNAL     5
#define SQLU_CALLERAC_LOAD_ABORT        9
#define SQLU_CALLERAC_LOAD_INTERRUPT    10

/*
** sqluvtld() optype values
*/
#define SQLU_OPTYPE_LOAD_REPLACE 'R'
#define SQLU_OPTYPE_LOAD_INSERT  'I'
#define SQLU_OPTYPE_LOAD_ANALYZE 'A'
#define SQLU_OPTYPE_LOAD_REPLACE_RDICT 'S'
#define SQLU_OPTYPE_LOAD_REPLACE_KDICT 'K'
#define SQLU_OPTYPE_LOAD_REPLACE_ODICT 'O'

/*
** sqluvtld() copy_flag values
*/
#define SQLU_NO_COPY_MADE 0
#define SQLU_COPY_MADE    1

/*
** Backup and load copy image structures
*/

/* Eyecatcher strings
*/
#define SQLU_EYECAT_LEN  13
#define MEDIA_HEAD             "SQLUBRMEDHEAD "
#define OBJECT_HEAD            "SQLUBROBJHEAD "
#define BACKUP_TAIL            "SQLUBRCKTAIL  "
#define MH_TAIL                "SQLUBRMHTAIL  "

/*
** media header constants
*/
#define SQLU_DBNAME_SZ    8
#define SQLU_CMT_SZ       30
#define SQLU_LOGPATH_LEN  255

/*
** dbCfgType constants
*/
#define SQLUB_DBCFGTYPE_NORMAL       0
#define SQLUB_DBCFGTYPE_SHARED_DATA  1

/* additional backup types - "external" backup types are defined in sqlutil.h
*/
#define SQLUB_LOAD_COPY  4
#define SQLUB_UNLOAD     5

#define SQLU_MEDIA_HEAD_RSRVD_SPACE   4096

typedef struct SQLUV_BMH
{
   unsigned char           eyecat[16];                /* DBM B/R Media Header eyecatcher    */
                                                      /* 'SQLUBRMEDHEAD' left justified,    */
                                                      /* padded with blanks                 */
   unsigned char           serverDBAlias[SQLU_ALIAS_SZ+1];   /* Server database alias       */
   unsigned char           timestamp[SQLU_TIME_STAMP_LEN+1]; /* timestamp of backup         */
   db2NodeType             dbnode;                    /* My partition number                */
   unsigned char           db2instance[SQL_INSTNAME_SZ+1];   /* db2instance name            */
   unsigned char           dbCfgType;                 /* Database configuration type:       */
                                                      /*  0 = SQLUB_DBCFGTYPE_NORMAL        */
                                                      /*  1 = SQLUB_DBCFGTYPE_SHARED_DATA   */
   unsigned short          sequence;                  /* The sequence number of this backup */
   db2NodeType             member_id;                 /* My member id                       */
   sqluint32               size;                      /* Size of the B/R Media Header       */
   unsigned short          rel_id;                    /* DBM release id                     */
   unsigned char           serverDBName[SQLU_DBNAME_SZ+1];   /* Server database name        */
   unsigned char           mergedBackupImg;           /* Backup image is a merged image     */
                                                      /* 0 = no, 1 = yes                    */

   sqluint32               db_seed;                   /* Database seed                      */
   signed short            com_codepage;              /* Database comment's codepage (volume) */
   unsigned char           comment[SQLU_CMT_SZ+1];    /* Database comment (volume)          */
   unsigned char           logs;                      /* Backup includes logs.              */
                                                      /* 0 = no, 1 = yes                    */
   signed short            d_com_codepage;            /* Database comment's codepage (system) */
   unsigned char           d_comment[SQLU_CMT_SZ+1];  /* Database comment (system)          */
   unsigned char           authentication;            /* Database authentication value      */
   unsigned char           backup_mode;               /* Mode of the backup                 */
                                                      /* '0' = offline                      */
                                                      /* '1' = online                       */
   unsigned char           compr;                     /* Backup is compressed               */
                                                      /* 0 = no, 1 = yes                    */
   unsigned short          dbStatusFlags;             /* status flags such as:              */
                                                      /* SQLF_DBTN_CONSISTENT (111)         */
                                                      /* SQLF_DBTN_BACKUP_PENDING (112)     */
                                                      /* SQLF_DBTN_ROLLFWD_PENDING (113)    */
                                                      /* SQLF_DBTN_LOG_RETAIN_STATUS (114)  */
                                                      /* SQLF_DBTN_USER_EXIST_STATUS (115)  */

   unsigned char           bkp_granu;                 /* backup granularity:                */
                                                      /*     '0' =  database                */
   unsigned char           backup_type;               /* backup type:                       */
                                                      /*     '0' =  SQLUB_FULL              */
                                                      /*     '3' =  SQLUB_TABLESPAC         */
                                                      /*     '4' =  SQLUB_LOAD_COPY         */
   SQLE_DB_TERRITORY_INFO  dbcinfo;                   /* code page info                     */
   unsigned char           isRRL;                     /* reduced redo logging enabled       */
   unsigned char           is2ndPhase;                /* this seq is part of data phase 2   */
   sqluint32               LogID;
   unsigned char           LogPath[SQLU_LOGPATH_LEN+5];
   sqluint32               backupBufferSize;          /* buffer size used during backup     */
                                                      /* Useful to determine restore buf    */
                                                      /* size. */
   sqluint32               sessions;                  /* number of sessions used for backup */

   sqluint32               platform;                  /* Platform                           */
   unsigned short          systemCatsIncluded;        /* indicate if the system catalogs    */
                                                      /* are included in this backup        */
                                                      /* 0 = NO,  1 = YES                   */
   unsigned char           clientDBAlias[SQLU_ALIAS_SZ+1];/* Client database alias          */
   db2NodeType             cat_node;                  /* Catalog node number                */
   unsigned short          valRawDevHeadExt;  /* indicate if rawDevHeadExt needs to be validated */
                                              /* 0 = NO,  1 = YES */
   sqluint32               rawDevHeadExt;     /* first extent on raw device for log */
   sqluint32               start_time;        /* Backup start time */

   // Kepler+ version field. Future migration will use this rather than the
   // old rel_id. In general BAR has always only made changes to the backup
   // image format on major release boundaries (which the old rel_id provides),
   // but this will allow us finer-grained control in the future shoudl we
   // decide to follow the RU model of allowing changes in fixpacks.
   //
   // Once pre-Kepler releases are outside of restore limitations we can remove
   // the old rel_id.
   // -------------------------------------------------------------------------
   sqluint64                  alVersion;

   // 105fp5 added the following fields
   // -------------------------------------------------------------------------
   unsigned char           encryptionDEK[552];
   sqluint16               encryptInfoFlags;
   unsigned char           pad[6];            /* Extra padding at the end so struct size is aligned to 8-bytes */
} SQLUV_BMH;

/* Backup and copy image object Types
*/
#define SQLUDAT               0x00
#define SQLUINX               0x01
#define SQLULF                0x02
#define SQLULOB               0x03
#define SQLULOBA              0x04
#define SQLUDATALINKCFGFILE   0x05
#define SQLUMDCBMP            0x06
#define SQLUGLOBALCFG         0x07
#define SQLUCNF               SQLUGLOBALCFG
#define SQLULFH               0x08
#define SQLULOG               SQLULFH
#define SQLULISTOFTABLESPACES 0x09
#define SQLUHISTORYFILE       0x0a
#define SQLUTABLESPACETABLE   0x0b
#define SQLUDMSTABLESPACEDATA 0x0c
#define SQLUBACKUPTAIL        0x0d
#define SQLUDMSLOBDATA        0x0e
#define SQLUDMSLONGDATA       0x0f
#define SQLUSMSLOBDATA        0x10
#define SQLUSMSLONGDATA       0x11
#define SQLUBUFFERPOOLFILE    0x12
#define SQLUSTARTBACKUPREC    0x13
#define SQLUDLCONTROLINFO     0x14
#define SQLUDLMREQUESTS       0x15
#define SQLUDBFILE            0x16
#define SQLUSMSLOBFREESPACE   0x17
#define SQLUSMSLFFREESPACE    0x18
#define SQLUMIRRORLOG         0x19
#define SQLULOGFILE           0x1a
#define SQLUCOMPRLIB          0x1b
#define SQLUCOMPRLIB_DATA     0x1c
#define SQLUSTORAGEGROUPFILE  0x1d
#define SQLUXDA               0x1e
#define SQLUENDOFINITIALDATA  0x1f
#define SQLUDMSINDEXDATA      0x20
#define SQLUDMSXMLDATA        0x21
#define SQLUSMSXMLDATA        0x22
#define SQLUREDISTCTRLFILE    0x23
#define SQLUGLFH              0x24
#define SQLUVENDORDATA_V95    0x24        /* Value used for v9.5 and v9.7     */
                                          /* releases.                        */
#define SQLULOGATBKP          0x25
#define SQLUMEMBERCFG         0x26
#define SQLUVENDORDATA        0x27
#define SQLUPADDING           0x28
#define SQLUXTIDFILE          0x29
#define SQLUXMETAFILE         0x2a
#define SQLUBRGFILE           0xff

/* type def for parameters
*/
typedef unsigned short SQLU_TOKEN;
typedef unsigned char  SQLU_TYPE ;

/*
 * Parameters for 'flags'
 */

// Object is compressed
// ----------------------------------------------------------------------------
#define SQLUB_COMPRESSED           0x0010

/* Object Header Structure
*/

#define SQLU_OBJ_HEAD_RSRVD_SPACE   512
typedef struct sqluboh
{
     char           eyecat[16];           /* DBM B/R Object Header eyecatcher */
                                          /* 'SQLUBROBJHEAD '                 */
     sqluint32      size;                 /* Size of the B/R Object Header    */
     SQLU_TYPE      type;                 /* Type of object                   */
     char           padding1;             /* padding                          */
     unsigned short token;                /* Token of the object              */
     sqluint32      tableSpaceID;         /* Unique table space identifier    */
     char           objname[SQL_MAX_IDENT + SQL_MAX_IDENT + 2];
                                          /* "schema.tablename"               */
     sqluint16      flags;
     sqluint64      filesize;             /* Bytes in special files, pages    */
                                          /*   in data objects.               */
     sqluint64      objsize;              /* Size of the Object in bytes      */
     sqluint64      offset;               /* # of buffers into the file       */

     /* Added in DB2 V8.1                                                     */
     sqluint64      originalSize;         /* Uncompressed objsize             */
     sqluint32      originalCRC;          /* CRC of uncompressed data         */

     db2NodeType    memberNum;            /* Used for per-member metadata     */
     char           padding2[2];          /* padding                          */
} SQLU_OH_T ;

/* Backup Tail Structure
*/
typedef struct sqlubtail
{
     char           eyecat[16];          /* DBM B/R Backup Tail eyecatcher    */
                                         /* 'SQLUBRBCKTAIL'                   */
     sqluint32      size;                /* Size of the Backup Tail structure */
     sqluint32      backupImagesUsed;    /* Number of images created for the  */
                                         /* backup or copy                    */
} SQLUB_TAIL_T;

/* If an object of SQLUVENDORDATA is found in a backup image, this structure
 * must be contained at the beginning of that object to identify what vendor
 * placed this object in the backup image.
 */
#define DB2VENDORDATA_SZ          256
#define DB2VENDOR_MAX_VENDORID_SZ 63
typedef struct db2VendorData
{
   sqluint32  version;                              /* Vendor version        */
   sqluint32  release;                              /* Vendor release        */
   sqluint32  level;                                /* Vendor level          */
   char       signature[DB2VENDOR_MAX_VENDORID_SZ + 1]; /* Vendor id         */
   char       reserved[180];                        /* For future use        */
} db2VendorData;


/*
** APIs to support vendor unload and load utilities
*/

SQL_API_RC  SQL_API_FN
    sqluvtun ( char          *table_name,      /* fully qualified table name */
               sqlint32       callerac,        /* caller action              */
               struct sqlu_media_list          /* list of media target       */
                             *copy_target_list,
               void          *reserve0,        /* reserve for internal use   */
               void          *reserve,         /* reserve for future use     */
               struct sqlca  *sqlca);          /* SQLCA                      */


SQL_API_RC  SQL_API_FN
    sqluvtld ( char           *table_name,      /* fully qualified table name */
               sqlint32        callerac,        /* caller action              */
               char            optype,          /* operation type             */
               sqlint32        copy_flag,       /* indicate copy is done      */
               struct sqlu_media_list
                              *src_location,    /* source location            */
               struct sqlu_media_list           /* list of media target       */
                              *media_list,
               SQLU_LSN       *lsn,             /* log sequence number        */
               SQLUV_BMH      *media_header,    /* media header for copy      */
               void           *reserve0,        /* reserved                   */
               void           *reserve,         /* reserve for future use     */
               struct sqlca   *sqlca);          /* SQLCA                      */

#if defined(DB2NT)
#pragma pack()
#elif defined(DB2AIX)
#pragma options align=reset
#endif

#ifdef __cplusplus
}
#endif

#endif  /* SQLUV_INCLUDED */
