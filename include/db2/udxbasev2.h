// udxbasev2.h
// Base class for Ver 2 User Defined Functions and Aggregates
//
// (C) Copyright IBM Corp. 2009, 2011  All Rights Reserved.

#ifndef UDX_BASEV2_INCLUDED
#define UDX_BASEV2_INCLUDED

// These macros are what should be used to return from evaluate or finalResult
#define NZ_UDX_RETURN_NULL() do { setReturnNull(true); return (void*)NULL; } while (0)

#define NZ_UDX_RETURN_STRING(x) return stringReturn(x)
#define NZ_UDX_RETURN_BOOL(x) return boolReturn(x)
#define NZ_UDX_RETURN_DATE(x) return dateReturn(x)
#define NZ_UDX_RETURN_TIME(x) return timeReturn(x)
#define NZ_UDX_RETURN_TIMETZ(x) return timetzReturn(x)
#define NZ_UDX_RETURN_NUMERIC32(x) return numeric32Return(x)
#define NZ_UDX_RETURN_NUMERIC64(x) return numeric64Return(x)
#define NZ_UDX_RETURN_NUMERIC128(x) return numeric128Return(x)
#define NZ_UDX_RETURN_FLOAT(x) return floatReturn(x)
#define NZ_UDX_RETURN_DOUBLE(x)  return doubleReturn(x)
#define NZ_UDX_RETURN_INTERVAL(x) return intervalReturn(x)
#define NZ_UDX_RETURN_INT64(x) return int64Return(x)
#define NZ_UDX_RETURN_INT32(x) return int32Return(x)
#define NZ_UDX_RETURN_INT16(x) return int16Return(x)
#define NZ_UDX_RETURN_INT8(x) return int8Return(x)
#define NZ_UDX_RETURN_TIMESTAMP(x) return timestampReturn(x)

#define CHECK_FUNC_RANGE(n) if (n < 0 || n >= m_pInfo->numArgs) \
		  throwError(NZ_ERROR_ILLVAL, "Argument number out of range", n)
#define CHECK_INFO_AVAILABLE() if (!m_pInfo) \
                  throwError(NZ_ERROR_ILLVAL, "UDF/UDA not yet initialized", 0)

#define CHECK_SIZER_RANGE(n) if (n < 0 || n >= m_pInfo->numArgs) \
        throwError(NZ_ERROR_ILLVAL, "Sizer argument number out of range", n)

#define CHECK_SIZER_TYPMOD(n, typmod) if (typmod == 0xFFFFFFFFFFFFFFFFLL) \
        throwError(NZ_ERROR_ILLVAL, "Sizer argument invalid", n)

// The maximum number of output columns in a user-defined table function
// is equal to SQLUDF_MAXCOL_TABFUN in include/sqludf.h.

// The following definition is kept for backward compatibility purpose.
// It may be deprecated in a future release.
#ifndef MAX_TABLE_COLUMNS
#define MAX_TABLE_COLUMNS 255
#endif

// The following definition is kept for backward compatibility purpose.
// It may be deprecated in a future release.
#define NZ_TABLE_FUNC_MAX_RETURN_COLUMNS MAX_TABLE_COLUMNS 
#define NZ_MISSING_CALCULATE_SHAPE_ERROR_MSG "Must implement calculateShape for UDX returning generic table"

void sqlerRoutineLogMessage(bool isLogEnabled, const char *format, va_list vaList); 

namespace nz {
namespace udx_ver2 {

class UdxEnvironment;
class UdxParameter;
class UdxOutputShaper;

class CallInfo
{
public:
    int     	numArgs;
    int*	argTypes;
    FuncDatum*	args;
    bool*       argNulls;
    bool*       argConsts;
};

class FuncCallInfo : public CallInfo
{
 public:
  FuncDatum	pReturnInfo;
  bool*   	pReturnNull;
  int		returnType;
  
};

class RowFuncCallInfo : public CallInfo
{
public:

    int         numReturnColumns;
    int*        returnColumnTypes;
    FuncDatum*  returnColumns;
    bool*       returnColumnNulls;

};

class TableFuncCallInfo : public RowFuncCallInfo
{
public:
    

};

class AggCallInfo : public FuncCallInfo
{
 public:
  int           numStateVars;
  int*		stateVarTypes;
  FuncDatum*	stateVars;
  bool*		pStateVarNulls;
};

class RowAggCallInfo : public RowFuncCallInfo
{
 public:
  int           numStateVars;
  int*          stateVarTypes;
  FuncDatum*    stateVars;
  bool*         pStateVarNulls;
};

class UdxInit
{
public:
    UdxInit() { m_dummy = 0; }
    ~UdxInit() { }

protected:
    int m_dummy;
};

class UdxBase
{
 public:
    UdxBase(UdxInit *pInit)
    {
      m_pInfo = NULL;
      m_pInit = pInit;
      m_memory = 0;
      m_pEnviron = NULL;
      m_pParameter = NULL;
    }
  virtual ~UdxBase() {
  } 
  
  enum DataType
    {
      UDX_UNKNOWN=-1,
      UDX_FIXED,
      UDX_VARIABLE,
      UDX_NATIONAL_FIXED,
      UDX_NATIONAL_VARIABLE,
      UDX_BOOL,
      UDX_DATE,
      UDX_TIME,
      UDX_TIMETZ,
      UDX_NUMERIC32,
      UDX_NUMERIC64,
      UDX_NUMERIC128,
      UDX_FLOAT,
      UDX_DOUBLE,
      UDX_INTERVAL,
      UDX_INT8,
      UDX_INT16,
      UDX_INT32,
      UDX_INT64,
      UDX_TIMESTAMP,
      UDX_GEOMETRY,
      UDX_VARBINARY,
      UDX_NUMERIC=UDX_NUMERIC128 // Used for calculateSize only
    };

  // Log methods
  void logMsg(bool isLogEnabled, const char* fmt, ...) const ;
  
  // Arguments methods
  int numArgs() const;

  bool isArgConst(int n) const;
  int argType(int n) const;
  bool isArgNull(int n) const;

  StringArg* stringArg(int n) const;
  bool  boolArg(int n)  const;
  int16 int16Arg(int n) const;
  int32 int32Arg(int n) const;
  int64 int64Arg(int n) const;
  float floatArg(int n) const;
  double doubleArg(int n) const;
  int32 dateArg(int n) const;
  int64 timeArg(int n) const;
  int64 timestampArg(int n)const; 
  UdxEnvironment* getEnvironment() {
      return m_pEnviron;
  }

  virtual void calculateShape(UdxOutputShaper *shaper) {
      throwUdxException(NZ_MISSING_CALCULATE_SHAPE_ERROR_MSG);
  }
 
 private:
  int  m_memory;

 public:
  CallInfo *m_pInfo;
  UdxEnvironment *m_pEnviron;
  UdxParameter *m_pParameter;
  UdxInit *m_pInit;
};



class UdxScalarBase : public UdxBase
{
public:
    UdxScalarBase(UdxInit *pInit) : UdxBase(pInit) {}
  
  // Return methods
  ReturnValue stringReturn(StringReturn* ret) const;
  ReturnValue int16Return(int16 ret) const;
  ReturnValue int32Return(int32 ret) const;
  ReturnValue int64Return(int64 ret) const;
  ReturnValue floatReturn(float ret) const;
  ReturnValue doubleReturn(double ret) const;
  ReturnValue dateReturn(int32 ret) const;
  ReturnValue timeReturn(int64 ret) const;
  ReturnValue timestampReturn(int64 ret)const;
  ReturnValue boolReturn(bool ret) const;

  void setReturnNull(bool val) const;
  int returnType() const;

  StringReturn* stringReturnInfo() const;

  // The following will be used to support generic sized
  // string and numeric return values.

  // Can return UDX_NUMERIC, UDX_FIXED, UDX_VARIABLE,
  // UDX_NATIONAL_FIXED or UDX_NATIONAL_VARIABLE,
  int sizerReturnType() const;

  // Number of arguments the function is called with
  int numSizerArgs() const;

  // The type of the specified argument. (Can be any
  // of the DataType enums (Except UDX_NUMERIC). Will throw
  // an exception if n is out of range.
  int sizerArgType(int n) const;

  // Returns the size of the specified argument. Will throw
  // exceptions if the n is out of range, if the specified
  // argument is not a string type or the specified argument
  // does not have a size. (shouldn't happen) This size is in characters,
  // not bytes. (this matters for the national strings)
  int sizerStringArgSize(int n) const;

  // Builds a return value for calculateSize given the
  // string length desired. Will throw an exception if
  // teturn xza type is not a string. This length is in characters,
  // not bytes. (this matters for the national strings)
  uint64 sizerStringSizeValue(int len) const;

  // In order to support certain methods like round(val, scale)
  // we have a mechanism for passing constant arguments to the
  // sizer so long as they are of int32 type.
  // This method will return true if the specified argument is
  // a constant integer. Will throw and exception if the n is
  // out of range or if the specified argument is not an int32
  // If the constant specified is -1, isSizerArgConstants
  // will return false.
  bool isSizerArgConstant(int n) const;

  // This method will return the specified constant int32 argument
  // Will throw and exception if the n is out of range, the specified
  // argument is not an int32 or the specified argument is
  // not constant.
  int32 sizerGetConstantArg(int n) const;

  // This method must be overriden to provide sizing
  // capabilities for numerics and strings. This must
  // be done in conjunction with the DDL.
  virtual uint64 calculateSize() const
  {
      return 0xFFFFFFFFFFFFFFFFLL;
  }

  void setSizer(bool val) { sizer = val; }
  bool isSizer() const;

private:
  bool sizer;
};

inline bool UdxScalarBase::isSizer() const
{
  return sizer;
}

inline void UdxBase::logMsg(bool isLogEnabled, const char* fmt, ...) const
{
    int returnCode = 0;
    va_list vaList;
    va_start(vaList, fmt);

    sqlerRoutineLogMessage(isLogEnabled, 
                           fmt, 
                           vaList);
    va_end(vaList);
}

inline int UdxBase::numArgs() const
{
    CHECK_INFO_AVAILABLE();
  return m_pInfo->numArgs;
}

inline bool UdxBase::isArgConst(int n) const
{
    CHECK_INFO_AVAILABLE();
  CHECK_FUNC_RANGE(n);
  
  return m_pInfo->argConsts[n];
}
inline int UdxBase::argType(int n) const
{
    CHECK_INFO_AVAILABLE();
  CHECK_FUNC_RANGE(n);
  return m_pInfo->argTypes[n];
}
inline bool UdxBase::isArgNull(int n) const
{
    CHECK_INFO_AVAILABLE();
  CHECK_FUNC_RANGE(n);
  return m_pInfo->argNulls[n];
}

inline StringArg* UdxBase::stringArg(int n) const
{
    CHECK_INFO_AVAILABLE();
  CHECK_FUNC_RANGE(n);
  int nType = argType(n);
  if (nType != UDX_FIXED && nType != UDX_VARIABLE && nType != UDX_NATIONAL_FIXED &&
      nType != UDX_NATIONAL_VARIABLE && nType != UDX_GEOMETRY && nType != UDX_VARBINARY)
    throwError(NZ_ERROR_ILLVAL, "Expected string, st_geometry, or varbinary argument", nType);
  return ((StringArg*)m_pInfo->args[n]);
}
inline float UdxBase::floatArg(int n) const
{
    CHECK_INFO_AVAILABLE();
  CHECK_FUNC_RANGE(n);
  int nType = argType(n);
  if (nType != UDX_FLOAT)
    throwError(NZ_ERROR_ILLVAL, "Expected float argument", nType);
  return *((float*)m_pInfo->args[n]);
}

inline double UdxBase::doubleArg(int n) const
{
    CHECK_INFO_AVAILABLE();
  CHECK_FUNC_RANGE(n);
  int nType = argType(n);
  if (nType != UDX_DOUBLE)
    throwError(NZ_ERROR_ILLVAL, "Expected double argument", nType);
  return *((double*)m_pInfo->args[n]);
}

inline bool UdxBase::boolArg(int n) const
{
  CHECK_INFO_AVAILABLE();
  CHECK_FUNC_RANGE(n);
  int nType = argType(n);
  if (nType != UDX_BOOL)
    throwError(NZ_ERROR_ILLVAL, "Expected bool argument", nType);
  return *((bool*)m_pInfo->args[n]);
}
inline int16 UdxBase::int16Arg(int n) const
{
    CHECK_INFO_AVAILABLE();
  CHECK_FUNC_RANGE(n);
  int nType = argType(n);
  if (nType != UDX_INT16)
    throwError(NZ_ERROR_ILLVAL, "Expected int16 argument", nType);
  return *((int16*)m_pInfo->args[n]);
}
inline int32 UdxBase::int32Arg(int n) const
{
    CHECK_INFO_AVAILABLE();
  CHECK_FUNC_RANGE(n);
  int nType = argType(n);
  if (nType != UDX_INT32)
    throwError(NZ_ERROR_ILLVAL, "Expected int32 argument", nType);
  return *((int32*)m_pInfo->args[n]);
} 
inline int64 UdxBase::int64Arg(int n) const
{
    CHECK_INFO_AVAILABLE();
  CHECK_FUNC_RANGE(n);
  int nType = argType(n);
  if (nType != UDX_INT64)
    throwError(NZ_ERROR_ILLVAL, "Expected int64 argument", nType);
  return *((int64*)m_pInfo->args[n]);
}

inline int32 UdxBase::dateArg(int n) const
{
    CHECK_INFO_AVAILABLE();
  CHECK_FUNC_RANGE(n);
  int nType = argType(n);
  if (nType != UDX_DATE)
    throwError(NZ_ERROR_ILLVAL, "Expected date argument", nType);
  return *((int32*)m_pInfo->args[n]);
}
inline int64 UdxBase::timeArg(int n) const
{
    CHECK_INFO_AVAILABLE();
  CHECK_FUNC_RANGE(n);
  int nType = argType(n);
  if (nType != UDX_TIME)
    throwError(NZ_ERROR_ILLVAL, "Expected time argument", nType);
  return *((int64*)m_pInfo->args[n]);
}

inline int64 UdxBase::timestampArg(int n) const
{
    CHECK_INFO_AVAILABLE();
  CHECK_FUNC_RANGE(n);
  int nType = argType(n);
  if (nType != UDX_TIMESTAMP)
    throwError(NZ_ERROR_ILLVAL, "Expected timestamp argument", nType);
  return *((int64*)m_pInfo->args[n]);
}

inline int UdxScalarBase::returnType() const
{
    CHECK_INFO_AVAILABLE();
  return ((FuncCallInfo*)m_pInfo)->returnType;
}

inline ReturnValue UdxScalarBase::stringReturn(StringReturn* ret) const
{
    CHECK_INFO_AVAILABLE();
  if (((FuncCallInfo*)m_pInfo)->returnType != UDX_FIXED && ((FuncCallInfo*)m_pInfo)->returnType != UDX_VARIABLE &&
      ((FuncCallInfo*)m_pInfo)->returnType != UDX_NATIONAL_FIXED && ((FuncCallInfo*)m_pInfo)->returnType != UDX_NATIONAL_VARIABLE &&
      ((FuncCallInfo*)m_pInfo)->returnType != UDX_GEOMETRY && ((FuncCallInfo*)m_pInfo)->returnType != UDX_VARBINARY)
    throwError(NZ_ERROR_ILLVAL, "Expected string, st_geometry, or varbinary return type", ((FuncCallInfo*)m_pInfo)->returnType);
  setReturnNull(false);
  return (ReturnValue)(void*)ret;
}

inline ReturnValue UdxScalarBase::int16Return(int16 ret) const
{
    CHECK_INFO_AVAILABLE();
  if (((FuncCallInfo*)m_pInfo)->returnType != UDX_INT16)
    throwError(NZ_ERROR_ILLVAL, "Expected int16 return type", ((FuncCallInfo*)m_pInfo)->returnType);
  setReturnNull(false);
  return (ReturnValue)ret;
}
inline ReturnValue UdxScalarBase::int32Return(int32 ret) const
{
    CHECK_INFO_AVAILABLE();
  if (((FuncCallInfo*)m_pInfo)->returnType != UDX_INT32)
    throwError(NZ_ERROR_ILLVAL, "Expected int32 return type", ((FuncCallInfo*)m_pInfo)->returnType);
  setReturnNull(false);
  return (ReturnValue)ret;
}
inline ReturnValue UdxScalarBase::int64Return(int64 ret) const
{
    CHECK_INFO_AVAILABLE();
  if (((FuncCallInfo*)m_pInfo)->returnType != UDX_INT64)
    throwError(NZ_ERROR_ILLVAL, "Expected int64 return type", ((FuncCallInfo*)m_pInfo)->returnType);
  setReturnNull(false);
  return (ReturnValue)ret;
}
inline ReturnValue UdxScalarBase::floatReturn(float ret) const
{
    CHECK_INFO_AVAILABLE();
  if (((FuncCallInfo*)m_pInfo)->returnType != UDX_FLOAT)
    throwError(NZ_ERROR_ILLVAL, "Expected float return type", ((FuncCallInfo*)m_pInfo)->returnType);
  setReturnNull(false);
  return (ReturnValue)ret;
}
inline ReturnValue UdxScalarBase::doubleReturn(double ret) const
{
    CHECK_INFO_AVAILABLE();
  if (((FuncCallInfo*)m_pInfo)->returnType != UDX_DOUBLE)
    throwError(NZ_ERROR_ILLVAL, "Expected double return type", ((FuncCallInfo*)m_pInfo)->returnType);
  setReturnNull(false);
  return (ReturnValue)ret;
}
inline ReturnValue UdxScalarBase::dateReturn(int32 ret) const
{
    CHECK_INFO_AVAILABLE();
  if (((FuncCallInfo*)m_pInfo)->returnType != UDX_DATE)
    throwError(NZ_ERROR_ILLVAL, "Expected date return type", ((FuncCallInfo*)m_pInfo)->returnType);
  setReturnNull(false);
  return (ReturnValue)ret;
}
inline ReturnValue UdxScalarBase::timeReturn(int64 ret) const
{
    CHECK_INFO_AVAILABLE();
  if (((FuncCallInfo*)m_pInfo)->returnType != UDX_TIME)
    throwError(NZ_ERROR_ILLVAL, "Expected time return type", ((FuncCallInfo*)m_pInfo)->returnType);
  setReturnNull(false);
  return (ReturnValue)ret;
}

inline ReturnValue UdxScalarBase::timestampReturn(int64 ret) const
{
    CHECK_INFO_AVAILABLE();
  if (((FuncCallInfo*)m_pInfo)->returnType != UDX_TIMESTAMP)
    throwError(NZ_ERROR_ILLVAL, "Expected timestamp return type", ((FuncCallInfo*)m_pInfo)->returnType);
  setReturnNull(false);
  return (ReturnValue)ret;
}

inline ReturnValue UdxScalarBase::boolReturn(bool ret) const
{
    CHECK_INFO_AVAILABLE();
  if (((FuncCallInfo*)m_pInfo)->returnType != UDX_BOOL)
    throwError(NZ_ERROR_ILLVAL, "Expected bool return type", ((FuncCallInfo*)m_pInfo)->returnType);
  setReturnNull(false);
  return (ReturnValue)ret;
}

inline void UdxScalarBase::setReturnNull(bool val) const
{
    CHECK_INFO_AVAILABLE();
  *((FuncCallInfo*)m_pInfo)->pReturnNull = val;
}

inline StringReturn* UdxScalarBase::stringReturnInfo() const
{
    CHECK_INFO_AVAILABLE();
  if (((FuncCallInfo*)m_pInfo)->returnType != UDX_FIXED && ((FuncCallInfo*)m_pInfo)->returnType != UDX_VARIABLE && 
      ((FuncCallInfo*)m_pInfo)->returnType != UDX_NATIONAL_FIXED && ((FuncCallInfo*)m_pInfo)->returnType != UDX_NATIONAL_VARIABLE &&
      ((FuncCallInfo*)m_pInfo)->returnType != UDX_GEOMETRY && ((FuncCallInfo*)m_pInfo)->returnType != UDX_VARBINARY)
    throwError(NZ_ERROR_ILLVAL, "Expected string, st_geometry, or varbinary return type.", ((FuncCallInfo*)m_pInfo)->returnType);
  if (!((FuncCallInfo*)m_pInfo)->pReturnInfo)
    throwError(NZ_ERROR_ILLVAL, "Expected string return, geometry structure, or varbinary structure.", 0);
  return (StringReturn*)((FuncCallInfo*)m_pInfo)->pReturnInfo;
}

inline bool UdxScalarBase::isSizerArgConstant(int n) const
{
  if(isSizer()) 
  {
    CHECK_INFO_AVAILABLE();
    CHECK_SIZER_RANGE(n);
    int nType = sizerArgType(n);
    if (nType != UDX_INT32 )
        throwError(NZ_ERROR_ILLVAL, "Expected int32 argument", nType);
    return m_pInfo->argConsts[n];
  }
  return throwError(NZ_ERROR_UDX_EXCEPTION,"Invalid use of API isSizerArgConstant", 0);
}
inline int32 UdxScalarBase::sizerGetConstantArg(int n) const
{
  if(isSizer()) 
  {
    CHECK_INFO_AVAILABLE();
    CHECK_SIZER_RANGE(n);
    int nType = sizerArgType(n);
    if (nType != UDX_INT32)
        throwError(NZ_ERROR_ILLVAL, "Expected int32 argument", nType);
    return *((int32*)m_pInfo->args[n]);
  }
  return throwError(NZ_ERROR_UDX_EXCEPTION,"Invalid use of API sizerGetConstantArg",0); 
}
inline int UdxScalarBase::sizerReturnType() const
{
  if(isSizer())
  {
    CHECK_INFO_AVAILABLE();
    return ((FuncCallInfo*)m_pInfo)->returnType;
  }
  return throwError(NZ_ERROR_UDX_EXCEPTION,"Invalid use of API sizerReturnType",0); 
}
inline int UdxScalarBase::numSizerArgs() const
{
  if(isSizer())
  {
    CHECK_INFO_AVAILABLE();
    return m_pInfo->numArgs;
  }
  return throwError(NZ_ERROR_UDX_EXCEPTION,"Invalid use of API numSizerArgs",0);
}
inline int UdxScalarBase::sizerArgType(int n) const
{
  if(isSizer())
  {
    CHECK_INFO_AVAILABLE();
    CHECK_SIZER_RANGE(n);
    return m_pInfo->argTypes[n];
  }
  return throwError(NZ_ERROR_UDX_EXCEPTION,"Invalid use of API sizerArgType",0);
}

inline int UdxScalarBase::sizerStringArgSize(int n) const
{
  if(isSizer())
  {
    CHECK_INFO_AVAILABLE();
    CHECK_SIZER_RANGE(n);

    int nType = sizerArgType(n);
    if (nType != UDX_FIXED && nType != UDX_VARIABLE && nType != UDX_NATIONAL_FIXED &&
            nType != UDX_NATIONAL_VARIABLE && nType != UDX_GEOMETRY && nType != UDX_VARBINARY)
        throwError(NZ_ERROR_ILLVAL, "Expected string, st_geometry, or varbinary argument", nType);
    return ((StringArg *)m_pInfo->args[n])->dec_length;
  }
  return throwError(NZ_ERROR_UDX_EXCEPTION,"Invalid use of API sizerStringArgSize",0);
}

inline uint64 UdxScalarBase::sizerStringSizeValue(int len) const
{
  if(isSizer())
  {
    CHECK_INFO_AVAILABLE();
 
/*  int nType = ((FuncCallInfo*)m_pInfo)->returnType; 
    if (nType != UDX_FIXED && nType != UDX_VARIABLE && nType != UDX_NATIONAL_FIXED && 
        nType != UDX_NATIONAL_VARIABLE && nType != UDX_GEOMETRY && nType != UDX_VARBINARY) 
        throwError(NZ_ERROR_ILLVAL, "Expected string, st_geometry, or varbinary return type", nType); 
*/
    return len;
  }
  return throwError(NZ_ERROR_UDX_EXCEPTION,"Invalid use of API sizerStringSizeValue",0);
}
} // end namespace udx_ver2
} // end namespace nz

#endif //  #ifndef UDX_BASEV2_INCLUDED
