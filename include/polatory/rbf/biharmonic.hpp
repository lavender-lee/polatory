// Copyright (c) 2016, GSI and The Polatory Authors.

#pragma once

#include "rbf_base.hpp"

namespace polatory {
namespace rbf {

class biharmonic : public rbf_base {
public:
  using rbf_base::rbf_base;

  static double evaluate(double r, const double *params) {
    auto slope = params[0];

    return -slope * r;
  }

  double evaluate(double r) const override {
    return evaluate(r, parameters().data());
  }

  void evaluate_gradient(
    double& gradx, double& grady, double& gradz,
    double x, double y, double z, double r) const override {
    auto slope = parameters()[0];

    auto c = -slope / r;
    gradx = c * x;
    grady = c * y;
    gradz = c * z;
  }

  double nugget() const override {
    return parameters()[1];
  }

  int order_of_cpd() const override {
    return 1;
  }
};

} // namespace rbf
} // namespace polatory