/**********************************************************************
* 
* Source File Name: asn.h
* 
* (C) COPYRIGHT International Business Machines Corp. 1987, 2004
* All Rights Reserved
* Licensed Materials - Property of IBM
* 
* US Government Users Restricted Rights - Use, duplication or
* disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
* 
* Function = Include File defining:
*               System Constants
*               Various Asn Structs
*               API Functions
*
* Operating System: AIX, NT
* 
************************************************************************/

#ifndef __ASN__
#define __ASN__

#ifndef OS_ZOS 
//----------------------------------------------------------------------
// NOTE: About ASN version
//  Since inclusion into the IIDR, these values must reflect the
//  IIDR level, and NOT the db2 build environment variable.
//  (The same goes for z/OS, but the version for z/OS builds is 
//   specified in the build script rather than in this file)
//  Sometimes IIDR level and db2 build environment are the same, but it
//  might not always be that way in the future.
//
//  DB2 Release              IIDR Release
//   Galileo
//  10.1.2                   10.1.3
//  10.1.3                   10.1.3
//
//   Kepler
//  10.5.0                   10.2.0 ?
//  10.5.6                   10.2.1
//  
//   V11
//  11.1.1                   11.4.0
//----------------------------------------------------------------------
#define AsnVersion       11
#define AsnRelease       4 
#define AsnModification  0
#endif  //OS_ZOS 

#define MAXASNPARMLENGTH 128

/****************************************************************/
/* This structure asnParm is used to represent each parameter   */
/* field of Apply or Capture invocation. The two fields are:    */
/*   short bytecount -- total length of the parameter field.    */
/*   char val[128]   -- stores the parameter value.             */
/****************************************************************/
struct asnParm
{
   short byteCount;
   char val[MAXASNPARMLENGTH];
};

/****************************************************************/
/* This structure asnParms is a handle to the individual        */
/* structure above (asnParm). The two fields are:               */
/*    int parmCount          -- count of the total number of    */
/*                              parameters for Apply or Capture */
/*    struct asnParm **parms -- parms is basically an array of  */
/*                              pointers to data type struct    */
/*                              asnParm.                        */
/****************************************************************/
struct asnParms
{
   int parmCount;
   struct asnParm **parms;
};

/****************************************************************/
/* SQL Replication                                              */
/****************************************************************/

#ifdef __cplusplus                               
extern "C" {
#endif
#if OS_NT                                                  
__declspec(dllexport)                                        
#endif
int asnCapture(struct asnParms *pAsnParms);    /* API invocation */
#ifdef __cplusplus  
}
#endif

#ifdef __cplusplus                                        
extern "C" {
#endif
#if OS_NT                                                     
__declspec(dllexport)                                       
#endif                                                     
int asnApply(struct asnParms *pAsnParms);      /* API invocation */
#ifdef __cplusplus
}
#endif                                                        

#ifdef __cplusplus                                        
extern "C" {
#endif
#if OS_NT                                                     
__declspec(dllexport)                                       
#endif                                                     
int asnMonitor(struct asnParms *pAsnParms);    /* API invocation */
#ifdef __cplusplus
}
#endif        

#if OS_NT                                            
__declspec(dllexport)                                       
#else                                                          
extern                                                      
#endif                                                      
/* actual Capture code main */
int _asnCapture(int argc, char *argv[]); 

#ifdef __cplusplus                       
extern "C" {
#endif
#if OS_NT 
__declspec(dllexport)
#endif
/* actual Apply code main */
int _asnApply(int argc, char *argv[]);   

#if OS_NT                                            
__declspec(dllexport)                                       
#else                                                          
extern                                                      
#endif                                                      
/* actual Monitor code main */
int _asnMonitor(int argc, char *argv[]);

#if OS_NT                                            
__declspec(dllexport)                                       
#else                                                          
extern                                                      
#endif                                                      
/* actual Trace code main */
int _asnTrace(int argc, char *argv[]); 

#ifdef __cplusplus
}
#endif  

/****************************************************************/
/* Q Replication                                                */
/****************************************************************/
#ifdef __cplusplus                               
extern "C" {
#endif
#if OS_NT                                                  
__declspec(dllexport)                                        
#endif
int asnQCapture(int argc, char *argv[]);   /* API invocation */
#ifdef __cplusplus  
}
#endif

#ifdef __cplusplus                               
extern "C" {
#endif
#if OS_NT                                                  
__declspec(dllexport)                                        
#endif
int asnQApply(int argc, char *argv[]);    /* API invocation */
#ifdef __cplusplus  
}
#endif

#ifndef sysnti
#ifndef sysos2

#define Ccatgets(p1, p2, p3, p4) catgets((p1), (p2), (p3), (p4))

#endif
#endif

#endif /* __ASN__ */
