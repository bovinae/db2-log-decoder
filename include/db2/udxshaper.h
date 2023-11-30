// udxshaper.h
// Shaper classes for User Defined Functions and Aggregates
//
// (C) Copyright IBM Corp. 2009, 2011  All Rights Reserved.

#ifndef UDX_SHAPER_INCLUDED
#define UDX_SHAPER_INCLUDED

namespace nz {  
namespace udx_ver2 {

class UdxArgumentInfo
{
public:
    struct ArgInfo
    {
        // Need to change these into dynamic arrays
        int     	numArgs;
        int*	        argTypes;
        FuncDatum*	args;
        bool*		argNulls;
        bool*           argConsts;
        uint64*         argTypmods;
    };

    UdxArgumentInfo(ArgInfo *pInfo) {
        m_pInfo = pInfo;
    }
    
    typedef  nz::udx_ver2::UdxBase dt;

    FuncDatum getArg(int n) const;

    // Input Information
    int numArgs() const;
    int argType(int n) const;
    int stringArgSize(int n) const;
    
    bool isArgConst(int n) const;
    bool isArgNull(int n) const;
    StringArg* stringArg(int n) const;
    int32 dateArg(int n) const;
    int64 timeArg(int n) const;
    int64 timestampArg(int n) const;
    int16 int16Arg(int n) const;
    int32 int32Arg(int n) const;
    int64 int64Arg(int n) const;
    float floatArg(int n) const;
    double doubleArg(int n) const;
    
    const ArgInfo * getArgInfo() { return m_pInfo; }

protected:
    // used by derived classes
    // named columns are output
    // unnamed columns are state

    // For strings, first will be size.
    // For numerics first will be precision, second will be scale.
    void addNamedColumn(int nType, const char* strName, int first=-1, int second=-1); 
    int numNamedColumns() {
        return m_namedColumns.numColumns();
    }
    const UdxColumnInfo* getNamedColumn(int n) {
        return m_namedColumns.getEntry(n);
    }

    void addUnNamedColumn(int nType, int first=-1, int second=-1); 
    int numUnNamedColumns() {
        return m_unNamedColumns.numColumns();
    }
    const UdxColumnInfo* getUnNamedColumn(int n) {
        return m_unNamedColumns.getEntry(n);
    }

private:
    ArgInfo *m_pInfo;
    UdxColumnInfoList m_namedColumns;
    UdxColumnInfoList m_unNamedColumns;
};

inline void UdxArgumentInfo::addNamedColumn(int nType, const char* strName, int first, int second)
{
    int idx = m_namedColumns.findEntry(strName);
    if (idx != -1)
        throwUdxException("Column already exists with same name");
    if (nType == dt::UDX_FIXED || nType == dt::UDX_VARIABLE || 
        nType == dt::UDX_NATIONAL_FIXED || nType == dt::UDX_NATIONAL_VARIABLE ||
        nType == dt::UDX_GEOMETRY || nType == dt::UDX_VARBINARY)
    {
        if (first == -1)
            throwUdxException("Must specify a size for string, st_geometry, and varbinary columns.");
        if (second != -1)
            throwUdxException("Cannot specify a scale value for string, st_geometry, or varbinary columns.");
    }
    else if (nType == dt::UDX_NUMERIC32 || nType == dt::UDX_NUMERIC64 || nType == dt::UDX_NUMERIC128)
    {
        if (first == -1)
            throwUdxException("Must specify a precision for numeric columns");
        if (second == -1)
            throwUdxException("Must specify a scale for numeric columns");
    }
    else 
    {
        if (first != -1)
            throwUdxException("Can't specify a size or precision for this column type");
        if (second != -1)
            throwUdxException("Can't specify a scale for this column type");
    }
    m_namedColumns.addEntry(nType, strName, first, second);
}

inline void UdxArgumentInfo::addUnNamedColumn(int nType, int first, int second)
{
    if (nType == dt::UDX_FIXED || nType == dt::UDX_VARIABLE || 
        nType == dt::UDX_NATIONAL_FIXED || nType == dt::UDX_NATIONAL_VARIABLE ||
        nType == dt::UDX_GEOMETRY || nType == dt::UDX_VARBINARY)
    {
        if (first == -1)
            throwUdxException("Must specify a size for string, st_geometry, and varbinary columns.");
        if (second != -1)
            throwUdxException("Cannot specify a scale value for string, st_geometry, or varbinary columns.");
    }
    else if (nType == dt::UDX_NUMERIC32 || nType == dt::UDX_NUMERIC64 || nType == dt::UDX_NUMERIC128)
    {
        if (first == -1)
            throwUdxException("Must specify a precision for numeric columns");
        if (second == -1)
            throwUdxException("Must specify a scale for numeric columns");
    }
    else 
    {
        if (first != -1)
            throwUdxException("Can't specify a size or precision for this column type");
        if (second != -1)
            throwUdxException("Can't specify a scale for this column type");
    }
    m_unNamedColumns.addEntry(nType, first, second);
}

#undef CHECK_INFO_AVAILABLE

#define CHECK_SIZER_TYPMOD(n, typmod) if (typmod == 0xFFFFFFFFFFFFFFFFLL) \
                  throwError(NZ_ERROR_ILLVAL, "Sizer argument invalid", n)

#define CHECK_FUNC_RANGE(n) if (n < 0 || n >= m_pInfo->numArgs) \
          throwError(NZ_ERROR_ILLVAL, "Argument number out of range", n)

#define CHECK_INFO_AVAILABLE() if (!m_pInfo) \
          throwUdxException("UDX information not available yet")

inline FuncDatum UdxArgumentInfo::getArg(int n) const
{
    CHECK_INFO_AVAILABLE();
    CHECK_FUNC_RANGE(n);
    return m_pInfo->args[n];
}
inline int UdxArgumentInfo::numArgs() const
{
    CHECK_INFO_AVAILABLE();
    return m_pInfo->numArgs;
}
inline int UdxArgumentInfo::argType(int n) const
{
    CHECK_INFO_AVAILABLE();
    CHECK_FUNC_RANGE(n);
    return m_pInfo->argTypes[n];
}
inline int UdxArgumentInfo::stringArgSize(int n) const
{
    CHECK_INFO_AVAILABLE();
    CHECK_FUNC_RANGE(n);
    int nType = argType(n);
    if (nType != dt::UDX_FIXED && nType != dt::UDX_VARIABLE &&
        nType != dt::UDX_NATIONAL_FIXED && nType != dt::UDX_NATIONAL_VARIABLE &&
        nType != dt::UDX_GEOMETRY && nType != dt::UDX_VARBINARY)
        throwUdxException("Expected a string, st_geometry, or varbinary argument");
/*
    uint64 typmod = m_pInfo->argTypmods[n] & 0xFFFFFFFF;
    CHECK_SIZER_TYPMOD(n, typmod);
    typmod -= VARHDRSZ;
    return (int32)typmod;
*/
return ((StringArg*)m_pInfo->args[n])->dec_length;
}
inline bool UdxArgumentInfo::isArgConst(int n) const
{   
    CHECK_INFO_AVAILABLE();
    CHECK_FUNC_RANGE(n);
    return m_pInfo->argConsts[n];
}
inline bool UdxArgumentInfo::isArgNull(int n) const
{
    CHECK_INFO_AVAILABLE();
    CHECK_FUNC_RANGE(n);
    return m_pInfo->argNulls[n];
}
inline StringArg* UdxArgumentInfo::stringArg(int n) const
{
    CHECK_INFO_AVAILABLE();
    CHECK_FUNC_RANGE(n);
    int nType = argType(n);
    if (nType != dt::UDX_FIXED && nType != dt::UDX_VARIABLE &&
        nType != dt::UDX_NATIONAL_FIXED && nType != dt::UDX_NATIONAL_VARIABLE &&
        nType != dt::UDX_GEOMETRY && nType != dt::UDX_VARBINARY)
        throwUdxException("Expected string, st_geometry, or varbinary argument");
    return ((StringArg*)m_pInfo->args[n]);

}
inline int32 UdxArgumentInfo::dateArg(int n) const
{
    CHECK_INFO_AVAILABLE();
    CHECK_FUNC_RANGE(n);
    int nType = argType(n);
    if (nType != dt::UDX_DATE)
        throwUdxException("Expected date argument");
    return *((int32*)m_pInfo->args[n]);

}
inline int64 UdxArgumentInfo::timeArg(int n) const
{
    CHECK_INFO_AVAILABLE();
    CHECK_FUNC_RANGE(n);
    int nType = argType(n);
    if (nType != dt::UDX_TIME)
        throwUdxException("Expected time argument");
    return *((int64*)m_pInfo->args[n]);

}

inline int64 UdxArgumentInfo::timestampArg(int n) const
{
    CHECK_INFO_AVAILABLE();
    CHECK_FUNC_RANGE(n);
    int nType = argType(n);
    if (nType != dt::UDX_TIMESTAMP)
        throwUdxException("Expected timestamp argument");
    return *((int64*)m_pInfo->args[n]);
}

inline int16 UdxArgumentInfo::int16Arg(int n) const
{
    CHECK_INFO_AVAILABLE();
    CHECK_FUNC_RANGE(n);
    int nType = argType(n);
    if (nType != dt::UDX_INT16)
        throwUdxException("Expected int16 argument");
    return *((int16*)m_pInfo->args[n]);
}
inline int32 UdxArgumentInfo::int32Arg(int n) const
{
    CHECK_INFO_AVAILABLE();
    CHECK_FUNC_RANGE(n);
    int nType = argType(n);
    if (nType != dt::UDX_INT32)
        throwUdxException("Expected int32 argument");
    return *((int32*)m_pInfo->args[n]);

}
inline int64 UdxArgumentInfo::int64Arg(int n) const
{
    CHECK_INFO_AVAILABLE();
    CHECK_FUNC_RANGE(n);
    int nType = argType(n);
    if (nType != dt::UDX_INT64)
        throwUdxException("Expected int64 argument");
    return *((int64*)m_pInfo->args[n]);

}
inline float UdxArgumentInfo::floatArg(int n) const
{
    CHECK_INFO_AVAILABLE();
    CHECK_FUNC_RANGE(n);
    int nType = argType(n);
    if (nType != dt::UDX_FLOAT)
        throwUdxException("Expected float argument");
    return *((float*)m_pInfo->args[n]);
}
inline double UdxArgumentInfo::doubleArg(int n) const
{
    CHECK_INFO_AVAILABLE();
    CHECK_FUNC_RANGE(n);
    int nType = argType(n);
    if (nType != dt::UDX_DOUBLE)
        throwUdxException("Expected double argument");
    return *((double*)m_pInfo->args[n]);

}
class UdxOutputInfo : public UdxArgumentInfo
{
public:
    UdxOutputInfo(ArgInfo *pInfo) : UdxArgumentInfo(pInfo) {}
    int numOutputColumns() {
        return numNamedColumns();
    }
    const UdxColumnInfo* getOutputColumn(int n) {
        return getNamedColumn(n);
    }
};

class UdxOutputShaper : public UdxOutputInfo
{
public:
    UdxOutputShaper(ArgInfo *pInfo, bool caseUpper) : UdxOutputInfo(pInfo) {
        m_bSystemCaseUpper = caseUpper;
    }
    // Output information    
    void addOutputColumn(int nType, const char* strName, int nSize) {
        addNamedColumn(nType, strName, nSize);
    }
    void addOutputColumn(int nType, const char* strName, int precision, int scale) {
        addNamedColumn(nType, strName, precision, scale);
    }
    void addOutputColumn(int nType, const char* strName) {
        addNamedColumn(nType, strName);
    }   
    bool isSystemCaseUpper() {
        return m_bSystemCaseUpper;
    }
private:
    bool m_bSystemCaseUpper;
};

#undef CHECK_INFO_AVAILABLE
#undef CHECK_FUNC_RANGE
#undef CHECK_SIZER_TYPMOD

} // end namespace udx_ver2
} // end namespace nz

#endif //  #ifndef UDX_SHAPER_INCLUDED
