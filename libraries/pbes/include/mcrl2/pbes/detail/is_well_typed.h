// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/pbes/detail/is_well_typed.h
/// \brief add your file description here.

#ifndef MCRL2_PBES_DETAIL_IS_WELL_TYPED_H
#define MCRL2_PBES_DETAIL_IS_WELL_TYPED_H

#include <boost/iterator/transform_iterator.hpp>
#include "mcrl2/data/detail/equal_sorts.h"
#include "mcrl2/data/detail/sequence_algorithm.h"
#include "mcrl2/data/detail/sorted_sequence_algorithm.h"
#include "mcrl2/data/detail/data_functional.h"
#include "mcrl2/data/detail/data_utility.h"
#include "mcrl2/pbes/pbes_equation.h"
#include "mcrl2/pbes/detail/pbes_functional.h"
#include "mcrl2/pbes/detail/quantifier_visitor.h"

namespace mcrl2 {

namespace pbes_system {

namespace detail {

/// \brief Checks if the equation is well typed
/// \return True if
/// <ul>
/// <li>the binding variable parameters have unique names</li>
/// <li>the names of the quantifier variables in the equation are disjoint with the binding variable parameter names</li>
/// <li>within the scope of a quantifier variable in the formula, no other quantifier variables with the same name may occur</li>
/// </ul>
inline
bool is_well_typed(const pbes_equation& eqn)
{
  // check 1)
  if (data::detail::sequence_contains_duplicates(
        boost::make_transform_iterator(eqn.variable().parameters().begin(), data::detail::variable_name()),
        boost::make_transform_iterator(eqn.variable().parameters().end()  , data::detail::variable_name())
      )
     )
  {
    mCRL2log(log::error) << "pbes_equation::is_well_typed() failed: the names of the binding variable parameters are not unique" << std::endl;
    return false;
  }

  // check 2)
  detail::quantifier_visitor qvisitor;
  qvisitor.visit(eqn.formula());
  if (data::detail::sequences_do_overlap(
        boost::make_transform_iterator(eqn.variable().parameters().begin(), data::detail::variable_name()),
        boost::make_transform_iterator(eqn.variable().parameters().end()  , data::detail::variable_name()),
        boost::make_transform_iterator(qvisitor.variables.begin()     , data::detail::variable_name()),
        boost::make_transform_iterator(qvisitor.variables.end()       , data::detail::variable_name())
      )
     )
  {
    mCRL2log(log::error) << "pbes_equation::is_well_typed() failed: the names of the quantifier variables and the names of the binding variable parameters are not disjoint in expression " << pbes_system::pp(eqn.formula()) << std::endl;
    return false;
  }

  // check 3)
  detail::quantifier_name_clash_visitor nvisitor;
  nvisitor.visit(eqn.formula());
  if (nvisitor.result)
  {
    mCRL2log(log::error) << "pbes_equation::is_well_typed() failed: the quantifier variable " << data::pp(nvisitor.name_clash) << " occurs within the scope of a quantifier variable with the same name." << std::endl;
    return false;
  }

  return true;
}

/// \brief Checks if the propositional variable instantiation v has a conflict with the
/// sequence of propositional variable declarations [first, last).
/// \param first Start of a sequence of propositional variable declarations
/// \param last End of a sequence of propositional variable declarations
/// \return True if a conflict has been detected
/// \param v A propositional variable instantiation
template <typename Iter>
bool has_conflicting_type(Iter first, Iter last, const propositional_variable_instantiation& v, const data::data_specification& data_spec)
{
  for (Iter i = first; i != last; ++i)
  {
    if (i->name() == v.name() && !data::detail::equal_sorts(i->parameters(), v.parameters(), data_spec))
    {
      return true;
    }
  }
  return false;
}

inline
bool is_well_typed_equation(const pbes_equation& eqn,
                            const std::set<data::sort_expression>& declared_sorts,
                            const std::set<data::variable>& declared_global_variables,
                            const data::data_specification& data_spec
                           )
{
  // check 2)
  const data::variable_list& variables = eqn.variable().parameters();
  if (!data::detail::check_sorts(
        boost::make_transform_iterator(variables.begin(), data::detail::sort_of_variable()),
        boost::make_transform_iterator(variables.end()  , data::detail::sort_of_variable()),
        declared_sorts
      )
     )
  {
    mCRL2log(log::error) << "pbes::is_well_typed() failed: some of the sorts of the binding variable "
              << data::pp(eqn.variable())
              << " are not declared in the data specification "
              << data::pp(data_spec)
              << std::endl;
    return false;
  }

  // check 3)
  std::set<data::variable> quantifier_variables = detail::find_quantifier_variables(eqn.formula());
  if (!data::detail::check_sorts(
        boost::make_transform_iterator(quantifier_variables.begin(), data::detail::sort_of_variable()),
        boost::make_transform_iterator(quantifier_variables.end()  , data::detail::sort_of_variable()),
        declared_sorts
      )
     )
  {
    mCRL2log(log::error) << "pbes::is_well_typed() failed: some of the sorts of the quantifier variables "
              << data::pp(quantifier_variables)
              << " are not declared in the data specification "
              << data::pp(data_spec)
              << std::endl;
    return false;
  }

  // check 7)
  if (!data::detail::set_intersection(declared_global_variables, quantifier_variables).empty())
  {
    mCRL2log(log::error) << "pbes::is_well_typed() failed: the declared free variables and the quantifier variables have collisions" << std::endl;
    return false;
  }
  return true;
}

inline
bool is_well_typed_pbes(const std::set<data::sort_expression>& declared_sorts,
                        const std::set<data::variable>& declared_global_variables,
                        const std::set<data::variable>& occurring_global_variables,
                        const std::set<propositional_variable>& declared_variables,
                        const std::set<propositional_variable_instantiation>& occ,
                        const propositional_variable_instantiation& init,
                        const data::data_specification& data_spec
                       )
{
  // check 1)
  if (!data::detail::check_sorts(
        boost::make_transform_iterator(declared_global_variables.begin(), data::detail::sort_of_variable()),
        boost::make_transform_iterator(declared_global_variables.end()  , data::detail::sort_of_variable()),
        declared_sorts
      )
     )
  {
    mCRL2log(log::error) << "pbes::is_well_typed() failed: some of the sorts of the free variables "
              << data::pp(declared_global_variables)
              << " are not declared in the data specification "
              << data::pp(data_spec)
              << std::endl;
    return false;
  }

  // check 4)
  if (data::detail::sequence_contains_duplicates(
        boost::make_transform_iterator(declared_variables.begin(), detail::propositional_variable_name()),
        boost::make_transform_iterator(declared_variables.end()  , detail::propositional_variable_name())
      )
     )
  {
    mCRL2log(log::error) << "pbes::is_well_typed() failed: the names of the binding variables are not unique" << std::endl;
    return false;
  }

  // check 5)
  if (!std::includes(declared_global_variables.begin(),
                     declared_global_variables.end(),
                     occurring_global_variables.begin(),
                     occurring_global_variables.end()
                    )
     )
  {
    mCRL2log(log::error) << "pbes::is_well_typed() failed: not all of the free variables are declared\n"
              << "free variables: " << data::pp(occurring_global_variables) << "\n"
              << "declared free variables: " << data::pp(declared_global_variables)
              << std::endl;
    return false;
  }

  // check 6)
  if (data::detail::sequence_contains_duplicates(
        boost::make_transform_iterator(occurring_global_variables.begin(), data::detail::variable_name()),
        boost::make_transform_iterator(occurring_global_variables.end()  , data::detail::variable_name())
      )
     )
  {
    mCRL2log(log::error) << "pbes::is_well_typed() failed: the free variables have no unique names" << std::endl;
    return false;
  }

  // check 8)
  for (std::set<propositional_variable_instantiation>::iterator i = occ.begin(); i != occ.end(); ++i)
  {
    if (has_conflicting_type(declared_variables.begin(), declared_variables.end(), *i, data_spec))
    {
      mCRL2log(log::error) << "pbes::is_well_typed() failed: the occurring variable " << pbes_system::pp(*i) << " conflicts with its declaration!" << std::endl;
      return false;
    }
  }

  // check 9)
  if (has_conflicting_type(declared_variables.begin(), declared_variables.end(), init, data_spec))
  {
    mCRL2log(log::error) << "pbes::is_well_typed() failed: the initial state " << pbes_system::pp(init) << " conflicts with its declaration!" << std::endl;
    return false;
  }
  return true;
}

} // namespace detail

} // namespace pbes_system

} // namespace mcrl2

#endif // MCRL2_PBES_DETAIL_IS_WELL_TYPED_H
