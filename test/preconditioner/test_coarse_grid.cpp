#include <gtest/gtest.h>

#include <algorithm>
#include <numeric>
#include <polatory/interpolation/rbf_direct_evaluator.hpp>
#include <polatory/model.hpp>
#include <polatory/point_cloud/random_points.hpp>
#include <polatory/polynomial/lagrange_basis.hpp>
#include <polatory/preconditioner/coarse_grid.hpp>
#include <polatory/preconditioner/domain.hpp>
#include <polatory/rbf/multiquadric1.hpp>
#include <polatory/types.hpp>
#include <random>
#include <utility>
#include <vector>

using polatory::index_t;
using polatory::model;
using polatory::common::valuesd;
using polatory::geometry::sphere3d;
using polatory::interpolation::rbf_direct_evaluator;
using polatory::point_cloud::random_points;
using polatory::polynomial::lagrange_basis;
using polatory::preconditioner::coarse_grid;
using polatory::preconditioner::domain;
using polatory::rbf::multiquadric1;

TEST(coarse_grid, trivial) {
  auto mu = index_t{512};
  auto sigma = index_t{256};
  auto dim = 3;
  auto deg = 0;
  auto absolute_tolerance = 1e-10;

  auto points = random_points(sphere3d(), mu);
  auto grad_points = random_points(sphere3d(), sigma);

  model model(multiquadric1({1.0, 1e-3}), dim, deg);
  model.set_nugget(0.01);
  auto l = model.poly_basis_size();

  domain domain;
  {
    std::random_device rd;
    std::mt19937 gen(rd());

    std::vector<index_t> indices(mu);
    std::iota(indices.begin(), indices.end(), 0);
    std::shuffle(indices.begin(), indices.end(), gen);

    std::vector<index_t> grad_indices(sigma);
    std::iota(grad_indices.begin(), grad_indices.end(), 0);
    std::shuffle(grad_indices.begin(), grad_indices.end(), gen);

    domain.point_indices = std::move(indices);
    domain.grad_point_indices = std::move(grad_indices);
  }

  coarse_grid coarse(model, std::move(domain));
  lagrange_basis lagrange_basis(dim, deg, points.topRows(model.poly_basis_size()));
  auto lagrange_pt = lagrange_basis.evaluate(points, grad_points);
  coarse.setup(points, grad_points, lagrange_pt);

  valuesd rhs = valuesd::Random(mu + dim * sigma);
  coarse.solve(rhs);

  valuesd sol = valuesd::Zero(mu + dim * sigma + l);
  coarse.set_solution_to(sol);

  auto eval = rbf_direct_evaluator(model, points, grad_points);
  eval.set_weights(sol);
  eval.set_field_points(points);
  valuesd values_fit = eval.evaluate() + sol.head(mu) * model.nugget();

  auto max_residual = (rhs.head(mu) - values_fit).lpNorm<Eigen::Infinity>();
  EXPECT_LT(max_residual, absolute_tolerance);
}
