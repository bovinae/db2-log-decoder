// udxcolumninfo.h
// ColumnInfo classes for User Defined Functions and Aggregates
//
// (C) Copyright IBM Corp. 2009, 2011  All Rights Reserved.

#ifndef UDX_COLUMNINFO_INCLUDED
#define UDX_COLUMNINFO_INCLUDED

namespace nz {  
namespace udx_ver2 {

class UdxColumnInfo
{
public:
    // For strings, first will be size.
    // For numerics first will be precision, second will be scale.
    UdxColumnInfo(int nType, const char* strName, int first=-1, int second=-1) {
        m_type = nType;
        if (strName)
            m_strName = strdup(strName);
        else
            m_strName = NULL;
        m_first = first;
        m_second = second;
        
    }
    UdxColumnInfo(int nType, int first=-1, int second=-1) {
        m_type = nType;
        m_strName = NULL;
        m_first = first;
        m_second = second;
        
    }

    ~UdxColumnInfo() {
        if (m_strName)
            free(m_strName);
    }

    int getType() const {
        return m_type; 
    }
    int getSize() const {
        return m_first;
    }
    int getPrecision() const {
        return m_first;
    }
    int getScale() const {
        return m_second;
    }
    const char* getName() const {
        return m_strName;
    }

private:
    int m_type;
    int m_first;
    int m_second;
    char *m_strName;
};

class UdxColumnInfoList
{
public:
    UdxColumnInfoList() {
        m_nSize = 0;
        m_nAllocated = 8;
        m_pEntries = new UdxColumnInfo*[m_nAllocated];
        if (m_pEntries == NULL)
            throwUdxException("Couldn't allocate entries array");
    }
    ~UdxColumnInfoList() {
        if (m_pEntries) 
        {
            for (int i=0; i < m_nSize; i++)
                delete m_pEntries[i];
            delete [] m_pEntries;
        }
    }

    int numColumns() const { return m_nSize; } 
    const UdxColumnInfo * getEntry(int idx) const;
    int findEntry(const char* strName) const ;
    void addEntry(int nType, const char* strName, int first, int second);
    void addEntry(int nType, int first, int second) {
        addEntry(nType, NULL, first, second);
    }

private:
    int m_nSize;
    int m_nAllocated;
    UdxColumnInfo** m_pEntries;
};

inline void UdxColumnInfoList::addEntry(int nType, const char* strName, int first, int second)
{    
    if (m_nSize == m_nAllocated-1)
    {
        m_nAllocated *= 2;
        UdxColumnInfo ** temp = new UdxColumnInfo*[m_nAllocated];
        if (temp == NULL)
            throwUdxException("Couldn't allocate new entries array");
        for (int i=0; i < m_nSize; i++)
            temp[i] = m_pEntries[i];
        delete [] m_pEntries;
        m_pEntries = temp;
    }       
    m_pEntries[m_nSize] = new UdxColumnInfo(nType, strName, first, second);
    if (m_pEntries[m_nSize] == NULL)
        throwUdxException("Couldn't allocate new column entry");
    m_nSize++;
    
}

inline const UdxColumnInfo* UdxColumnInfoList::getEntry(int idx) const
{
    if (idx < 0 || idx >= m_nSize)
        throwUdxException("Index is out of bounds");
    return m_pEntries[idx];

}
inline int UdxColumnInfoList::findEntry(const char* strKey) const
{
    for (int i=0; i < m_nSize;i++)
    {
        if (m_pEntries[i]->getName())
        if (strcmp(strKey, m_pEntries[i]->getName()) == 0)
            return i;
    }
    return -1;
}
} // end namespace udx_ver2
} // end namespace nz

#endif //  #ifndef UDX_COLUMNINFO_INCLUDED
