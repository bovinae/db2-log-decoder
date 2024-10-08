      *************************************************************************
      * 
      * Source File Name: SQLUTBSQ
      * 
      * (C) COPYRIGHT International Business Machines Corp. 1987, 1995
      * All Rights Reserved
      * Licensed Materials - Property of IBM
      * 
      * US Government Users Restricted Rights - Use, duplication or
      * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
      * 
      * Function: Table space query
      * 
      * Operating System: LINUX
      * 
      **************************************************************************
       01 SQLB-TBSQRY-DATA.
      * internal id for the pool
           05 SQL-ID                 PIC 9(9) COMP-5.
      * length of the name (for cobol & other nonC lang.)
           05 SQL-NAME-LEN           PIC 9(9) COMP-5.
      * NUL terminated name
           05 SQL-NAME               PIC X(128).
      * DMS : Pages specified by CREATE TABLESPACE.
      * SMS : N/A
           05 SQL-TOTAL-PAGES        PIC 9(9) COMP-5.
      * DMS : totalPages - overhead
      * SMS : N/A
           05 SQL-USEABLE-PAGES      PIC 9(9) COMP-5.
      * bit attributes for the pool -- includes 'class' & other stuff to be
      * defined
           05 SQL-FLAGS              PIC 9(9) COMP-5.
      * pagesize of tablespace (in bytes) .. one of 4K, 8K, 16K, or 32K
           05 SQL-PAGE-SIZE          PIC 9(9) COMP-5.
      * extent size (in pages)
           05 SQL-EXT-SIZE           PIC 9(9) COMP-5.
      * prefetch size
           05 SQL-PREFETCH-SIZE      PIC 9(9) COMP-5.
      * number of containers
           05 SQL-N-CONTAINERS       PIC 9(9) COMP-5.
      * SQLB_NORMAL, SQLB_LOAD_PENDING, etc.
           05 SQL-TBS-STATE          PIC 9(9) COMP-5.
      * 'timestamp' identifying the origin of the tbs
           05 SQL-LIFE-LSN           PIC X(6).
      * for alignment
           05 SQL-PAD                PIC X(2).
