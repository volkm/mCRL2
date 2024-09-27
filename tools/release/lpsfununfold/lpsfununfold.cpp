// Author(s): Jan Friso Groote
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file lpsfununfold.cpp

// C++
#include <climits>

//LPS framework
#include "mcrl2/lps/io.h"

//DATA
#include "mcrl2/data/parse.h"

//LPSPARUNFOLDLIB
// #include "mcrl2/lps/lpsfununfoldlib.h"

#include "mcrl2/utilities/input_output_tool.h"
#include "mcrl2/data/rewriter_tool.h"

using namespace mcrl2::utilities;
using namespace mcrl2::data;

using namespace mcrl2;
using namespace mcrl2::utilities::tools;
using namespace mcrl2::log;

using mcrl2::data::tools::rewriter_tool;

class lpsfununfold_tool: public  rewriter_tool<input_output_tool>
{
  protected:

    typedef rewriter_tool<input_output_tool> super;

    std::set< std::size_t > m_set_index; ///< Options of the algorithm
    std::string m_unfoldsort;
    std::size_t m_repeat_unfold;
    bool m_alt_case_placement;
    bool m_possibly_inconsistent;
    bool m_disable_pattern_unfolding;

    void add_options(interface_description& desc)
    {
      super::add_options(desc);
      /* desc.add_option("index", make_mandatory_argument("[NUM]"),
                      "unfolds process parameters for comma separated indices", 'i');
      desc.add_option("sort", make_mandatory_argument("NAME"),
                      "unfolds all process parameters of sort NAME", 's');
      desc.add_option("repeat", make_mandatory_argument("NUM"),
                      "repeat unfold NUM times", 'n');
      desc.add_option("alt-case",
                      "use an alternative placement method for case functions", 'a');
      desc.add_option("possibly-inconsistent",
                      "add rewrite rules that can make a data specification inconsistent", 'p');
      desc.add_option("no-pattern",
                      "do not unfold pattern matching functions in state updates", 'x'); */
    }

    void parse_options(const command_line_parser& parser)
    {
      super::parse_options(parser);

//      if (((0 == parser.options.count("index")) && (0 == parser.options.count("sort"))) ||
//          ((0 < parser.options.count("index") && (0 < parser.options.count("sort")))))
//      {
//        parser.error("Use either --sort or --index to unfold process parameters.");
//      }

      // Parse string argument to [NUM] (a set of indices)
/*      if (0 < parser.options.count("index"))
      {
        std::string indices_str(parser.option_argument_as<std::string>("index"));
        std::stringstream indices(indices_str);

        std::size_t index;
        char comma;
        while (indices.good())
        {
          indices >> index;
          if (indices.bad())
          {
            throw mcrl2::runtime_error("Not a valid comma-separated list of indices: " + indices_str);
          }
          m_set_index.insert(index);
          if (indices.good())
          {
            indices >> comma;
            if (comma != ',')
            {
              throw mcrl2::runtime_error("Not a valid comma-separated list of indices: " + indices_str);
            }
          }
        }
        if (!indices.eof())
        {
          throw mcrl2::runtime_error("Not a valid comma-separated list of indices: " + indices_str);
        }
      }

      // Parse string argument NAME for sort argument
      if (0 < parser.options.count("sort"))
      {
        m_unfoldsort = parser.option_argument_as< std::string  >("sort");
      }

      m_repeat_unfold = 1;
      if (0 < parser.options.count("repeat"))
      {
        m_repeat_unfold = parser.option_argument_as< std::size_t  >("repeat");
      }

      m_alt_case_placement = parser.options.count("alt-case") > 0;
      m_possibly_inconsistent = parser.options.count("possibly-inconsistent") > 0;
      m_disable_pattern_unfolding = parser.options.count("no-pattern") > 0; */

    }

  public:

    lpsfununfold_tool()
      : super(
        "lpsfununfold",
        "Jan Friso Groote",
        "Unfolds process parameter of type function sort with a finite domain",
        "Unfolds each parameters with a function type D1#D2#...#Dn -> D into a sequence "
        "of parameters of sort D in the lps "
        "in INFILE and writes the result to OUTFILE. If INFILE is not present, stdin is "
        "used. If OUTFILE is not present, stdout is used."
      )
    {}

    ///Reads a specification from input_file,
    ///applies instantiation of sums to it and writes the result to output_file.
    bool run()
    {
      lps::stochastic_specification spec;
      load_lps(spec, input_filename());

      /* lpsfununfold-cache is used to avoid the introduction of equations for already unfolded sorts */

      /* data::variable_list resulting_parameters;
      for (data::variable v: spec.proc().parameters())
      {
        if (is_function_sort(v.sort())
        {
          function_sort s=atermpp::down_cast<function_sort>(v.sort());
          mCRL2log(verbose) << "Considering parameter " << v << "\n";

          for(sort_expression codom: s.codomain())
          {
            if (is_certainly_finite(codom))
            {
              
            }
          }
          data::variable_list vars;
          std::vector<data::data_expression_list>
          enumerate(vars, true, sigma, ZZZZ
        }

        //Calculate process parameters indices where m_unfoldsort occurs
        if (!m_unfoldsort.empty())
        {
          m_set_index.clear();

          data::sort_expression b_sort = mcrl2::data::parse_sort_expression( m_unfoldsort, spec.data() );
          mcrl2::data::sort_expression sort = normalize_sorts(b_sort, spec.data());


          if (!search_sort_expression(spec.data().sorts(), sort))
          {
            mCRL2log(warning) << "No sorts found of name " << m_unfoldsort << std::endl;
            break;
          }
          mcrl2::data::data_expression_list el = spec.initial_process().expressions();
          std::size_t index=0;
          for (const data_expression& e: spec.initial_process().expressions())
          {
            if (e.sort() == sort)
            {
              m_set_index.insert(index);
            }
            index++;
          }

          if (m_set_index.empty())
          {
            mCRL2log(warning) << "No process parameters found of sort " << m_unfoldsort << std::endl;
            break;
          }
        }

        //Unfold process parameters for calculated indices
        std::set< std::size_t > h_set_index = m_set_index;

        while (!h_set_index.empty())
        {
          lps::lpsfunufold lpsfununfold(spec, unfold_cache, m_alt_case_placement, m_possibly_inconsistent, !m_disable_pattern_unfolding);
          std::size_t index = *(max_element(h_set_index.begin(), h_set_index.end()));
          lpsfununfold.algorithm(index);
          // Rewriting intermediate results helps counteract blowup of the intermediate results
          rewriter R = create_rewriter(spec.data());
          lps::rewrite(spec, R);
          h_set_index.erase(index);
        }
      } */
      save_lps(spec, output_filename());

      return true;
    }

};

int main(int argc, char** argv)
{
  return lpsfununfold_tool().execute(argc, argv);
}
