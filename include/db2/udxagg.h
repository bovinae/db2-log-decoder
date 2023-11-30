// udxagg.h
// Aggregate class for User Defined Functions and Aggregates
//
// (C) Copyright IBM Corp. 2009, 2011  All Rights Reserved.

#ifndef UDX_AGGV2_INCLUDED
#define UDX_AGGV2_INCLUDED


#define CHECK_AGG_RANGE(n) if (n < 0 || n >= ((AggCallInfo*)m_pInfo)->numStateVars) \
		  throwError(NZ_ERROR_ILLVAL, "State number out of range", n)


namespace nz {  
namespace udx_ver2 {


class Uda : public UdxScalarBase
{
public:
    Uda(UdxInit *pInit) : UdxScalarBase(pInit)
        {
            m_aggregateType = UnknownAggregateType;
        }

	virtual ~Uda() {}
	
    enum AggregateType
    {
        UnknownAggregateType,
        Grouped,
        Analytic    
    };

    AggregateType aggregateType() const {
        return m_aggregateType;
    }
    void setAggregateType(AggregateType type) {
        m_aggregateType = type;
    }

	// Interface methods
				
	// Must initialize all state variables and state null indicators in this method
	virtual void initializeState() = 0;

	// Called once per each row (called by the  engine)
	// Called with state values. which will be updated when method returns
	virtual void accumulate() = 0;

	// Merge serialized state of another aggregate with this one.
	virtual void merge() = 0;

	// Finish up and return result.
	// Will not cleanup memory, destructor should do this.
	virtual ReturnValue finalResult() = 0;

	// The derived class must have a static method like such:
	// static Uda* instantiate();

	// State methods
	int numStateVars() const;
	int stateType(int n) const;
	void setStateNull(int n, bool val) const;
	bool isStateNull(int n) const;	
	StringArg* stringState(int n) const;
	float* floatState(int n) const;	
	double* doubleState(int n) const;
	int16* int16State(int n) const;
	int32* int32State(int n) const;
	int64* int64State(int n) const;
	int32* dateState(int n) const;
	int64* timeState(int n) const;
	int64* timestampState(int n) const;
        bool*  boolState(int n) const;

	// Called from Engine
	void performInitializeState(AggCallInfo *const pAcInfo);
	void performAccumulate(AggCallInfo *const pAcInfo);	
	void performMerge(AggCallInfo *const pAcInfo);
	ReturnValue performFinalResult(AggCallInfo *const pAcInfo);

	

private:
  AggregateType m_aggregateType;

};


inline int Uda::numStateVars() const
{
        CHECK_INFO_AVAILABLE();
        return ((AggCallInfo *)m_pInfo)->numStateVars;
}

inline int Uda::stateType(int n) const
{
    CHECK_INFO_AVAILABLE();
    CHECK_AGG_RANGE(n);
    return ((AggCallInfo *)m_pInfo)->stateVarTypes[n];
}
inline void Uda::setStateNull(int n, bool val) const
{
    CHECK_INFO_AVAILABLE();
    CHECK_AGG_RANGE(n);
    ((AggCallInfo *)m_pInfo)->pStateVarNulls[n] = val;
}
inline bool Uda::isStateNull(int n) const
{
    CHECK_INFO_AVAILABLE();
    CHECK_AGG_RANGE(n);
    return ((AggCallInfo *)m_pInfo)->pStateVarNulls[n];
}
	
inline StringArg* Uda::stringState(int n) const
{
    CHECK_INFO_AVAILABLE();
    CHECK_AGG_RANGE(n);
    int nType = stateType(n);
    if (nType != UDX_FIXED && nType != UDX_VARIABLE && nType != UDX_NATIONAL_FIXED &&
        nType != UDX_NATIONAL_VARIABLE  && nType != UDX_GEOMETRY && nType != UDX_VARBINARY)
        throwError(NZ_ERROR_ILLVAL, "Expected string, st_geometry, or varbinary state value", nType);
    return ((StringArg*)((AggCallInfo *)m_pInfo)->stateVars[n]);
}
inline float* Uda::floatState(int n) const
{
    CHECK_INFO_AVAILABLE();
    CHECK_AGG_RANGE(n);
    int nType = stateType(n);
    if (nType != UDX_FLOAT)
        throwError(NZ_ERROR_ILLVAL, "Expected float state value", nType);
    return ((float*)((AggCallInfo *)m_pInfo)->stateVars[n]);
}
	
inline double* Uda::doubleState(int n) const
{
    CHECK_INFO_AVAILABLE();
    CHECK_AGG_RANGE(n);
    int nType = stateType(n);
    if (nType != UDX_DOUBLE)
        throwError(NZ_ERROR_ILLVAL, "Expected double state value", nType);
    return ((double*)((AggCallInfo *)m_pInfo)->stateVars[n]);
}
inline int16* Uda::int16State(int n) const
{
    CHECK_INFO_AVAILABLE();
    CHECK_AGG_RANGE(n);
    int nType = stateType(n);
    if (nType != UDX_INT16)
        throwError(NZ_ERROR_ILLVAL, "Expected int16 state value", nType);
    return ((int16*)((AggCallInfo *)m_pInfo)->stateVars[n]);
}
inline int32* Uda::int32State(int n) const
{
    CHECK_INFO_AVAILABLE();
    CHECK_AGG_RANGE(n);
    int nType = stateType(n);
    if (nType != UDX_INT32)
        throwError(NZ_ERROR_ILLVAL, "Expected int32 state value", nType);
    return ((int32*)((AggCallInfo *)m_pInfo)->stateVars[n]);
}
inline int64* Uda::int64State(int n) const
{
    CHECK_INFO_AVAILABLE();
    CHECK_AGG_RANGE(n);
    int nType = stateType(n);
    if (nType != UDX_INT64)
        throwError(NZ_ERROR_ILLVAL, "Expected int64 state value", nType);
    return ((int64*)((AggCallInfo *)m_pInfo)->stateVars[n]);
}
inline int32* Uda::dateState(int n) const
{
    CHECK_INFO_AVAILABLE();
    CHECK_AGG_RANGE(n);
    int nType = stateType(n);
    if (nType != UDX_DATE)
        throwError(NZ_ERROR_ILLVAL, "Expected date state value", nType);
    return ((int32*)((AggCallInfo *)m_pInfo)->stateVars[n]);
}
inline int64* Uda::timeState(int n) const
{
    CHECK_INFO_AVAILABLE();
    CHECK_AGG_RANGE(n);
    int nType = stateType(n);
    if (nType != UDX_TIME)
        throwError(NZ_ERROR_ILLVAL, "Expected time state value", nType);
    return ((int64*)((AggCallInfo *)m_pInfo)->stateVars[n]);
}
 
 inline int64* Uda::timestampState(int n) const
 {
   CHECK_INFO_AVAILABLE();
   CHECK_AGG_RANGE(n);
   int nType = stateType(n);
   if (nType != UDX_TIMESTAMP)
     throwError(NZ_ERROR_ILLVAL, "Expected timestamp state value", nType);
   return ((int64*)((AggCallInfo *)m_pInfo)->stateVars[n]);
 }

inline bool* Uda::boolState(int n) const
{
    CHECK_INFO_AVAILABLE();
    CHECK_AGG_RANGE(n);
    int nType = stateType(n);
    if (nType != UDX_BOOL)
        throwError(NZ_ERROR_ILLVAL, "Expected bool state value", nType);
    return ((bool*)((AggCallInfo *)m_pInfo)->stateVars[n]);
}
 
inline void Uda::performInitializeState(AggCallInfo *const pAcInfo)
{
    m_pInfo = pAcInfo;
    initializeState();
    m_pInfo = NULL;
}
	
inline void Uda::performAccumulate(AggCallInfo *const pAcInfo)
{
    m_pInfo = pAcInfo;
    accumulate();
    m_pInfo = NULL;
}

inline void Uda::performMerge(AggCallInfo *const pAcInfo)
{
    m_pInfo = pAcInfo;
    merge();
    m_pInfo = NULL;
}

inline ReturnValue Uda::performFinalResult(AggCallInfo *const pAcInfo)
{
    ReturnValue oRet;
    m_pInfo = pAcInfo;
    oRet = finalResult();
    m_pInfo = NULL;
    return oRet;
}


} // end namespace udx_ver2
} // end namespace nz

#endif //  #ifndef UDX_AGGV2_INCLUDED
