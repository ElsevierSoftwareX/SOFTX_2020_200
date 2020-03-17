/* ---------------------------------------------------------------------
 *
 * Copyright (C) 2019 -  by the Lethe authors
 *
 * This file is part of the Lethe library
 *
 * The Lethe library is free software; you can use it, redistribute
 * it, and/or modify it under the terms of the GNU Lesser General
 * Public License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * The full text of the license can be found in the file LICENSE at
 * the top level of the Lethe distribution.
 *
 * ---------------------------------------------------------------------

 *
 * Author: Bruno Blais, Polytechnique Montreal, 2019 -
 */

#ifndef LETHE_BOUNDARYCONDITIONS_H
#define LETHE_BOUNDARYCONDITIONS_H

#include <deal.II/base/auto_derivative_function.h>
#include <deal.II/base/function.h>
#include <deal.II/base/parsed_function.h>

using namespace dealii;

namespace BoundaryConditions
{
  enum class BoundaryType
  {
    noslip,
    slip,
    function,
    periodic
  };

  template <int dim>
  class BoundaryFunction
  {
  public:
    // Velocity components
    std::shared_ptr<AutoDerivativeFunction<dim>> u;
    std::shared_ptr<AutoDerivativeFunction<dim>> v;
    std::shared_ptr<AutoDerivativeFunction<dim>> w;

    // Point for the center of rotation
    Point<dim> cor;
  };

  template <int dim>
  class NSBoundaryConditions
  {
  public:
    // ID of boundary condition
    std::vector<unsigned int> id;

    // List of boundary type for each number
    std::vector<BoundaryType> type;

    // Functions for (u,v,w) for all boundaries
    std::vector<BoundaryFunction<dim>> bcFunctions;

    // Number of boundary conditions
    unsigned int size;
    unsigned int max_size;

    // Periodic boundary condition matching
    std::vector<unsigned int> periodic_id;
    std::vector<unsigned int> periodic_direction;

    void
    parse_boundary(ParameterHandler &prm, unsigned int i_bc);
    void
    declareDefaultEntry(ParameterHandler &prm, unsigned int i_bc);
    void
    declare_parameters(ParameterHandler &prm);
    void
    parse_parameters(ParameterHandler &prm);
    void
    parse_parameters(boost::property_tree::ptree &root);
    void
    createDefaultNoSlip();
  };

  template <int dim>
  void
  NSBoundaryConditions<dim>::createDefaultNoSlip()
  {
    id.resize(1);
    id[0] = 0;
    type.resize(1);
    type[0] = BoundaryType::noslip;
    size    = 1;
  }

  template <int dim>
  void
  NSBoundaryConditions<dim>::declareDefaultEntry(ParameterHandler &prm,
                                                 unsigned int      i_bc)
  {
    prm.declare_entry("type",
                      "noslip",
                      Patterns::Selection("noslip|slip|function|periodic"),
                      "Type of boundary conditoin"
                      "Choices are <noslip|slip|function>.");

    prm.declare_entry("id",
                      Utilities::int_to_string(i_bc, 2),
                      Patterns::Integer(),
                      "Mesh id for boundary conditions");

    prm.declare_entry("periodic_id",
                      "0",
                      Patterns::Integer(),
                      "Mesh id for periodic face matching");

    prm.declare_entry("periodic_direction",
                      "0",
                      Patterns::Integer(),
                      "Direction for periodic boundary condition");

    prm.enter_subsection("u");
    std::shared_ptr<Functions::ParsedFunction<dim>> u_function =
      std::make_shared<Functions::ParsedFunction<dim>>();
    u_function->declare_parameters(prm, 1);
    bcFunctions[i_bc].u = u_function;
    prm.set("Function expression", "0");
    prm.leave_subsection();

    prm.enter_subsection("v");
    std::shared_ptr<Functions::ParsedFunction<dim>> v_function =
      std::make_shared<Functions::ParsedFunction<dim>>();
    v_function->declare_parameters(prm, 1);
    bcFunctions[i_bc].v = v_function;
    prm.set("Function expression", "0");
    prm.leave_subsection();

    prm.enter_subsection("w");
    std::shared_ptr<Functions::ParsedFunction<dim>> w_function =
      std::make_shared<Functions::ParsedFunction<dim>>();
    w_function->declare_parameters(prm, 1);
    bcFunctions[i_bc].w = w_function;
    prm.set("Function expression", "0");
    prm.leave_subsection();

    prm.enter_subsection("cor");
    prm.declare_entry("x", "0", Patterns::Double(), "X COR");
    prm.declare_entry("y", "0", Patterns::Double(), "Y COR");
    prm.declare_entry("z", "0", Patterns::Double(), "Z COR");
    prm.leave_subsection();
  }

  template <int dim>
  void
  NSBoundaryConditions<dim>::parse_boundary(ParameterHandler &prm,
                                            unsigned int      i_bc)
  {
    const std::string op = prm.get("type");
    if (op == "noslip")
      type[i_bc] = BoundaryType::noslip;
    if (op == "slip")
      type[i_bc] = BoundaryType::slip;
    if (op == "function")
      {
        static auto throw_error = []() {
          throw std::runtime_error(
            "Could not convert boundary function to a ParsedFunction");
        };

        type[i_bc] = BoundaryType::function;
        prm.enter_subsection("u");
        if (auto function = dynamic_cast<Functions::ParsedFunction<dim> *>(
              bcFunctions[i_bc].u.get()))
          function->parse_parameters(prm);
        else
          throw_error();
        prm.leave_subsection();

        prm.enter_subsection("v");
        if (auto function = dynamic_cast<Functions::ParsedFunction<dim> *>(
              bcFunctions[i_bc].v.get()))
          function->parse_parameters(prm);
        else
          throw_error();
        prm.leave_subsection();

        prm.enter_subsection("w");
        if (auto function = dynamic_cast<Functions::ParsedFunction<dim> *>(
              bcFunctions[i_bc].w.get()))
          function->parse_parameters(prm);
        else
          throw_error();
        prm.leave_subsection();

        prm.enter_subsection("cor");
        bcFunctions[i_bc].cor[0] = prm.get_double("x");
        bcFunctions[i_bc].cor[1] = prm.get_double("y");
        if (dim == 3)
          bcFunctions[i_bc].cor[2] = prm.get_double("z");
        prm.leave_subsection();
      }
    if (op == "periodic")
      {
        type[i_bc]               = BoundaryType::periodic;
        periodic_id[i_bc]        = prm.get_integer("periodic_id");
        periodic_direction[i_bc] = prm.get_integer("periodic_direction");
      }

    id[i_bc] = prm.get_integer("id");
  }

  template <int dim>
  void
  NSBoundaryConditions<dim>::declare_parameters(ParameterHandler &prm)
  {
    max_size = 4;

    prm.enter_subsection("boundary conditions");
    {
      prm.declare_entry("number",
                        "0",
                        Patterns::Integer(),
                        "Number of boundary conditions");
      id.resize(max_size);
      periodic_id.resize(max_size);
      periodic_direction.resize(max_size);
      type.resize(max_size);
      bcFunctions.resize(max_size);

      prm.enter_subsection("bc 0");
      {
        declareDefaultEntry(prm, 0);
      }
      prm.leave_subsection();

      prm.enter_subsection("bc 1");
      {
        declareDefaultEntry(prm, 1);
      }
      prm.leave_subsection();

      prm.enter_subsection("bc 2");
      {
        declareDefaultEntry(prm, 2);
      }
      prm.leave_subsection();

      prm.enter_subsection("bc 3");
      {
        declareDefaultEntry(prm, 3);
      }
      prm.leave_subsection();
    }
    prm.leave_subsection();
  }

  template <int dim>
  void
  NSBoundaryConditions<dim>::parse_parameters(ParameterHandler &prm)
  {
    prm.enter_subsection("boundary conditions");
    {
      size = prm.get_integer("number");
      type.resize(size);
      id.resize(size);
      periodic_direction.resize(size);
      periodic_id.resize(size);

      if (size >= 1)
        {
          prm.enter_subsection("bc 0");
          {
            parse_boundary(prm, 0);
          }
          prm.leave_subsection();
        }
      if (size >= 2)
        {
          prm.enter_subsection("bc 1");
          {
            parse_boundary(prm, 1);
          }
          prm.leave_subsection();
        }
      if (size >= 3)
        {
          prm.enter_subsection("bc 2");
          {
            parse_boundary(prm, 2);
          }
          prm.leave_subsection();
        }
      if (size >= 4)
        {
          prm.enter_subsection("bc 3");
          {
            parse_boundary(prm, 3);
          }
          prm.leave_subsection();
        }
    }
    prm.leave_subsection();
  }
} // namespace BoundaryConditions

template <int dim>
class FunctionDefined : public Function<dim>
{
private:
  std::shared_ptr<AutoDerivativeFunction<dim>> u;
  std::shared_ptr<AutoDerivativeFunction<dim>> v;
  std::shared_ptr<AutoDerivativeFunction<dim>> w;

public:
  FunctionDefined(std::shared_ptr<AutoDerivativeFunction<dim>> p_u,
                  std::shared_ptr<AutoDerivativeFunction<dim>> p_v,
                  std::shared_ptr<AutoDerivativeFunction<dim>> p_w)
    : Function<dim>(dim + 1)
    , u(p_u)
    , v(p_v)
    , w(p_w)
  {}

  virtual double
  value(const Point<dim> &p, const unsigned int component) const;
};

template <int dim>
double
FunctionDefined<dim>::value(const Point<dim> & p,
                            const unsigned int component) const
{
  Assert(component < this->n_components,
         ExcIndexRange(component, 0, this->n_components));
  if (component == 0)
    {
      return u->value(p);
    }
  else if (component == 1)
    {
      return v->value(p);
    }
  else if (component == 2)
    {
      return w->value(p);
    }
  return 0.;
}


#endif
