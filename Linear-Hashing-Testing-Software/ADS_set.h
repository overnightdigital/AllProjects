#ifndef ADS_SET_H
#define ADS_SET_H

#include <functional>
#include <algorithm>
#include <iostream>
#include <stdexcept>
//#include <fstream> //

template <typename Key, size_t N=2>
class ADS_set {
public:
  class Iterator;
  using value_type = Key;
  using key_type = Key;
  using reference = key_type&;
  using const_reference = const key_type&;
  using size_type = size_t;
  using difference_type = std::ptrdiff_t;
  using iterator = Iterator;
  using const_iterator = Iterator;
  using key_compare = std::less<key_type>;  
  using key_equal = std::equal_to<key_type>; 
  using hasher = std::hash<key_type>;  

private:
//Vector

template<typename T>
class Vector{
    private:
        size_t sz;
        T* values;
        size_t space ;

    public:

using value_type = T ;
using size_type = size_t;
using difference_type = std::ptrdiff_t;
using reference = T&;
using const_reference = const T&;
using pointer = T* ;
using const_pointer = const T*;
        
        
        Vector() :sz(0),values(0),space(0){}
        Vector(std::initializer_list<T> il) {
        sz=0;
        space=0;
        values=new T[0];
            for(T elem : il)
                this->push_back(elem);
          }   
        Vector(int s) :sz(s),values(new T[s])
            {
        for(int i=0;i<sz;++i)
            values[i]=0;
          }  
        Vector(const Vector& arg) :sz(arg.sz),values(new T[arg.sz]),space(arg.sz)
              {
        for(int i=0;i<arg.sz;++i) values[i]=arg.values[i];
          }
        Vector(Vector&& src) {
        values=src.values;
        sz=src.sz;
        space=src.space;
        src.values=nullptr;
        src.sz=src.space=0;
            }   
        Vector& operator=(const Vector& arg) {
        if(this==&arg) return *this;

        if(arg.sz<=space){
            for(int i=0;i<arg.sz;++i) values[i]=arg.values[i];
            sz=arg.sz;
            return *this;
            }

            T* p=new T[arg.sz];
       for(int i=0;i<arg.sz;++i) p[i]=arg.values[i];
       delete[] values;
       space=sz=arg.sz;
       return *this;

        }  
        Vector& operator=(Vector&& rhs) {
        delete[] values;
        values=rhs.values;
        sz=rhs.sz;
        space=rhs.space;
        rhs.values=nullptr;
        rhs.sz=rhs.space=0;
          return *this;
          }
        ~Vector() {delete[] values;}

        int capacity() const {return space;}
        size_type size() const {return sz;}

        T& operator[](int n) {
   // if(n<0 || sz<=n) throw std::runtime_error("Range error") ;
    return values[n];
           }
        const T& operator[](int n) const {
   // if(n<0 || sz<=n) throw std::runtime_error("Range error") ;
    return values[n];
        }   

        T& at(int n) {
          //  if(n<0 || sz<=n) throw runtime_error("Range error") ;
           return values[n];
    }   

        void reserve(size_type newalloc) {
            if(newalloc<=space) return;
            if(newalloc<sz) throw std::runtime_error("Buffer to small");
            T* new_buf = new value_type[newalloc];
            for(size_type i{0};i<sz;++i)
                new_buf[i]=values[i];
                delete[] values;
                space = newalloc;
                values = new_buf;
          }
        void resize(int newsize) {
        reserve(newsize);
        for(int i=0;i<newsize;++i) values[i]=0;
        sz=newsize; 
          }
        void shrink_to_fit() {
        T* transfer {values};
        space=sz;
        values=new T[space];
        size_t i{0};
            while(i<sz){
                *(values+i)=*(transfer+i);
                ++i;
            }
            delete[] transfer;
            
          }   
        void push_back(const T& d) {
        if(space==0) reserve(space*2+1);
        else if(sz==space) reserve(space*2+1);
        values[sz]=d;
        ++sz;
        }

        bool empty() {
        return !sz;
          } 
        void clear() {
        delete[] values;
        values=0;
        sz=0;
        space=0;
          }   
        void pop_back() {
        if(sz<=0) throw std::runtime_error("Size is 0");
            
           else {
                --sz;
                this->shrink_to_fit();
            }   
    }  

    class Iterator{

T *ptr;
const Vector<T>* og;
        
        public:

using value_type = Vector::value_type;
using difference_type = Vector::difference_type;
using reference = Vector::reference;
using pointer = Vector::pointer;
using iterator_category = std::forward_iterator_tag;
         
            Iterator(T* p,const Vector<T>* o)
                :ptr(p),og(o){}
                
                Iterator& operator++() {++ptr;return *this;} 
                Iterator& operator--() {--ptr;return *this;}
                Iterator& operator+(int op){;ptr+=op;return *this;}
                Iterator operator++(int) { Iterator old{*this};++*this;return old;}
                friend difference_type operator-(const Iterator& lop,const Iterator& rop) { return lop.ptr-rop.ptr; }
                
                bool operator!=(const Iterator& rop) { return ptr!=rop.ptr; }
                bool operator==(const Iterator& rop) { return ptr==rop.ptr; }
                T& operator*() { return *ptr; }
                T* operator->() { return ptr; }
              
        };

using iterator = Iterator;

Iterator begin() {return Iterator{values,this};}
Iterator end() {return Iterator{values+sz,this};}

};

//Internal Details
  static const size_type bucket_nr = 3;
  const float threshold = 0.55;  
  //const size_type aspect_st = 2;


  enum class Mode_bucket { free, inuse };
  enum class Mode_element { standard, overflow };

struct bucket{
  key_type cell;
  Mode_bucket mode {Mode_bucket::free};
};

struct element{
  bucket line[bucket_nr];
  element * next = nullptr;
  element * prev = nullptr;
  element * head = nullptr;
  bool is_head = false;
  bool over = false;
  Mode_element mode {Mode_element::standard};
};

  Vector<element*> table ;
  size_type max_sz{0}; //N*2+1
  size_type sz{0}; //N-1
  float utilization{0};
  size_type global_depth{0}; //aspect_st
  size_type NextToSplit{0};
  size_type vertical{0}; // For iterator;

  void configure(size_type n) {
    if(n>8) n = 8; //This should not be bigger
    if(n%2 != 0) ++n; //should make exception when N = 0;
    sz = range_calc(n)-1;
    max_sz = sz*2+1;
    utilization = 0; //Because it is being called from different functions#
    NextToSplit = 0;
    vertical = 0;
    global_depth = n;
  }

  size_type range_calc(const size_type n) const {  return 1<<n;  }
  size_type hash_idx(const key_type& k, const size_type range) const { return hasher{}(k)%(range_calc(range)); }
  size_type size_sz() { return sz; }
  size_type bucket_number() { return bucket_nr; }

  void construct(size_type n){
    for(size_type i{0}; i<n; ++i){ 
      table.push_back(new element) ;
      table[table.size()-1]->head = table[table.size()-1];
      table[table.size()-1]->head->mode = Mode_element::standard;
    }
  }

 element * find_help(const key_type& key,const int& n) const {
   size_type idx{hash_idx(key,global_depth+n)};
   if(idx>sz ) return 0;
   element * help{table[idx]->head};
   while(help!=0){
      for(size_type i{0}; i<bucket_nr; ++i)
        if(help->line[i].mode == Mode_bucket::inuse && key_equal{}(help->line[i].cell,key)) return help;
      help = help->next;
      }
    return 0;  
 } 

 element * find_pos(const key_type& key) const {
   int i{1};
   int i2 = (int)global_depth;
   i2*=-1;
    while(i>=i2){
     if(element * b = find_help(key,i)) return b;
      --i;
    } 
   return 0;  
  }

  bool erase_help(const key_type& key,const int& n) {
    bool seed{false};
   size_type idx{hash_idx(key,global_depth+n)};
   if(idx>sz ) return 0;
   element * help{table[idx]->head};
   while(help!=0){
      for(size_type i{0}; i<bucket_nr; ++i)
        if(help->line[i].mode == Mode_bucket::inuse && key_equal{}(help->line[i].cell,key)) { help->line[i].mode = Mode_bucket::free; --utilization; seed = true; break; }
      help = help->next;
      }  
      shift(table[idx]->head);
      help = table[idx]->head;
      while(help!=0) {     // Uberprufen ob welche leer sind und loschen // Anderung nur letztes loschen // delte_bucket uberflusig
      if(is_bucket_empty(help->line) && help->mode == Mode_element::overflow) { help->prev->next = 0; delete help; break; }
       help = help->next;
          }
    return seed;  
  }

  void erase_pos(const key_type& key) {
    int i{1};
    int i2 = (int)global_depth;
    i2*=-1;
      while(i>=i2){
      if(find_help(key,i)) { erase_help(key,i); return; }
      --i;
    }  
  }

  bool insert_unchecked(const key_type& k) {
	
    if(sz>=table.size()-2) construct(10); //sz-1>=-1
    ++utilization;
//*********************    
 /*  if(((utilization)/((sz+1)*bucket_nr))>=threshold) {
      table[++sz] = new element;
      table[sz]->head = table[sz];
      help = table[NextToSplit++];
      while(help!=0){
      split(help->line);
      if(is_bucket_empty(help->line) && help->mode == Mode_element::overflow) delete_bucket(help);
      help = help->next;
      } 
    } // Fur Effizienz
    help = table[idx]->head;*/
//****************    
    if(NextToSplit == range_calc(global_depth)){
    NextToSplit = 0;
    ++global_depth;
    }
    
    size_type idx{hash_idx(k,global_depth)};
    if(idx < NextToSplit)  idx = hash_idx(k,global_depth+1);
    element * help{table[idx]->head};

    if(help!=NULL && help->mode == Mode_element::standard) {
    for(size_type i{0}; i<bucket_nr; ++i)
    if(help->line[i].mode == Mode_bucket::free){
      help->line[i].mode = Mode_bucket::inuse;
      help->line[i].cell =k;
      help = nullptr;
      return true;
      }
    }

    if(help!=NULL && help->next!=0 && help->next->mode == Mode_element::overflow && !is_bucket_empty(help->next->line)){
      help = help->next;
     while(help!=0){
     for(size_type i{0}; i<bucket_nr; ++i)
     if(help->line[i].mode == Mode_bucket::free){
      help->line[i].mode = Mode_bucket::inuse;
      help->line[i].cell =k;
      help->over = true;
      help = nullptr;
      return true;
        }
        help = help->next;
      }
    }

    help = table[idx]->head;
    while(help->next!=0)  help = help->next; 
    help->next = new element;
    element * curr = help;
    help = help->next;
    help->prev = curr;
    curr = nullptr;
    help->mode = Mode_element::overflow;
    for(size_type i{0}; i<bucket_nr; ++i)
      if(help->line[i].mode == Mode_bucket::free){
      help->line[i].cell = k;
      help->line[i].mode = Mode_bucket::inuse;
      break;
      }

    /*table[++sz] = new element;  
    table[sz]->head = table[sz];*/ // unnotig seit construct
    ++sz;
    help = table[NextToSplit]->head;
    while(help!=0){       // Buckets einzeln aufspliten
      split(help->line);
      help = help->next;
    }
    shift(table[NextToSplit]->head);
    help = table[NextToSplit]->head;
    while(help!=0) {     // Uberprufen ob welche leer sind und loschen // Anderung nur letztes loschen // delte_bucket uberflusig
      if(is_bucket_empty(help->line) && help->mode == Mode_element::overflow) { help->prev->next = 0; delete help; break; }
       help = help->next;
       }
    ++NextToSplit;
    return 1;    
  }

  void split(bucket * split) {
  for(size_type i{0}; i<bucket_nr; ++i)
    if(split[i].mode == Mode_bucket::inuse){
    size_type idx_first{hash_idx(split[i].cell, global_depth+1)};
      if(idx_first == sz)
        for(size_type i2{0}; i2<bucket_nr; ++i2)
        if(table[sz]->line[i2].mode == Mode_bucket::free){
        table[sz]->line[i2].mode = Mode_bucket::inuse;
        table[sz]->line[i2].cell = split[i].cell;
        split[i].mode = Mode_bucket::free;
        break;
        } 
    }
 }  

  void shift(element * bucket){
      element* help;
      help = bucket->head;
      size_type el_num{0};
      size_type count{0};
      size_type size{0};
      while(help!=0){
        size++;
        help = help->next;
      }
      const size_type array = bucket_nr*size+1; 
      key_type * arr = new key_type[array];
      help = bucket->head;
      size_type a{0};
      while(help!=0){
        for(size_type i{0}; i<bucket_nr; ++i) 
        if(help->line[i].mode == Mode_bucket::inuse){
        arr[a++] = help->line[i].cell;
        help->line[i].mode = Mode_bucket::free;
        ++el_num;
        }
        help = help->next;
      }
      a = 0;
      help = bucket;
      while(help!=0){
        for(size_type i{0}; i<bucket_nr; ++i)
	        if(count < el_num){
          help->line[i].cell = arr[a++];
          help->line[i].mode = Mode_bucket::inuse;
	        ++count;
        }
          help = help->next;
      }  
      delete[] arr;
  }

  void delete_bucket(element * bucket){
      element * help;
      element * curr;
      help = bucket;
      if(help->next == 0) { delete help; return; }
      curr = help->prev;
      curr->next = help->next;
      delete help;

  }

  bool is_bucket_empty(bucket * bucket){
    for(size_type i{0}; i<bucket_nr; ++i)
      if(bucket[i].mode == Mode_bucket::inuse) return false;
        return true;
  }

    //------Iter-------//

 std::pair<size_type, size_type> find_help_iter(const key_type& key,const int& n) const {
   size_type idx{hash_idx(key,global_depth+n)};
   if(idx>sz ) return std::make_pair(0,0);
   element * help{table[idx]->head};
   while(help!=0){
      for(size_type i{0}; i<bucket_nr; ++i)
        if(help->line[i].mode == Mode_bucket::inuse && key_equal{}(help->line[i].cell,key)) return std::make_pair(idx,i);
      help = help->next;
      }
    return std::make_pair(0,99);  
 } 

 std::pair<size_type, size_type> find_pos_iter(const key_type& key) const {
   int i{1};
   int i2 = (int)global_depth;
   i2*=-1;
    while(i>=i2){
      std::pair<size_type, size_type> b = find_help_iter(key,i);
     if(b.first!=0 && b.second != 99) return b;
      --i;
    } 
   return std::make_pair(0,0);   
  }

  //------Iter------//

//Internal Details
public:      
  
  ADS_set() { configure(N); construct(max_sz); } 
  ADS_set(std::initializer_list<key_type> ilist) :ADS_set{} { insert(ilist); }
  template<typename InputIt> ADS_set(InputIt first, InputIt last) :ADS_set{} { insert(first, last); }
  ADS_set(const ADS_set& other) {
    clear(); //not neccessary but there is a construct in clear
    for (const auto& k: other) insert_unchecked(k);
  }
   ~ADS_set() { 
     for (size_type i {0}; i < table.size(); i++){ //Problem was table.size()-1 !!!
				element * prev {0};
				element * help = table[i]->head;
				while (help != 0){
					prev = help;
					help = help -> next;
					delete prev;
				}
			}
       table.clear(); // Not necessary
		}

  ADS_set& operator=(const ADS_set& other) {
    if (this == &other) return *this;
    clear(); 
    for (const auto& k: other) insert_unchecked(k);
    return *this;
  }
  ADS_set& operator=(std::initializer_list<key_type> ilist) {
    clear();
    insert(ilist);
    return *this;
  }

  size_type size() const { return utilization; }
  bool empty() const { return !utilization; }

  size_type count(const key_type& key) const {  return !!find_pos(key);  }
  iterator find(const key_type& key) const { if(element * pos{find_pos(key)}) return const_iterator{find_pos_iter(key), key, pos, this}; return end(); }

  void clear() { 
   for (size_type i {0}; i < table.size(); i++){ 
				element * prev {0};
				element * help = table[i]->head;
				while (help != 0){
					prev = help;
					help = help -> next;
					delete prev;
				}
			}
       table.clear(); // Not necessary
	
  configure(N);
  construct(max_sz);
  }

  void swap(ADS_set& other) {
    using std::swap;
    swap(sz, other.sz);
    swap(utilization, other.utilization);
    swap(global_depth, other.global_depth);
    swap(vertical, other.vertical);
    swap(NextToSplit, other.NextToSplit);
    swap(table, other.table);

  }

  void insert(std::initializer_list<key_type> ilist) { for(const key_type& k : ilist) if(!find_pos(k))  insert_unchecked(k); }
  std::pair<iterator,bool> insert(const key_type& key) { 
    if (element * pos {find_pos(key)})
      return std::make_pair(const_iterator{find_pos_iter(key), key, pos, this}, false);
      insert_unchecked(key);
      return std::make_pair(const_iterator{find_pos_iter(key), key, find_pos(key), this}, true);
    }
  template<typename InputIt> void insert(InputIt first, InputIt last) {
    for(auto it = first; it!=last; ++it) { if(!find_pos(*it)) insert_unchecked(*it); }
    }

  size_type erase(const key_type& key) { if(find_pos(key)) { erase_pos(key); return 1; } return 0; }

  const_iterator begin() const { 
    if(empty()) return const_iterator(std::make_pair(0, 0), 0, this);
      for(size_type i{0}; i < table.size(); ++i){
        element * help{table[i]->head};
        while(help!=0){
          for(size_type i2{0}; i2 < bucket_nr; ++i2){
            if(help->line[i2].mode == Mode_bucket::inuse) return const_iterator(std::make_pair(i, i2), help->line[i2].cell, help, this);
          }
          help = help->next;
        }
      } 
    return const_iterator(std::make_pair(0, 0), 0, this);
    }
  const_iterator end() const { //macht probleme beim insert liefert falsches ergebniss zuruck
    if(empty()) return const_iterator(std::make_pair(0, 0), 0, this);
     return const_iterator(std::make_pair(sz+1, 0), table[sz+1]->line[0].cell, table[sz+1]->head, this);
     }

  void dump(std::ostream& o = std::cerr) const {
    if(empty()) { o << "[size = 0]" << std::endl; return; }
    for(size_type i{0}; i<=sz; ++i){
      size_type counter{0};
      element * help {table[i]->head};
      o << i << " : ";
      switch(help->mode){
        
        case Mode_element::standard:
          for(size_type i2{0}; i2<bucket_nr; ++i2)
            switch(help->line[i2].mode){
              case Mode_bucket::inuse:
              o << '[' << help->line[i2].cell << ']';
              break;
              case Mode_bucket::free:
              o << '[' << '-' << ']';
              break;
            }
          
        while(help->next!=0){
        counter = 0;  
        help = help->next;
        case Mode_element::overflow:
        if(counter<1) { o << "->" ;++counter; }
        for(size_type i3{0}; i3<bucket_nr; ++i3)
          switch(help->line[i3].mode){
              case Mode_bucket::inuse:
              o << '[' << help->line[i3].cell << ']';
              break;
              case Mode_bucket::free:
              o << '[' << '-' << ']';
              break;
            }
          }
        }
       o << '\n'; 
    }
  }
/*
  void print(std::ofstream& o) const {
    for(size_type i{0}; i<=sz; ++i){
      size_type counter{0};
      element * help {table[i]->head};
      o << i << " : ";
      switch(help->mode){
        
        case Mode_element::standard:
          for(size_type i2{0}; i2<bucket_nr; ++i2)
            switch(help->line[i2].mode){
              case Mode_bucket::inuse:
              o << '[' << help->line[i2].cell << ']';
              break;
              case Mode_bucket::free:
              o << '[' << '-' << ']';
              break;
            }
          
        while(help->next!=0){
        counter = 0;  
        help = help->next;
        case Mode_element::overflow:
        if(counter<1) { o << "->" ;++counter; }
        for(size_type i3{0}; i3<bucket_nr; ++i3)
          switch(help->line[i3].mode){
              case Mode_bucket::inuse:
              o << '[' << help->line[i3].cell << ']';
              break;
              case Mode_bucket::free:
              o << '[' << '-' << ']';
              break;
            }
          }
        }
       o << '\n'; 
    }
  }
*/
  friend bool operator==(const ADS_set& lhs, const ADS_set& rhs) { 
    if (lhs.utilization != rhs.utilization) return false;
    for (const auto& k: lhs) 
      if (!rhs.find_pos(k)) return false;
    return true;
    }
  friend bool operator!=(const ADS_set& lhs, const ADS_set& rhs) { return !(lhs==rhs); }
};

template <typename Key, size_t N>
class ADS_set<Key,N>::Iterator {
  size_type pos_vertical{0};
  size_type pos_horizontal{0};
  key_type value;
  element * pos{nullptr};
  const ADS_set<Key,N> * set; 
  key_type * equal{nullptr};

public:
  using value_type = Key;
  using difference_type = std::ptrdiff_t;
  using reference = const value_type&;
  using pointer = const value_type*;
  using iterator_category = std::forward_iterator_tag;

  explicit Iterator(std::pair<size_type, size_type> xy, key_type value_tr, const element * pos, const ADS_set<Key,N> * set) :
        pos_vertical(xy.first),
        pos_horizontal(xy.second),
        value(value_tr),
        pos(const_cast<element*>(pos)), 
        set{set}  { equal = const_cast<key_type*>(&pos->line[pos_horizontal].cell); }

  explicit Iterator(std::pair<size_type, size_type> xy, const element * pos, const ADS_set<Key,N> * set) :
        pos_vertical(xy.first),
        pos_horizontal(xy.second),
        pos(const_cast<element*>(pos)), 
        set{set} { equal = const_cast<key_type*>(&pos->line[pos_horizontal].cell); } // need just to give hor_pos now
 
  bool bucket_is_free(element * b, size_type p) {for(size_type i{++p}; i<bucket_nr; ++i) if(b->line[i].mode == Mode_bucket::inuse) return false; return true; }
  bool is__empty(bucket * bucket, size_type begin=0){
    for(size_type i{begin}; i<bucket_nr; ++i)
      if(bucket[i].mode == Mode_bucket::inuse) return false;
        return true;
  }

  reference operator*() const { 
    if(set->empty()) throw std::runtime_error("dereference 0");
      return value;
      }

  pointer operator->() const { 
    if(set->empty()) throw std::runtime_error("dereference 0");
      return &value;
      }

  Iterator& operator++() { 
      for(;pos_vertical<=set->sz+1;){
      if(pos == 0) { pos = set->table[set->sz+1]->head; equal = &set->table[set->sz+1]->line[0].cell; return *this; }//prob was testing pos->next while pos = 0
        if(pos_vertical == set->sz+1) { pos = set->table[set->sz+1]->head; equal = &set->table[set->sz+1]->line[0].cell; return *this; }
        if(pos->next==0 && bucket_is_free(pos, pos_horizontal)) { 
          while(pos_vertical<set->sz && is__empty(set->table[pos_vertical+1]->line))  ++pos_vertical; 
           pos = set->table[++pos_vertical]->head; pos_horizontal=0; 
        }
        if(pos->next!=0 && pos_horizontal>=bucket_nr-1) { pos = pos->next; pos_horizontal=0; }
        while(pos!=0){
          for(; pos_horizontal<bucket_nr; ++pos_horizontal){
              if(pos->line[pos_horizontal].mode == Mode_bucket::inuse && !key_equal{}(*equal, pos->line[pos_horizontal].cell)){
                value = pos->line[pos_horizontal].cell; 
                equal = &pos->line[pos_horizontal].cell;
                return *this;
              }
            } 
          pos = pos->next;
        } 
    }
    return *this;
  }

  Iterator operator++(int) { 
    auto rc = *this;
    ++*this;
    return rc;
   }

  friend bool operator==(const Iterator& lhs, const Iterator& rhs) { return (lhs.equal == rhs.equal); }
  friend bool operator!=(const Iterator& lhs, const Iterator& rhs) { return (lhs.equal != rhs.equal); }

};

template <typename Key, size_t N> void swap(ADS_set<Key,N>& lhs, ADS_set<Key,N>& rhs) { lhs.swap(rhs); }

#endif // ADS_SET_H
