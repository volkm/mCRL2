/*{{{  includes */

#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <stdexcept>

#include <set>
#include <vector>
#include <string.h>
#include <sstream>


#include "mcrl2/utilities/logger.h"
#include "mcrl2/utilities/detail/memory_utility.h"
#include "mcrl2/atermpp/detail/memory.h"
#include "mcrl2/atermpp/detail/util.h"
#include "mcrl2/atermpp/aterm.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

namespace atermpp
{

/*{{{  globals */

/* The constants below are not static to prevent some compiler warnings */
const size_t MIN_TERM_SIZE = TERM_SIZE_APPL(0);
const size_t INITIAL_MAX_TERM_SIZE = 256;


/* To change the block size, modify BLOCK_SHIFT only! */
static const size_t BLOCK_SHIFT = 13;

static const size_t BLOCK_SIZE = 1<<BLOCK_SHIFT;

typedef struct Block
{
  /* We need platform alignment for this data block! */
  size_t data[BLOCK_SIZE];

  size_t size;
#ifndef NDEBUG
  struct Block* next_by_size;
#endif

  size_t* end;
} Block;

typedef struct TermInfo
{
  Block*       at_block;
  size_t* top_at_blocks;
  detail::_aterm*       at_freelist;

  TermInfo():at_block(NULL),top_at_blocks(NULL),at_freelist(NULL)
  {}

} TermInfo;


static const size_t INITIAL_TERM_TABLE_CLASS = 17;

static std::vector<TermInfo> terminfo(INITIAL_MAX_TERM_SIZE);

namespace detail
{
  aterm_administration adm;
  constant_function_symbols function_adm;

void initialise_administration()
{
  // Explict initialisation on first use. This first
  // use is when a function symbol is created for the first time,
  // which may be due to the initialisation of a global variable in
  // a .cpp file, or due to the initialisation of a pre-main initialisation
  // of a static variable, which some compilers do.
  detail::adm.initialise_aterm_administration();
  detail::function_adm.initialise_function_symbols();
}

static aterm static_undefined_aterm;

_aterm *aterm_administration::undefined_aterm()
{
  if (static_undefined_aterm.m_term==NULL)
  {
    initialise_administration();
    new (&static_undefined_aterm) aterm(detail::function_adm.AS_DEFAULT); // Use placement new as static_undefined_aterm
                                                                          // may not have initialised when this is called, 
                                                                          // causing a problem with reference counting.
  }

  return static_undefined_aterm.m_term;
} 

static aterm static_empty_aterm_list;

_aterm *aterm_administration::empty_aterm_list()
{
  if (static_empty_aterm_list.m_term==NULL || static_empty_aterm_list==static_undefined_aterm )
  {
    initialise_administration();
    new (&static_empty_aterm_list) aterm(detail::function_adm.AS_EMPTY_LIST); // Use placement new as static_empty_atermlist
                                                                              // may not have initialised when this is called, 
                                                                              // causing a problem with reference counting.
  }

  return static_empty_aterm_list.m_term;
} 
}  // namespace detail

static size_t total_nodes = 0;


static void remove_from_hashtable(const detail::_aterm *t)
{
  /* Remove the node from the aterm_hashtable */
  detail::_aterm *prev=NULL;
  const HashNumber hnr = hash_number(t, term_size(t)) & detail::adm.table_mask;
  detail::_aterm *cur = detail::adm.aterm_hashtable[hnr];

  do
  {
    if (!cur)
    {
      throw mcrl2::runtime_error("Internal error: cannot find term in hashtable."); // If only occurs if the internal administration is in error.
    }
    if (cur == t)
    {
      if (prev)
      {
        prev->next() = cur->next();
      }
      else
      {
        detail::adm.aterm_hashtable[hnr] = cur->next();
      }
      /* Put the node in the appropriate free list */
      total_nodes--;
      return;
    }
  }
  while (((prev=cur), (cur=cur->next())));
  assert(0);
}

#ifndef NDEBUG
  bool check_that_all_objects_are_free();
#endif

void aterm::free_term()
{
  detail::_aterm* t=this->m_term;
  assert(t->reference_count()==0);
  /* if (function().number()<4) // The default term and the empty list are not removed,
                             // as the datastructures may not exist anymore when this 
                             // happens. */
  if (detail::adm.aterm_hashtable.size()==0)
  {
    return;
  } 
  remove_from_hashtable(t);  // Remove from hash_table

  for(size_t i=0; i<function().arity(); ++i)
  {
    reinterpret_cast<detail::_aterm_appl<aterm> *>(t)->arg[i].decrease_reference_count();
  }
#ifndef NDEBUG
  const size_t function_symbol_index=function().number();
  size_t ref_count=detail::adm.at_lookup_table[function_symbol_index]->reference_count;
#endif
  const size_t size=term_size(t);

  t->function()=function_symbol(); 

  TermInfo &ti = terminfo[size];
  t->next()  = ti.at_freelist;
  ti.at_freelist = t; 

#ifndef NDEBUG
  if (function_symbol_index==detail::function_adm.AS_INT.number() && ref_count==1) // When destroying ATempty, it appears that all other terms have been removed.
  {
    assert(check_that_all_objects_are_free());
    return;
  }
#endif
}

static void resize_aterm_hashtable()
{
  detail::adm.table_class++;
  // detail::adm.table_size = ((HashNumber)1)<<detail::adm.table_class;
  detail::adm.table_mask = (((HashNumber)1)<<detail::adm.table_class)-1;
  std::vector < detail::_aterm* > new_hashtable;

  /*  Create new term table */
  try
  {
    new_hashtable.resize(((HashNumber)1)<<detail::adm.table_class,NULL);
  }
  catch (std::bad_alloc &e)
  {
    mCRL2log(mcrl2::log::warning) << "could not resize hashtable to class " << detail::adm.table_class << ". " << e.what() << std::endl;
    detail::adm.table_class--;
    // detail::adm.table_size = ((HashNumber)1)<<detail::adm.table_class;
    detail::adm.table_mask = (((HashNumber)1)<<detail::adm.table_class)-1;
    return;
  }
  
  /*  Rehash all old elements */
  for (std::vector < detail::_aterm*>::const_iterator p=detail::adm.aterm_hashtable.begin(); p !=detail::adm.aterm_hashtable.end(); p++)
  {
    detail::_aterm* aterm_walker=*p;

    while (aterm_walker)
    {
      assert(aterm_walker->reference_count()>0);
      detail::_aterm* next = aterm_walker->next();
      const HashNumber hnr = hash_number(aterm_walker, term_size(aterm_walker)) & detail::adm.table_mask;
      assert(hnr<new_hashtable.size());
      aterm_walker->next() = new_hashtable[hnr];
      new_hashtable[hnr] = aterm_walker;
      assert(aterm_walker->next()!=aterm_walker);
      aterm_walker = next;
    }
  }
  new_hashtable.swap(detail::adm.aterm_hashtable);

}

#ifndef NDEBUG
bool check_that_all_objects_are_free()
{
  bool result=true;

  for(size_t size=0; size<terminfo.size(); ++size)
  {
    TermInfo *ti=&terminfo[size];
    for(Block* b=ti->at_block; b!=NULL; b=b->next_by_size)
    {
      for(detail::_aterm* p=(detail::_aterm*)b->data; p!=NULL && ((b==ti->at_block && p<(detail::_aterm*)ti->top_at_blocks) || p<(detail::_aterm*)b->end); p=p + size)
      {
        if (p->reference_count()!=0 && p->function()!=detail::function_adm.AS_EMPTY_LIST)
        {
          fprintf(stderr,"CHECK: Non free term %p (size %lu). ",&*p,size);
          fprintf(stderr,"Reference count %ld\n",p->reference_count());
          result=false;
        }
      }
    }
  }

  for(size_t i=0; i<detail::adm.at_lookup_table.size(); ++i)
  {
    if (i!=detail::function_adm.AS_EMPTY_LIST.number() && detail::adm.at_lookup_table[i]->reference_count>0)  // ATempty is not destroyed, so is AS_EMPTY_LIST.
    {
      result=false;
      fprintf(stderr,"Symbol %s has positive reference count (nr. %ld, ref.count %ld)\n",
                detail::adm.at_lookup_table[i]->name.c_str(),detail::adm.at_lookup_table[i]->id,detail::adm.at_lookup_table[i]->reference_count);
    }

  }

  return result;
}
#endif

/*}}}  */
/*{{{  static void allocate_block(size_t size)  */

static void allocate_block(size_t size)
{
  Block* newblock = (Block*)calloc(1, sizeof(Block));
  if (newblock == NULL)
  {
    std::runtime_error("allocate_block: out of memory!");
  }

  assert(size >= MIN_TERM_SIZE && size < terminfo.size());

  TermInfo &ti = terminfo[size];

  newblock->end = (newblock->data) + (BLOCK_SIZE - (BLOCK_SIZE % size));

  newblock->size = size;
#ifndef NDEBUG
  newblock->next_by_size = ti.at_block;
#endif
  ti.at_block = newblock;
  ti.top_at_blocks = newblock->data;
  assert(ti.at_block != NULL);
  assert(ti.at_freelist == NULL);
}



detail::_aterm* detail::allocate_term(const size_t size)
{
  if (size >= terminfo.size())
  {
    terminfo.resize(size+1);
  }

  if (total_nodes>=(detail::adm.aterm_hashtable.size()>>1))
  {
    // The hashtable is not big enough to hold nr_of_nodes_for_the_next_garbage_collect. So, resizing
    // is wise (although not necessary, due to the structure of the hastable, which allows is to contain
    // an arbitrary number of element, at some performance penalty.
    resize_aterm_hashtable();
  }

  detail::_aterm *at;
  TermInfo &ti = terminfo[size];
  if (ti.at_block && ti.top_at_blocks < ti.at_block->end)
  {
    /* the first block is not full: allocate a cell */
    at = (detail::_aterm *)ti.top_at_blocks;
    ti.top_at_blocks += size;
    at->reference_count()=0;
    new (&at->function()) function_symbol;  // placement new, as the memory calloc'ed.
  }
  else if (ti.at_freelist)
  {
    /* the freelist is not empty: allocate a cell */
    at = ti.at_freelist;
    ti.at_freelist = ti.at_freelist->next();
    assert(ti.at_block != NULL);
    assert(ti.top_at_blocks == ti.at_block->end);
    assert(at->reference_count()==0);
  }
  else
  {
    /* there is no more memory of the current size allocate a block */
    allocate_block(size);
    assert(ti.at_block != NULL);
    at = (detail::_aterm *)ti.top_at_blocks;
    ti.top_at_blocks += size;
    at->reference_count()=0;
    new (&at->function()) function_symbol;  // placement new, as the memory calloc'ed.
  }

  total_nodes++;
  return at;
} 

aterm::aterm(const function_symbol &sym)
{
  detail::_aterm *cur, *prev, **hashspot;
  HashNumber hnr;


  assert(sym.arity()==0);

  hnr = FINISH(START(sym.number()));

  prev = NULL;
  hashspot = &(detail::adm.aterm_hashtable[hnr & detail::adm.table_mask]);

  cur = *hashspot;
  while (cur)
  {
    if (cur->function()==sym)
    {
      /* Promote current entry to front of hashtable */
      if (prev!=NULL)
      {
        prev->next() = cur->next();
        cur->next() = (detail::_aterm*) &**hashspot;
        *hashspot = cur;
      }

      m_term=cur;
      increase_reference_count<false>(m_term);
      return;
    }
    prev = cur;
    cur = cur->next();
  }

  cur = detail::allocate_term(TERM_SIZE_APPL(0));
  /* Delay masking until after allocate */
  hnr &= detail::adm.table_mask;
  cur->function() = sym;
  cur->next() = &*detail::adm.aterm_hashtable[hnr];
  detail::adm.aterm_hashtable[hnr] = cur;

  m_term=cur;
  increase_reference_count<false>(m_term);
}


/**
 * Create an aterm_int
 */

aterm_int::aterm_int(int val)
{
  detail::_aterm* cur;
  /* The following emulates the encoding trick that is also used in the definition
   * of aterm_int. Not using a union here leads to incorrect hashing results.
   */
  union
  {
    int value;
    size_t reserved;
  } _val;

  _val.reserved = 0;
  _val.value = val;

  // header_type header = INT_HEADER;
  HashNumber hnr = START(detail::function_adm.AS_INT.number());
  hnr = COMBINE(hnr, _val.reserved);
  hnr = FINISH(hnr);

  cur = detail::adm.aterm_hashtable[hnr & detail::adm.table_mask];
  while (cur && (cur->function()!=detail::function_adm.AS_INT || (reinterpret_cast<detail::_aterm_int*>(cur)->value != _val.value)))
  {
    cur = cur->next();
  }

  if (!cur)
  {
    cur = detail::allocate_term(TERM_SIZE_INT);
    /* Delay masking until after allocate */
    hnr &= detail::adm.table_mask;
    cur->function() = detail::function_adm.AS_INT;
    reinterpret_cast<detail::_aterm_int*>(cur)->reserved = _val.reserved;
    // reinterpret_cast<detail::_aterm_int*>(cur)->value = _val.value;

    cur->next() = detail::adm.aterm_hashtable[hnr];
    detail::adm.aterm_hashtable[hnr] = cur;
  }

  assert((hnr & detail::adm.table_mask) == (hash_number(cur, TERM_SIZE_INT) & detail::adm.table_mask));
  m_term=cur;
  increase_reference_count<false>(m_term);
}

/*{{{  defines */

static const size_t MAGIC_PRIME = 7;

char afun_id[] = "$Id$";

/*{{{  function declarations */

static HashNumber AT_hashAFun(const std::string &name, const size_t arity);

#if !(defined __USE_SVID || defined __USE_BSD || defined __USE_XOPEN_EXTENDED || defined __APPLE__ || defined _MSC_VER)
extern char* _strdup(const char* s);
#endif


static void resize_function_symbol_hashtable()
{
  ++detail::adm.afun_table_class;

  // detail::adm.afun_table_size  = AT_TABLE_SIZE(detail::adm.afun_table_class);
  detail::adm.afun_table_mask  = AT_TABLE_MASK(detail::adm.afun_table_class);

  detail::adm.function_symbol_hashtable.clear();
  detail::adm.function_symbol_hashtable.resize(AT_TABLE_SIZE(detail::adm.afun_table_class),size_t(-1));

  for (size_t i=0; i<detail::adm.at_lookup_table.size(); i++)
  {
    detail::_function_symbol* entry = detail::adm.at_lookup_table[i];
    assert(entry->reference_count>0);

    HashNumber hnr = AT_hashAFun(entry->name, entry->arity() );
    hnr &= detail::adm.afun_table_mask;
    entry->next = detail::adm.function_symbol_hashtable[hnr];
    detail::adm.function_symbol_hashtable[hnr] = i;
  }
}

/*}}}  */

/*{{{  int AT_printAFun(function_symbol sym, FILE *f) */

/**
  * Print an afun.
  */

size_t AT_printAFun(const size_t fun, FILE* f)
{
  assert(fun<detail::adm.at_lookup_table.size());
  detail::_function_symbol* entry = detail::adm.at_lookup_table[fun];
  std::string::const_iterator id = entry->name.begin();
  size_t size = 0;

  if (entry->is_quoted())
  {
    /* This function symbol needs quotes */
    fputc('"', f);
    size++;
    while (id!=entry->name.end())
    {
      /* We need to escape special characters */
      switch (*id)
      {
        case '\\':
        case '"':
          fputc('\\', f);
          fputc(*id, f);
          size += 2;
          break;
        case '\n':
          fputc('\\', f);
          fputc('n', f);
          size += 2;
          break;
        case '\t':
          fputc('\\', f);
          fputc('t', f);
          size += 2;
          break;
        case '\r':
          fputc('\\', f);
          fputc('r', f);
          size += 2;
          break;
        default:
          fputc(*id, f);
          size++;
          break;
      }
      id++;
    }
    fputc('"', f);
    size++;
  }
  else
  {
    fputs(entry->name.c_str(), f);
    size += entry->name.size();
  }
  return size;
}

/*}}}  */

std::string ATwriteAFunToString(const function_symbol &fun)
{
  std::ostringstream oss;
  assert(fun.number()<detail::adm.at_lookup_table.size());
  detail::_function_symbol* entry = detail::adm.at_lookup_table[fun.number()];
  std::string::const_iterator id = entry->name.begin();

  if (entry->is_quoted())
  {
    /* This function symbol needs quotes */
    oss << "\"";
    while (id!=entry->name.end())
    {
      /* We need to escape special characters */
      switch (*id)
      {
        case '\\':
        case '"':
          oss << "\\" << *id;
          break;
        case '\n':
          oss << "\\n";
          break;
        case '\t':
          oss << "\\t";
          break;
        case '\r':
          oss << "\\r";
          break;
        default:
          oss << *id;
          break;
      }
      ++id;
    }
    oss << "\"";
  }
  else
  {
    oss << entry->name;
  }

  return oss.str();
}


/**
 * Calculate the hash value of a symbol.
 */

static HashNumber AT_hashAFun(const std::string &name, const size_t arity)
{
  HashNumber hnr = arity*3;

  for (std::string::const_iterator i=name.begin(); i!=name.end(); i++)
  {
    hnr = 251 * hnr + *i;
  }

  return hnr*MAGIC_PRIME;
}


/*}}}  */

/*{{{  function_symbol ATmakeAFun(const char *name, int arity, ATbool quoted) */

function_symbol::function_symbol(const std::string &name, const size_t arity, const bool quoted)
{
  if (detail::adm.aterm_hashtable.size()==0)
  {
    detail::initialise_administration();
  }
  const HashNumber hnr = AT_hashAFun(name, arity) & detail::adm.afun_table_mask;
  /* Find symbol in table */
  size_t cur = detail::adm.function_symbol_hashtable[hnr];
  while (cur!=size_t(-1) && !(detail::adm.at_lookup_table[cur]->arity()==arity &&
                              detail::adm.at_lookup_table[cur]->is_quoted()==quoted &&
                              detail::adm.at_lookup_table[cur]->name==name))
  {
    cur = detail::adm.at_lookup_table[cur]->next;
  }

  if (cur == size_t(-1))
  {
    const size_t free_entry = detail::adm.first_free;
    assert(detail::adm.at_lookup_table.size()<detail::adm.function_symbol_hashtable.size());

    if (free_entry!=size_t(-1)) // There is a free place in at_lookup_table() to store an function_symbol.
    {
      assert(detail::adm.first_free<detail::adm.at_lookup_table.size());
      cur=detail::adm.first_free;
      detail::adm.first_free = (size_t)detail::adm.at_lookup_table[detail::adm.first_free]->id;
      assert(detail::adm.first_free==size_t(-1) || detail::adm.first_free<detail::adm.at_lookup_table.size());
      assert(free_entry<detail::adm.at_lookup_table.size());
      detail::adm.at_lookup_table[cur]->id = cur;
      assert(detail::adm.at_lookup_table[cur]->reference_count==0);
      detail::adm.at_lookup_table[cur]->reference_count=0;
      detail::adm.at_lookup_table[cur]->header = detail::_function_symbol::make_header(arity,quoted);
      detail::adm.at_lookup_table[cur]->count = 0;
      detail::adm.at_lookup_table[cur]->index = -1;
    }
    else
    {
      cur = detail::adm.at_lookup_table.size();
      detail::adm.at_lookup_table.push_back(new detail::_function_symbol(arity,quoted,cur,0,size_t(-1)));
    }


    detail::adm.at_lookup_table[cur]->name = name;

    detail::adm.at_lookup_table[cur]->next = detail::adm.function_symbol_hashtable[hnr];
    detail::adm.function_symbol_hashtable[hnr] = cur;
  }

  m_number=cur;
  detail::increase_reference_count<false>(m_number);

  if (detail::adm.at_lookup_table.size()>=detail::adm.function_symbol_hashtable.size())
  {
    resize_function_symbol_hashtable();
  }
}

/*}}}  */
/*{{{  void AT_freeAFun(SymEntry sym) */

/**
 * Free a symbol
 */

void detail::at_free_afun(const size_t n)
{
  if (adm.aterm_hashtable.size()==0)
  {
    // The aterm administration is destroyed. We cannot remove
    // this aterm anymore.
    return;
  }
  detail::_function_symbol* sym=detail::adm.at_lookup_table[n];

  assert(!sym->name.empty());
  assert(sym->id==n);

  /* Calculate hashnumber */
  const HashNumber hnr = AT_hashAFun(sym->name, sym->arity()) & detail::adm.afun_table_mask;

  /* Update hashtable */
  if (detail::adm.function_symbol_hashtable[hnr] == n)
  {
    detail::adm.function_symbol_hashtable[hnr] = sym->next;
  }
  else
  {
    size_t cur;
    size_t prev = detail::adm.function_symbol_hashtable[hnr];
    for (cur = detail::adm.at_lookup_table[prev]->next; cur != n; prev = cur, cur = detail::adm.at_lookup_table[cur]->next)
    {
      assert(cur != size_t(-1));
    }
    detail::adm.at_lookup_table[prev]->next = detail::adm.at_lookup_table[cur]->next;
  }

  assert(n<detail::adm.at_lookup_table.size());
  detail::adm.at_lookup_table[n]->id = detail::adm.first_free;
  detail::adm.first_free = n;
}

} // namespace atermpp

