#include <iostream>
#include <list>
#include <typeinfo>
#include <cstdlib>
#include "gc_details.h"
#include "gc_iterator.h"

/*
    Pointer implements custom pointer type that uses
    garbage collection to release unused memory.
*/
template <class T, int size = 0>
class Pointer{
private:
    //garbage collection list.
    static std::list<PtrDetails<T> > refContainer;
    // addr points to the allocated memory to which
    // this Pointer pointer currently points.
    T *addr;

  	//Checks if Pointer is pointing to an array
    bool isArray;

    //In case of array, this is its size
    unsigned arraySize;

    static bool first; // true when first Pointer is created
    // Return an iterator to pointer details in refContainer.
    typename std::list<PtrDetails<T> >::iterator findPtrInfo(T *ptr);
public:
    //Iterator type for Pointer<T>.
    typedef Iter<T> GCiterator;
    //Empty constructor
    Pointer(){
        Pointer(NULL);
    }
    Pointer(T*);
    // Copy constructor.
    Pointer(const Pointer &);
    // Destructor for Pointer.
    ~Pointer();
    // Collect garbage. Returns true if at least
    // one object was freed.
    static bool collect();
    // Overload assignment of pointer to Pointer.
    T *operator=(T *t);
    // Overload assignment of Pointer to Pointer.
    Pointer &operator=(Pointer &rv);
    // Return a reference to the object pointed
    // to by this Pointer.
    T &operator*(){
        return *addr;
    }
    // Return the address being pointed to.
    T *operator->() { return addr; }
    // Return a reference to the object at the
    // index specified by i.
    T &operator[](int i){ return addr[i];}
    // Conversion function to T *.
    operator T *() { return addr; }
    // Return an Iter to the start of the allocated memory.
    Iter<T> begin(){
        int _size;
        if (isArray)
            _size = arraySize;
        else
            _size = 1;
        return Iter<T>(addr, addr, addr + _size);
    }
    // Return an Iter to one past the end of an allocated array.
    Iter<T> end(){
        int _size;
        if (isArray)
            _size = arraySize;
        else
            _size = 1;
        return Iter<T>(addr + _size, addr, addr + _size);
    }
    // Return the size of refContainer for this type of Pointer.
    static int refContainerSize() { return refContainer.size(); }
    // A utility function that displays refContainer.
    static void showlist();
    // Clear refContainer when program exits.
    static void shutdown();
};

// STATIC INITIALIZATION
// Creates storage for the static variables
template <class T, int size>
std::list<PtrDetails<T> > Pointer<T, size>::refContainer;
template <class T, int size>
bool Pointer<T, size>::first = true;

// Constructor for both initialized and uninitialized objects. -> see class interface
template<class T,int size>
Pointer<T,size>::Pointer(T *t){
    //Shutdown() as an exit function.
    if (first){
       atexit(shutdown);
    }
    first = false;

    //Iterator needed for reference update
    typename std::list <PtrDetails<T> >::iterator itr;
    //If it doesn't locate memory, returns end
    itr = findPtrInfo(t);

    //Add to list if not found
    if(itr != refContainer.end()){
    	itr->refcount += 1;
    }else{
      PtrDetails<T> g_collect(t, size);
      //Add to list
      refContainer.push_front(g_collect);
    }

    arraySize = size;

    addr = t;

    if(size>0){
    	isArray = true;
    }else{
     isArray = false;
    }

}
// Copy constructor.
template< class T, int size>
Pointer<T,size>::Pointer(const Pointer &ob){

    typename std::list<PtrDetails<T> >::iterator itr;

    itr = findPtrInfo(ob.addr);
    itr->refcount+=1;
    addr = ob.addr;
    arraySize = ob.arraySize;


    //Check for array case
    if(arraySize>0){
    	isArray=true;
    }
    else{
    	isArray = false;
    }

}

// Destructor for Pointer.
template <class T, int size>
Pointer<T, size>::~Pointer(){


    typename std::list<PtrDetails<T> >::iterator itr;
    //Locate memory object
    itr = findPtrInfo(addr);
    if(itr->refcount){
    	//Decrement
    	itr->refcount -= 1;
    }

    //Might alter heuristic for removal
    //Something less frequent
}


template <class T, int size>
bool Pointer<T, size>::collect(){
	//Garbage collection
    //Returns true if it frees space
    //Destructor calls it
	bool freeMem{false};
    typename std::list<PtrDetails<T> >::iterator itr;
    do{
        // Scan refContainer looking for unreferenced pointers.
        for (itr = refContainer.begin(); itr != refContainer.end(); itr++){

            if(itr->refcount > 0){
            	//Pass if occupied
            	continue;
            }
            freeMem = true;

            refContainer.remove(*itr);

            if(itr->memPtr != NULL){
            	if(itr->isArray == true){
                	//Delete array
                    delete [] itr->memPtr;
                }else{
                	//Single element removal
                    delete itr->memPtr;
                }
            }
            break;
        }
    } while (itr != refContainer.end());
    return freeMem;
}

// Overload assignment of pointer to Pointer.
template <class T, int size>
T *Pointer<T, size>::operator=(T *t){

    typename std::list<PtrDetails<T> >::iterator itr;
    //Alter reference count
    itr = findPtrInfo(addr);
    itr->refcount -= 1;

    itr = findPtrInfo(t);
    if(itr != refContainer.end()){
    	itr->refcount += 1;
    }else{
    	//Create new entry
        PtrDetails<T> g_collect(t,size);
        refContainer.push_front(g_collect);
    }
    //Storing address
    addr = t;
    return t;
}

// Overload assignment of Pointer to Pointer.
template <class T, int size>
Pointer<T, size> &Pointer<T, size>::operator=(Pointer &rv){

    typename std::list<PtrDetails<T> >::iterator itr;
    itr = findPtrInfo(addr);
    //Decrement reference
    itr->refcount -=1;

    //Increment other one
    itr = findPtrInfo(rv.addr);
    itr->refcount +=1;
    addr=rv.addr;

    return rv;

}

// A utility function that displays refContainer.
template <class T, int size>
void Pointer<T, size>::showlist(){
    typename std::list<PtrDetails<T> >::iterator p;
    std::cout << "refContainer<" << typeid(T).name() << ", " << size << ">:\n";
    std::cout << "memPtr refcount value\n ";
    if (refContainer.begin() == refContainer.end())
    {
        std::cout << " Container is empty!\n\n ";
    }
    for (p = refContainer.begin(); p != refContainer.end(); p++)
    {
        std::cout << "[" << (void *)p->memPtr << "]"
             << " " << p->refcount << " ";
        if (p->memPtr)
            std::cout << " " << *p->memPtr;
        else
            std::cout << "---";
        std::cout << std::endl;
    }
    std::cout << std::endl;
}
// Find a pointer in refContainer.
template <class T, int size>
typename std::list<PtrDetails<T> >::iterator
Pointer<T, size>::findPtrInfo(T *ptr){
    typename std::list<PtrDetails<T> >::iterator p;
    // Find ptr in refContainer.
    for (p = refContainer.begin(); p != refContainer.end(); p++)
        if (p->memPtr == ptr)
            return p;
    return p;
}
// Clear refContainer when program exits.
template <class T, int size>
void Pointer<T, size>::shutdown(){
    if (refContainerSize() == 0)
        return; // list is empty
    typename std::list<PtrDetails<T> >::iterator p;
    for (p = refContainer.begin(); p != refContainer.end(); p++)
    {
        // Set all reference counts to zero
        p->refcount = 0;
    }
    collect();
}
