// Author(s): Jeroen van der Wulp
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/utilities/exception.h
/// \brief Exception classes for use in libraries and tools

#ifndef __MCRL2_EXCEPTION_H__
#define __MCRL2_EXCEPTION_H__

#include <stdexcept>
#include <sstream>
#include <iostream>
#include <cassert>

namespace mcrl2
{

/**
 * \brief Standard exception class for reporting runtime errors.
 **/
class runtime_error : public std::runtime_error
{
  public:
    /// \brief Constructor
    /// \param[in] message the exception message
    runtime_error(const std::string& message) : std::runtime_error(message)
    {
    }
};

/**
 * \brief Exception class for errors raised by the command-line parser.
 **/
class command_line_error : public runtime_error
{
private:
  std::string m_msg;
public:
  command_line_error(const std::string& name, const std::string& message) throw()
    : runtime_error("")
  {
    // We're storing the message in a separate string because we cannot
    // alter the string that is used by std::runtime_error::what(). The
    // inheritance relation between command_line_error and runtime_error
    // is therefore only logical, not functional.
    std::stringstream s;
    s << name << ": " << message << "\n"
      << "Try '" << name << " --help' for more information.";
    m_msg = s.str();
  }
  virtual const char* what() const throw()
  {
    return m_msg.c_str();
  }
  virtual ~command_line_error() throw()
  {};
};

/**
 * \brief Standard exception class for reporting runtime errors.
 **/
class logic_error : public std::logic_error
{
  public:
    /// \brief Constructor
    /// \param[in] message the exception message
    logic_error(std::string const& message) : std::logic_error(message)
    {
    }
    /// \brief error: to message
    const char* what() const throw()
    {
      return (std::string("error: ") + std::logic_error::what()).c_str();
    }
};
}

#endif
