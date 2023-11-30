#ifndef UDX_ENVIRONMENT_INCLUDED
#define UDX_ENVIRONMENT_INCLUDED

namespace nz {
namespace udx_ver2 {


class UdxEnvironmentEntry
{
private:
    const char *key;
    const char *value;
public:
    const char* getKey() const { return key; }
    const char* getValue() const { return value; }
    UdxEnvironmentEntry(const char *strKey, const char *strValue)
    {
       key = strKey ;
       value = strValue; 
    }
};

class UdxEnvironment
{
public:
    int numEnv;
    UdxEnvironmentEntry **envEntries;

    int getNumEntries() const;
    int findEntry(const char* strKey) const;
    const UdxEnvironmentEntry* getEntry(int idx) const;
    const UdxEnvironmentEntry* getEntry(const char* strKey) const;
     
};

inline int UdxEnvironment::getNumEntries() const
{
  return numEnv;
} 

inline const UdxEnvironmentEntry* UdxEnvironment::getEntry(int idx) const
{
  if (idx < 0 || idx >= numEnv)
     throwUdxException("Index is out of bounds");
  return envEntries[idx];
}

inline int UdxEnvironment::findEntry(const char* strKey) const
{
    for (int i=0; i < numEnv;i++)
    {
        if (strcmp(strKey, envEntries[i]->getKey()) == 0)
            return i;
    }
    return -1;
}

inline const UdxEnvironmentEntry* UdxEnvironment::getEntry(const char* strKey) const
{
    int idx = findEntry(strKey);
    if (idx == -1)
        return NULL;
    return envEntries[idx];
}

} // end namespace udx_ver2
} // end namespace nz


#endif //  #ifndef UDX_ENVIRONMENT_INCLUDED
