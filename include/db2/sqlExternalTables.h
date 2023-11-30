/******************************************************************************
**
** Source File Name: sqlExternalTables.h
**
** (C) COPYRIGHT International Business Machines Corp. 2013
** All Rights Reserved
** Licensed Materials - Property of IBM
**
** US Government Users Restricted Rights - Use, duplication or
** disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
**
** Function = Include File defining:
**              Class and constants for external tables.
**
** Operating System: LINUX
**
*******************************************************************************/
#ifndef SQLEXTERNALTABLES_H_SQL
#define SQLEXTERNALTABLES_H_SQL

#include "sql.h"
#include "sqlter.h"
#include "db2pd.h"
#include "pdlog.h"
#include "osstype.h"
#include "sqlomem.h"
#include "sqloswap.h"
#include "sqlnlsnls2.h"

#include <string.h>
//this data is in sqls_file/File_Typedefs.h
#define MAX_DECIMAL_SIZE    34
// We need 21 for 00:00:00.000000000000. making it 25.
#define MAX_TIME_SIZE       25
// We need 33 for 25-JAN-2016 00:00:00.000000000000 making it 40.
#define MAX_TIMESTAMP_SIZE  40
#define MAX_VARCHAR_LENGTH  32672
//this data is not defined so currently defining here
#define SQL_FLOAT_STRLEN 317
#define SQL_BOOLEAN_STRLEN 5
//=====================================================================
// Fixed format nullif ocnstant
#define SQL_ET_ZONE_NULLIF_UNKNOWN     0
#define SQL_ET_ZONE_NULLIF_NULL        1
#define SQL_ET_ZONE_NULLIF_STRING      2
#define SQL_ET_ZONE_NULLIF_INT         3
#define SQL_ET_ZONE_NULLIF_SUBSTRING   4
//===========================================================================
// Constants

// External Column reference version 1.
#define SQL_EXT_COL_REFS_V1                1

// Column referenced in projection list.
#define SQL_EXT_COL_REF_BY_PROJ_LIST       0x0001

// Column referenced in predicate.
#define SQL_EXT_COL_REF_BY_PRED            0x0002

// Binary file versions
#define SQL_EXT_BINARY_HEADER              0xDB2B0000
#define SQL_EXT_BINARY_VERSION_1           (SQL_EXT_BINARY_HEADER | 0x00000001)

// Binary file header flags
#define SQL_EXT_BINARY_ET_LITTLE_ENDIAN    0x00000001
#define SQL_EXT_BINARY_ET_CLI_LOAD         0x00000002
#define SQL_EXT_BINARY_ET_LZ4_DATA         0x00000004
#define SQL_EXT_BINARY_ET_ZLIB_DATA        0x00000008
#define SQL_EXT_BINARY_ET_RUNTIME_RESERVE  0x00000080 // byte-swapped HEADER_CHECKED
#define SQL_EXT_BINARY_ET_HEADER_CHECKED   0x80000000

//===========================================================================
// Binary file header

class sqlExternalBinaryHeader
{
   private:
      sqluint32 m_FileVersion;
      sqluint32 m_HeaderFlags;
      sqluint16 m_ProductIDLength;
      char      m_ProductID[70];
      sqluint32 m_ExpansionLength;

   public:
      // NOTE:
      // This class is used in a union, so it cannot have a constructor

      // File version
      sqluint32 fileVersion() const
      {
         return m_FileVersion;
      }

      // Header flags
      sqluint32 headerFlags() const
      {
         return m_HeaderFlags;
      }

      // If numeric data in the file needs byte reversal
      bool needsByteReversal() const
      {
#ifdef SQLZ_BIG_ENDIAN
         return (m_HeaderFlags & SQL_EXT_BINARY_ET_LITTLE_ENDIAN);
#else
         return !(m_HeaderFlags & SQL_EXT_BINARY_ET_LITTLE_ENDIAN);
#endif
      }

      // Correct byte order if necessary
      bool checkHeaderAfterRead(size_t bytes_read, sqlzRc rc)
      {
         bool rValue = (0 == rc &&
                        sizeof(sqlExternalBinaryHeader) == bytes_read);
#ifdef SQLZ_LITTLE_ENDIAN
         // Check the header appears valid if the header has already been byte-swapped
         if (rValue &&
             !(SQL_EXT_BINARY_ET_HEADER_CHECKED & m_HeaderFlags))
         {
            m_FileVersion      = sqloSwap(m_FileVersion);
            m_HeaderFlags      = sqloSwap(m_HeaderFlags);
            m_ProductIDLength  = sqloSwap(m_ProductIDLength);
            m_ExpansionLength  = sqloSwap(m_ExpansionLength);
            m_HeaderFlags     |= SQL_EXT_BINARY_ET_HEADER_CHECKED;
         }
#endif
         return rValue &&
                (SQL_EXT_BINARY_HEADER == (SQL_EXT_BINARY_HEADER & m_FileVersion));
      }

      // Expansion space in the header used before the start of the column data
      sqluint32 expansionLength() const
      {
         return m_ExpansionLength;
      }

      // Length of the product ID information (<=70)
      sqluint64 productIDLength() const
      {
         return m_ProductIDLength;
      }

      // Product ID information
      const char* productID() const
      {
         return m_ProductID;
      }

      // Initialize the object before writing it to an ET
      inline sqlzRc initForWriting(Uint32 flags=0);
};

//===========================================================================
// Column reference

class sqlExternalColRef
{
private:

   // Index of column in the table, starting at 0.
   sqlint32              mFieldId;

   // Length of the column data type.
   sqlint32              mTypeLength;

   // What context was this column referenced in.
   sqluint16             mReferencedBy;

   // SQL Type of the column.
   sqlint16              mSQLType;

   // Data Codepage
   sqluint16             mCodepage;

   // Data CodeUnits
   sqluint8              mCodeUnits;

   // Padding required for 8 byte alignment
   sqluint8              mConvFactor;
//   char                  mPad1[1];

public:
   // Performs byte swap when reading from a binary file
   // from a different-endian platform
   inline void byteSwap()
   {
      mFieldId      = sqloSwap(mFieldId);
      mTypeLength   = sqloSwap(mTypeLength);
      mReferencedBy = sqloSwap(mReferencedBy);
      mSQLType      = sqloSwap(mSQLType);
      mCodepage     = sqloSwap(mCodepage);
   }

   // Get the index of this column in the table that owns it.
   // are appropriate to the version that generated the structure.
   inline sqlint32 getColumnIndex(void) const
   {
      return mFieldId;
   }

   // Get the SQL type of the column
   inline sqlint16 getColumnSQLType(void) const
   {
      return mSQLType;
   }

   // Set the SQL type of the column
   inline bool setColumnSQLType(sqlint16 type)
   {
       if (type < SQL_TYP_DATE || type > SQL_TYP_NDBCLOB_LOCATOR)
           return false;

       mSQLType = type;
       return true;
   }

   // Get the codepage of the column
   inline sqluint16 getColumnCodepage(void) const
   {
      return mCodepage;
   }

   // Set the codepage of the column
   inline void setColumnCodepage(sqluint16 colCP)
   {
      mCodepage = colCP;
      return;
   }

   // Get the codeunit of the column
   inline sqluint8 getColumnCodeUnit(void) const
   {
      return mCodeUnits;
   }

   // Set the codeunit of the column
   inline void setColumnCodeUnit(sqluint8 colCU)
   {
      mCodeUnits = colCU;
      return;
   }

   // Get the conversion factor of the column
   inline sqluint8 getColumnConversionFactor(void) const
   {
      return mConvFactor;
   }

   // Set the conversion factor of the column
   inline void setColumnConversionFactor(sqluint8 colCF)
   {
      mConvFactor = colCF;
      return;
   }

   // Get the name length of the column
   inline sqluint8 getColumnNameLength(void) const
   {
      return 0;
   }

   // Get the name of the column
   inline char* getColumnName() const
   {
      return NULL;
   }

   // Set the precision of the column data type.Should only be used on
   // TIMESTAMP and DECIMAL typed columns.

   inline bool setPrecision(sqlint16 prec)
   {
      sqlint16 sqlTypeNoNull = (mSQLType & ~SQL_TYP_NULINC);

      // The length of TIMESTAMP/DECIMAL types encodes the precision and
      // scale.  The two high order bytes encode the precision and the
      // two low order bytes encode the scale.
      if (  (SQL_TYP_STAMP != sqlTypeNoNull)
          &&(SQL_TYP_DECIMAL != sqlTypeNoNull))
      {
          return false;
      }
      mTypeLength = (mTypeLength & 0x0000FFFF) | ((sqlint32)prec << 16);
      return true;
   }

   // Get the precision of the column data type.  Should only be used on
   // TIMESTAMP and DECIMAL typed columns.
   inline sqlint16 getPrecision(void) const
   {
      sqlint16 precision     = 0;
      sqlint16 sqlTypeNoNull = (mSQLType & ~SQL_TYP_NULINC);

      // The length of TIMESTAMP/DECIMAL types encodes the precision and
      // scale.  The two high order bytes encode the precision and the
      // two low order bytes encode the scale.
      if (  (SQL_TYP_STAMP == sqlTypeNoNull)
          ||(SQL_TYP_DECIMAL == sqlTypeNoNull))
      {
         precision = mTypeLength >> 16;
      }

      return precision;
   }

   // Set the scale of the column data type.  Should only be used on
   // TIMESTAMP and DECIMAL typed columns.
   inline bool setScale(sqlint16 scale)
   {
      sqlint16 sqlTypeNoNull = (mSQLType & ~SQL_TYP_NULINC);

      // The length of TIMESTAMP/DECIMAL types encodes the precision and
      // scale.  The two high order bytes encode the precision and the
      // two low order bytes encode the scale.
      if (  (SQL_TYP_STAMP != sqlTypeNoNull)
          &&(SQL_TYP_DECIMAL != sqlTypeNoNull))
      {
         return false;
      }

      mTypeLength =  ((mTypeLength & 0xFFFF0000) | scale);
      return true;
   }

   // Get the scale of the column data type.  Should only be used on
   // TIMESTAMP and DECIMAL typed columns.
   inline sqlint16 getScale(void) const
   {
      sqlint16 scale         = 0;
      sqlint16 sqlTypeNoNull = (mSQLType & ~SQL_TYP_NULINC);

      // The length of TIMESTAMP/DECIMAL types encodes the precision and
      // scale.  The two high order bytes encode the precision and the
      // two low order bytes encode the scale.
      if (  (SQL_TYP_STAMP == sqlTypeNoNull)
          ||(SQL_TYP_DECIMAL == sqlTypeNoNull))
      {
         scale = (mTypeLength & 0x0000FFFF);
      }

      return scale;
   }

   // Get the byte length required for the column.  For fixed length types
   // this is the # of bytes required to represent the column data.  For
   // variable length types this is the maximum byte length.
   inline sqlint32 getByteLength(void) const
   {
      sqlint32 byteLength    = 0;
      sqlint16 sqlTypeNoNull = (mSQLType & ~SQL_TYP_NULINC);

      // The precision must be used to calculate the byte length of
      // TIMESTAMP and DECIMAL types.
      //
      // For all other types the byte length is simple mTypeLength.
      if (  (SQL_TYP_STAMP == sqlTypeNoNull)
          ||(SQL_TYP_DECIMAL == sqlTypeNoNull))
      {
         sqluint16 precision = getPrecision();

         // The byte length of a TIMESTAMP can be calculated as:
         // length = (precision + 1) / 2
         // The byte length of a DECIMAL can be calculated as:
         // length = (precision + 2) / 2
         if (SQL_TYP_STAMP == sqlTypeNoNull)
         {
            byteLength = (precision + 1) >> 1;
         }
         else
         {
            byteLength = (precision + 2) >> 1;
         }
      }
      else
      {
         byteLength = mTypeLength;
      }

      return byteLength;
   }

   // Get the type length of the column.
   inline sqlint32 getTypeLength(void) const
   {
      return mTypeLength;
   }

   // Get the packed byte length of the type
   inline sqluint32 getMaxPackedByteLength(void) const
   {
      sqluint32 size_length=0;
      switch (mSQLType & ~SQL_TYP_NULINC)
      {
         case SQL_TYP_BLOB:
         case SQL_TYP_CLOB:
         case SQL_TYP_DBCLOB:
         case SQL_TYP_VARBINARY:
         case SQL_TYP_VARGRAPH:
         case SQL_TYP_VARCHAR:
            size_length += sizeof(sqlint16);
            break;
      }
      return size_length + getByteLength();
   }

   // Is this column referenced in the queries projection list.
   inline bool columnRefByProjList(void) const
   {
      return (mReferencedBy & SQL_EXT_COL_REF_BY_PROJ_LIST);
   }

   // Is this column referenced in the queries predicate.
   inline bool columnRefByPredicate(void) const
   {
      return (mReferencedBy & SQL_EXT_COL_REF_BY_PRED);
   }

   // Is this reference describing an identical column
   inline bool isIdentical(const sqlExternalColRef& target) const
   {
      return (mTypeLength == target.mTypeLength &&
              mSQLType    == target.mSQLType    &&
              mCodepage   == target.mCodepage   &&
              mCodeUnits  == target.mCodeUnits);
   }
};

//===========================================================================
// List of column references

class sqlExternalColRefsHeader
{
public:
   // External Column reference version.
   sqluint16             mExtColRefsVersion;

   // Number of column references.
   sqluint16             mNumColRefs;

   // Size of column reference structure in bytes.
   sqluint32             mTotalPackedSize;

   // Performs byte swap when reading from a binary file
   // from a different-endian platform
   inline void byteSwap()
   {
      mExtColRefsVersion = sqloSwap(mExtColRefsVersion);
      mNumColRefs        = sqloSwap(mNumColRefs);
      mTotalPackedSize   = sqloSwap(mTotalPackedSize);
   }

   // Total byte size of packed column reference list.
   sqlint32 getTotalByteSize(void) const
   {
      return mTotalPackedSize;
   }

   // Get the number of column references.
   inline sqluint16 getNumberOfColumnRefs(void) const
   {
      return mNumColRefs;
   }
};


class sqlExternalColRefs : public sqlExternalColRefsHeader
{
   friend class sqlCombinedBinaryHeader;
   friend class sqlExternalColRefIter;
private:

   // Size of column reference structure in bytes.
#ifdef OSS_USING_IBM_XLC_COMPILER
   // Strange compiler error on xlC. zero length array seems to be ok.
   sqlExternalColRef     mColRefs[0];
#else
   sqlExternalColRef     mColRefs[];
#endif

   // Max Text size of dataype
   static sqlint32 getTextLenBySQLType(const sqlint16 SQLType)
   {
       static const ECFID FUNCTION_TRACE_ID = SQLT_getTextLenBySQLType;
       switch(SQLType)
       {
          case SQL_TYP_NBIGINT:
          case SQL_TYP_BIGINT:
             {
                return OSS_SINT64_DEC_CHAR_LEN;
             }
          case SQL_TYP_NINTEGER:
          case SQL_TYP_INTEGER:
             {
                return OSS_SINT16_DEC_CHAR_LEN;
             }
          case SQL_TYP_NSMALL:
          case SQL_TYP_SMALL:
             {
                return OSS_SINT8_DEC_CHAR_LEN;
             }
          case SQL_TYP_NFLOAT:
          case SQL_TYP_FLOAT:
             {
                return SQL_FLOAT_STRLEN;
             }
          case SQL_TYP_NCHAR:
          case SQL_TYP_CHAR:
             {
                return SQL_MAXCHAR;
             }
          case SQL_TYP_NDATE:
          case SQL_TYP_DATE:
             {
                return SQL_DATE_STRLEN;
             }
          case SQL_TYP_STAMP:
          case SQL_TYP_NSTAMP:
             {
                // The Netezza external table file could have nanoseconds
                // We need a higher buffer size.
                return MAX_TIMESTAMP_SIZE;
             }
          case SQL_TYP_NDECIMAL:
          case SQL_TYP_DECIMAL:
          case SQL_TYP_NUMERIC:
          case SQL_TYP_NNUMERIC:
             {
              return MAX_DECIMAL_SIZE;
             }
          case SQL_TYP_NTIME:
          case SQL_TYP_TIME:
             {
                // The Netezza external table file could have nanoseconds
                // but we read that and throw away. Hence we have to account
                // for a higher buffer size.
                return MAX_TIME_SIZE;
             }
          case SQL_TYP_NBOOLEAN:
          case SQL_TYP_BOOLEAN:
             {
                return SQL_BOOLEAN_STRLEN;
             }
          case SQL_TYP_NBINARY:
          case SQL_TYP_BINARY:
             {
               return SQL_MAXBINARY;
             }
          case SQL_TYP_DECFLOAT:
          case SQL_TYP_NDECFLOAT:
             {
               return SQLZ_MAX_DECFLOAT_CANONICAL_STRLEN;
             }
          case SQL_TYP_GRAPHIC:
          case SQL_TYP_NGRAPHIC:
             {
               return SQL_MAXGRAPH;
             }
          default:
             {
                PD_SASSERT_HARD(false, FUNCTION_TRACE_ID,1, PD_UINT(SQLType));
             }
        }
        return 0;
   }

public:
   // MAX char size of packed column reference list.
   sqlint32 getTotalTextSize(void) const
   {
      sqlint32 tableCharLen = 0;
      for (sqluint16 colRefIdx = mNumColRefs; colRefIdx-- > 0;)
      {
        const sqlExternalColRef& colRef = mColRefs[colRefIdx];
        const sqlint16 sqlType = colRef.getColumnSQLType();
        tableCharLen +=
          (SQL_TYP_NVARCHAR == sqlType || SQL_TYP_VARCHAR == sqlType ||
           SQL_TYP_NVARBINARY == sqlType || SQL_TYP_VARBINARY == sqlType ||
           SQL_TYP_VARGRAPH == sqlType || SQL_TYP_NVARGRAPH == sqlType ||
           SQL_TYP_NCLOB == sqlType || SQL_TYP_CLOB == sqlType ||
           SQL_TYP_NBLOB == sqlType || SQL_TYP_BLOB == sqlType ||
           SQL_TYP_DBCLOB == sqlType || SQL_TYP_NDBCLOB == sqlType ) ?
            colRef.getByteLength() :  getTextLenBySQLType(sqlType);
      }
      return tableCharLen;
   }

   // Get the column reference at the index.  Returns NULL if the index is out of range.
   inline const sqlExternalColRef* getColRef(const sqlint32 index) const
   {
      return (index < mNumColRefs ? &mColRefs[index] : NULL);
   }

   // Get the column reference for a specific column index.  Returns NULL if the
   // column index is not in the reference list.
   inline const sqlExternalColRef* getColRefByColumnIndex(const sqlint32 colIndex) const
   {
      const sqlExternalColRef* colRef = NULL;

      if (mNumColRefs > 0)
      {
         sqluint16                localNumColRefs = mNumColRefs;
         const sqlExternalColRef* localColRef     = &mColRefs[0];

         do
         {
            if (localColRef->getColumnIndex() == colIndex)
            {
               colRef = localColRef;
               break;
            }

            ++localColRef;
         }
         while (--localNumColRefs);
      }

      return colRef;
   }

   // Get the maximum packed size of for these columns in a data stream
   inline sqluint64 getMaxPackedByteLength(void) const
   {
      sqluint64 maxTotalColumnSize=0;
      sqluint16 numNullableColumns=0;
      for (int index=0; index<mNumColRefs; ++index)
      {
         const sqlExternalColRef& localColRef = mColRefs[index];
         if (localColRef.getColumnSQLType() & SQL_TYP_NULINC)
         {
            ++numNullableColumns;
         }
         maxTotalColumnSize += localColRef.getMaxPackedByteLength();
      }
      return maxTotalColumnSize + ((numNullableColumns + 7) / 8);
   }

   // Check if the two column refs describe identical tables
   inline bool matches(sqlExternalColRefs* target) const
   {
      bool rValue = (0 == sqlomcmp(this, target,
                                   sizeof(sqlExternalColRefsHeader)));
      // Col refs may have different information, so a straight
      // memcmp can't be done.
      // Only compare the relevant information.
      for (int index=0; rValue && index < mNumColRefs; ++index)
      {
         rValue = mColRefs[index].isIdentical(target->mColRefs[index]);
      }
      return rValue;
   }
};

//===========================================================================
// Combined Header

class sqlCombinedBinaryHeader
{
   private:
      sqlExternalBinaryHeader m_binHeader;
      sqlExternalColRefs*     m_extColRefs;
      sqluint64               m_maxBytesPerRow;
      sqluint16               m_nullBytesPerRow;
      bool                    m_needsProcessing;

   public:
      inline sqlExternalBinaryHeader* getBinaryHeader()
      {
         return &m_binHeader;
      }
      inline sqlExternalColRefs* getExternalColRefs()
      {
         return m_extColRefs;
      }
      inline bool requiresColumnProcessing() const
      {
         return m_needsProcessing;
      }
      inline sqluint64 getMaxBytesPerRow() const
      {
         return m_maxBytesPerRow;
      }
      sqluint16 nullableBytesPerRow() const { return m_nullBytesPerRow; }
      bool setAndCheckExternalColRefs(sqlExternalColRefs* source,
                                      const bool swapBytes,
                                      const sqlExternalColRefs* const target)
      {
         sqluint64 totalByteSize(0);
         sqluint16 numNulls(0);
         sqluint16 numColRefs(source->getNumberOfColumnRefs());
         bool      needsProcessing(m_binHeader.needsByteReversal());

         const sqlExternalColRefsHeader* const sourceHeader = source;
         const sqlExternalColRefsHeader* const targetHeader = target;

         // Compare the headers
         bool      rValue(0 == sqlomcmp(sourceHeader, targetHeader, sizeof(sqlExternalColRefsHeader)));

         // Initialize the values
         m_nullBytesPerRow = 0;
         m_needsProcessing = 0;
         m_maxBytesPerRow = 0;
         m_extColRefs = NULL;

         // While there are columns and they match
         for(sqluint16 index = 0; rValue && index < numColRefs; ++index)
         {
            sqlExternalColRef*             srce = &source->mColRefs[index];
            const sqlExternalColRef* const dest = &target->mColRefs[index];

            // Swap the source bytes if necessary
            if (swapBytes) srce->byteSwap();

            sqlint16                       srce_type = srce->getColumnSQLType();
            sqlint16                       dest_type = dest->getColumnSQLType();

            // If the source is NULLABLE but the target is not
            if ((srce_type & SQL_TYP_NULINC) && !(dest_type & SQL_TYP_NULINC))
            {
               // will allow but will reject entire file if NULL encountered
               needsProcessing = true;
            }

            // If the source is NOT NULL but the target is NULL, processing
            // will be required to ensure the NULL indicators are set up
            // correctly
            if ((srce_type & SQL_TYP_NULINC) != (dest_type & SQL_TYP_NULINC))
            {
               needsProcessing = true;
            }

            // Count the nullable columns
            if (srce->getColumnSQLType() & SQL_TYP_NULINC) ++numNulls;

            totalByteSize += srce->getMaxPackedByteLength();

            // Remove the NULINC flag for simplicity
            srce_type &= ~SQL_TYP_NULINC;
            dest_type &= ~SQL_TYP_NULINC;

            // Only support reading to the same SQL_TYP
            if (srce_type != dest_type)
            {
               rValue = false;
               break;
            }

            // Support limited conversions within the same type
            switch (srce_type)
            {
               // Fixed size and format types are fine
               case SQL_TYP_SMALL:
               case SQL_TYP_INTEGER:
               case SQL_TYP_BIGINT:
               case SQL_TYP_DATE:
               case SQL_TYP_TIME:
               case SQL_TYP_BOOLEAN:
               {
                  break;
               }

               // The lengths of these types must match.
               // Note that for stamp and decimal, the
               // length checks the precision/scale values.
               case SQL_TYP_FLOAT:
               case SQL_TYP_DECFLOAT:
               case SQL_TYP_STAMP:
               case SQL_TYP_DECIMAL:
               {
                  rValue = (srce->getTypeLength() == dest->getTypeLength());
                  break;
               }

               // Destination length might not be equal to target length
               case SQL_TYP_BINARY:
               {
                  // Source must not be longer than destination for fixed length types
                  rValue = (dest->getTypeLength() >= srce->getTypeLength());
                  needsProcessing = needsProcessing || (srce->getTypeLength() < dest->getTypeLength());
                  break;
               }
               case SQL_TYP_VARBINARY:
               case SQL_TYP_BLOB:
               {
                  // If the source string is shorter or equal to the destination, then no work is necessary.
                  // If the source string is potentially longer, then allow it to go through but fail at runtime.
                  needsProcessing = needsProcessing || (srce->getTypeLength() > dest->getTypeLength());
                  break;
               }

               // Check for codepage conversion
               case SQL_TYP_CHAR:
               case SQL_TYP_GRAPHIC:
               {
                  // Source must not be longer than destination for fixed length types
                  rValue = (dest->getTypeLength() >= srce->getTypeLength());
               }
               OSS_HINT_FALLTHROUGH;
               case SQL_TYP_VARCHAR:
               case SQL_TYP_VARGRAPH:
               case SQL_TYP_CLOB:
               case SQL_TYP_DBCLOB:
               {
                  if (SQL_TYP_CHAR == srce_type || SQL_TYP_GRAPHIC == srce_type)
                  {
                     // If the destination is longer, will need processing for padding
                     needsProcessing = needsProcessing || (srce->getTypeLength() < dest->getTypeLength());
                  }
                  else
                  {
                     // If the source string is shorter or equal to the destination, then no processing is necessary.
                     // If the source string is potentially longer, then allow it to go through but fail at runtime.
                     needsProcessing =  needsProcessing || (srce->getTypeLength() > dest->getTypeLength());
                  }

                  sqluint16 srce_cp = srce->getColumnCodepage();
                  sqluint16 dest_cp = dest->getColumnCodepage();
                  sqluint32 srce_ln = srce->getTypeLength();
                  sqluint32 dest_ln = dest->getTypeLength();

                  // If codepage conversion is needed, ensure the converted string
                  // can fit into the destination string
                  if (srce_cp != dest_cp &&
                      srce_cp != 0 &&
                      dest_cp != 0 &&
                      sqlnlsIsCpcvNeeded(srce_cp, dest_cp))
                  {
                      needsProcessing = true;

                      // Check the conversion factor, and if the converted source
                      // string can fit into the target string's length
                      sqluint8 conv_factor = sqlnls_ConvFactor(srce_cp, dest_cp);

                      // This may be discarded, but for now remember the conversion factor
                      srce->setColumnConversionFactor(conv_factor);
                  }
                  // Do not allow FOR BIT DATA to go into a non-FOR BIT DATA field
                  // The reverse is fine, however
                  else if(srce_cp == 0 && dest_cp != 0)
                  {
                     rValue = false;
                  }
                  break;
               }

               // These types are NOT supported yet; should be blocked
               case SQL_TYP_XML:
               default:
               {
                  sqlzAssert(false);
                  rValue = false;
                  break;
               }
            }
         }

         // Set the values if there is a match
         if (rValue)
         {
            m_needsProcessing = needsProcessing;
            m_nullBytesPerRow = (numNulls + 7) / 8;
            m_maxBytesPerRow = totalByteSize + m_nullBytesPerRow;
            m_extColRefs = source;
         }
         return rValue;
      }
      sqlzRc deepCopy(const sqlCombinedBinaryHeader* const copyHeader)
      {
         sqlzRc rc(SQL_RC_OK);

         // NULL the references to start
         m_extColRefs = NULL;

         // Straight copy of the file header
         sqlomcpy(&m_binHeader, &copyHeader->m_binHeader, sizeof(sqlExternalBinaryHeader));
         m_nullBytesPerRow = copyHeader->m_nullBytesPerRow;
         m_needsProcessing = copyHeader->m_needsProcessing;
         m_maxBytesPerRow  = copyHeader->m_maxBytesPerRow;

         // Allocate the memory for the column references (includes initial header)
         rc = sqloOSSMemAlloc((void**)&m_extColRefs,
                              NULL,
                              (size_t)copyHeader->m_extColRefs->getTotalByteSize(),
                              OSS_MEM_ZERO_FILL);
         if (SQL_RC_OK != rc)
         {
            goto exit;
         }

         // Copy the file references
         sqlomcpy(m_extColRefs, copyHeader->m_extColRefs,
                  copyHeader->m_extColRefs->getTotalByteSize());
      exit:
         return rc;
      }
      void clear()
      {
         if (m_extColRefs)
         {
            sqloOSSMemFree((void**)&m_extColRefs);
            m_extColRefs = NULL;
         }
      }
};

//===========================================================================
// Constants

// External Column name version 1.
#define SQL_EXT_COL_NAMES_V1                1

//===========================================================================
// Column name

class sqlExternalColName
{
private:

   // Index of column in the table, starting at 0.
   sqlint32              mFieldId;

   // Length of the column name.
   sqluint8              mColNameLength;

   // Column name.
   char                  mColName[SQL_MAX_IDENT + 1];

   // Padding required for 8 byte alignment
   char                  mPad1[2];

   // Reserved space
   void*                 reserved1[2];

   // Expansion pointer
   void*                 expansion;

public:

   // Get the index of this column in the table that owns it.
   // are appropriate to the version that generated the structure.
   inline sqlint32 getColumnIndex(void) const
   {
      return mFieldId;
   }

   // Get the name length of the column
   inline sqluint8 getColumnNameLength(void) const
   {
      return mColNameLength;
   }

   // Get the name of the column
   inline char* getColumnName() const
   {
      return ((char *)&mColName);
   }
};

//===========================================================================
// List of column names

class sqlExternalColNames
{
private:

   // External Column name version.
   sqluint16             mExtColNamesVersion;

   // Number of column names.
   sqluint16             mNumColNames;

   // Size of column name structure in bytes.
   sqluint32             mTotalPackedSize;

   // Size of column name structure in bytes.
   sqlExternalColName     mColNames[];

public:
   // Total byte size of packed column name list.
   sqlint32 getTotalByteSize(void) const
   {
      return mTotalPackedSize;
   }

   // Get the number of column names.
   inline sqluint16 getNumberOfColumnNames(void) const
   {
      return mNumColNames;
   }

   // Get the column name at the index.  Returns NULL if the index is out of range.
   inline const sqlExternalColName* getColName(const sqlint32 index) const
   {
      const sqlExternalColName* colName = NULL;

      if (index < mNumColNames)
      {
         colName = &mColNames[index];
      }

      return colName;
   }

   // Get the column name for a specific column index.  Returns NULL if the
   // column index is not in the name list.
   inline const sqlExternalColName* getColNameByColumnIndex(const sqlint32 colIndex) const
   {
      const sqlExternalColName* colName = NULL;

      if (mNumColNames > 0)
      {
         sqluint16                localNumColNames = mNumColNames;
         const sqlExternalColName* localColName     = &mColNames[0];

         do
         {
            if (localColName->getColumnIndex() == colIndex)
            {
               colName = localColName;
               break;
            }

            ++localColName;
         }
         while (--localNumColNames);
      }

      return colName;
   }
};

//===========================================================================
// Constants

// External Column layout version 1.
#define SQL_EXT_COL_LAYOUTS_V1                1

//===========================================================================
// Column name

class sqlExternalColLayout
{
private:

   // Length of the zone in byte.
   sqlint32              mZoneSize;

   // Index of zone in the layout, starting at 0.
   sqlint32              mZoneId;

   // Index of owner in the layout.
   sqlint32              mZoneOwnerId;

   // Length of the zone data type.
   sqlint32              mZoneLength;

   // Alignment of the zone.
   sqlint32              mZoneAlignment;

   // Modulus of the zone.
   sqlint32              mZoneModulus;

   // SQL Type of the zone.
   sqlint16              mZoneType;

   // Use Type of the zone.
   sqlint16              mZoneUseType;

   // Length Type of the zone.
   sqlint16              mZoneLenType;

   // Endian Type of the zone.
   sqlint16              mZoneEndianType;

   // Style of the zone.
   sqlint16              mZoneStyle;

   // Date Style of the zone.
   sqlint16              mDateStyle;

   // Time Style of the zone.
   sqlint16              mTimeStyle;

   // Boolean Style of the zone.
   sqlint16              mBoolStyle;

   // NullIf Type of the zone.
   sqlint16              mZoneNullIfType;

   // Length of NullIf string for the zone.
   sqlint16              mZoneNullIfStrLen;

   // NullIf string for the zone.
   char                  mZoneNullIfStr[40];

   // Decimal delimiter for the zone.
   Uint8                 mDecimalDelim;

   // Date delimiter for the zone.
   Uint8                 mDateDelim;

   // Time delimiter for the zone.
   Uint8                 mTimeDelim;

   // Whether zone type is UINT
   Uint8                 mUintType;

   // Referenced zone for size
   sqlint32              mRefLengthZone;

   // Referenced zone for nullIf
   sqlint32              mRefNullIfZone;

   // Zone Ref type
   sqlint16              mZoneRefType;

   // CCSID that specifies the encoding for string data types.
   sqluint16             mZoneCCSID;

   // Operand to add/sub from zone length
   sqlint32              mOperand;

   // Operator to add/sub from zone length
   Uint8                 mOp1;

   // Padding required for 8 byte alignment
   char                  mPad2[7];

   // Reserved space
   void*                 reserved1[2];

   // Expansion pointer
   void*                 expansion;

public:

   // Get the index of this zone in the table that owns it.
   inline sqlint32 getZoneId(void) const
   {
      return mZoneId;
   }

   // Get the index of the zone that owns it.
   inline sqlint32 getZoneOwnerId(void) const
   {
      return mZoneOwnerId;
   }

   // Get the number of bytes for zone
   inline sqlint32 getZoneNumBytes(void) const
   {
      return mZoneSize;
   }

   // Get the zone length for type
   inline sqluint32 getZoneLength(void) const
   {
      return mZoneLength;
   }

   // Get the zone alignment
   inline sqluint32 getZoneAlignment(void) const
   {
      return mZoneAlignment;
   }

   // Get the zone modulus
   inline sqluint32 getZoneModulus(void) const
   {
      return mZoneModulus;
   }

   // Get the zone type
   inline sqlint16 getZoneType(void) const
   {
      return mZoneType;
   }

   // Get the zone use type
   inline sqlint16 getZoneUseType(void) const
   {
      return mZoneUseType;
   }

   // Get the zone length type
   inline sqlint16 getZoneLenType(void) const
   {
      return mZoneLenType;
   }

   // Get the zone endian type
   inline sqlint16 getZoneEndianType(void) const
   {
      return mZoneEndianType;
   }

   // Get the zone style
   inline sqlint16 getZoneStyle(void) const
   {
      return mZoneStyle;
   }

   // Get the zone date style
   inline sqlint16 getZoneDateStyle(void) const
   {
      return mDateStyle;
   }

   // Get the zone time style
   inline sqlint16 getZoneTimeStyle(void) const
   {
      return mTimeStyle;
   }

   // Get the zone bool style
   inline sqlint16 getZoneBoolStyle(void) const
   {
      return mBoolStyle;
   }

   // Get the zone decimal delim
   inline Uint8 getZoneDecimalDelim(void) const
   {
      return mDecimalDelim;
   }

   // Get the zone date delim
   inline Uint8 getZoneDateDelim(void) const
   {
      return mDateDelim;
   }

   // Get the zone time delim
   inline Uint8 getZoneTimeDelim(void) const
   {
      return mTimeDelim;
   }

   // Get the zone nullIf Type
   inline sqlint16 getZoneNullIfType(void) const
   {
      return mZoneNullIfType;
   }

   // Get the zone nullIf string length
   inline sqlint16 getZoneNullIfStrLen(void) const
   {
      return mZoneNullIfStrLen;
   }

   // Get the zone nullIf string
   inline char* getZoneNullIfStr(void) const
   {
      return ((char *)mZoneNullIfStr);
   }

   // Nullable zone or not
   inline bool isNullable(void) const
   {
      return (mZoneNullIfType > SQL_ET_ZONE_NULLIF_NULL);
   }

   // Get the zone time delim
   inline bool isUintType(void) const
   {
      return (mUintType == 1);
   }

   // Get the referenced zone for size
   inline sqlint32 getRefLengthZone(void) const
   {
      return mRefLengthZone;
   }

   // Get the referenced zone for nullIf
   inline sqlint32 getRefNullIfZone(void) const
   {
      return mRefNullIfZone;
   }

   // Get the reference Type
   inline sqlint16 getZoneRefType(void) const
   {
      return mZoneRefType;
   }

   // Get the CCSID that specifies the encoding for string data types.
   inline sqlint16 getZoneCCSID() const
   {
      return mZoneCCSID;
   }

   // Get the operand
   inline sqlint32 getOperand(void) const
   {
      return mOperand;
   }

   // Get the operator
   inline Uint8 getOp1(void) const
   {
      return mOp1;
   }
};


//===========================================================================
// List of column layouts

class sqlExternalColLayouts
{
private:

   // External Column layout version.
   sqluint16             mExtColLayoutsVersion;

   // Number of column Layouts.
   sqluint16             mNumColLayouts;

   // Size of column Layout structure in bytes.
   sqluint32             mTotalPackedSize;

   // Size of column Layout structure in bytes.
   sqlExternalColLayout     mColLayouts[];

public:
   // Total byte size of packed column Layout list.
   sqlint32 getTotalByteSize(void) const
   {
      return mTotalPackedSize;
   }

   // Get the number of column Layouts.
   inline sqluint16 getNumberOfColumnLayouts(void) const
   {
      return mNumColLayouts;
   }

   // Get the column Layout at the index.  Returns NULL if the index is out of range.
   inline const sqlExternalColLayout* getColLayout(const sqlint32 index) const
   {
      const sqlExternalColLayout* colLayout = NULL;

      if (index < mNumColLayouts)
      {
         colLayout = &mColLayouts[index];
      }

      return colLayout;
   }

   // Get the column Layout for a specific column index.  Returns NULL if the
   // column index is not in the Layout list.
   inline const sqlExternalColLayout* getColLayoutByColumnIndex(const sqlint32 colIndex) const
   {
      const sqlExternalColLayout* colLayout = NULL;

      if (mNumColLayouts > 0)
      {
         sqluint16                localNumColLayouts = mNumColLayouts;
         const sqlExternalColLayout* localColLayout     = &mColLayouts[0];

         do
         {
            if (localColLayout->getZoneId() == colIndex)
            {
               colLayout = localColLayout;
               break;
            }

            ++localColLayout;
         }
         while (--localNumColLayouts);
      }

      return colLayout;
   }
};

/**
   \brief
      Iterator of list of column references.  A column may be referenced in a
      table accessor's projection list, or by a predicate that must be applied
      to each row.

      The iterator may be forwards (increasing column index) or backwards
      (decreasing column index) through the column references.

      The following example illustrates how to determine which columns are
      referenced in the table access projection list.

   \code
      sqlExternalColRefIter colRefIter;
      db2uint16             columnIndex;

      // Iterate over the columns in colRefList which were referenced in the
      // projection list
      colRefIter.iterOverProjectionList(colRefList);

      // Position the iterator at the first column referenced in the projection
      // list.
      colRefIter.firstColumnReference();

      // While the iterator is positioned over a column reference, fetch the
      // the column index of the current reference and move the iterator to
      // the next column referenced in the projection list.
      while (!colRefIter.isNull())
      {
         columnIndex = colRefIter.nextColumnIndex();

         // Handle the column index
      }
   \endcode

      The following example illustrates how to move backthrough the columns
      referenced in the table access predicate.

   \code
      sqlExternalColRefIter colRefIter;
      db2uint16             columnIndex;

      // Iterate over the columns in colRefList which were referenced in the
      // predicate.
      colRefIter.iterOverPredicateList(colRefList);

      // Position the iterator at the last column referenced in the predicate.
      colRefIter.lastColumnReference();

      // While the iterator is positioned over a column reference, peek at
      // the index of the current column reference, then move the iterator
      // to the previous column referenced in the predicate.
      while (!colRefIter.isNull())
      {
         columnIndex = colRefIter.peekColumnIndex();

         // Handle the column index.

         (void)colRefIter.prevColumnIndex();
      }
   \endcode
 */
class sqlExternalColRefIter
{
private:

   //! Column reference list
   const sqlExternalColRefs*       mColRefList;

   //! Current position in the column reference list
   sqlint32                        mColListPos;

   //! Modes available for iterating through the column reference list
   enum colRefListIterMode
   {
      //! Iterator mode is not set
      COL_REF_LIST_ITER_MODE_NONE,

      //! Iterate over all column references (both in the projection list and
      //! in the predicate).
      COL_REF_LIST_ITER_MODE_ALL,

      //! Iterate over columns referenced in the projection list
      COL_REF_LIST_ITER_MODE_PROJ,

      //! Iterate over columns referenced in the predicate
      COL_REF_LIST_ITER_MODE_PREDS
   };

   //! Current iterator mode
   colRefListIterMode             mIterMode;

   /**
      \brief
         Check if the iterator is currently positioned over a column reference.

      \retval true  - Iterator is positioned over a column reference
      \retval false - Iterator is not positioned over a column reference
    */
   bool positionedOverReference(void) const
   {
      return (mColListPos >= 0 && mColListPos < mColRefList->mNumColRefs);
   }

   /**
      \brief
         Check if the column reference that the iterator is currently
         positioned over is valid for the current iterator mode.

      \retval true  - Column reference at the current iterator position
                      matches iterator mode.
      \retval false - Column refernce at the current iterator position
                      doesn't match iterator mode, or iterator is not
                      positioned over a column reference.
    */
   bool columnReferenceMatchesIterMode(void) const
   {
      return (  (positionedOverReference())
              &&(  (COL_REF_LIST_ITER_MODE_ALL == mIterMode)
                 ||(  (COL_REF_LIST_ITER_MODE_PROJ == mIterMode)
                    &&(mColRefList->getColRef(mColListPos)->columnRefByProjList()))
                 ||(  (COL_REF_LIST_ITER_MODE_PREDS == mIterMode)
                    &&(mColRefList->getColRef(mColListPos)->columnRefByPredicate()))));
   }

   /**
      \brief
         Position the iterator over the next matching column reference.

         The iterator must be positioned over a column reference before this
         method is invoked.

         If no subsequent matching column reference is found the iterator is
         left in the NULL state (isNull() returns true).
    */
   void positionOverNextReference(void)
   {
      if (positionedOverReference())
      {
         do
         {
            ++mColListPos;
            if (columnReferenceMatchesIterMode())
            {
              break;
            }
         } while (positionedOverReference());

         if (!columnReferenceMatchesIterMode())
         {
            mColListPos = -1;
         }
      }
   }

   /**
      \brief
         Position the iterator over the previous matching column reference.

         The iterator must be positioned over a column reference before this
         method is invoked.

         If no previous matching column reference is found the iterator is
         left in the NULL state (isNull() returns true).
    */
   void positionOverPrevReference(void)
   {
      if (positionedOverReference())
      {
         do
         {
            --mColListPos;
            if (columnReferenceMatchesIterMode())
            {
              break;
            }
         } while (positionedOverReference());

         if (!columnReferenceMatchesIterMode())
         {
            mColListPos = -1;
         }
      }
   }

   /**
      \brief
         Reset the iterator.  The iterator mode is set to NONE,
         the column reference list is discarded and the position
         set to NULL.
    */
   void mReset(void)
   {
      mColRefList = NULL;
      mColListPos = -1;
      mIterMode = COL_REF_LIST_ITER_MODE_NONE;
   }

public:

   /**
      \brief
         Iterator constructor.  Resets the iterator state.
    */
   sqlExternalColRefIter(void)
   {
      mReset();
   }

   /**
      \brief
         Setup to iterator over all columns in a column reference list.

      \param colRefList [in] - Column reference list to iterate over.
    */
   void iterOverAllColumns(const sqlExternalColRefs* colRefList)
   {
      mReset();
      mColRefList = colRefList;
      mIterMode = COL_REF_LIST_ITER_MODE_ALL;
   }

   /**
      \brief
         Setup to iterator over the columns in a column reference list which
         were refernced in the projection list.

      \param colRefList [in] - Column reference list to iterate over.
    */
   void iterOverProjectionList(const sqlExternalColRefs* colRefList)
   {
      mReset();
      mColRefList = colRefList;
      mIterMode = COL_REF_LIST_ITER_MODE_PROJ;
   }

   /**
      \brief
         Setup to iterator over the columns in a column reference list which
         were refernced in the predicate.

      \param colRefList [in] - Column reference list to iterate over.
    */
   void iterOverPredicateList(const sqlExternalColRefs* colRefList)
   {
      mReset();
      mColRefList = colRefList;
      mIterMode = COL_REF_LIST_ITER_MODE_PREDS;
   }

   /**
      \brief
         Check if the iterator's current column reference is null.
    */
   bool isNull(void) const
   {
      return !positionedOverReference();
   }

   /**
      \brief
         Position the iterator over the first column reference in the list.
    */
   void firstColumnReference(void)
   {
      // Position the iterator at the first element.
      mColListPos = 0;

      // If the first element is not a match for the iterator mode, move
      // the position forwards to the next matching reference.
      if (!columnReferenceMatchesIterMode())
      {
         positionOverNextReference();
      }
   }

   /**
      \brief
         Position the iterator over the last column reference in the list.
    */
   void lastColumnReference(void)
   {
      // Position the iterator at the last element
      mColListPos = mColRefList->mNumColRefs;

      // If the last element is not a match for the iterator mode, move
      // the position backwards to the last matching reference
      if (!columnReferenceMatchesIterMode())
      {
         positionOverPrevReference();
      }
   }

   /**
      \brief
         Get the column field index of the column reference that the iterator
         is currently positioned over.  The iterator position does not change.

      \retval A valid column index (>= 0) if the iterator is currently
              positioned over a column reference.
      \retval -1 if the iterator is not positioned over a column reference.
    */
   sqlint32 peekColumnIndex(void) const
   {
      sqlint32 colIndex = -1;
      if (!isNull())
      {
         colIndex = mColRefList->getColRef(mColListPos)->getColumnIndex();
      }
      return colIndex;
   }

   /**
      \brief
         Get the SQL type of the column that the iterator is currently
         positioned at.  The iterator position does not change.

      \retval A valid SQL type if the iterator is currently positioned over
              a column reference.
      \retval -1 if the iterator is not positioned over a column reference.
    */
   sqlint16 peekSQLType(void) const
   {
      sqlint16 sqlType = -1;
      if (!isNull())
      {
         sqlType = mColRefList->getColRef(mColListPos)->getColumnSQLType();
      }
      return sqlType;
   }

   /**
      \brief
         Get the type length of the column that the iterator is currently
         positioned at.  The iterator position does not change.

      \retval A valid type length if the iterator is currently positioned over
              a column reference.
      \retval -1 if the iterator is not positioned over a column reference.
    */
   sqlint32 peekTypeLength(void) const
   {
      sqlint32 typeLength = -1;
      if (!isNull())
      {
         typeLength = mColRefList->getColRef(mColListPos)->getTypeLength();
      }
      return typeLength;
   }

   /**
      \brief
         Determine if the column was referenced in the projection list.

      \retval true  - The column was referenced in the projection list.
      \retval false - The column was not referenced in the projection list.
    */
   bool peekColumnReferencedByProjList(void) const
   {
      bool refByProjList = false;
      if (!isNull())
      {
         refByProjList = mColRefList->getColRef(mColListPos)->columnRefByProjList();
      }
      return refByProjList;
   }

   /**
      \brief
         Determine if the column was referenced by a predicate.

      \retval true  - The column was referenced by a predicate.
      \retval false - The column was not referenced by a predicate.
    */
   bool peekColumnReferencedByPredicate(void) const
   {
      bool refByPredicate = false;
      if (!isNull())
      {
         refByPredicate = mColRefList->getColRef(mColListPos)->columnRefByPredicate();
      }
      return refByPredicate;
   }

   /**
      \brief
         Get the column field index of the column reference that the iterator
         is currently positioned over.  The iterator position moves to the next
         column index.

      \retval A valid column index (>= 0) if the iterator is currently
              positioned over a column reference.
      \retval -1 if the iterator is not positioned over a column reference.
    */
   sqlint32 nextColumnIndex()
   {
      sqlint32 colIndex = peekColumnIndex();
      positionOverNextReference();
      return colIndex;
   }

   /**
      \brief
         Get the column field index of the column reference that the iterator
         is currently positioned over.  The iterator position moves to the
         previous column index.

      \retval A valid column index (>= 0) if the iterator is currently
              positioned over a column reference.
      \retval -1 if the iterator is not positioned over a column reference.
    */
   sqlint32 prevColumnIndex()
   {
      sqlint32 colIndex = peekColumnIndex();
      positionOverNextReference();
      return colIndex;
   }
};

#endif
