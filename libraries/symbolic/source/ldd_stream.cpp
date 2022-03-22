// Author(s): Maurice Laveaux
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "mcrl2/symbolic/ldd_stream.h"

#include "mcrl2/utilities/exception.h"

#include <stack>

using namespace sylvan::ldds;
using namespace mcrl2::symbolic;

/// Iterates over all nodes in the given LDD. The LDDs in nodes will not be traversed.
struct node_iterator
{
public:
  node_iterator(const ldd& U, mcrl2::utilities::indexed_set<sylvan::ldds::ldd>& nodes)
   : m_nodes(nodes)
  {    
    m_stack.emplace(std::make_pair(U, false));
    this->operator++();
  }

  void operator++()
  {
    while (!m_stack.empty())
    {    
      auto& current = m_stack.top();

      if (current.second)
      {
        m_current = current.first;
        m_stack.pop();
        return; // Top of the stack is the current item.        
      }
      else
      {
        // Add children and continue.        
        if (m_nodes.find(current.first.down()) == m_nodes.end())
        {
          m_stack.push(std::make_pair(current.first.down(), false));
        }
        if (m_nodes.find(current.first.right()) == m_nodes.end())
        {
          m_stack.push(std::make_pair(current.first.right(), false));
        }
        current.second = true; // Next time, we can actually process current.
      }
    }

    m_current = false_();
  }

  ldd* operator->()
  {
    return &m_current;
  }

  const ldd* operator->() const
  {
    return &m_current;
  }

  ldd& operator*()
  {
    return m_current;
  }

  const ldd& operator*() const
  {
    return m_current;
  }

  operator bool() const
  {
    return m_current != false_();
  }

private:
  // Traverse the ldd bottom up, where the boolean indicates all children have been returned.
  std::stack<std::pair<ldd, bool>> m_stack;
  
  mcrl2::utilities::indexed_set<sylvan::ldds::ldd>& m_nodes;

  ldd m_current = false_();
};

/// \brief The magic value for a binary LDD format stream.
static constexpr std::uint16_t BLF_MAGIC = 0x8baf;
static constexpr std::uint16_t BLF_VERSION = 0x8306;

binary_ldd_ostream::binary_ldd_ostream(std::ostream& os)
  : m_stream(os)
{
  // Write the header of the binary LDD format.
  m_stream.write_bits(BLF_MAGIC, 16);
  m_stream.write_bits(BLF_VERSION, 16);

  // Add the true and false constants.
  m_nodes.insert(false_());
  m_nodes.insert(true_());
}

binary_ldd_ostream::~binary_ldd_ostream()
{

}

void binary_ldd_ostream::put(const ldd& U)
{
  // Write all children of U.
  node_iterator it(U, m_nodes);

  while (it)
  {
    const auto& [index, inserted] = m_nodes.insert(*it);
    if (inserted)
    {
      // New LDD that must be written to stream.
      m_stream.write_bits(0, 1);
      m_stream.write_integer(it->value());
      m_stream.write_bits(m_nodes.index(it->down()), ldd_index_width());
      m_stream.write_bits(m_nodes.index(it->right()), ldd_index_width());
    }

    if (*it == U)
    {        
      m_stream.write_bits(1, 1); // Is output.
      m_stream.write_bits(index, ldd_index_width());
    }

    ++it;
  }
}

unsigned int binary_ldd_ostream::ldd_index_width()
{
  return std::log2(m_nodes.size()) + 1;
}

binary_ldd_istream::binary_ldd_istream(std::istream& is)
  : m_stream(is)
{  
  // The term with function symbol index 0 indicates the end of the stream.
  m_nodes.emplace_back(false_());
  m_nodes.emplace_back(true_());

  // Read the binary aterm format header.
  if (m_stream.read_bits(16) != BLF_MAGIC)
  {
    throw mcrl2::runtime_error("Error while reading: missing the BLF_MAGIC control sequence.");
  }

  std::size_t version = m_stream.read_bits(16);
  if (version != BLF_VERSION)
  {
    throw mcrl2::runtime_error("The BLF version (" + std::to_string(version) + ") of the input file is incompatible with the version (" + std::to_string(BLF_VERSION) +
                               ") of this tool. The input file must be regenerated. ");
  }
}

ldd binary_ldd_istream::get()
{
  while (true)
  {
    bool is_output = m_stream.read_bits(1);

    if (is_output)
    {
      std::size_t index = m_stream.read_bits(ldd_index_width());
      // The output is simply an index of the LDD.
      return m_nodes[index];
    }
    else
    {
      std::size_t value = m_stream.read_integer();
      std::size_t down_index = m_stream.read_bits(ldd_index_width());
      std::size_t right_index = m_stream.read_bits(ldd_index_width());

      ldd down = m_nodes[down_index];
      ldd right = m_nodes[right_index];

      ldd result = ldd(sylvan::lddmc_makenode(value, down.get(), right.get()));
      m_nodes.emplace_back(result);
    }
  }
}

unsigned int binary_ldd_istream::ldd_index_width()
{
  return std::log2(m_nodes.size() + 1) + 1; // Assume that size is one larger to contain the current ldd.
}