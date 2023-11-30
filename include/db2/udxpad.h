/******************************************************************************
 *
 * Source File Name = UDXPAD.H
 *
 * (C) COPYRIGHT International Business Machines Corp. 2014
 * All Rights Reserved
 * Licensed Materials - Property of IBM
 *
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 *
 * Function = Include File defining:
 *              Information for Scratchpad support class and it's utilities
 *              for User-defined functions.
 *
 * Operating System = Common C++ Include File
 *
 *****************************************************************************/

#ifndef UDXPAD_INCLUDED
#define UDXPAD_INCLUDED

namespace nz {
namespace udx_ver2 {

class CPad
{
public:
  virtual ~CPad()
    {}

  // Allocate specified amount of memory and return it.
  // array should generally be false unless called by PAD_NEW
  // in an array context
  virtual void *allocate(size_t sz, bool array=false) = 0;

  // Deallocate specified memory. Memory must have been allocated
  // by allocate() or an exception will be thrown.
  virtual void deallocate(void *ptr) = 0;

  // Set the root object for the pad, along with its size.
  // This may only be called once, otherwise an exception will
  // be thrown. The size is the size of the root object, not
  // the root object and all its children
  virtual void setRootObject(void *ptr, size_t size) = 0;

  // Get the root object from the pad, or NULL if it is
  // not already set. The size must match the size that was
  // provided in setRootObject as a run-time check that
  // the root object is the expected one.
  virtual void *getRootObject(size_t size) = 0;

  // The following are routines needed for the implementation, but
  // not generally of use to the end user.

  // Get the size associated with the provided object. The
  // object must have been created via allocate().
  virtual size_t getObjectSize(void *ptr) = 0;

  // Get the boolean array flag associated with the provided
  // object. The object must have been allocated via allocate();
  virtual bool isObjectArray(void * ptr) = 0;

  // Get the total size in bytes of all objectes allocated by
  // the pad.
  virtual int32 getTotalSize() = 0;
  
};


 template <class T>
   class PadAllocator
   {
   public:
     PadAllocator(CPad *pPad)
     {
        m_pPad = pPad;
     }
     operator T*()
     {
        int size = sizeof(T);
        void *ptr = m_pPad->allocate(size);
        if (!ptr)
        {
	   return NULL;
        }
	return new (ptr) T;	
     }
     T* operator[] (int n) const
     {
        int size = sizeof(T);
	void *ptr = m_pPad->allocate(size*n, true);
	if (!ptr)
        {
	   return NULL;
        }
	T *pObj = (T*)ptr;
	for (int i=0 ; i < n; i++)
        {
	   new (&pObj[i]) T;
        }
	return pObj;
     }
     
   private:
     CPad *m_pPad;
   };

 template <class T>
   void padDeallocate(CPad *pPad, T* ptr)
   {
     if (ptr)
     {
        if (pPad != NULL )
        { 
           bool isArray = pPad->isObjectArray(ptr);
           if (isArray)
           {
              int num = pPad->getObjectSize(ptr) / sizeof(T);
            
              for (int i=0 ; i < num; i++)
              {
                 ptr[i].~T();
              }
           }
           else
           {
              ptr->~T();
           }
           pPad->deallocate(ptr);
        }
      }
   }

 extern CPad* getPad(const char* strName);
 extern bool isUserQuery();

} // end namespace udx_ver2
} // end namespace nz


#define PAD_NEW(pad, type) (type*)(nz::udx_ver2::PadAllocator<type>(pad))
#define PAD_DELETE(pad, ptr) nz::udx_ver2::padDeallocate(pad, ptr)


#endif //  #ifndef UDXPAD_INCLUDED
