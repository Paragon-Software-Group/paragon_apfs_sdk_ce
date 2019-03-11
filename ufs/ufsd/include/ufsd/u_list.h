// <copyright file="u_list.h" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>
////////////////////////////////////////////////////////////////
//
// This file contains declarations and definitions of intrusive list
// Simple double/single lists
//
////////////////////////////////////////////////////////////////

namespace UFSD{

//
// Simple doubly linked list implementation
//

struct list_head
{
  list_head *next, *prev;

  void init() { next = prev = this; }
  void add( list_head* _prev, list_head* _next ){
    _next->prev = this;
    next = _next;
    prev = _prev;
    _prev->next = this;
  }
  // Insert element after 'lh'
  // To insert in head use 'insert_after( head )'
  void insert_after( list_head* lh ){ add( lh, lh->next ); }
  // Insert element before 'lh'
  // To insert in tail use 'insert_before( head )'
  void insert_before( list_head* lh ){ add( lh->prev, lh ); }
  void remove(){ next->prev = prev; prev->next = next; }
  bool is_empty() const{ return next == this; }
  size_t count() const {
    size_t count = 0;
    for ( list_head* lh = next; lh != this; lh = lh->next )
      count += 1;
    return count;
  }

  void append( IN list_head* lh )
  {
  //  assert( !"Check this" );
    list_head* end  = prev;
    prev->next      = lh;
    prev            = lh->prev;
    lh->prev->next  = this;
    lh->prev        = end;
  }
};


///////////////////////////////////////////////////////////
// list_entry
//
// Get the struct for this entry
///////////////////////////////////////////////////////////
#define list_entry(ptr, type, member) \
  ((type *)((char*)(ptr) - ((char*)&(((type *)(sizeof(type)))->member) - sizeof(type))))


///////////////////////////////////////////////////////////
// list_for_each
//
// Iterate over a list
///////////////////////////////////////////////////////////
#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

///////////////////////////////////////////////////////////
// list_for_each_back
//
// Iterate over a list
///////////////////////////////////////////////////////////
#define list_for_each_back(pos, head) \
    for (pos = (head)->prev; pos != (head); pos = pos->prev)

///////////////////////////////////////////////////////////
// list_for_each_safe
//
// Iterate over a list safe against removal of list entry
///////////////////////////////////////////////////////////
#define list_for_each_safe(pos, n, head) \
  for ( pos = (head)->next, n = pos->next; pos != (head); \
        pos = n, n = pos->next)

///////////////////////////////////////////////////////////
// list_for_each_back_safe
//
// Iterate over a list safe against removal of list entry
///////////////////////////////////////////////////////////
#define list_for_each_back_safe(pos, n, head) \
  for ( pos = (head)->prev, n = pos->prev; pos != (head); \
        pos = n, n = pos->prev)

//
// Double linked lists with a single pointer list head.
//
struct hlist_node;
struct hlist_head
{
  struct hlist_node*  first;

  void init() { first = NULL; }
  bool is_empty() const { return NULL == first; }

};

struct hlist_node
{
  hlist_node *next, **pprev;

  void init()
  {
    next  = NULL;
    pprev = NULL;
  }

  void remove()
  {
    hlist_node** pp = pprev;
    if ( NULL != pp )
    {
      hlist_node* n   = next;
      *pp = n;
      if ( NULL != n )
        n->pprev = pp;
    }
  }

  void add_head( hlist_head* h )
  {
    hlist_node* f = h->first;
    next = f;
    if ( NULL != f )
      f->pprev = &next;
    h->first  = this;
    pprev     = &h->first;
  }

/* Not correct function(!)
  void add_before( hlist_node* n )
  {
    pprev = n->pprev;
    next  = n;

    n->pprev = &n;
    *pprev = this;
  }*/

  void add_after( hlist_node* n)
  {
    n->next = next;
    next = n;
    n->pprev = &next;

    if( NULL != n->next )
      n->next->pprev  = &n->next;
  }
};


#define hlist_for_each(pos, head) \
  for ( pos = (head)->first; NULL != pos; pos = pos->next )

#define hlist_for_each_safe(pos, n, head) \
  for ( pos = (head)->first; pos && ( n = pos->next, true ); pos = n )



//
// Simple single linked list implementation
//
struct slist_entry
{
  slist_entry* next;
};

struct slist_head
{
  slist_entry head;

  void init() { head.next = NULL; }

  void push( slist_entry* entry ){
    entry->next = head.next;
    head.next   = entry;
  }

  slist_entry* pop(){
    slist_entry* r = head.next;
    if ( NULL != r )
      head.next = r->next;
    return r;
  }

  bool is_empty() const{ return NULL == head.next; }
};


#define slist_for_each(pos, shead) \
  for ( pos = (shead)->head.next; NULL != pos; pos = pos->next )

#define slist_for_each_safe(pos, n, shead) \
  for ( pos = (shead)->head.next; NULL != pos && ( n = pos->next, true ); pos = n )


} // namespace UFSD{
