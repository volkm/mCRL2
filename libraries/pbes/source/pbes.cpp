// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file pbes.cpp
/// \brief

#include "mcrl2/pbes/find.h"
#include "mcrl2/pbes/normalize_sorts.h"
#include "mcrl2/pbes/print.h"
#include "mcrl2/pbes/replace.h"
#include "mcrl2/pbes/translate_user_notation.h"

namespace mcrl2
{

namespace pbes_system
{

//--- start generated pbes_system overloads ---//
std::string pp(const pbes_system::fixpoint_symbol& x) { return pbes_system::pp< pbes_system::fixpoint_symbol >(x); }
std::string pp(const pbes_system::pbes<>& x) { return pbes_system::pp< pbes_system::pbes<> >(x); }
std::string pp(const pbes_system::pbes_equation& x) { return pbes_system::pp< pbes_system::pbes_equation >(x); }
std::string pp(const pbes_system::pbes_equation_vector& x) { return pbes_system::pp< pbes_system::pbes_equation_vector >(x); }
std::string pp(const pbes_system::pbes_expression& x) { return pbes_system::pp< pbes_system::pbes_expression >(x); }
std::string pp(const pbes_system::pbes_expression_list& x) { return pbes_system::pp< pbes_system::pbes_expression_list >(x); }
std::string pp(const pbes_system::pbes_expression_vector& x) { return pbes_system::pp< pbes_system::pbes_expression_vector >(x); }
std::string pp(const pbes_system::propositional_variable& x) { return pbes_system::pp< pbes_system::propositional_variable >(x); }
// std::string pp(const pbes_system::propositional_variable_list& x) { return pbes_system::pp< pbes_system::propositional_variable_list >(x); }
std::string pp(const pbes_system::propositional_variable_vector& x) { return pbes_system::pp< pbes_system::propositional_variable_vector >(x); }
std::string pp(const pbes_system::propositional_variable_instantiation& x) { return pbes_system::pp< pbes_system::propositional_variable_instantiation >(x); }
std::string pp(const pbes_system::propositional_variable_instantiation_list& x) { return pbes_system::pp< pbes_system::propositional_variable_instantiation_list >(x); }
std::string pp(const pbes_system::propositional_variable_instantiation_vector& x) { return pbes_system::pp< pbes_system::propositional_variable_instantiation_vector >(x); }
void normalize_sorts(pbes_system::pbes_equation_vector& x, const data::data_specification& dataspec) { pbes_system::normalize_sorts< pbes_system::pbes_equation_vector >(x, dataspec); }
void normalize_sorts(pbes_system::pbes<>& x, const data::data_specification& /* dataspec */) { pbes_system::normalize_sorts< pbes_system::pbes<> >(x, x.data()); }
void translate_user_notation(pbes_system::pbes<>& x) { pbes_system::translate_user_notation< pbes_system::pbes<> >(x); }
std::set<data::sort_expression> find_sort_expressions(const pbes_system::pbes<>& x) { return pbes_system::find_sort_expressions< pbes_system::pbes<> >(x); }
std::set<data::variable> find_variables(const pbes_system::pbes<>& x) { return pbes_system::find_variables< pbes_system::pbes<> >(x); }
std::set<data::variable> find_free_variables(const pbes_system::pbes<>& x) { return pbes_system::find_free_variables< pbes_system::pbes<> >(x); }
std::set<data::variable> find_free_variables(const pbes_system::pbes_equation& x) { return pbes_system::find_free_variables< pbes_system::pbes_equation >(x); }
std::set<data::function_symbol> find_function_symbols(const pbes_system::pbes<>& x) { return pbes_system::find_function_symbols< pbes_system::pbes<> >(x); }
std::set<pbes_system::propositional_variable_instantiation> find_propositional_variable_instantiations(const pbes_system::pbes_expression& x) { return pbes_system::find_propositional_variable_instantiations< pbes_system::pbes_expression >(x); }
std::set<core::identifier_string> find_identifiers(const pbes_system::pbes_expression& x) { return pbes_system::find_identifiers< pbes_system::pbes_expression >(x); }
bool search_variable(const pbes_system::pbes_expression& x, const data::variable& v) { return pbes_system::search_variable< pbes_system::pbes_expression >(x, v); }
//--- end generated pbes_system overloads ---//

// TODO: These should be removed when the ATerm code has been replaced.
std::string pp(const atermpp::aterm& x) { return x.to_string(); }
std::string pp(const atermpp::aterm_appl& x) { return x.to_string(); }
std::string pp(const core::identifier_string& x) { return core::pp(x); }

} // namespace pbes_system

} // namespace mcrl2

